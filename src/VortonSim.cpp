#include "VortonSim.hpp"
#include "VortonClusterAux.hpp"
#include"UniformGridMath.hpp"
#include "Mat3.hpp"
#include <thread>

#define VELOCITY_FROM_TREE 1

#if USE_TBB

/*! \brief Function object to compute velocity grid using Threading Building Blocks
 */
class VortonSim_ComputeVelocityGrid_TBB
{
    VortonSim * mVortonSim ;    ///< Address of VortonSim object
public:
    void operator() ( const tbb::blocked_range<size_t> & r ) const
    {   // Compute subset of velocity grid.
        mVortonSim->ComputeVelocityGridSlice( r.begin() , r.end() ) ;
    }
    VortonSim_ComputeVelocityGrid_TBB( VortonSim * pVortonSim )
    : mVortonSim( pVortonSim ) {}
} ;

/*! \brief Function object to advect passive tracer particles using Threading Building Blocks
 */
class VortonSim_AdvectTracers_TBB
{
    VortonSim * mVortonSim ;    ///< Address of VortonSim object
    const float & mTimeStep ;
    const unsigned & mFrame ;
public:
    void operator() ( const tbb::blocked_range<size_t> & r ) const
    {   // Advect subset of tracers.
        mVortonSim->AdvectTracersSlice( mTimeStep , mFrame , r.begin() , r.end() ) ;
    }
    VortonSim_AdvectTracers_TBB( VortonSim * pVortonSim , const float & timeStep , const unsigned & uFrame )
    : mVortonSim( pVortonSim )
    , mTimeStep( timeStep )
    , mFrame( uFrame )
    {}
} ;
#endif


/*! \brief Update axis-aligned bounding box corners to include given point

 \param vMinCorner - minimal corner of axis-aligned bounding box

 \param vMaxCorner - maximal corner of axis-aligned bounding box

 \param vPoint - point to include in bounding box

 */
void UpdateBoundingBox(ofVec3f & vMinCorner, ofVec3f & vMaxCorner, const ofVec3f & vPoint) {
	vMinCorner.x = std::min(vPoint.x, vMinCorner.x);
	vMinCorner.y = std::min(vPoint.y, vMinCorner.y);
	vMinCorner.z = std::min(vPoint.z, vMinCorner.z);
	vMaxCorner.x = std::max(vPoint.x, vMaxCorner.x);
	vMaxCorner.y = std::max(vPoint.y, vMaxCorner.y);
	vMaxCorner.z = std::max(vPoint.z, vMaxCorner.z);
}

void VortonSim::Initialize(size_t numTracersPerCellCubeRoot) {
	ConservedQuantities(mCirculationInitial, mLinearImpulseInitial);
	ComputeAverageVorticity();
	CreateInfluenceTree(); // This is a marginally superfluous call.  We only need the grid geometry to seed passive tracer particles.
	InitializePassiveTracers(numTracersPerCellCubeRoot);

	{
		float domainVolume = mInfluenceTree[0].GetExtent().x * mInfluenceTree[0].GetExtent().y * mInfluenceTree[0].GetExtent().z;
		if (0.0f == mInfluenceTree[0].GetExtent().z)
		{   // Domain is 2D in XY plane.
			domainVolume = mInfluenceTree[0].GetExtent().x * mInfluenceTree[0].GetExtent().y;
		}
		const float totalMass = domainVolume * mFluidDensity;
		const size_t numTracersPerCell = numTracersPerCellCubeRoot * numTracersPerCellCubeRoot * numTracersPerCellCubeRoot;
		mMassPerParticle = totalMass / float(mInfluenceTree[0].GetGridCapacity() * numTracersPerCell);
	}
}

/*! \brief Assign vortons from a uniform grid of vorticity

 \param vortGrid - uniform grid of vorticity values

 */
void VortonSim::AssignVortonsFromVorticity(UniformGrid< ofVec3f > & vortGrid) {
	mVortons.clear(); // Empty out any existing vortons.

	// Obtain characteristic size of each grid cell.

	const UniformGridGeometry & ug = vortGrid;
	const float     fVortonRadius = powf(ug.GetCellSpacing().x * ug.GetCellSpacing().y  * ug.GetCellSpacing().z, 1.0f / 3.0f) * 0.5f;
	const ofVec3f      Nudge(ug.GetExtent() * FLT_EPSILON * 4.0f);
	const ofVec3f      vMin(ug.GetMinCorner() + Nudge);
	const ofVec3f      vSpacing(ug.GetCellSpacing() * (1.0f - 0.0f * FLT_EPSILON));
	const size_t  numPoints[3] = { ug.GetNumPoints(0) , ug.GetNumPoints(1) , ug.GetNumPoints(2) };
	const size_t  numXY = numPoints[0] * numPoints[1];
	size_t idx[3];
	for (idx[2] = 0; idx[2] < numPoints[2]; ++idx[2])
	{
		ofVec3f vPositionOfGridCellCenter;
		vPositionOfGridCellCenter.z = vMin.z + float(idx[2]) * vSpacing.z;
		const size_t offsetZ = idx[2] * numXY;
		for (idx[1] = 0; idx[1] < numPoints[1]; ++idx[1])
		{
			vPositionOfGridCellCenter.y = vMin.y + float(idx[1]) * vSpacing.y;
			const size_t offsetYZ = idx[1] * vortGrid.GetNumPoints(0) + offsetZ;
			for (idx[0] = 0; idx[0] < numPoints[0]; ++idx[0])
			{
				vPositionOfGridCellCenter.x = vMin.x + float(idx[0]) * vSpacing.x;
				const size_t offsetXYZ = idx[0] + offsetYZ;
				const ofVec3f & rVort = vortGrid[offsetXYZ];
				if (rVort.lengthSquared() > FLT_EPSILON)
				{   // This grid cell contains significant vorticity.
					mVortons.emplace_back(vPositionOfGridCellCenter, rVort, fVortonRadius);
				}
			}
		}
	}
}

