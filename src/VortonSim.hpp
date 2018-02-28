#pragma once

#include <vector>
#include "Vorton.hpp"
#include "NestedGrid.hpp"
#include "UniformGrid.hpp"
#include "Particle.hpp"
#include "ofVec3f.h"

class VortonSim {
    
public:
    /// Construct a vorton simulation
    VortonSim( float viscosity = 0.0f , float density = 1.0f )
    : mMinCorner(FLT_MAX,FLT_MAX,FLT_MAX)
    , mMaxCorner( -mMinCorner )
    , mViscosity( viscosity )
    , mCirculationInitial( 0.0f , 0.0f , 0.0f )
    , mLinearImpulseInitial( 0.0f , 0.0f , 0.0f )
    , mAverageVorticity( 0.0f , 0.0f , 0.0f )
    , mFluidDensity( density )
    , mMassPerParticle( 0.0f )
    {}
    
private:
    std::vector< Vorton >   mVortons                ;   ///< Dynamic array of tiny vortex elements
    NestedGrid< Vorton >    mInfluenceTree          ;   ///< Influence tree
    UniformGrid< ofVec3f >  mVelGrid                ;   ///< Uniform grid of velocity values
    ofVec3f                 mMinCorner              ;   ///< Minimal corner of axis-aligned bounding box
    ofVec3f                 mMaxCorner              ;   ///< Maximal corner of axis-aligned bounding box
    float                   mViscosity              ;   ///< Viscosity.  Used to compute viscous diffusion.
    ofVec3f                 mCirculationInitial     ;   ///< Initial circulation, which should be conserved when viscosity is zero.
    ofVec3f                 mLinearImpulseInitial   ;   ///< Initial linear impulse, which should be conserved when viscosity is zero.
    ofVec3f                 mAverageVorticity       ;   ///< Hack, average vorticity used to compute a kind of viscous vortex diffusion.
    float                   mFluidDensity           ;   ///< Uniform density of fluid.
    float                   mMassPerParticle        ;   ///< Mass of each fluid particle (vorton or tracer).
    std::vector< Particle > mTracers                ;   ///< Passive tracer particles
};
