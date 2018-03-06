#pragma once
#include "RigidBody.hpp"

class SphereRigidBody: public RigidBody {
public:
    SphereRigidBody();
    SphereRigidBody( const ofVec3f & vPos , const ofVec3f & vVelocity , const float & fMass , const float & fRadius );
    
private:
    float mRadius;
};
