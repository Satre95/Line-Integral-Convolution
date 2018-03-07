#pragma once

#include <vector>
#include "VortonSim.hpp"
#include "RbSphere.hpp"

class FluidSim
{
public:
	FluidSim(float viscosity, float density);
	~FluidSim();
	FluidSim(const FluidSim &) = delete;
	FluidSim & operator=(const FluidSim &) = delete;

	void                    Initialize(size_t numTracersPerCellCubeRoot);
	void                    Update(float timeStep, size_t uFrame);
	VortonSim &             GetVortonSim() { return mVortonSim; }
	void                    Clear() { mVortonSim.Clear(); }

private:
	void RemoveEmbeddedParticles();
	void SolveBoundaryConditions();

	VortonSim               mVortonSim;
    std::vector<RbSphere>   mSpheres;
};
