#include "FluidRenderer.hpp"
#include "ofBufferObject.h"
#include "VorticityDistribution.hpp"
#include "ofMain.h"

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
    
    fRadius = 1.0f;
    fThickness = 10.f;
    fMagnitude = 200.f;
    numCellsPerDim = 16;
    numVortonsMax = numCellsPerDim * numCellsPerDim * numCellsPerDim;
    numTracersPerCubeRoot = 6;

	auto & vortons = mFluidSim->GetVortonSim().GetVortons();
    
//    AssignVorticity( vortons , 0.125f * FLT_EPSILON , 2048 , VortexNoise( ofVec3f( fThickness , fThickness , fThickness ) ) ) ;
    
    //Jet vortex ring, velocity in [0,1]
//    AssignVorticity( vortons , fMagnitude , numVortonsMax , JetRing( fRadius , fThickness , ofVec3f( 1.0f , 0.0f , 0.0f ) ) ) ;
    
    // 2 orthogonal vortex tubes
//    AssignVorticity( vortons , fMagnitude , numVortonsMax , VortexTube( fThickness , /* variation */ 0.0f , /* width */ 4.0f * fThickness , 2 , -1 ) ) ;
//    AssignVorticity( vortons , fMagnitude , numVortonsMax , VortexTube( fThickness , /* variation */ 0.0f , /* width */ 4.0f * fThickness , 2 ,  1 ) ) ;
    
//    AssignVorticity(vortons, fMagnitude, numVortonsMax, VortexNoise(ofVec3f(5.f)));
    
    //Cool Planet formation!
    AssignVorticity(vortons, fMagnitude, numVortonsMax, VortexTube(fThickness, 0.2f, 4.0f * fThickness, 2, 1));
    
    
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
    ofSetColor(255, 255, 255);
    mVbo->draw(GL_POINTS, 0, (int)mFluidSim->GetVortonSim().GetTracers().size());
    
//    ofSetColor(255, 0, 0);
//    //Draw the vortons for debugging
//    auto & vortons = mFluidSim->GetVortonSim().GetVortons();
//    for(Vorton & aVorton: vortons)
//        ofDrawSphere(aVorton.mPosition, aVorton.mRadius);
    
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
