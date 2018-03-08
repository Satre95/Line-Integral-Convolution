#pragma once
#include <vector>
#include <algorithm>

#include "ofVec3f.h"
#include "Vorton.hpp"
#include "UniformGrid.hpp"

/// \brief Abstract base class for various vortex distros
class VorticityDistribution {
public:
    virtual ofVec3f GetDomainSize() const = 0;
    virtual void AssignVorticity( ofVec3f & vorticity , const ofVec3f & position , const ofVec3f & vCenter ) const = 0 ;
    virtual ~VorticityDistribution() {}
};

class VortexSheet: public VorticityDistribution {
public:
    /*! \brief Initialize parameters for a vortex sheet with spanwise variation
     
     \param fThickness - characteristic vertical thickness of shear layer
     
     \param fVariation - Amplitude of variation of thickness.
     Choose values in [0,0.2].
     
     \param fWidth - spanwise width of shear layer
     
     */
    VortexSheet( const float & fThickness , const float & fVariation , const float & fWidth )
    : mThickness( fThickness ), mVariation( fVariation ) , mWidth( fWidth )
    {}
    
    virtual ofVec3f GetDomainSize() const override {
        return ofVec3f( 14.0f * mThickness , mWidth , 14.0f * mThickness ) ;
    }
    
    virtual void AssignVorticity( ofVec3f & vorticity , const ofVec3f & position , const ofVec3f & vCenter ) const override;
    
    float   mThickness  ;
    float   mVariation  ;
    float   mWidth      ;
};

/// \brief Global generic fn to create a vortex field based on a vorticity distribution.
extern void AssignVorticity( std::vector<Vorton> & vortons , float fMagnitude , size_t numVortonsMax , const VorticityDistribution & vorticityDistribution ) ;
