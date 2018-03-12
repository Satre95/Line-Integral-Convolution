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
	mBuf->allocate(sizeof(Particle) * tracers.size(), tracers.data(), GL_DYNAMIC_DRAW);
	mVbo->setVertexBuffer(*mBuf, 3, sizeof(Particle));

	auto & vortons = mFluidSim->GetVortonSim().GetVortons();

    //Jet vortex ring, velocity in [0,1]
    AssignVorticity( vortons , fMagnitude , numVortonsMax , JetRing( fRadius , fThickness , ofVec3f( 1.0f , 0.0f , 0.0f ) ) ) ;
    
	mFluidSim->Initialize(numTracersPerCubeRoot);
}

void FluidRenderer::Update(float timeStep, size_t uFrame) {
	mFluidSim->Update(timeStep, uFrame);

	//Upload particles to GPU
	const std::vector<Particle> & tracers = mFluidSim->GetVortonSim().GetTracers();
	mBuf->allocate(sizeof(Particle) * tracers.size(), tracers.data(), GL_DYNAMIC_DRAW);
	mVbo->setVertexBuffer(*mBuf, 3, sizeof(Particle));
}

void FluidRenderer::Draw() {
	mVbo->draw(GL_POINTS, 0, (int)mFluidSim->GetVortonSim().GetTracers().size());
}

void FluidRenderer::KeyPressed( int key ) {
    auto & vortons = mFluidSim->GetVortonSim().GetVortons();

    switch (key) {
        case '1':
            //Jet vortex ring, velocity in [0,1]
            AssignVorticity( vortons , fMagnitude , numVortonsMax , JetRing( fRadius , fThickness , ofVec3f( 1.0f , 0.0f , 0.0f ) ) ) ;
            break;
            
        case '2':
            // Vortex sheet with spanwise variation
            AssignVorticity(vortons, fMagnitude, numVortonsMax, VortexSheet(fThickness, /* variation */ 0.2f, /* width */ 7.0f * fThickness));
            break;
            
        case '3':
            // 2D sheet
            AssignVorticity(vortons, fMagnitude, numVortonsMax, VortexSheet(fThickness, /* variation */ 0.0f, /* width */ 2.f * fThickness));
            break;
            
        case '4':
            // Vortex Tube
            AssignVorticity(vortons, fMagnitude, numVortonsMax, VortexTube(fThickness, /* variation */ 0.0f, /* width */ 2.0f * fThickness, 2, 0));
            break;
            
        case 'r': {
            mFluidSim->Clear();
            // 2D sheet
            AssignVorticity(vortons, fMagnitude, numVortonsMax, VortexSheet(fThickness, /* variation */ 0.0f, /* width */ 2.f * fThickness));
            mFluidSim->Initialize(numTracersPerCubeRoot);
            break;
        }
        default:
            break;
    }
}
