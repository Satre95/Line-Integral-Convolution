#include "FluidSim.hpp"

FluidSim::FluidSim(float viscosity, float density) : mVortonSim(viscosity, density)
{
}

FluidSim::~FluidSim()
{
}

void FluidSim::Initialize(size_t numTracersPerCellCubeRoot) {
    RemoveEmbeddedParticles();
    mVortonSim.Initialize(numTracersPerCellCubeRoot);
    RemoveEmbeddedParticles();
}

void FluidSim::Update(float timeStep, size_t uFrame) {
}

void FluidSim::RemoveEmbeddedParticles() {
    const size_t numBodies    = mSpheres.size() ;
    for( size_t uBody = 0 ; uBody < numBodies ; ++ uBody )
    {   // For each sphere in the simulation...
        RbSphere &  rSphere         = mSpheres.at(uBody) ;
        auto & vortons = mVortonSim.GetVortons();
        
        for(auto vortonIt = vortons.begin(); vortonIt != vortons.end(); )
        {
            // For each vorton in the simulation...
            Vorton & rVorton = *vortonIt;
            const ofVec3f  vSphereToVorton = rVorton.mPosition - rSphere.mPosition ;   // vector from sphere center to vorton
            const float fSphereToVorton = vSphereToVorton.length() ;
            if( fSphereToVorton < ( rVorton.mRadius + rSphere.mRadius ) )
            {   // Vorton is inside body.
                // Delete vorton.
                vortons.erase(vortonIt);
                // Deleted vorton so now this same index contains a different vorton.
            } else vortonIt++;
        }
        auto & tracers = mVortonSim.GetTracers();
        for( size_t iTracer = 0 ; iTracer < tracers.size() ; )
        {   // For each passive tracer particle in the simulation...
            Particle & rTracer = tracers.at(iTracer) ;
            const ofVec3f  vSphereToTracer = rTracer.mPosition - rSphere.mPosition ;   // vector from sphere center to tracer
            const float fSphereToTracer = vSphereToTracer.length() ;
            if( fSphereToTracer < ( rTracer.mSize + rSphere.mRadius ) )
            {   // Tracer particle is inside body.
                // Delete tracer.
                mVortonSim.KillTracer( iTracer ) ;
                // Deleted tracer so now this same index contains a different tracer.
            }
            else
            {   // Did not delete tracer so skip past it.
                ++ iTracer ;
            }
        }
    }
}

void FluidSim::SolveBoundaryConditions() {
}
