#include "FluidRenderer.hpp"
#include "ofBufferObject.h"
#include "VorticityDistribution.hpp"

float gViscosity = 0.05f;
float gDensity = 1.0f;

FluidRenderer::FluidRenderer() {
    mFluidSim = std::make_unique<FluidSim>(gViscosity, gDensity);
    mVbo = std::make_unique<ofVbo>();
    mBuf = std::make_shared<ofBufferObject>();
    
    //Upload particles to a generic buffer object and then assign to VBO
    const std::vector<Particle> & tracers = mFluidSim->GetVortonSim().GetTracers();
    mBuf->allocate(sizeof(Particle) * tracers.size() , tracers.data(), GL_DYNAMIC_DRAW);
    mVbo->setVertexBuffer(*mBuf, 3, sizeof(Particle));
    
    auto & vortons = mFluidSim->GetVortonSim().GetVortons();
    static const float      fRadius         = 1.0f ;
    static const float      fThickness      = 1.0f ;
    static const float      fMagnitude      = 20.0f ;
    static const size_t     numCellsPerDim  = 16 ;
    static const size_t     numVortonsMax   = numCellsPerDim * numCellsPerDim * numCellsPerDim ;
    unsigned                numTracersPer   = 3 ;
    
    // Vortex sheet with spanwise variation
    AssignVorticity(vortons, fMagnitude, numVortonsMax, VortexSheet( fThickness , /* variation */ 0.2f , /* width */ 7.0f * fThickness ));
    
    mFluidSim->Initialize(numTracersPer);
}

void FluidRenderer::Update(float timeStep, size_t uFrame) {
    mFluidSim->Update(timeStep, uFrame);
    
    //Upload particles to GPU
    const std::vector<Particle> & tracers = mFluidSim->GetVortonSim().GetTracers();
    mBuf->allocate(sizeof(Particle) * tracers.size() , tracers.data(), GL_DYNAMIC_DRAW);
    mVbo->setVertexBuffer(*mBuf, 3, sizeof(Particle));
}

void FluidRenderer::Draw() {
    mVbo->draw(GL_POINTS, 0, (int)mFluidSim->GetVortonSim().GetTracers().size());
    
}
