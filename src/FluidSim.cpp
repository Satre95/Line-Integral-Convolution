#include "FluidSim.hpp"

FluidSim::FluidSim(float viscosity, float density) : mVortonSim(viscosity, density)
{
}

FluidSim::~FluidSim()
{
}

void FluidSim::Initialize(size_t numTracersPerCellCubeRoot) {
}

void FluidSim::Update(float timeStep, size_t uFrame) {
}

void FluidSim::RemoveEmbeddedParticles() {
}

void FluidSim::SolveBoundaryConditions() {
}