#include "Vorton.hpp"
#include "math_helper.hpp"

static const float  sAvoidSingularity   = powf( FLT_MIN , 1.0f / 3.0f ) ;

Vorton::Vorton() :
mPosition(0), mVorticity(1.f), mRadius(1.f), mVelocity(0.f)
{
	
}

Vorton::Vorton(ofVec3f pos, ofVec3f vorticity, float rad, ofVec2f velocity) :
mPosition(pos), mVorticity(vorticity), mRadius(rad), mVelocity(velocity)
{
}

Vorton::Vorton(const Vorton & other) :
mPosition(other.mPosition), mVorticity(other.mVorticity),
mRadius(other.mRadius), mVelocity(other.mVelocity)
{
}

Vorton::~Vorton() {
	
}

void Vorton::AccumulateVelocity(ofVec3f &velocityOut, const ofVec3f &posQuery) {
    static float oneOverFourPi = 1.f / (FOUR_PI);
    
    const ofVec3f          vNeighborToSelf     = posQuery - mPosition;
    const float         radius2             = mRadius * mRadius;
    const float         dist2               = vNeighborToSelf.lengthSquared() + sAvoidSingularity;
    const float         oneOverDist         = finvsqrtf( dist2 );
//    const ofVec3f          vNeighborToSelfDir  = vNeighborToSelf * oneOverDist;
    /* If the reciprocal law is used everywhere then when 2 vortices get close, they tend to jettison. */
    /* Mitigate this by using a linear law when 2 vortices get close to each other. */
    const float distLaw = ( dist2 < radius2 )
    ?   /* Inside vortex core */
    ( oneOverDist / radius2 )
    :   /* Outside vortex core */
    ( oneOverDist / dist2 );
    velocityOut +=  (oneOverFourPi * ( 8.0f * radius2 * mRadius ) * mVorticity).getCrossed(vNeighborToSelf) * distLaw;

}

void Vorton::AssignByVelocity(const ofVec3f &queryPosition, const ofVec3f velocity) {
    const ofVec3f  posRelative    = queryPosition - mPosition;
    const float dist            = posRelative.length();
    
    mVorticity = (FOUR_PI * dist * posRelative).getCrossed(velocity) / ( 8.0f * mRadius * mRadius * mRadius ) ;
}
