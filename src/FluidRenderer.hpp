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
    
private:
    FluidSimRef mFluidSim;
    ofVboRef mVbo;
    ofBufRef mBuf;
    
};