/*! \brief Compute the total circulation and linear impulse of all vortons in this simulation.

 \param vCirculation - Total circulation, the volume integral of vorticity, computed by this routine.

 \param vLinearImpulse - Volume integral of circulation weighted by position, computed by this routine.

 */
void    VortonSim::ConservedQuantities(ofVec3f & vCirculation, ofVec3f & vLinearImpulse) const {
	// Zero accumulators.
	vCirculation = vLinearImpulse = ofVec3f(0.0f, 0.0f, 0.0f);
	const size_t numVortons = mVortons.size();
	for (size_t iVorton = 0; iVorton < numVortons; ++iVorton)
	{   // For each vorton in this simulation...
		const Vorton &  rVorton = mVortons[iVorton];
		const float     volumeElement = (rVorton.mRadius * rVorton.mRadius * rVorton.mRadius) * 8.0f;
		// Accumulate total circulation.
		vCirculation += rVorton.mVorticity * volumeElement;
		// Accumulate total linear impulse.
		vLinearImpulse += rVorton.mPosition.getCrossed(rVorton.mVorticity) * volumeElement;
	}
}

/*! \brief Compute the average vorticity of all vortons in this simulation.

 \note This is used to compute a hacky, non-physical approximation to
 viscous vortex diffusion.

 */
void VortonSim::ComputeAverageVorticity() {
	// Zero accumulators.
	mAverageVorticity = ofVec3f(0.0f, 0.0f, 0.0f);
	const size_t numVortons = mVortons.size();
	for (size_t iVorton = 0; iVorton < numVortons; ++iVorton)
	{   // For each vorton in this simulation...
		const Vorton &  rVorton = mVortons[iVorton];
		mAverageVorticity += rVorton.mVorticity;
	}
	mAverageVorticity /= float(numVortons);
}

/// Find axis-aligned bounding box for all vortons in this simulation.
void VortonSim::FindBoundingBox()
{
	//    QUERY_PERFORMANCE_ENTER ;
	const size_t numVortons = mVortons.size();
	mMinCorner.x = mMinCorner.y = mMinCorner.z = FLT_MAX;
	mMaxCorner = -mMinCorner;
	for (size_t iVorton = 0; iVorton < numVortons; ++iVorton)
	{   // For each vorton in this simulation...
		const Vorton & rVorton = mVortons[iVorton];
		// Find corners of axis-aligned bounding box.
		UpdateBoundingBox(mMinCorner, mMaxCorner, rVorton.mPosition);
	}
	//    QUERY_PERFORMANCE_EXIT( VortonSim_CreateInfluenceTree_FindBoundingBox_Vortons ) ;

	//    QUERY_PERFORMANCE_ENTER ;
	const size_t numTracers = mTracers.size();
	for (size_t iTracer = 0; iTracer < numTracers; ++iTracer)
	{   // For each passive tracer particle in this simulation...
		const Particle & rTracer = mTracers[iTracer];
		// Find corners of axis-aligned bounding box.
		UpdateBoundingBox(mMinCorner, mMaxCorner, rTracer.mPosition);
	}
	//    QUERY_PERFORMANCE_EXIT( VortonSim_CreateInfluenceTree_FindBoundingBox_Tracers ) ;

		// Slightly enlarge bounding box to allow for round-off errors.
	const ofVec3f extent(mMaxCorner - mMinCorner);
	const ofVec3f nudge(extent * FLT_EPSILON);
	mMinCorner -= nudge;
	mMaxCorner += nudge;
}

/*! \brief Create base layer of vorton influence tree.

 This is the leaf layer, where each grid cell corresponds (on average) to
 a single vorton.  Some cells might contain multiple vortons and some zero.
 Each cell effectively has a single "supervorton" which its parent layers
 in the influence tree will in turn aggregate.

 \note This implementation of gridifying the base layer is NOT suitable
 for Eulerian operations like approximating spatial derivatives
 of vorticity or solving a vector Poisson equation, because this
 routine associates each vortex with a single corner point of the
 grid cell that contains it.  To create a grid for Eulerian calculations,
 each vorton would contribute to all 8 corner points of the grid
 cell that contains it.

 We could rewrite this to suit "Eulerian" operations, in which case
 we would want to omit "size" and "position" since the grid would
 implicitly represent that information.  That concern goes hand-in-hand
 with the method used to compute velocity from vorticity.
 Ultimately we need to make sure theoretically conserved quantities behave as expected.

 \note This method assumes the influence tree skeleton has already been created,
 and the leaf layer initialized to all "zeros", meaning it contains no
 vortons.

 */
void VortonSim::MakeBaseVortonGrid(void)
{
	const size_t numVortons = mVortons.size();

	UniformGrid< VortonClusterAux > ugAux(mInfluenceTree[0]); // Temporary auxilliary information used during aggregation.
	ugAux.Init();

	// Compute preliminary vorticity grid.
	for (size_t uVorton = 0; uVorton < numVortons; ++uVorton)
	{   // For each vorton in this simulation...
		const Vorton     &  rVorton = mVortons[uVorton];
		const ofVec3f    &  rPosition = rVorton.mPosition;
		const size_t      uOffset = mInfluenceTree[0].OffsetOfPosition(rPosition);
		Vorton           &  rVortonCell = mInfluenceTree[0][uOffset];
		VortonClusterAux &  rVortonAux = ugAux[uOffset];
		const float         vortMag = rVorton.mVorticity.length();

		rVortonCell.mPosition += rVorton.mPosition * vortMag; // Compute weighted position -- to be normalized later.
		rVortonCell.mVorticity += rVorton.mVorticity; // Tally vorticity sum.
		rVortonCell.mRadius = rVorton.mRadius; // Assign volume element size.
																// OBSOLETE. See comments below: UpdateBoundingBox( rVortonAux.mMinCorner , rVortonAux.mMaxCorner , rVorton.mPosition ) ;
		rVortonAux.mVortNormSum += vortMag;
	}

	// Post-process preliminary grid; normalize center-of-vorticity and compute sizes, for each grid cell.
	const size_t num[3] = {
		mInfluenceTree[0].GetNumPoints(0) ,
		mInfluenceTree[0].GetNumPoints(1) ,
		mInfluenceTree[0].GetNumPoints(2)
	};
	const size_t numXY = num[0] * num[1];
	size_t idx[3];
	for (idx[2] = 0; idx[2] < num[2]; ++idx[2])
	{
		const size_t zShift = idx[2] * numXY;
		for (idx[1] = 0; idx[1] < num[1]; ++idx[1])
		{
			const size_t yzShift = idx[1] * num[0] + zShift;
			for (idx[0] = 0; idx[0] < num[0]; ++idx[0])
			{
				const size_t      offset = idx[0] + yzShift;
				VortonClusterAux &  rVortonAux = ugAux[offset];
				if (rVortonAux.mVortNormSum != FLT_MIN)
				{   // This cell contains at least one vorton.
					Vorton & rVortonCell = mInfluenceTree[0][offset];
					// Normalize weighted position sum to obtain center-of-vorticity.
					rVortonCell.mPosition /= rVortonAux.mVortNormSum;
				}
			}
		}
	}
}

