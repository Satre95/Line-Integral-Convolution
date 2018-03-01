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
    
    /*! \brief Initialize a vortex particle fluid simulation
     
     \note This method assumes the vortons have been initialized.
     That includes removing any vortons embedded inside
     rigid bodies.
     */
    void Initialize( size_t numTracersPerCellCubeRoot ) ;

    /*! \brief Kill the tracer at the given index
     */
    void KillTracer( size_t iTracer ) {
        mTracers[ iTracer ] = mTracers[ mTracers.size() - 1 ] ;
        mTracers.pop_back() ;
    }
    
    const ofVec3f GetTracerCenterOfMass( void ) const ;
    
    const UniformGrid< ofVec3f > & GetVelocityGrid() const       { return mVelGrid ; }
    const float & GetMassPerParticle() const    { return mMassPerParticle ; }
    void Update( float timeStep , unsigned uFrame ) ;
    void Clear() {
        mVortons.clear() ;
        mInfluenceTree.Clear() ;
        mVelGrid.Clear() ;
        mTracers.clear() ;
    }
    
    std::vector< Vorton > & GetVortons() { return mVortons; }
    const std::vector< Vorton > & GetVortons() const { return mVortons; }
    std::vector<Particle> & GetTracers() { return mTracers; }
    const std::vector<Particle> & GetTracers() const { return mTracers; }
    
private:
    void    AssignVortonsFromVorticity( UniformGrid< ofVec3f > & vortGrid ) ;
    void    ConservedQuantities( ofVec3f & vCirculation , ofVec3f & vLinearImpulse ) const ;
    void    FindBoundingBox( void ) ;
    void    MakeBaseVortonGrid( void ) ;
    void    AggregateClusters( size_t uParentLayer ) ;
    void    CreateInfluenceTree( void ) ;
    ofVec3f ComputeVelocity( const ofVec3f & vPosition , const size_t idxParent[3] , size_t iLayer ) ;
    ofVec3f ComputeVelocityBruteForce( const ofVec3f & vPosition ) ;
    void    ComputeVelocityGridSlice( size_t izStart , size_t izEnd ) ;
    void    ComputeVelocityGrid( void ) ;
    void    StretchAndTiltVortons( const float & timeStep , const size_t & uFrame ) ;
    void    ComputeAverageVorticity( void ) ;
    void    DiffuseVorticityGlobally( const float & timeStep , const size_t & uFrame ) ;
    void    DiffuseVorticityPSE( const float & timeStep , const size_t & uFrame ) ;
    void    AdvectVortons( const float & timeStep ) ;
    
    void    InitializePassiveTracers( size_t multiplier ) ;
    void    AdvectTracersSlice( const float & timeStep , const size_t & uFrame , size_t izStart , size_t izEnd ) ;
    void    AdvectTracers( const float & timeStep , const size_t & uFrame ) ;
    
    std::vector< Vorton >   mVortons                ;   ///< Dynamic array of tiny vortex elements
    NestedGrid< Vorton >    mInfluenceTree          ;   ///< Influence tree
    UniformGrid< ofVec3f >  mVelGrid                ;   ///< Uniform grid of velocity values
    ofVec3f                 mMinCorner              ;   ///< Minimal corner of axis-aligned bounding box
    ofVec3f                 mMaxCorner              ;   ///< Maximal corner of axis-aligned bounding box
    float                   mViscosity              ;   ///< Viscosity. Used to compute viscous diffusion.
    ofVec3f                 mCirculationInitial     ;   ///< Initial circulation, which should be conserved when viscosity is zero.
    ofVec3f                 mLinearImpulseInitial   ;   ///< Initial linear impulse, which should be conserved when viscosity is zero.
    ofVec3f                 mAverageVorticity       ;   ///< Hack, average vorticity used to compute a kind of viscous vortex diffusion.
    float                   mFluidDensity           ;   ///< Uniform density of fluid.
    float                   mMassPerParticle        ;   ///< Mass of each fluid particle (vorton or tracer).
    std::vector< Particle > mTracers                ;   ///< Passive tracer particles
};
