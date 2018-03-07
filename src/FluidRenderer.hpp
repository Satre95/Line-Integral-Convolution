#pragma once

#include <memory>
#include "FluidSim.hpp"

typedef std::unique_ptr<FluidSim> FluidSimRef;


class FluidRenderer {
public:
    FluidRenderer();
    
private:
    FluidSimRef mFluidSim;
};
