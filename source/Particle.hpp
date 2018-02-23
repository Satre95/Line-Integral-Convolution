#pragma once
#include <cinder/Vector.h>


class Particle
{
public:
	Particle()
		: mPosition( 0.0f , 0.0f , 0.0f )
        , mVelocity( 0.0f , 0.0f , 0.0f )
        , mOrientation( 0.0f , 0.0f , 0.0f )
        , mAngularVelocity( 0.0f , 0.0f , 0.0f )
        , mMass( 0.0f )
        , mSize( 0.0f )
        , mBirthTime( 0 )
    {}
	
	glm::vec3    mPosition	        ;   ///< Position (in world units) of center of particle
    glm::vec3    mVelocity	        ;   ///< Velocity of particle
    glm::vec3    mOrientation	    ;   ///< Orientation of particle, in axis-angle form where angle=|orientation|
    glm::vec3    mAngularVelocity	;   ///< Angular velocity of particle
    float	mMass               	;   ///< Mass of particle
    float	mSize		        	;   ///< Size of particle
    int     mBirthTime          	;   ///< Birth time of particle, in "ticks"
};