/*! \brief Aggregate vorton clusters from a child layer into a parent layer of the influence tree

 This routine assumes the given parent layer is empty and its child layer (i.e. the layer
 with index uParentLayer-1) is populated.

 \param uParentLayer - index of parent layer into which aggregated influence information will be stored.
 This must be greater than 0 because the base layer, which has no children, has index 0.

 \see CreateInfluenceTree

 */
void VortonSim::AggregateClusters(size_t uParentLayer)
{
	UniformGrid<Vorton> & rParentLayer = mInfluenceTree[uParentLayer];

	// number of cells in each grid cluster
	const size_t * const pClusterDims = mInfluenceTree.GetDecimations(uParentLayer);

	const size_t  numCells[3] = { rParentLayer.GetNumCells(0) , rParentLayer.GetNumCells(1) , rParentLayer.GetNumCells(2) };
	const size_t  numXY = rParentLayer.GetNumPoints(0) * rParentLayer.GetNumPoints(1);
	// (Since this loop writes to each parent cell, it should readily parallelize without contention.)
	size_t idxParent[3];
	for (idxParent[2] = 0; idxParent[2] < numCells[2]; ++idxParent[2])
	{
		const size_t offsetZ = idxParent[2] * numXY;
		for (idxParent[1] = 0; idxParent[1] < numCells[1]; ++idxParent[1])
		{
			const size_t offsetYZ = idxParent[1] * rParentLayer.GetNumPoints(0) + offsetZ;
			for (idxParent[0] = 0; idxParent[0] < numCells[0]; ++idxParent[0])
			{   // For each cell in the parent layer...
				const size_t offsetXYZ = idxParent[0] + offsetYZ;
				UniformGrid<Vorton> & rChildLayer = mInfluenceTree[uParentLayer - 1];
				Vorton              & rVortonParent = rParentLayer[offsetXYZ];
				VortonClusterAux vortAux;
				size_t clusterMinIndices[3];
				mInfluenceTree.GetChildClusterMinCornerIndex(clusterMinIndices, pClusterDims, idxParent);
				size_t increment[3] = { 0 , 0 , 0 };
				const size_t & numXchild = rChildLayer.GetNumPoints(0);
				const size_t   numXYchild = numXchild * rChildLayer.GetNumPoints(1);
				// For each cell of child layer in this grid cluster...
				for (increment[2] = 0; increment[2] < pClusterDims[2]; ++increment[2])
				{
					const size_t offsetZ = (clusterMinIndices[2] + increment[2]) * numXYchild;
					for (increment[1] = 0; increment[1] < pClusterDims[1]; ++increment[1])
					{
						const size_t offsetYZ = (clusterMinIndices[1] + increment[1]) * numXchild + offsetZ;
						for (increment[0] = 0; increment[0] < pClusterDims[0]; ++increment[0])
						{
							const size_t  offsetXYZ = (clusterMinIndices[0] + increment[0]) + offsetYZ;
							Vorton &        rVortonChild = rChildLayer[offsetXYZ];
							const float     vortMag = rVortonChild.mVorticity.length();

							// Aggregate vorton cluster from child layer into parent layer:
							rVortonParent.mPosition += rVortonChild.mPosition * vortMag;
							rVortonParent.mVorticity += rVortonChild.mVorticity;
							vortAux.mVortNormSum += vortMag;
							if (rVortonChild.mRadius != 0.0f)
							{
								rVortonParent.mRadius = rVortonChild.mRadius;
							}
						}
					}
				}
				// Normalize weighted position sum to obtain center-of-vorticity.
				// (See analogous code in MakeBaseVortonGrid.)
				rVortonParent.mPosition /= vortAux.mVortNormSum;
			}
		}
	}
}

/*! \brief Create nested grid vorticity influence tree

 Each layer of this tree represents a simplified, aggregated version of
 all of the information in its "child" layer, where each
 "child" has higher resolution than its "parent".

 \see MakeBaseVortonGrid, AggregateClusters

 Derivation:

 Using conservation properties, I_0 = I_0' , I_1 = I_1' , I_2 = I_2'

 I_0 : wx d = w1x d1 + w2x d2
 : wy d = w1y d1 + w2y d2
 : wz d = w1z d1 + w2z d2

 These 3 are not linearly independent:
 I_1 : ( y wz - z wy ) d = ( y1 wz1 - z1 wy1 ) d1 + ( y2 wz2 - z2 wy2 ) d2
 : ( z wx - x wz ) d = ( z1 wx1 - x1 wz1 ) d1 + ( z2 wx2 - x2 wz2 ) d2
 : ( x wy - y wx ) d = ( x1 wy1 - y1 wx1 ) d1 + ( x2 wy2 - y2 wx2 ) d2

 I_2 : ( x^2 + y^2 + z^2 ) wx d = (x1^2 + y1^2 + z1^2 ) wx1 d1 + ( x2^2 + y2^2 + z2^2 ) wx2 d2
 : ( x^2 + y^2 + z^2 ) wy d = (x1^2 + y1^2 + z1^2 ) wy1 d1 + ( x2^2 + y2^2 + z2^2 ) wy2 d2
 : ( x^2 + y^2 + z^2 ) wz d = (x1^2 + y1^2 + z1^2 ) wz1 d1 + ( x2^2 + y2^2 + z2^2 ) wz2 d2

 Can replace I_2 with its magnitude:
 ( x^2  + y^2  + z^2  ) ( wx^2  + wy^2  + wz^2  )^(1/2) d
 = ( x1^2 + y1^2 + z1^2 ) ( wx1^2 + w1y^2 + w1z^2 )^(1/2) d1
 + ( x2^2 + y2^2 + z2^2 ) ( wx2^2 + w2y^2 + w2z^2 )^(1/2) d2

 */
