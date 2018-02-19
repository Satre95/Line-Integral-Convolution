#include "Vorton.hpp"
#include "math_helper.hpp"

using namespace cinder;
static const float  sAvoidSingularity   = powf( FLT_MIN , 1.0f / 3.0f ) ;

Vorton::Vorton() :
mPosition(0), mVorticity(1.f), mRadius(1.f), mVelocity(0.f)
{
	
}

Vorton::Vorton(vec3 pos, vec3 vorticity, float rad, vec2 velocity) :
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

void Vorton::AccumulateVelocity(vec3 &velocityOut, const vec3 &posQuery) {
    static float oneOverFourPi = 1.f / (4.f * glm::pi<float>());
    
    const vec3          vNeighborToSelf     = posQuery - mPosition;
    const float         radius2             = mRadius * mRadius;
    const float         dist2               = glm::length2(vNeighborToSelf) + sAvoidSingularity;
    const float         oneOverDist         = finvsqrtf( dist2 );
//    const vec3          vNeighborToSelfDir  = vNeighborToSelf * oneOverDist;
    /* If the reciprocal law is used everywhere then when 2 vortices get close, they tend to jettison. */
    /* Mitigate this by using a linear law when 2 vortices get close to each other. */
    const float distLaw = ( dist2 < radius2 )
    ?   /* Inside vortex core */
    ( oneOverDist / radius2 )
    :   /* Outside vortex core */
    ( oneOverDist / dist2 );
    velocityOut +=  glm::cross(oneOverFourPi * ( 8.0f * radius2 * mRadius ) * mVorticity, vNeighborToSelf) * distLaw ;   \

}

void Vorton::AssignByVelocity(const vec3 &queryPosition, const vec3 velocity) {
    const vec3  posRelative    = queryPosition - mPosition;
    const float dist            = glm::length(posRelative);
    static float fourPi = 4.f * glm::pi<float>();
    
    mVorticity = glm::cross(fourPi * dist * posRelative, velocity) / ( 8.0f * mRadius * mRadius * mRadius ) ;
}
