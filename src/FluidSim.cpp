#include "FluidSim.hpp"

/*! \brief Select boundary condition handling scheme
 
 The vorticity reassigned to this vortex should be such that
 the fluid velocity, relative to the body surface, is zero.
 
 We can approximate this in a number of ways.  Here are some options:
 
 (a) Ignore all other contributions other than that of this vortex.
 Reassign this vorton's vorticity so that the fluid velocity,
 due only to this one vorton, is zero at the body surface.
 This simple scheme yields surprisingly pleasing results,
 perhaps better than (b) below, probably because, for all its simplicitiy,
 it does not double-count thie influence of the vorton being moved.
 And also, if the vorton is in contact with the body, its influence is
 likely dominate the ambient field anyway.
 
 (b) Interpolate the velocity at this point using the existing velocity field
 (which spuriously includes the influence of this vorton at its old position),
 then reassign this vorton's vorticity to counteract that.
 This can lead to instability if not damped.
 This technique is probably the worst of the three supplied here.
 
 (c) Interpolate velocity at this point using the existing velocity field,
 remove the contribution to that due to this vorton's old position and vorticity,
 then reassing this vorton's vorticity to counteract that.
 This arguably the most accurate of those mentioned here.
 
 (d) Like (c) but also (hypothetically) updating the ambient flow each time
 a vorton gets moved.  This would entail a tremendous amount of computation,
 so I mention it here only for the sake of providing a coherent sense of
 what is theoretically possible.
 
 In lieu of treating this as accurately as hypothetically possible, we recognize
 the following fact: Each of these schemes applies a correction to any vortons
 interacting with the body.  Choices (b) and (c) are likely to over-correct,
 not under-correct, since they omit the persistent change in the ambient flow
 (for this time step).  That implies we can (and should) apply only a portion
 of the correction, to each vorton, at each time step.  See DELAY_SHEDDING in
 the code below.
 
 */
#define BOUNDARY_NO_SLIP_NO_THRU 1
// BOUNDARY_RESPECTS_AMBIENT_FLOW only takes effect if BOUNDARY_NO_SLIP_NO_THRU is true
#define BOUNDARY_RESPECTS_AMBIENT_FLOW 1
// BOUNDARY_AMBIENT_FLOW_OMITS_VORTON_OLD_POSITION only takes effect if BOUNDARY_RESPECTS_AMBIENT_FLOW is true.
#define BOUNDARY_AMBIENT_FLOW_OMITS_VORTON_OLD_POSITION 1




/*! \brief Whether flow affects rigid bodies immersed in the fluid.
 
 Normally we will leave this enabled but for testing we can disable it.
 */
#define FLOW_AFFECTS_BODY 1

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

/*! \brief Update the fluid and rigid bodies
 
 \param timeStep - change in virtual time since last update
 
 \param uFrame - frame counter
 */
void FluidSim::Update(float timeStep, size_t uFrame) {
    // Update fluid, temporarily ignoring rigid bodies and boundary conditions.
    mVortonSim.Update( timeStep , uFrame ) ;
    
    // Apply boundary conditions and calculate impulses to apply to rigid bodies.
    SolveBoundaryConditions() ;
    
    // Update rigid bodies.
    RbSphere::UpdateSystem( mSpheres , timeStep , uFrame ) ;
}

/*! \brief Remove particles within rigid bodies
 
 This routine should only be called initially, to remove
 excess particles initially inside rigid bodies.
 
 This is not meant to be called during simulation updates.
 Instead, run collision detection and response.
 
 \see SolveBoundaryConditions
 
 */
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
            } else // Did not delete vorton so skip past it.
                ++vortonIt;
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
            else // Did not delete tracer so skip past it.
                ++ iTracer ;
        }
    }
}

/*! \brief Collide particles with rigid bodies
 
 This uses a simplified form of "penalty" scheme
 which transfers linear and angular momentum between
 fluid particles and bodies immersed in the fluid.
 
 This implementation has some physical inaccuracies,
 described in comments in the code.
 
 A proper treatment of fluid-body interaction typically
 entails computing potential flow terms to add to the
 velocity induced by vorticity (which satisfies no-through
 boudnary conditions), and computing vorticity flux generated
 by viscous interactions between the fluid and body (which
 satisfies no-slip boundary conditions).  Or alternatively,
 one can introduce a "penalty" term into the fluid equations
 which pushes particles outside of bodies.
 
 In contrast, this scheme immediately projects particles embedded
 in bodies, to outside the bodies, and reassigns vorton vorticity
 (and hence angular momentum). This change in angular momentum in
 the flow is countered by an equal change in the body.
 
 Such accounting would only ever cause bodies to spin, not to
 translate, so in addition, any particle in contact with the body
 also imparts a linear momentum in proportion to the difference in
 relative velocity between the particle and the body.  This second
 part, the transfer of linear momentum, is not physically accurate,
 but the scheme is simpler than the alternatives, and leads
 to visually plausible results.
 
 Upon tracking total kinetic energy of the fluid and bodies,
 we should see it decrease (not increase) which would be
 consistent with transfering kinetic energy to heat.
 If the equations governing the fluid motion took into account
 heat (e.g. through a change in density which would then
 appear as a baroclinic term in the vorticity equation),
 then we could balance the energy budget.
 
 */