void VortonSim::CreateInfluenceTree(void)
{
	//    QUERY_PERFORMANCE_ENTER ;
	FindBoundingBox(); // Find axis-aligned bounding box that encloses all vortons.
//    QUERY_PERFORMANCE_EXIT( VortonSim_CreateInfluenceTree_FindBoundingBox ) ;

	// Create skeletal nested grid for influence tree.
	const size_t numVortons = mVortons.size();
	{
		UniformGrid< Vorton >   ugSkeleton;   ///< Uniform grid with the same size & shape as the one holding aggregated information about mVortons.
		ugSkeleton.DefineShape(numVortons, mMinCorner, mMaxCorner, true);
		mInfluenceTree.Initialize(ugSkeleton); // Create skeleton of influence tree.
	}

	//    QUERY_PERFORMANCE_ENTER ;
	MakeBaseVortonGrid();
	//    QUERY_PERFORMANCE_EXIT( VortonSim_CreateInfluenceTree_MakeBaseVortonGrid ) ;

	//    QUERY_PERFORMANCE_ENTER ;
	const size_t numLayers = mInfluenceTree.GetDepth();
	for (size_t uParentLayer = 1; uParentLayer < numLayers; ++uParentLayer)
	{   // For each layer in the influence tree...
		AggregateClusters(uParentLayer);
	}
	//    QUERY_PERFORMANCE_EXIT( VortonSim_CreateInfluenceTree_AggregateClusters ) ;
}

/*! \brief Compute velocity at a given point in space, due to influence of vortons

 \param vPosition - point in space whose velocity to evaluate

 \param indices - indices of cell to visit in the given layer

 \param iLayer - which layer to process

 \return velocity at vPosition, due to influence of vortons

 \note This is a recursive algorithm with time complexity O(log(N)).
 The outermost caller should pass in mInfluenceTree.GetDepth().

 */
ofVec3f VortonSim::ComputeVelocity(const ofVec3f & vPosition, const size_t indices[3], size_t iLayer)
{
	UniformGrid< Vorton > & rChildLayer = mInfluenceTree[iLayer - 1];
	size_t                clusterMinIndices[3];
	const size_t *        pClusterDims = mInfluenceTree.GetDecimations(iLayer);
	mInfluenceTree.GetChildClusterMinCornerIndex(clusterMinIndices, pClusterDims, indices);

	const ofVec3f &            vGridMinCorner = rChildLayer.GetMinCorner();
	const ofVec3f              vSpacing = rChildLayer.GetCellSpacing();
	size_t                increment[3];
	const size_t &        numXchild = rChildLayer.GetNumPoints(0);
	const size_t          numXYchild = numXchild * rChildLayer.GetNumPoints(1);
	ofVec3f                    velocityAccumulator(0.0f, 0.0f, 0.0f);

	// The larger this is, the more accurate (and slower) the evaluation.
	// Reasonable values lie in [0.00001,4.0].
	// Setting this to 0 leads to very bad errors, but values greater than (tiny) lead to drastic improvements.
	// Changes in margin have a quantized effect since they effectively indicate how many additional
	// cluster subdivisions to visit.
	static const float  marginFactor = 0.0001f; // 0.4f ; // ship with this number: 0.0001f ; test with 0.4
													// When domain is 2D in XY plane, min.z==max.z so vPos.z test below would fail unless margin.z!=0.
	const ofVec3f          margin = marginFactor * vSpacing + (0.0f == vSpacing.z ? ofVec3f(0, 0, FLT_MIN) : ofVec3f(0, 0, 0));

	// For each cell of child layer in this grid cluster...
	for (increment[2] = 0; increment[2] < pClusterDims[2]; ++increment[2])
	{
		size_t idxChild[3];
		idxChild[2] = clusterMinIndices[2] + increment[2];
		ofVec3f vCellMinCorner, vCellMaxCorner;
		vCellMinCorner.z = vGridMinCorner.z + float(idxChild[2]) * vSpacing.z;
		vCellMaxCorner.z = vGridMinCorner.z + float(idxChild[2] + 1) * vSpacing.z;
		const size_t offsetZ = idxChild[2] * numXYchild;
		for (increment[1] = 0; increment[1] < pClusterDims[1]; ++increment[1])
		{
			idxChild[1] = clusterMinIndices[1] + increment[1];
			vCellMinCorner.y = vGridMinCorner.y + float(idxChild[1]) * vSpacing.y;
			vCellMaxCorner.y = vGridMinCorner.y + float(idxChild[1] + 1) * vSpacing.y;
			const size_t offsetYZ = idxChild[1] * numXchild + offsetZ;
			for (increment[0] = 0; increment[0] < pClusterDims[0]; ++increment[0])
			{
				idxChild[0] = clusterMinIndices[0] + increment[0];
				vCellMinCorner.x = vGridMinCorner.x + float(idxChild[0]) * vSpacing.x;
				vCellMaxCorner.x = vGridMinCorner.x + float(idxChild[0] + 1) * vSpacing.x;
				if (
					(iLayer > 1)
					&& (vPosition.x >= vCellMinCorner.x - margin.x)
					&& (vPosition.y >= vCellMinCorner.y - margin.y)
					&& (vPosition.z >= vCellMinCorner.z - margin.z)
					&& (vPosition.x < vCellMaxCorner.x + margin.x)
					&& (vPosition.y < vCellMaxCorner.y + margin.y)
					&& (vPosition.z < vCellMaxCorner.z + margin.z)
					)
				{   // Test position is inside childCell and currentLayer > 0...
					// Recurse child layer.
					velocityAccumulator += ComputeVelocity(vPosition, idxChild, iLayer - 1);
				}
				else {
					// Test position is outside childCell, or reached leaf node.
					//    Compute velocity induced by cell at corner point x.
					//    Accumulate influence, storing in velocityAccumulator.
					const size_t  offsetXYZ = idxChild[0] + offsetYZ;
					const Vorton &  rVortonChild = rChildLayer[offsetXYZ];
					VORTON_ACCUMULATE_VELOCITY(velocityAccumulator, vPosition, rVortonChild);
				}
			}
		}
	}
	return velocityAccumulator;
}

