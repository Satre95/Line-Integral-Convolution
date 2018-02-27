#pragma once

#include "ofVec3f.h"
#include "ofVec2f.h"

class Vorton
{
public:
	Vorton();
    Vorton(ofVec3f pos, ofVec3f vorticity, float radius = 1.f, ofVec2f velocity = ofVec2f(0));
    Vorton(const Vorton & other);
	~Vorton();
    
    /*! \brief Computes the velocity induced by this tiny vortex element.
        \param velocityOut var in which to accumulate the velocity
        \param posQuery position where we want to know the velocity.
        \note mRadius currently serves double-duty for two things which should probably be kept separate.
        One is the radius of the finite-size vorton,
        where the vorticity distribution inside the radius
        is finite, to avoid evaluating a singularity.
        The other is the volume of the "infinitesimal"
        volume element, used to compute a contribution
        to a velocity field.
    */
    void AccumulateVelocity(ofVec3f & velocityOut, const ofVec3f & posQuery);
    
    /*! \brief Computes the voriticty required to obtain a given velocity.
     
     This assigns the vorticity

        w = 4 Pi r^2 v / volumeElement

        where
     
        * r is the distance from the vorton (which here is also the radius of the vorton)
 
        * v is the velocity induced by the vorton
 
        * volumeElement is the volume occupied by the vorton
 
        * w_hat is r_hat cross v_hat.

        This assumes v and r are orthogonal, so this is a very special-purpose
        routine. This routine also assumes this vorton's position and radius are where they need to be.
     */
    void AssignByVelocity(const ofVec3f & queryPosition, const ofVec3f velocity);
    
private:
    ofVec3f mPosition;
    ofVec3f mVorticity;
    float mRadius;
    ofVec2f mVelocity;
	
};

