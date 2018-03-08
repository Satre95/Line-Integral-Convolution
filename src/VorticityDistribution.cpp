#include "VorticityDistribution.hpp"
#include "ofConstants.h"
#include "math_helper.hpp"

void VortexSheet::AssignVorticity(ofVec3f &vorticity, const ofVec3f &position, const ofVec3f &vCenter) const {
    const float yOverWidth = position.y / mWidth ;
    const float d = 1.0f - 0.5f * mVariation * ( cosf( TWO_PI * yOverWidth ) - 1.0f ) ;
    const float zOverD = position.z / d ;
    vorticity.x = 0.0f ;
    const float s = sechf( zOverD ) ;
    vorticity.y = s * s / d ;
    const float t = tanhf( zOverD ) ;
    vorticity.z = t * t * PI * mVariation * zOverD / ( mWidth * d ) * sinf( TWO_PI * yOverWidth ) ;
    if( vorticity.lengthSquared() < 0.01f ) {
        // When vorticity is small, force it to zero, to keep number of vortons down.
        vorticity = ofVec3f( 0.0f , 0.0f , 0.0f ) ;
    }
}