/*! \brief Compute velocity at a given point in space, due to influence of vortons

\param vPosition - point in space

\return velocity at vPosition, due to influence of vortons

\note This is a brute-force algorithm with time complexity O(N)
where N is the number of vortons.  This is too slow
for regular use but it is useful for comparisons.

*/
ofVec3f VortonSim::ComputeVelocityBruteForce(const ofVec3f & vPosition)
{
	const size_t  numVortons = mVortons.size();
	ofVec3f            velocityAccumulator(0.0f, 0.0f, 0.0f);

	for (size_t iVorton = 0; iVorton < numVortons; ++iVorton)
	{   // For each vorton...
		const Vorton &  rVorton = mVortons[iVorton];
		VORTON_ACCUMULATE_VELOCITY(velocityAccumulator, vPosition, rVorton);
	}

	return velocityAccumulator;
}

/*! \brief Compute velocity due to vortons, for a subset of points in a uniform grid

\param izStart - starting value for z index

\param izEnd - ending value for z index

\see CreateInfluenceTree, ComputeVelocityGrid

\note This routine assumes CreateInfluenceTree has already executed,
and that the velocity grid has been allocated.

*/
void VortonSim::ComputeVelocityGridSlice(size_t izStart, size_t izEnd)
{
#if VELOCITY_FROM_TREE
	const size_t        numLayers = mInfluenceTree.GetDepth();
#endif

	const ofVec3f &        vMinCorner = mVelGrid.GetMinCorner();
	static const float  nudge = 1.0f - 2.0f * FLT_EPSILON;
	const ofVec3f          vSpacing = mVelGrid.GetCellSpacing() * nudge;
	const size_t      dims[3] = { mVelGrid.GetNumPoints(0)
		, mVelGrid.GetNumPoints(1)
		, mVelGrid.GetNumPoints(2) };
	const size_t      numXY = dims[0] * dims[1];
	size_t            idx[3];
	for (idx[2] = izStart; idx[2] < izEnd; ++idx[2])
	{   // For subset of z index values...
		ofVec3f vPosition;
		// Compute the z-coordinate of the world-space position of this gridpoint.
		vPosition.z = vMinCorner.z + float(idx[2]) * vSpacing.z;
		// Precompute the z contribution to the offset into the velocity grid.
		const size_t offsetZ = idx[2] * numXY;
		for (idx[1] = 0; idx[1] < dims[1]; ++idx[1])
		{   // For every gridpoint along the y-axis...
			// Compute the y-coordinate of the world-space position of this gridpoint.
			vPosition.y = vMinCorner.y + float(idx[1]) * vSpacing.y;
			// Precompute the y contribution to the offset into the velocity grid.
			const size_t offsetYZ = idx[1] * dims[0] + offsetZ;
			for (idx[0] = 0; idx[0] < dims[0]; ++idx[0])
			{   // For every gridpoint along the x-axis...
				// Compute the x-coordinate of the world-space position of this gridpoint.
				vPosition.x = vMinCorner.x + float(idx[0]) * vSpacing.x;
				// Compute the offset into the velocity grid.
				const size_t offsetXYZ = idx[0] + offsetYZ;

				// Compute the fluid flow velocity at this gridpoint, due to all vortons.
#if VELOCITY_FROM_TREE
				static const size_t zeros[3] = { 0 , 0 , 0 }; // Starter indices for recursive algorithm
				mVelGrid[offsetXYZ] = ComputeVelocity(vPosition, zeros, numLayers - 1);
#else   // Slow accurate direct summation algorithm
				mVelGrid[offsetXYZ] = ComputeVelocityBruteForce(vPosition);
#endif
			}
		}
	}
}

/*! \brief Compute velocity due to vortons, for every point in a uniform grid

\see CreateInfluenceTree

\note This routine assumes CreateInfluenceTree has already executed.

*/
void VortonSim::ComputeVelocityGrid(void)
{
	mVelGrid.Clear();                                  // Clear any stale velocity information
	mVelGrid.CopyShape(mInfluenceTree[0]);           // Use same shape as base vorticity grid. (Note: could differ if you want.)
	mVelGrid.Init();                                   // Reserve memory for velocity grid.

	const size_t numZ = mVelGrid.GetNumPoints(2);

#if USE_TBB
	// Estimate grain size based on size of problem and number of processors.
    const size_t grainSize = std::max(size_t(1), numZ / std::thread::hardware_concurrency());
	// Compute velocity grid using multiple threads.
    tbb::parallel_for(tbb::blocked_range<size_t>(0, numZ, grainSize), VortonSim_ComputeVelocityGrid_TBB(this));
#else
	ComputeVelocityGridSlice(0, numZ);
#endif
}

