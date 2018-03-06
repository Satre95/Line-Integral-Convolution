#include "RigidBody.hpp"

RigidBody::RigidBody()
: mPosition( 0.0f , 0.0f , 0.0f )
, mVelocity( 0.0f , 0.0f , 0.0f )
, mOrientation( 0.0f , 0.0f , 0.0f )
, mAngVelocity( 0.0f , 0.0f , 0.0f )
, mInverseMass( 0.0f )
, mInertiaInv()
, mForce( 0.0f , 0.0f , 0.0f )
, mTorque( 0.0f , 0.0f , 0.0f )
, mMomentum( 0.0f , 0.0f , 0.0f )
, mAngMomentum( 0.0f , 0.0f , 0.0f )
{}

RigidBody::RigidBody( const ofVec3f & vPos , const ofVec3f & vVelocity , const float & fMass )
: mPosition( vPos )
, mVelocity( vVelocity )
, mOrientation( 0.0f , 0.0f , 0.0f )
, mAngVelocity( 0.0f , 0.0f , 0.0f )
, mInverseMass( 1.0f / fMass )
, mInertiaInv( Mat3::sIdentity * mInverseMass ) // Not really valid but better than uninitialized, and derived class should assign.
, mForce( 0.0f , 0.0f , 0.0f )
, mTorque( 0.0f , 0.0f , 0.0f )
, mMomentum( vVelocity * fMass )
, mAngMomentum( 0.0f , 0.0f , 0.0f )
{
}

void RigidBody::ApplyForce( const ofVec3f & vForce , const ofVec3f & vPosition) {
    mForce += vForce ;                  // Accumulate forces
    const ofVec3f vPosRelBody = vPosition - mPosition ;
    mTorque += vPosRelBody.getCrossed(vForce) ;   // Accumulate torques
}

void RigidBody::ApplyImpulse( const ofVec3f & vImpulse )
{
    mMomentum  += vImpulse ;                    // Apply impulse
    mVelocity   = mInverseMass * mMomentum ;    // Update linear velocity accordingly
}

void RigidBody::ApplyImpulse( const ofVec3f & vImpulse , const ofVec3f & vPosition )
{
    mMomentum += vImpulse ;                         // Apply impulse
    mVelocity   = mInverseMass * mMomentum ;        // Update linear velocity accordingly
    const ofVec3f vPosRelBody = vPosition - mPosition ;
    ApplyImpulsiveTorque( vPosRelBody.getCrossed(vImpulse) ) ;
}

void RigidBody::ApplyImpulsiveTorque( const ofVec3f & vImpulsiveTorque )
{
    mAngMomentum += vImpulsiveTorque ;          // Apply impulsive torque
    mAngVelocity = mInertiaInv * mAngMomentum ; // Update angular velocity accordingly
}

void RigidBody::Update( const float & timeStep )
{
    mMomentum       += mForce * timeStep ;
    mVelocity        = mInverseMass * mMomentum ;
    mPosition       += mVelocity * timeStep ;
    mAngMomentum    += mTorque * timeStep ;
    // Correctly updating angular velocity and orientation involves these formulae:
    // Create an orientation matrix (which is unitary), called xOrient.
    // Update angular velocity using this formula:
    // mAngVelocity = xOrient * mInertiaInv * xOrient.Transpose() * mAngMomentum ;
    // Create a skew-symmetric matrix Omega from mAngVel using Rodriques' formula.
    // Update orientation using this formula:
    // xOrient += Omega * xOrient * timeStep ;
    // Re-orthonormalize xOrient:
    // xOrient.x.Normalize() ;
    // xOrient.z = xOrient.x ^ xOrient.y ;
    // xOrient.z.Normalize() ;
    // xOrient.y = xOrient.z ^ xOrient.x ;
    // Compute axis-angle form from xOrient and store in mOrientation.
    // Instead, we here assume mInertiaInv is symmetric and uniform (i.e. spherical),
    // thus inertia tensor is the same in body and world frames.
    mAngVelocity = mInertiaInv * mAngMomentum ;
    // This code also treat orientation as though it updates
    // like linear quantities, which is incorrect, but this
    // will get us through the day, since for this fluid sim we
    // only care about angular momentum of rigid bodies, not orientation.
    mOrientation += mAngVelocity * timeStep ; // This is a weird hack but it serves our purpose for this situation.
    
    // Zero out force and torque accumulators, for next update.
    mForce = mTorque = ofVec3f( 0.0f , 0.0f , 0.0f ) ;
}

void RigidBody::UpdateSystem( std::vector< RigidBody > & rigidBodies , float timeStep) {
    for(auto & rBody: rigidBodies)
        rBody.Update(timeStep);
}

