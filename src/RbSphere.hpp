#pragma once
#include "RigidBody.hpp"

class RbSphere: public RigidBody {
public:
    RbSphere();
    RbSphere( const ofVec3f & vPos , const ofVec3f & vVelocity , const float & fMass , const float & fRadius );
    
private:
    float mRadius;
};
