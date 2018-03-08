#include "VorticityDistribution.hpp"
#include "ofConstants.h"
#include "math_helper.hpp"

/// \brief A very small number, between FLT_EPSILON and FLT_MIN.
static const float sTiny = expf( 0.5f * ( logf( FLT_EPSILON ) + logf( FLT_MIN ) ) ) ;

void VortexSheet::AssignVorticity(ofVec3f &vorticity, const ofVec3f &position, const ofVec3f &vCenter) const {
    const float yOverWidth = position.y / mWidth ;
    const float d = 1.0f - 0.5f * mVariation * ( cosf( TWO_PI * yOverWidth ) - 1.0f ) ;
    const float zOverD = position.z / d ;
    vorticity.x = 0.0f ;
    const float s = sechf( zOverD ) ;
    vorticity.y = (s * s) / d ;
    const float t = tanhf( zOverD ) ;
    vorticity.z = t * t * PI * mVariation * zOverD / ( mWidth * d ) * sinf( TWO_PI * yOverWidth ) ;
    if( vorticity.lengthSquared() < 0.01f ) {
        // When vorticity is small, force it to zero, to keep number of vortons down.
        vorticity = ofVec3f( 0.0f , 0.0f , 0.0f ) ;
    }
}


void AssignVorticity( std::vector<Vorton> & vortons , float fMagnitude , size_t numVortonsMax , const VorticityDistribution & vorticityDistribution ){
    const static size_t one = 1;
    const ofVec3f          vDimensions     = vorticityDistribution.GetDomainSize() ;       // length of each side of grid box
    const ofVec3f          vCenter         ( 0.0f , 0.0f , 0.0f ) ;                        // Center of vorticity distribution
    const ofVec3f          vMin            ( vCenter - 0.5f * vDimensions ) ;              // Minimum corner of box containing vortons
    const ofVec3f          vMax            ( vMin + vDimensions ) ;                        // Maximum corner of box containing vortons
    UniformGridGeometry skeleton        ( numVortonsMax , vMin , vMax , true ) ;
    size_t            numCells[3]     = {     std::max( one , skeleton.GetNumCells(0))
        ,   std::max( one , skeleton.GetNumCells(1))
        ,   std::max( one , skeleton.GetNumCells(2)) } ;  // number of grid cells in each direction of virtual uniform grid
    
    // Total number of cells should be as close to numVortonsMax as possible without going over.
    // Worst case allowable difference would be numVortonsMax=7 and numCells in each direction is 1 which yields a ratio of 1/7.
    // But in typical situations, the user would like expect total number of virtual cells to be closer to numVortonsMax than that.
    // E.g. if numVortonsMax=8^3=512 somehow yielded numCells[0]=numCells[1]=numCells[2]=7 then the ratio would be 343/512~=0.67.
    while( numCells[0] * numCells[1] * numCells[2] > numVortonsMax )
    {   // Number of cells is excessive.
        // This can happen when the trial number of cells in any direction is less than 1 -- then the other two will likely be too large.
        numCells[0] = std::max( one , numCells[0] / 2 ) ;
        numCells[1] = std::max( one , numCells[1] / 2 ) ;
        numCells[2] = std::max( one , numCells[2] / 2 ) ;
    }
    
    const float     oneOverN[3]     = { 1.0f / float( numCells[0] ) , 1.0f / float( numCells[1] ) , 1.0f / float( numCells[2] ) } ;
    const ofVec3f   gridCellSize    ( vDimensions.x * oneOverN[0] , vDimensions.y * oneOverN[1] , vDimensions.z * oneOverN[2] ) ;
    float           vortonRadius    = powf( gridCellSize.x * gridCellSize.y * gridCellSize.z , 1.0f / 3.0f ) * 0.5f ;
    if( 0.0f == vDimensions.z ) {
        // z size is zero, so domain is 2D.
        vortonRadius = powf( gridCellSize.x * gridCellSize.y , 0.5f ) * 0.5f ;
    }
    const ofVec3f  vNoise          ( 0.0f * gridCellSize ) ;
    
    ofVec3f position = ofVec3f( 0.0f , 0.0f , 0.0f ) ; // vorton position
    size_t index[3];   // index of each position visited
                       // Iterate through each point in a uniform grid.
                       // If probe position is inside vortex core, add a vorton there.
                       // This loop could be rewritten such that it only visits points inside the core,
                       // but this loop structure can readily be reused for a wide variety of configurations.
    for( index[2] = 0 ; index[2] < numCells[2] ; ++ index[2] )
    {   // For each z-coordinate...
        position.z = ( float( index[2] ) + 0.25f ) * gridCellSize.z + vMin.z ;
        for( index[1] = 0 ; index[1] < numCells[1] ; ++ index[1] )
        {   // For each y-coordinate...
            position.y = ( float( index[1] ) + 0.25f ) * gridCellSize.y + vMin.y ;
            for( index[0] = 0 ; index[0] < numCells[0] ; ++ index[0] )
            {   // For each x-coordinate...
                position.x = ( float( index[0] ) + 0.25f ) * gridCellSize.x + vMin.x ;
                position += RandomSpread( vNoise ) ;
                ofVec3f vorticity ;
                vorticityDistribution.AssignVorticity( vorticity , position , vCenter ) ;
                Vorton vorton( position , vorticity * fMagnitude , vortonRadius ) ;
                if( vorticity.lengthSquared() > sTiny )
                {   // Vorticity is significantly non-zero.
                    vortons.push_back( vorton ) ;
                }
            }
        }
    }
}

