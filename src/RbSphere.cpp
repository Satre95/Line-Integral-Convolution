#include "RbSphere.hpp"

RbSphere::RbSphere() :
RigidBody(), mRadius(0.f)
{}

RbSphere::RbSphere( const ofVec3f & vPos , const ofVec3f & vVelocity , const float & fMass , const float & fRadius ) :
RigidBody(vPos, vVelocity, fMass), mRadius(fRadius)
{
    // Moments of inertia for a sphere are 2 M R^2 / 5.
    mInertiaInv = Mat3::sIdentity * 5.0f * mInverseMass / ( 2.0f * fRadius * fRadius ) ;
}

