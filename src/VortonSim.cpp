#include "VortonSim.hpp"
#include "VortonClusterAux.hpp"

/*! \brief Update axis-aligned bounding box corners to include given point
 
 \param vMinCorner - minimal corner of axis-aligned bounding box
 
 \param vMaxCorner - maximal corner of axis-aligned bounding box
 
 \param vPoint - point to include in bounding box
 
 */
void UpdateBoundingBox( ofVec3f & vMinCorner , ofVec3f & vMaxCorner , const ofVec3f & vPoint ) {
    vMinCorner.x = std::min( vPoint.x , vMinCorner.x ) ;
    vMinCorner.y = std::min( vPoint.y , vMinCorner.y ) ;
    vMinCorner.z = std::min( vPoint.z , vMinCorner.z ) ;
    vMaxCorner.x = std::max( vPoint.x , vMaxCorner.x ) ;
    vMaxCorner.y = std::max( vPoint.y , vMaxCorner.y ) ;
    vMaxCorner.z = std::max( vPoint.z , vMaxCorner.z ) ;
}

void VortonSim::Initialize(size_t numTracersPerCellCubeRoot) {
    ConservedQuantities( mCirculationInitial , mLinearImpulseInitial ) ;
    ComputeAverageVorticity() ;
    CreateInfluenceTree() ; // This is a marginally superfluous call.  We only need the grid geometry to seed passive tracer particles.
    InitializePassiveTracers( numTracersPerCellCubeRoot ) ;
    
    {
        float domainVolume = mInfluenceTree[0].GetExtent().x * mInfluenceTree[0].GetExtent().y * mInfluenceTree[0].GetExtent().z ;
        if( 0.0f == mInfluenceTree[0].GetExtent().z )
        {   // Domain is 2D in XY plane.
            domainVolume = mInfluenceTree[0].GetExtent().x * mInfluenceTree[0].GetExtent().y ;
        }
        const float totalMass = domainVolume * mFluidDensity ;
        const size_t numTracersPerCell = numTracersPerCellCubeRoot * numTracersPerCellCubeRoot * numTracersPerCellCubeRoot ;
        mMassPerParticle = totalMass / float( mInfluenceTree[0].GetGridCapacity() * numTracersPerCell ) ;
    }
}

/*! \brief Assign vortons from a uniform grid of vorticity
 
 \param vortGrid - uniform grid of vorticity values
 
 */
void VortonSim::AssignVortonsFromVorticity( UniformGrid< ofVec3f > & vortGrid ) {
    mVortons.clear() ; // Empty out any existing vortons.
    
    // Obtain characteristic size of each grid cell.
    
    const UniformGridGeometry & ug  = vortGrid ;
    const float     fVortonRadius   = powf( ug.GetCellSpacing().x * ug.GetCellSpacing().y  * ug.GetCellSpacing().z , 1.0f / 3.0f ) * 0.5f ;
    const ofVec3f      Nudge           ( ug.GetExtent() * FLT_EPSILON * 4.0f ) ;
    const ofVec3f      vMin            ( ug.GetMinCorner()   + Nudge ) ;
    const ofVec3f      vSpacing        ( ug.GetCellSpacing() * ( 1.0f - 0.0f * FLT_EPSILON ) ) ;
    const size_t  numPoints[3]    = { ug.GetNumPoints(0) , ug.GetNumPoints(1) , ug.GetNumPoints(2) } ;
    const size_t  numXY           = numPoints[0] * numPoints[1] ;
    size_t idx[3] ;
    for( idx[2] = 0 ; idx[2] < numPoints[2] ; ++ idx[2] )
    {
        ofVec3f vPositionOfGridCellCenter ;
        vPositionOfGridCellCenter.z = vMin.z + float( idx[2] ) * vSpacing.z ;
        const size_t offsetZ = idx[2] * numXY ;
        for( idx[1] = 0 ; idx[1] < numPoints[1] ; ++ idx[1] )
        {
            vPositionOfGridCellCenter.y = vMin.y + float( idx[1] ) * vSpacing.y ;
            const size_t offsetYZ = idx[1] * vortGrid.GetNumPoints(0) + offsetZ ;
            for( idx[0] = 0 ; idx[0] < numPoints[0] ; ++ idx[0] )
            {
                vPositionOfGridCellCenter.x = vMin.x + float( idx[0] ) * vSpacing.x ;
                const size_t offsetXYZ = idx[0] + offsetYZ ;
                const ofVec3f & rVort = vortGrid[ offsetXYZ ] ;
                if( rVort.lengthSquared() > FLT_EPSILON )
                {   // This grid cell contains significant vorticity.
                    mVortons.emplace_back( vPositionOfGridCellCenter , rVort , fVortonRadius ) ;
                }
            }
        }
    }
}