void FluidSim::SolveBoundaryConditions() {
    const size_t numBodies        = mSpheres.size() ;
    const size_t numVortons       = mVortonSim.GetVortons().size() ;
    const size_t numTracers       = mVortonSim.GetTracers().size() ;
    auto & vortons = mVortonSim.GetVortons();
    auto & tracers = mVortonSim.GetTracers();
    
#if FLOW_AFFECTS_BODY
    const float &  rMassPerParticle = mVortonSim.GetMassPerParticle() ;
#endif
    
    for( size_t uBody = 0 ; uBody < numBodies ; ++ uBody )
    {   // For each body in the simulation...
        RbSphere &  rSphere         = mSpheres.at(uBody) ;
        
        // Collide vortons with rigid body.
        for( size_t uVorton = 0 ; uVorton < numVortons ; ++ uVorton )
        {   // For each vorton in the simulation...
            Vorton & rVorton = vortons.at( uVorton ) ;
            const ofVec3f  vSphereToVorton     = rVorton.mPosition - rSphere.mPosition ;   // vector from body center to vorton
            const float fSphereToVorton     = vSphereToVorton.length() ;
            const ofVec3f  vSphereToVortonDir  = vSphereToVorton / fSphereToVorton ;
            // This boundary thickness compensates for low discretization resolution,
            // by spreading the influence of the body surface to just outside the body,
            // deeper into the fluid.  This also has an effect somewhat like
            // instantaneous viscous diffusion, in the immediate vicinity of
            // the boundary.  It should be kept as small as possible,
            // but must be at least 1.  A value of 1 means only vortons
            // colliding with the body receive influence.  A value of 2 seems
            // most appropriate since that is the size of a grid cell, so
            // 2 essentially means vortons within a grid cell receive influence.
            // So a value in [1,2] seems appropriate. But values over 1.2 trap
            // vortons inside the body, because the "bend" can draw vortons back
            // toward the body.
            // Note, the larger fBndThkFactor is, the more vortons get influenced,
            // which drives the simulation to instability and also costs more CPU
            // time due to the increased number of vortons involved.
            const float fBndThkFactor       = 1.2f ; // Thickness of boundary, in vorton radii.
            const float fBoundaryThickness  = fBndThkFactor * rVorton.mRadius ; // Thickness of boundary, i.e. region within which body sheds vorticity into fluid.
            
            if( fSphereToVorton < ( rSphere.mRadius + fBoundaryThickness ) )
            {   // Vorton is interacting with body.
                
                // Compute "contact" point, near where vorton touched body.
                const ofVec3f vContactPtRelBody        = vSphereToVortonDir * rSphere.mRadius ;
                const ofVec3f vContactPtWorld          = vContactPtRelBody + rSphere.mPosition ;
                
                // Compute velocity of body at contact point.
                const ofVec3f vVelDueToRotAtConPt      = rSphere.mAngVelocity ^ vContactPtRelBody ; // linear velocity, of body at contact point, due to its own rotation
                const ofVec3f vVelBodyAtConPt          = rSphere.mVelocity + vVelDueToRotAtConPt    ; // Total linear velocity of body at contact point
                
                const ofVec3f vVorticityOld            = rVorton.mVorticity ;  // Cache to compute change in angular momentum.
                
                // Each scheme below projects this vorton to the body surface,
                // but the exact location depends on the scheme.
                
#if ! BOUNDARY_NO_SLIP_NO_THRU   // Assign vorticity to spin like the object.
                                 // Place vorton tangent to body surface along surface normal.
                const float distRescale         = ( rSphere.mRadius + rVorton.mRadius ) * ( 1.0f + FLT_EPSILON ) ;
                const ofVec3f  vDisplacementNew    = vSphereToVortonDir * distRescale ;
                rVorton.mPosition               = rSphere.mPosition + vDisplacementNew ;
                const ofVec3f  vAngVelDiff         = rVorton.mVorticity - rSphere.mAngVelocity ;   // (negative of) change in angular velocity applied to vorton
                rVorton.mVorticity              = rSphere.mAngVelocity ;                        // Assign vorticity of vorton at its new position.
                
#else // BOUNDARY_NO_SLIP_NO_THRU:
#if ! BOUNDARY_RESPECTS_AMBIENT_FLOW
                // This assigns a vorticity such that the fluid velocity,
                // relative to the body velocity at the contact point, is zero.
                // NOTE: This neglects the ambient flow due to other vortons.
                const ofVec3f   velFlowRelBodyAtColPt = - vVelBodyAtConPt ;
                
#else // BOUNDARY_RESPECTS_AMBIENT_FLOW
      // Make relative fluid velocity at body nearest this vorton,
      // due to "ambient" flow, to be zero.
      // Interpolate ambient velocity at that point on the sphere.
                ofVec3f velAmbientAtContactPt ; // Velocity due to entire vorton field at collision point.
                mVortonSim.GetVelocityGrid().Interpolate( velAmbientAtContactPt , vContactPtWorld ) ;
                
#if ! BOUNDARY_AMBIENT_FLOW_OMITS_VORTON_OLD_POSITION
                // Compute relative velocity between body (at contact point) and ambient flow.
                // NOTE: This neglects the fact that the ambient flow in mVelGrid also includes the
                //       influence of this same vorton, at its previous position.  If this interaction
                //       did not displace this vorton much, that could be a significant omission.
                const ofVec3f velFlowRelBodyAtColPt( velAmbientAtContactPt - vVelBodyAtConPt ) ;
                
#else // BOUNDARY_AMBIENT_FLOW_OMITS_VORTON_OLD_POSITION
      // Compute velocity induced by this vorton, from its old location, at contact point.
                ofVec3f    velDueToVort( 0.0f , 0.0f , 0.0f ) ;
                rVorton.AccumulateVelocity( velDueToVort , vContactPtWorld ) ;
                
                // Compute relative velocity between body at contact point and ambient flow,
                // subtracting the influence due to the vorton from the interpolated velocity.
                const ofVec3f velFlowRelBodyAtColPt( velAmbientAtContactPt - velDueToVort - vVelBodyAtConPt ) ;
#endif
#endif
                // Place vorton tangent to body surface along a "bend" (b),
                //              b_hat = w_hat ^ v_hat
                //              |b|   = vortonRadius
                // which is not necessarily along surface normal, r_hat,
                // and where vorticity lies perpendicular to this plane
                // formed by the surface normal and the velocity:
                //              w_hat = r_hat ^ v_hat
                // Vorticity w is given by AssignByVelocity.
                //
                //          ,,.--..,           --:   ambient flow velocity
                //       .'`        `'.      v  /| relative to body velocity
                //     ,'              `\      /       at collition point
                //    /     body         \    / ,..-..,
                //   |                    |  /-`       `',
                //  |               r      |/             \
                //  |          o---------->*,   b          \
                //  |                     || `'-,           |
                //   |      * marks       |'     `'o        |
                //    \      contact     /|   vorton with   |
                //     `.    point.     /  \    counter-   /
                //       '.,         ,-`    \  clockwise  /
                //          `''--''``        `.,  flow _.`
                //                              `''-''`
                // This figure depicts the flow field after ejecting
                // the vorton from the body interior.  Vorticity
                // is assigned to the vorton such that the flow field
                // satisfies no-through and no-slip boundary conditions
                // at the contact point.
                const ofVec3f  vSurfNormal         = vSphereToVorton.getNormalized() ;
                const ofVec3f  vVelDir             = velFlowRelBodyAtColPt.getNormalized() ;
                const ofVec3f  vVortDir            = vSurfNormal ^ vVelDir ;
                ofVec3f  vBendDir                  = vVortDir ^ vVelDir ;
                vBendDir.normalize();
                const float fBodySurfToVortCtr  = fSphereToVorton - rSphere.mRadius ;
                // If vorton was inside body, push it outside body, otherwise just pivot vorton about contact point.
                const float fBendDist           = fBodySurfToVortCtr < rVorton.mRadius ? rVorton.mRadius : fBodySurfToVortCtr ;
                const ofVec3f  vBend               = fBendDist * vBendDir ;
                rVorton.mPosition               = vContactPtWorld - vBend ;
                
                {
                    // Assign the vorticity of that vorton at its new position.
                    // This assigns a vorticity such that the fluid velocity (relative
                    // to the body velocity) at the contact point, is zero.
                    rVorton.AssignByVelocity( vContactPtWorld , - velFlowRelBodyAtColPt ) ;
#define DELAY_SHEDDING 1
#if DELAY_SHEDDING
                    // Make vorticity change less abrupt.
                    // Some of the boundary condition techniques are unstable with
                    // gain>threshold, where threshold varies by technique.
                    // E.g. choice "b" requires fGain<0.5 (or so).
                    // Even when the technique is stable, lowering gain can help reduce
                    // spurious high enstrophy spikes that arise due to discretization errors.
                    // In a viscous simulation, diffusion would smooth out such spikes,
                    // but we want this sim to work with zero viscosity.
                    //
                    // It also seems likely that thicker boundaries would
                    // require smaller values of gain, since thicker boundaries
                    // imply more vortons get altered each frame, and none of the
                    // techniques take that into account until the next frame.
                    // The relationship is likely to turn out to be fGain ~ 1/(thickness^2)
                    // since the number of vortons affected is proportional to thicnkness^2.
                    //
                    // This time-averaging has a vaguely similar effect as a very
                    // localized diffusion, in that it keeps vorticity smoother.
                    //
                    // If fGain is too small then vortices might not shed fast enough.
                    
                    const float fGain         = 0.1f ;
                    const float fOneMinusGain = 1.0f - fGain ;
                    rVorton.mVorticity = fGain * rVorton.mVorticity + fOneMinusGain * vVorticityOld ;
#endif
                }
                
                const ofVec3f  vAngVelDiff     = rVorton.mVorticity - vVorticityOld ;   // Change in angular velocity applied to vorton
#endif
                
                // Transfer angular momentum from vorton to body.
                // Unlike with the linear momentum exchange above, this
                // exactly preserves angular momentum at each time step.
#if FLOW_AFFECTS_BODY
                const float fMomentOfInertialVorton = 0.3f * rMassPerParticle ;
                rSphere.ApplyImpulsiveTorque( vAngVelDiff * fMomentOfInertialVorton ) ;  // Apply angular impulse (impulsive torque) to body
#endif
                
                // Transfer linear momentum between vorton and body.
                // Note that this does not strictly conserve linear momentum, in the sense
                // that this "transaction" of linear momentum has no bearing on the fluid
                // advection.  That is because the advection step summarily discards the
                // vorton velocity assigned here.  For moving bodies, the problem is not
                // monotic.  In other words, if the flow move past the body then eventually
                // the body "catches up" with the flow, at which point the body stops
                // absorbing a lot of new momentum from the fluid.  Stationary objects never
                // move, so absorb momentum indefinitely, but again, the fluid never loses
                // that linear momentum (directly anyway), so no harm there.
                {
                    const ofVec3f  vVelChange          = rVorton.mVelocity - vVelBodyAtConPt ; // (negative of) total linear velocity change applied to vorton
#if FLOW_AFFECTS_BODY
                    rSphere.ApplyImpulse( vVelChange * rMassPerParticle ) ;                 // Apply linear impulse to body
#endif
                    rVorton.mVelocity = vVelBodyAtConPt ;  // If same vorton is involved in another contact before advection, this will conserve linear momentum within this phase.
                }
            }
        }
        
        // Collide tracers with rigid body.
        for( size_t uTracer = 0 ; uTracer < numTracers ; ++ uTracer )
        {   // For each tracer in the simulation...
            Particle & rTracer = mVortonSim.GetTracers()[ uTracer ] ;
            const ofVec3f  vSphereToTracer = rTracer.mPosition - rSphere.mPosition ;   // vector from body center to tracer
            const float fSphereToTracer = vSphereToTracer.length() ;
            if( fSphereToTracer < ( rTracer.mSize + rSphere.mRadius ) )
            {   // Tracer is colliding with body.
                // Project tracer to outside of body.
                // This places the particle on the body surface.
                const float distRescale         = ( rSphere.mRadius + rTracer.mSize ) * ( 1.0f + FLT_EPSILON ) / fSphereToTracer ;
                const ofVec3f  vDisplacementNew    = vSphereToTracer * distRescale ;
                rTracer.mPosition = rSphere.mPosition + vDisplacementNew ;
                // Transfer linear momentum between vorton and body.
                const ofVec3f  vVelDueToRotation   = rSphere.mAngVelocity ^ vDisplacementNew ; // linear velocity, at vorton new position, due to body rotation
                const ofVec3f  vVelNew             = rSphere.mVelocity + vVelDueToRotation ;   // Total linear velocity of vorton at its new position, due to sticking to body
                const ofVec3f  vVelChange          = rTracer.mVelocity - vVelNew ;             // (negative of) total linear velocity change applied to vorton
#if FLOW_AFFECTS_BODY
                rSphere.ApplyImpulse( vVelChange * rMassPerParticle ) ;                     // Apply linear impulse to body
#endif
                rTracer.mVelocity = vVelNew ;   // If same tracer is involved in another contact before advection, this will conserve momentum.
            }
        }
    }
}
