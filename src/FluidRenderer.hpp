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
    
    float      fRadius = 1.0f;
    float      fThickness = 1.0f;
    float      fMagnitude = 20.0f;
    size_t     numCellsPerDim = 16;
    size_t     numVortonsMax = numCellsPerDim * numCellsPerDim * numCellsPerDim;
    size_t                numTracersPerCubeRoot = 6;
    
};