/*! \brief Compute the total circulation and linear impulse of all vortons in this simulation.
 
 \param vCirculation - Total circulation, the volume integral of vorticity, computed by this routine.
 
 \param vLinearImpulse - Volume integral of circulation weighted by position, computed by this routine.
 
 */
void    VortonSim::ConservedQuantities( ofVec3f & vCirculation , ofVec3f & vLinearImpulse ) const {
    // Zero accumulators.
    vCirculation = vLinearImpulse = ofVec3f( 0.0f , 0.0f , 0.0f ) ;
    const size_t numVortons = mVortons.size() ;
    for( size_t iVorton = 0 ; iVorton < numVortons ; ++ iVorton )
    {   // For each vorton in this simulation...
        const Vorton &  rVorton         = mVortons[ iVorton ] ;
        const float     volumeElement   = ( rVorton.mRadius * rVorton.mRadius * rVorton.mRadius ) * 8.0f ;
        // Accumulate total circulation.
        vCirculation    += rVorton.mVorticity * volumeElement ;
        // Accumulate total linear impulse.
        vLinearImpulse  += rVorton.mPosition.getCrossed(rVorton.mVorticity) * volumeElement ;
    }
}

/*! \brief Compute the average vorticity of all vortons in this simulation.
 
 \note This is used to compute a hacky, non-physical approximation to
 viscous vortex diffusion.
 
 */
void VortonSim::ComputeAverageVorticity() {
    // Zero accumulators.
    mAverageVorticity = ofVec3f( 0.0f , 0.0f , 0.0f ) ;
    const size_t numVortons = mVortons.size() ;
    for( size_t iVorton = 0 ; iVorton < numVortons ; ++ iVorton )
    {   // For each vorton in this simulation...
        const Vorton &  rVorton         = mVortons[ iVorton ] ;
        mAverageVorticity += rVorton.mVorticity ;
    }
    mAverageVorticity /= float( numVortons ) ;
}

