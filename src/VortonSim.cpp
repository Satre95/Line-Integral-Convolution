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
