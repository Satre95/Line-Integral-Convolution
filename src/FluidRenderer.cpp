#include "FluidRenderer.hpp"

float gViscosity = 0.05f;
float gDensity = 1.0f;

FluidRenderer::FluidRenderer() {
    mFluidSim = std::make_unique<FluidSim>(gViscosity, gDensity);
    
}