/// Find axis-aligned bounding box for all vortons in this simulation.
void VortonSim::FindBoundingBox( )
{
//    QUERY_PERFORMANCE_ENTER ;
    const size_t numVortons = mVortons.size() ;
    mMinCorner.x = mMinCorner.y = mMinCorner.z =   FLT_MAX ;
    mMaxCorner = - mMinCorner ;
    for( size_t iVorton = 0 ; iVorton < numVortons ; ++ iVorton )
    {   // For each vorton in this simulation...
        const Vorton & rVorton = mVortons[ iVorton ] ;
        // Find corners of axis-aligned bounding box.
        UpdateBoundingBox( mMinCorner , mMaxCorner , rVorton.mPosition ) ;
    }
//    QUERY_PERFORMANCE_EXIT( VortonSim_CreateInfluenceTree_FindBoundingBox_Vortons ) ;
    
//    QUERY_PERFORMANCE_ENTER ;
    const size_t numTracers = mTracers.size() ;
    for( size_t iTracer = 0 ; iTracer < numTracers ; ++ iTracer )
    {   // For each passive tracer particle in this simulation...
        const Particle & rTracer = mTracers[ iTracer ] ;
        // Find corners of axis-aligned bounding box.
        UpdateBoundingBox( mMinCorner , mMaxCorner , rTracer.mPosition ) ;
    }
//    QUERY_PERFORMANCE_EXIT( VortonSim_CreateInfluenceTree_FindBoundingBox_Tracers ) ;
    
    // Slightly enlarge bounding box to allow for round-off errors.
    const ofVec3f extent( mMaxCorner - mMinCorner ) ;
    const ofVec3f nudge( extent * FLT_EPSILON ) ;
    mMinCorner -= nudge ;
    mMaxCorner += nudge ;
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
void VortonSim::MakeBaseVortonGrid( void )
{
    const size_t numVortons = mVortons.size() ;
    
    UniformGrid< VortonClusterAux > ugAux( mInfluenceTree[0] ) ; // Temporary auxilliary information used during aggregation.
    ugAux.Init() ;
    
    // Compute preliminary vorticity grid.
    for( size_t uVorton = 0 ; uVorton < numVortons ; ++ uVorton )
    {   // For each vorton in this simulation...
        const Vorton     &  rVorton     = mVortons[ uVorton ] ;
        const ofVec3f    &  rPosition   = rVorton.mPosition   ;
        const size_t      uOffset     = mInfluenceTree[0].OffsetOfPosition( rPosition ) ;
        Vorton           &  rVortonCell = mInfluenceTree[0][ uOffset ] ;
        VortonClusterAux &  rVortonAux  = ugAux[ uOffset ] ;
        const float         vortMag     = rVorton.mVorticity.length() ;
        
        rVortonCell.mPosition  += rVorton.mPosition * vortMag ; // Compute weighted position -- to be normalized later.
        rVortonCell.mVorticity += rVorton.mVorticity          ; // Tally vorticity sum.
        rVortonCell.mRadius     = rVorton.mRadius             ; // Assign volume element size.
                                                                // OBSOLETE. See comments below: UpdateBoundingBox( rVortonAux.mMinCorner , rVortonAux.mMaxCorner , rVorton.mPosition ) ;
        rVortonAux.mVortNormSum += vortMag ;
    }
    
    // Post-process preliminary grid; normalize center-of-vorticity and compute sizes, for each grid cell.
    const size_t num[3] = {
        mInfluenceTree[0].GetNumPoints( 0 ) ,
        mInfluenceTree[0].GetNumPoints( 1 ) ,
        mInfluenceTree[0].GetNumPoints( 2 )
    } ;
    const size_t numXY = num[0] * num[1] ;
    size_t idx[3] ;
    for( idx[2] = 0 ; idx[2] < num[2] ; ++ idx[2] )
    {
        const size_t zShift = idx[2] * numXY ;
        for( idx[1] = 0 ; idx[1] < num[1] ; ++ idx[1] )
        {
            const size_t yzShift = idx[1] * num[0] + zShift ;
            for( idx[0] = 0 ; idx[0] < num[0] ; ++ idx[0] )
            {
                const size_t      offset      = idx[0] + yzShift ;
                VortonClusterAux &  rVortonAux  = ugAux[ offset ] ;
                if( rVortonAux.mVortNormSum != FLT_MIN )
                {   // This cell contains at least one vorton.
                    Vorton & rVortonCell = mInfluenceTree[0][ offset ] ;
                    // Normalize weighted position sum to obtain center-of-vorticity.
                    rVortonCell.mPosition /= rVortonAux.mVortNormSum ;
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
void VortonSim::AggregateClusters( size_t uParentLayer )
{
    UniformGrid<Vorton> & rParentLayer  = mInfluenceTree[ uParentLayer ] ;
    
    // number of cells in each grid cluster
    const size_t * const pClusterDims = mInfluenceTree.GetDecimations( uParentLayer ) ;
    
    const size_t  numCells[3]         = { rParentLayer.GetNumCells( 0 ) , rParentLayer.GetNumCells( 1 ) , rParentLayer.GetNumCells( 2 ) } ;
    const size_t  numXY               = rParentLayer.GetNumPoints( 0 ) * rParentLayer.GetNumPoints( 1 ) ;
    // (Since this loop writes to each parent cell, it should readily parallelize without contention.)
    size_t idxParent[3] ;
    for( idxParent[2] = 0 ; idxParent[2] < numCells[2] ; ++ idxParent[2] )
    {
        const size_t offsetZ = idxParent[2] * numXY ;
        for( idxParent[1] = 0 ; idxParent[1] < numCells[1] ; ++ idxParent[1] )
        {
            const size_t offsetYZ = idxParent[1] * rParentLayer.GetNumPoints( 0 ) + offsetZ ;
            for( idxParent[0] = 0 ; idxParent[0] < numCells[0] ; ++ idxParent[0] )
            {   // For each cell in the parent layer...
                const size_t offsetXYZ = idxParent[0] + offsetYZ ;
                UniformGrid<Vorton> & rChildLayer   = mInfluenceTree[ uParentLayer - 1 ] ;
                Vorton              & rVortonParent = rParentLayer[ offsetXYZ ] ;
                VortonClusterAux vortAux ;
                size_t clusterMinIndices[ 3 ] ;
                mInfluenceTree.GetChildClusterMinCornerIndex( clusterMinIndices , pClusterDims , idxParent ) ;
                size_t increment[3] = { 0 , 0 , 0 } ;
                const size_t & numXchild  = rChildLayer.GetNumPoints( 0 ) ;
                const size_t   numXYchild = numXchild * rChildLayer.GetNumPoints( 1 ) ;
                // For each cell of child layer in this grid cluster...
                for( increment[2] = 0 ; increment[2] < pClusterDims[2] ; ++ increment[2] )
                {
                    const size_t offsetZ = ( clusterMinIndices[2] + increment[2] ) * numXYchild ;
                    for( increment[1] = 0 ; increment[1] < pClusterDims[1] ; ++ increment[1] )
                    {
                        const size_t offsetYZ = ( clusterMinIndices[1] + increment[1] ) * numXchild + offsetZ ;
                        for( increment[0] = 0 ; increment[0] < pClusterDims[0] ; ++ increment[0] )
                        {
                            const size_t  offsetXYZ       = ( clusterMinIndices[0] + increment[0] ) + offsetYZ ;
                            Vorton &        rVortonChild    = rChildLayer[ offsetXYZ ] ;
                            const float     vortMag         = rVortonChild.mVorticity.length() ;
                            
                            // Aggregate vorton cluster from child layer into parent layer:
                            rVortonParent.mPosition  += rVortonChild.mPosition * vortMag ;
                            rVortonParent.mVorticity += rVortonChild.mVorticity ;
                            vortAux.mVortNormSum     += vortMag ;
                            if( rVortonChild.mRadius != 0.0f )
                            {
                                rVortonParent.mRadius  = rVortonChild.mRadius ;
                            }
                        }
                    }
                }
                // Normalize weighted position sum to obtain center-of-vorticity.
                // (See analogous code in MakeBaseVortonGrid.)
                rVortonParent.mPosition /= vortAux.mVortNormSum ;
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
void VortonSim::CreateInfluenceTree( void )
{
//    QUERY_PERFORMANCE_ENTER ;
    FindBoundingBox() ; // Find axis-aligned bounding box that encloses all vortons.
//    QUERY_PERFORMANCE_EXIT( VortonSim_CreateInfluenceTree_FindBoundingBox ) ;
    
    // Create skeletal nested grid for influence tree.
    const size_t numVortons = mVortons.Size() ;
    {
        UniformGrid< Vorton >   ugSkeleton ;   ///< Uniform grid with the same size & shape as the one holding aggregated information about mVortons.
        ugSkeleton.DefineShape( numVortons , mMinCorner , mMaxCorner , true ) ;
        mInfluenceTree.Initialize( ugSkeleton ) ; // Create skeleton of influence tree.
    }
    
//    QUERY_PERFORMANCE_ENTER ;
    MakeBaseVortonGrid() ;
//    QUERY_PERFORMANCE_EXIT( VortonSim_CreateInfluenceTree_MakeBaseVortonGrid ) ;
    
//    QUERY_PERFORMANCE_ENTER ;
    const size_t numLayers = mInfluenceTree.GetDepth() ;
    for( size_t int uParentLayer = 1 ; uParentLayer < numLayers ; ++ uParentLayer )
    {   // For each layer in the influence tree...
        AggregateClusters( uParentLayer ) ;
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
ofVec3f VortonSim::ComputeVelocity( const ofVec3f & vPosition , const size_t indices[3] , size_t iLayer )
{
    UniformGrid< Vorton > & rChildLayer             = mInfluenceTree[ iLayer - 1 ] ;
    size_t                clusterMinIndices[3] ;
    const size_t *        pClusterDims             = mInfluenceTree.GetDecimations( iLayer ) ;
    mInfluenceTree.GetChildClusterMinCornerIndex( clusterMinIndices , pClusterDims , indices ) ;
    
    const ofVec3f &            vGridMinCorner          = rChildLayer.GetMinCorner() ;
    const ofVec3f              vSpacing                = rChildLayer.GetCellSpacing() ;
    size_t                increment[3]            ;
    const size_t &        numXchild               = rChildLayer.GetNumPoints( 0 ) ;
    const size_t          numXYchild              = numXchild * rChildLayer.GetNumPoints( 1 ) ;
    ofVec3f                    velocityAccumulator( 0.0f , 0.0f , 0.0f ) ;
    
    // The larger this is, the more accurate (and slower) the evaluation.
    // Reasonable values lie in [0.00001,4.0].
    // Setting this to 0 leads to very bad errors, but values greater than (tiny) lead to drastic improvements.
    // Changes in margin have a quantized effect since they effectively indicate how many additional
    // cluster subdivisions to visit.
    static const float  marginFactor    = 0.0001f ; // 0.4f ; // ship with this number: 0.0001f ; test with 0.4
                                                    // When domain is 2D in XY plane, min.z==max.z so vPos.z test below would fail unless margin.z!=0.
    const ofVec3f          margin          = marginFactor * vSpacing + ( 0.0f == vSpacing.z ? ofVec3f(0,0,FLT_MIN) : ofVec3f(0,0,0) );
    
    // For each cell of child layer in this grid cluster...
    for( increment[2] = 0 ; increment[2] < pClusterDims[2] ; ++ increment[2] )
    {
        size_t idxChild[3] ;
        idxChild[2] = clusterMinIndices[2] + increment[2] ;
        ofVec3f vCellMinCorner , vCellMaxCorner ;
        vCellMinCorner.z = vGridMinCorner.z + float( idxChild[2]     ) * vSpacing.z ;
        vCellMaxCorner.z = vGridMinCorner.z + float( idxChild[2] + 1 ) * vSpacing.z ;
        const size_t offsetZ = idxChild[2] * numXYchild ;
        for( increment[1] = 0 ; increment[1] < pClusterDims[1] ; ++ increment[1] )
        {
            idxChild[1] = clusterMinIndices[1] + increment[1] ;
            vCellMinCorner.y = vGridMinCorner.y + float( idxChild[1]     ) * vSpacing.y ;
            vCellMaxCorner.y = vGridMinCorner.y + float( idxChild[1] + 1 ) * vSpacing.y ;
            const size_t offsetYZ = idxChild[1] * numXchild + offsetZ ;
            for( increment[0] = 0 ; increment[0] < pClusterDims[0] ; ++ increment[0] )
            {
                idxChild[0] = clusterMinIndices[0] + increment[0] ;
                vCellMinCorner.x = vGridMinCorner.x + float( idxChild[0]     ) * vSpacing.x ;
                vCellMaxCorner.x = vGridMinCorner.x + float( idxChild[0] + 1 ) * vSpacing.x ;
                if(
                   ( iLayer > 1 )
                   &&  ( vPosition.x >= vCellMinCorner.x - margin.x )
                   &&  ( vPosition.y >= vCellMinCorner.y - margin.y )
                   &&  ( vPosition.z >= vCellMinCorner.z - margin.z )
                   &&  ( vPosition.x <  vCellMaxCorner.x + margin.x )
                   &&  ( vPosition.y <  vCellMaxCorner.y + margin.y )
                   &&  ( vPosition.z <  vCellMaxCorner.z + margin.z )
                   )
                {   // Test position is inside childCell and currentLayer > 0...
                    // Recurse child layer.
                    velocityAccumulator += ComputeVelocity( vPosition , idxChild , iLayer - 1 ) ;
                }
                else
                {   // Test position is outside childCell, or reached leaf node.
                    //    Compute velocity induced by cell at corner point x.
                    //    Accumulate influence, storing in velocityAccumulator.
                    const size_t  offsetXYZ       = idxChild[0] + offsetYZ ;
                    const Vorton &  rVortonChild    = rChildLayer[ offsetXYZ ] ;
                    VORTON_ACCUMULATE_VELOCITY( velocityAccumulator , vPosition , rVortonChild ) ;
                }
            }
        }
    }
    
    return velocityAccumulator ;
}
