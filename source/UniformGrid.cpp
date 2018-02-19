#include "UniformGrid.hpp"

using namespace cinder;

void UniformGridGeometry::DefineShape( size_t uNumElements , const vec3 & vMin , const vec3 & vMax , bool bPowerOf2 )
{
    mMinCorner  = vMin ;
    static const float Nudge = 1.0f + FLT_EPSILON ;  // slightly expand size to ensure robust containment even with roundoff
    mGridExtent     = ( vMax - vMin ) * Nudge ;
    
    vec3 vSizeEffective( GetExtent() ) ;
    int numDims = 3 ;   // Number of dimensions to region.
    if( 0.0f == vSizeEffective.x )
    {   // X size is zero so reduce dimensionality
        vSizeEffective.x = 1.0f ; // This component will not contribute to the total region volume/area/length.
        mGridExtent.x = 0.0f ;
        -- numDims ;
    }
    if( 0.0f == vSizeEffective.y )
    {   // Y size is zero so reduce dimensionality
        vSizeEffective.y = 1.0f ; // This component will not contribute to the total region volume/area/length.
        mGridExtent.y = 0.0f ;
        -- numDims ;
    }
    if( 0.0f == vSizeEffective.z )
    {   // Z size is zero so reduce dimensionality
        vSizeEffective.z = 1.0f ; // This component will not contribute to the total region volume/area/length.
        mGridExtent.z = 0.0f ;
        -- numDims ;
    }
    
    // Compute region volume, area or length (depending on dimensionality).
    const float volume              = vSizeEffective.x * vSizeEffective.y * vSizeEffective.z ;
    const float cellVolumeCubeRoot  = powf( volume / float( uNumElements ) , -1.0f / float( numDims ) ) ; // Approximate size of each cell in grid.
    // Compute number of cells in each direction of uniform grid.
    // Choose grid dimensions to fit as well as possible, so that the total number
    // of grid cells is nearly the total number of elements in the contents.
    unsigned numCells[3] = { MAX2( 1 , unsigned( GetExtent().x * cellVolumeCubeRoot + 0.5f ) ) ,
        MAX2( 1 , unsigned( GetExtent().y * cellVolumeCubeRoot + 0.5f ) ) ,
        MAX2( 1 , unsigned( GetExtent().z * cellVolumeCubeRoot + 0.5f ) ) } ;
    
    if( bPowerOf2 )
    {   // Choose number of gridcells to be powers of 2.
        // This will simplify subdivision in a NestedGrid.
        numCells[ 0 ] = NearestPowerOfTwo( numCells[ 0 ] ) ;
        numCells[ 1 ] = NearestPowerOfTwo( numCells[ 1 ] ) ;
        numCells[ 2 ] = NearestPowerOfTwo( numCells[ 2 ] ) ;
    }
    
    while( numCells[ 0 ] * numCells[ 1 ] * numCells[ 2 ] >= uNumElements * 8 )
    {   // Grid capacity is excessive.
        // This can occur when the trial numCells is below 0.5 in which case the integer arithmetic loses the subtlety.
        numCells[ 0 ] = MAX2( 1 , numCells[0] / 2 ) ;
        numCells[ 1 ] = MAX2( 1 , numCells[1] / 2 ) ;
        numCells[ 2 ] = MAX2( 1 , numCells[2] / 2 ) ;
    }
    mNumPoints[ 0 ] = numCells[ 0 ] + 1 ; // Increment to obtain number of points.
    mNumPoints[ 1 ] = numCells[ 1 ] + 1 ; // Increment to obtain number of points.
    mNumPoints[ 2 ] = numCells[ 2 ] + 1 ; // Increment to obtain number of points.
    
    PrecomputeSpacing() ;
}
