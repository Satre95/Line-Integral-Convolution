#pragma once

#include <memory>
#include "FluidSim.hpp"
#include "ofVbo.h"

typedef std::unique_ptr<FluidSim> FluidSimRef;
typedef std::unique_ptr<ofVbo> ofVboRef;
typedef std::shared_ptr<ofBufferObject> ofBufRef;

class FluidRenderer {
public:
    FluidRenderer();
    void Update(float timeStep, size_t uFrame);
    void Draw();
    void KeyPressed(int key);
    
private:
    FluidSimRef mFluidSim;
    ofVboRef mVbo;
    ofBufRef mBuf;
    
    const float      fRadius = 12.0f;
    const float      fThickness = 5.0f;
    const float      fMagnitude = 20.0f;
    const size_t     numCellsPerDim = 24;
    const size_t     numVortonsMax = numCellsPerDim * numCellsPerDim * numCellsPerDim;
    unsigned                numTracersPerCubeRoot = 6;
    
};