/*! \brief Stretch and tilt vortons using velocity field

\param timeStep - amount of time by which to advance simulation

\param uFrame - frame counter

\see AdvectVortons

\see J. T. Beale, A convergent three-dimensional vortex method with
grid-free stretching, Math. Comp. 46 (1986), 401-24, April.

\note This routine assumes CreateInfluenceTree has already executed.

*/
void VortonSim::StretchAndTiltVortons(const float & timeStep, const size_t & uFrame)
{
	// Compute all gradients of all components of velocity.
	UniformGrid< Mat3 > velocityJacobianGrid(mVelGrid);
	velocityJacobianGrid.Init();
	UniformGridMath::ComputeJacobian(velocityJacobianGrid, mVelGrid);

	if ((0.0f == mVelGrid.GetExtent().x)
		|| (0.0f == mVelGrid.GetExtent().y)
		|| (0.0f == mVelGrid.GetExtent().z))
	{   // Domain is 2D, so stretching & tilting does not occur.
		return;
	}

	const size_t numVortons = mVortons.size();

	for (size_t offset = 0; offset < numVortons; ++offset)
	{   // For each vorton...
		Vorton &    rVorton = mVortons[offset];
		Mat3       velJac;
		velocityJacobianGrid.Interpolate(velJac, rVorton.mPosition);
		const ofVec3f  stretchTilt = velJac * rVorton.mVorticity;    // Usual way to compute stretching & tilting
		rVorton.mVorticity += /* fudge factor for stability */ 0.5f * stretchTilt * timeStep;
	}
}

/*! \brief Diffuse vorticity globally

This uses an extremely crude approximation of viscous diffusion

\param timeStep - amount of time by which to advance simulation

\param uFrame - frame counter

\see StretchAndTiltVortons, AdvectVortons

\note This routine assumes CreateInfluenceTree has already executed.

*/
void VortonSim::DiffuseVorticityGlobally(const float & timeStep, const size_t & uFrame)
{
	const ofVec3f vAvgVorticity = mAverageVorticity;
	mAverageVorticity = ofVec3f(0.0f, 0.0f, 0.0f); // Zero this, which will be used as an accumulator.

	const size_t numVortons = mVortons.size();

	for (size_t offset = 0 /* Start at 0th vorton */; offset < numVortons; ++offset)
	{   // For each vorton...
		Vorton &    rVorton = mVortons[offset];
		ofVec3f &      rVorticitySelf = rVorton.mVorticity;
		// Recompute average vorticity, by summing here, then dividing after loop.
		mAverageVorticity += rVorticitySelf;
		// Bring this vorton's vorticity closer to the average.
		// This effectively exchange vorticity between vortons.
		// This is a non-physical HACK because the exchange occuring
		// here little bearing to physical reality.
		// A more realistic diffusion would exchange vorticity between
		// physically adjacent vortices in proportion to their separation.
		// But this scheme will diffuse vorticity, and this routine does not require adjacency information.
		const ofVec3f  vortDiff = rVorticitySelf - vAvgVorticity;
		const ofVec3f  exchange = mViscosity * timeStep * vortDiff;    // Amount of vorticity to exchange between particles.
		rVorticitySelf -= exchange;    // Make "self" vorticity a little closer to "prev".
	}

	mAverageVorticity /= float(numVortons); // Normalize sum to yield an average, to use in next iteration.
}

