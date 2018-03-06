#pragma once
#include <vector>

#include "ofVec3f.h"
#include "Mat3.hpp"

class RigidBody{
public:
    RigidBody();
    RigidBody( const ofVec3f & vPos , const ofVec3f & vVelocity , const float & fMass );
    
    ofVec3f     mPosition       ;   ///< Position (in world units) of center of vortex particle
    ofVec3f     mVelocity       ;   ///< Linear velocity of sphere
    ofVec3f     mOrientation    ;   ///< Orientation of sphere in axis-angle form
    ofVec3f     mAngVelocity    ;   ///< Angular velocity of sphere
    
    /// Apply a force to a rigid body at a given location
    void ApplyForce( const ofVec3f & vForce , const ofVec3f & vPosition );
    
    
    /// Apply an impulse to a rigid body through its center-of-mass (i.e. without applying a torque
    void ApplyImpulse( const ofVec3f & vImpulse );
    
    
    /// Apply an impulse to a rigid body at a given location
    void ApplyImpulse( const ofVec3f & vImpulse , const ofVec3f & vPosition );
    
    
    /// Apply an impulsive torque to a rigid body
    void ApplyImpulsiveTorque( const ofVec3f & vImpulsiveTorque );
    
    /*! \brief Update a rigid body from the previous to the next moment in time.
     
     \param timeStep - duration between previous and current time steps.
     */
    void Update( const float & timeStep );
    
    static void UpdateSystem( std::vector< RigidBody > & rigidBodies , float timeStep) ;

    
protected:
    float       mInverseMass    ;   ///< Reciprocal of the mass of this body
    Mat3        mInertiaInv     ;   ///< Inverse of inertial tensor
    
private:
    ofVec3f     mForce          ;   ///< Total force applied to this body for a single frame.
    ofVec3f     mTorque         ;   ///< Total torque applied to this body for a single frame.
    ofVec3f     mMomentum       ;   ///< Linear momentum of sphere
    ofVec3f     mAngMomentum    ;   ///< Angular momentum of sphere
    
};
