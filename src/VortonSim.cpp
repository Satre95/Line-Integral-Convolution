#include "VortonSim.hpp"

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