/*! \brief Diffuse vorticity using a particle strength exchange method.

This routine partitions space into cells using the same grid
as the "base vorton" grid.  Each vorton gets assigned to the
cell that contains it.  Then, each vorton exchanges some
of its vorticity with its neighbors in adjacent cells.

This routine makes some simplifying assumptions to speed execution:

-   Distance does not influence the amount of vorticity exchanged,
except in as much as only vortons within a certain region of
each other exchange vorticity.  This amounts to saying our kernel,
eta, is a top-hat function.

-   Theoretically, if an adjacent cell contains no vortons
then this simulation should generate vorticity within
that cell, e.g. by creating a new vorton in the adjacent cell.

-   This simulation reduces the vorticity of each vorton, alleging
that this vorticity is dissipated analogously to how energy
dissipates at Kolmogorov microscales.  This treatment is not
realistic but it retains qualitative characteristics that we
want, e.g. that the flow dissipates at a rate related to viscosity.
Dissipation in real flows is a more complicated phenomenon.

\see Degond & Mas-Gallic (1989): The weighted particle method for
convection-diffusion equations, part 1: the case of an isotropic viscosity.
Math. Comput., v. 53, n. 188, pp. 485-507, October.

\param timeStep - amount of time by which to advance simulation

\param uFrame - frame counter

\see StretchAndTiltVortons, AdvectVortons

\note This routine assumes CreateInfluenceTree has already executed.

*/
void VortonSim::DiffuseVorticityPSE(const float & timeStep, const size_t & uFrame)
{
	// Phase 1: Partition vortons

	// Create a spatial partition for the vortons.
	// Each cell contains a dynamic array of integers
	// whose values are offsets into mVortons.
	UniformGrid< std::vector< size_t > > ugVortRef(mInfluenceTree[0]);
	ugVortRef.Init();

	const size_t numVortons = mVortons.size();

	for (size_t offset = 0 /* Start at 0th vorton */; offset < numVortons; ++offset)
	{   // For each vorton...
		Vorton &    rVorton = mVortons[offset];
		// Insert the vorton's offset into the spatial partition.
		ugVortRef[rVorton.mPosition].push_back(offset);
	}

	// Phase 2: Exchange vorticity with nearest neighbors

	const size_t & nx = ugVortRef.GetNumPoints(0);
	const size_t   nxm1 = nx - 1;
	const size_t & ny = ugVortRef.GetNumPoints(1);
	const size_t   nym1 = ny - 1;
	const size_t   nxy = nx * ny;
	const size_t & nz = ugVortRef.GetNumPoints(2);
	const size_t   nzm1 = nz - 1;
	size_t idx[3];

	for (idx[2] = 0; idx[2] < nzm1; ++idx[2])
	{   // For all points along z except the last...
		const size_t offsetZ0 = idx[2] * nxy;
		const size_t offsetZp = (idx[2] + 1) * nxy;
		for (idx[1] = 0; idx[1] < nym1; ++idx[1])
		{   // For all points along y except the last...
			const size_t offsetY0Z0 = idx[1] * nx + offsetZ0;
			const size_t offsetYpZ0 = (idx[1] + 1) * nx + offsetZ0;
			const size_t offsetY0Zp = idx[1] * nx + offsetZp;
			for (idx[0] = 0; idx[0] < nxm1; ++idx[0])
			{   // For all points along x except the last...
				const size_t offsetX0Y0Z0 = idx[0] + offsetY0Z0;
				for (size_t ivHere = 0; ivHere < ugVortRef[offsetX0Y0Z0].size(); ++ivHere)
				{   // For each vorton in this gridcell...
					const size_t &    rVortIdxHere = ugVortRef[offsetX0Y0Z0][ivHere];
					Vorton &            rVortonHere = mVortons[rVortIdxHere];
					ofVec3f &              rVorticityHere = rVortonHere.mVorticity;

					// Diffuse vorticity with other vortons in this same cell:
					for (size_t ivThere = ivHere + 1; ivThere < ugVortRef[offsetX0Y0Z0].size(); ++ivThere)
					{   // For each OTHER vorton within this same cell...
						const size_t &    rVortIdxThere = ugVortRef[offsetX0Y0Z0][ivThere];
						Vorton &            rVortonThere = mVortons[rVortIdxThere];
						ofVec3f &              rVorticityThere = rVortonThere.mVorticity;
						const ofVec3f          vortDiff = rVorticityHere - rVorticityThere;
						const ofVec3f          exchange = 2.0f * mViscosity * timeStep * vortDiff;    // Amount of vorticity to exchange between particles.
						rVorticityHere -= exchange;   // Make "here" vorticity a little closer to "there".
						rVorticityThere += exchange;   // Make "there" vorticity a little closer to "here".
					}

					// Diffuse vorticity with vortons in adjacent cells:
					{
						const size_t offsetXpY0Z0 = idx[0] + 1 + offsetY0Z0; // offset of adjacent cell in +X direction
						for (size_t ivThere = 0; ivThere < ugVortRef[offsetXpY0Z0].size(); ++ivThere)
						{   // For each vorton in the adjacent cell in +X direction...
							const size_t &    rVortIdxThere = ugVortRef[offsetXpY0Z0][ivThere];
							Vorton &            rVortonThere = mVortons[rVortIdxThere];
							ofVec3f &              rVorticityThere = rVortonThere.mVorticity;
							const ofVec3f          vortDiff = rVorticityHere - rVorticityThere;
							const ofVec3f          exchange = mViscosity * timeStep * vortDiff;    // Amount of vorticity to exchange between particles.
							rVorticityHere -= exchange;   // Make "here" vorticity a little closer to "there".
							rVorticityThere += exchange;   // Make "there" vorticity a little closer to "here".
						}
					}

					{
						const size_t offsetX0YpZ0 = idx[0] + offsetYpZ0; // offset of adjacent cell in +Y direction
						for (size_t ivThere = 0; ivThere < ugVortRef[offsetX0YpZ0].size(); ++ivThere)
						{   // For each vorton in the adjacent cell in +Y direction...
							const size_t &    rVortIdxThere = ugVortRef[offsetX0YpZ0][ivThere];
							Vorton &            rVortonThere = mVortons[rVortIdxThere];
							ofVec3f &              rVorticityThere = rVortonThere.mVorticity;
							const ofVec3f          vortDiff = rVorticityHere - rVorticityThere;
							const ofVec3f          exchange = mViscosity * timeStep * vortDiff;    // Amount of vorticity to exchange between particles.
							rVorticityHere -= exchange;   // Make "here" vorticity a little closer to "there".
							rVorticityThere += exchange;   // Make "there" vorticity a little closer to "here".
						}
					}

					{
						const size_t offsetX0Y0Zp = idx[0] + offsetY0Zp; // offset of adjacent cell in +Z direction
						for (size_t ivThere = 0; ivThere < ugVortRef[offsetX0Y0Zp].size(); ++ivThere)
						{   // For each vorton in the adjacent cell in +Z direction...
							const size_t &    rVortIdxThere = ugVortRef[offsetX0Y0Zp][ivThere];
							Vorton &            rVortonThere = mVortons[rVortIdxThere];
							ofVec3f &              rVorticityThere = rVortonThere.mVorticity;
							const ofVec3f          vortDiff = rVorticityHere - rVorticityThere;
							const ofVec3f          exchange = mViscosity * timeStep * vortDiff;    // Amount of vorticity to exchange between particles.
							rVorticityHere -= exchange;   // Make "here" vorticity a little closer to "there".
							rVorticityThere += exchange;   // Make "there" vorticity a little closer to "here".
						}
					}

					// Dissipate vorticity.  See notes in header comment.
					rVorticityHere -= mViscosity * timeStep * rVorticityHere;   // Reduce "here" vorticity.
				}
			}
		}
	}
}

/*! \brief Advect vortons using velocity field

 \param timeStep - amount of time by which to advance simulation

 \see ComputeVelocityGrid

 */
void VortonSim::AdvectVortons(const float & timeStep)
{
	const size_t numVortons = mVortons.size();
#pragma omp parallel for
	for (int offset = 0; offset < numVortons; ++offset)
	{   // For each vorton...
		Vorton & rVorton = mVortons[offset];
		ofVec3f velocity;
		mVelGrid.Interpolate(velocity, rVorton.mPosition);
		rVorton.mPosition += velocity * timeStep;
		rVorton.mVelocity = velocity;  // Cache this for use in collisions with rigid bodies.
	}
}

/*! \brief Advect (subset of) passive tracers using velocity field

 \param timeStep - amount of time by which to advance simulation

 \param uFrame - frame counter

 \param itStart - index of first tracer to advect

 \param itEnd - index of last tracer to advect

 \see AdvectTracers

 */
void VortonSim::AdvectTracersSlice(const float & timeStep, const size_t & uFrame, size_t itStart, size_t itEnd)
{
	for (size_t offset = itStart; offset < itEnd; ++offset)
	{   // For each passive tracer in this slice...
		Particle & rTracer = mTracers.at(offset);
		ofVec3f velocity;
		mVelGrid.Interpolate(velocity, rTracer.mPosition);
		rTracer.mPosition += velocity * timeStep;
		rTracer.mVelocity = velocity; // Cache for use in collisions
	}
}

/*! \brief Advect passive tracers using velocity field

 \param timeStep - amount of time by which to advance simulation

 \param uFrame - frame counter

 \see AdvectVortons

 */
void VortonSim::AdvectTracers(const float & timeStep, const size_t & uFrame)
{
	const size_t numTracers = mTracers.size();

#if USE_TBB
	// Estimate grain size based on size of problem and number of processors.
    const size_t grainSize = std::max(size_t(1), numTracers / std::thread::hardware_concurrency());
	// Advect tracers using multiple threads.
    tbb::parallel_for(tbb::blocked_range<size_t>(0, numTracers, grainSize), VortonSim_AdvectTracers_TBB(this, timeStep, uFrame));
#else
	AdvectTracersSlice(timeStep, uFrame, 0, numTracers);
#endif
}

/*! \brief Update vortex particle fluid simulation to next time.

 \param timeStep - incremental amount of time to step forward

 \param uFrame - frame counter, used to generate files

 */
void VortonSim::Update(float timeStep, size_t uFrame)
{
	//    QUERY_PERFORMANCE_ENTER ;
	CreateInfluenceTree();
	//    QUERY_PERFORMANCE_EXIT( VortonSim_CreateInfluenceTree ) ;

	//    QUERY_PERFORMANCE_ENTER ;
	ComputeVelocityGrid();
	//    QUERY_PERFORMANCE_EXIT( VortonSim_ComputeVelocityGrid ) ;

	//    QUERY_PERFORMANCE_ENTER ;
	StretchAndTiltVortons(timeStep, uFrame);
	//    QUERY_PERFORMANCE_EXIT( VortonSim_StretchAndTiltVortons ) ;

	//    QUERY_PERFORMANCE_ENTER ;
	DiffuseVorticityPSE(timeStep, uFrame);
	//    QUERY_PERFORMANCE_EXIT( VortonSim_DiffuseVorticityPSE ) ;

	//    QUERY_PERFORMANCE_ENTER ;
	AdvectVortons(timeStep);
	//    QUERY_PERFORMANCE_EXIT( VortonSim_AdvectVortons ) ;

	//    QUERY_PERFORMANCE_ENTER ;
	AdvectTracers(timeStep, uFrame);
	//    QUERY_PERFORMANCE_EXIT( VortonSim_AdvectTracers ) ;
}

/*! \brief Initialize passive tracers

 \note This method assumes the influence tree skeleton has already been created,
 and the leaf layer initialized to all "zeros", meaning it contains no
 vortons.
 */
void VortonSim::InitializePassiveTracers(size_t multiplier)
{
	const ofVec3f vSpacing = mInfluenceTree[0].GetCellSpacing();
	// Must keep tracers away from maximal boundary by at least cell.  Note the +vHalfSpacing in loop.
	const size_t begin[3] = {
		1 * mInfluenceTree[0].GetNumCells(0) / 8 ,
		1 * mInfluenceTree[0].GetNumCells(1) / 8 ,
		1 * mInfluenceTree[0].GetNumCells(2) / 8
	};
	const size_t end[3] = {
		7 * mInfluenceTree[0].GetNumCells(0) / 8 ,
		7 * mInfluenceTree[0].GetNumCells(1) / 8 ,
		7 * mInfluenceTree[0].GetNumCells(2) / 8
	};
	const float pclSize = 2.0f * powf(vSpacing.x * vSpacing.y * vSpacing.z, 2.0f / 3.0f) / float(multiplier);
	const ofVec3f noise = vSpacing / float(multiplier);
	size_t idx[3];

	const size_t nt[3] = { multiplier , multiplier , multiplier };

	for (idx[2] = begin[2]; idx[2] <= end[2]; ++idx[2])
		for (idx[1] = begin[1]; idx[1] <= end[1]; ++idx[1])
			for (idx[0] = begin[0]; idx[0] <= end[0]; ++idx[0])
			{   // For each interior grid cell...
				ofVec3f vPosMinCorner;
				mInfluenceTree[0].PositionFromIndices(vPosMinCorner, idx);
				Particle pcl;
				pcl.mVelocity = ofVec3f(0.0f, 0.0f, 0.0f);
				pcl.mOrientation = ofVec3f(0.0f, 0.0f, 0.0f);
				pcl.mAngularVelocity = ofVec3f(0.0f, 0.0f, 0.0f);
				pcl.mMass = 1.0f;
				pcl.mSize = pclSize;
				pcl.mBirthTime = 0;

				size_t it[3];
				for (it[2] = 0; it[2] < nt[2]; ++it[2])
					for (it[1] = 0; it[1] < nt[1]; ++it[1])
						for (it[0] = 0; it[0] < nt[0]; ++it[0])
						{
							ofVec3f vShift(float(it[0]) / float(nt[0]) * vSpacing.x,
								float(it[1]) / float(nt[1]) * vSpacing.y,
								float(it[2]) / float(nt[2]) * vSpacing.z);
							pcl.mPosition = vPosMinCorner + vShift + RandomSpread(noise);
							mTracers.push_back(pcl);
						}
			}
}

const ofVec3f VortonSim::GetTracerCenterOfMass() const
{
	ofVec3f vCoM(0.0f, 0.0f, 0.0f);
	const size_t & numTracers = mTracers.size();
	for (size_t iTracer = 0; iTracer < numTracers; ++iTracer)
	{
		const Particle & pcl = mTracers[iTracer];
		vCoM += pcl.mPosition;
	}
	vCoM /= float(numTracers);
	return vCoM;
}
