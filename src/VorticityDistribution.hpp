#pragma once
#include <vector>
#include <algorithm>

#include "ofVec3f.h"
#include "Vorton.hpp"
#include "UniformGrid.hpp"

/// \brief Abstract base class for various vortex distros
class VorticityDistribution {
public:
	virtual ofVec3f GetDomainSize() const = 0;
	virtual void AssignVorticity(ofVec3f & vorticity, const ofVec3f & position, const ofVec3f & vCenter) const = 0;
	virtual ~VorticityDistribution() {}
};

class VortexSheet : public VorticityDistribution {
public:
	/*! \brief Initialize parameters for a vortex sheet with spanwise variation

	 \param fThickness - characteristic vertical thickness of shear layer

	 \param fVariation - Amplitude of variation of thickness.
	 Choose values in [0,0.2].

	 \param fWidth - spanwise width of shear layer

	 */
	VortexSheet(const float & fThickness, const float & fVariation, const float & fWidth)
		: mThickness(fThickness), mVariation(fVariation), mWidth(fWidth)
	{}

	virtual ofVec3f GetDomainSize() const override {
		return ofVec3f(14.0f * mThickness, mWidth, 14.0f * mThickness);
	}

	virtual void AssignVorticity(ofVec3f & vorticity, const ofVec3f & position, const ofVec3f & vCenter) const override;

	float   mThickness;
	float   mVariation;
	float   mWidth;
};

/*! \brief Specify vorticity in the shape of a vortex ring.

The vorticity specified by this class derives from taking the curl of
a localized jet.  The vorticity is therefore guaranteed to be solenoidal,
to within the accuracy the discretization affords.

\see VortexRing

*/
class JetRing : public VorticityDistribution
{
public:
	/*! \brief Initialize parameters for a vortex ring (using a different formula from the other).

	The vorticity profile resulting from this is such that the induced velocity is in [0,1].

	\param fRadiusSlug - radius of central region where velocity is constant

	\param fThickness - thickness of vortex ring, i.e. radius of annular core

	\param vDirection - vector of ring axis, also vector of propagation

	\param fSpeed   - speed of slug

	*/
	JetRing(const float & fRadiusSlug, const float & fThickness, const ofVec3f & vDirection)
		: mRadiusSlug(fRadiusSlug)
		, mThickness(fThickness)
		, mRadiusOuter(mRadiusSlug + mThickness)
		, mDirection(vDirection)
	{
	}

	virtual ofVec3f GetDomainSize(void) const
	{
		const float boxSideLength = 2.f * (mRadiusOuter);    // length of side of virtual cube
		return ofVec3f(1.0f, 1.0f, 1.0f) * boxSideLength;
	}

	virtual void AssignVorticity(ofVec3f & vorticity, const ofVec3f & position, const ofVec3f & vCenter) const
	{
		const  ofVec3f     vFromCenter = position - vCenter;              // displacement from ring center to vorton position
		const  float    tween = vFromCenter.dot(mDirection);        // projection of position onto axis
		const  ofVec3f     vPtOnLine = vCenter + mDirection * tween;    // closest point on axis to vorton position
		ofVec3f            vRho = position - vPtOnLine;            // direction radially outward from annulus core
		const  float    rho = vRho.length();                // distance from axis
		const  float    distAlongDir = mDirection.dot(vFromCenter);        // distance along axis of vorton position
		if ((rho < mRadiusOuter) && (rho > mRadiusSlug))
		{   // Probe position is inside jet region.
			const  float    streamwiseProfile = (fabsf(distAlongDir) < mRadiusSlug) ? 0.5f * (cos(PI* distAlongDir / mRadiusSlug) + 1.0f) : 0.0f;
			const  float    radialProfile = sin(PI * (rho - mRadiusSlug) / mThickness);
			const  float    vortPhi = streamwiseProfile * radialProfile * PI / mThickness;
			ofVec3f            rhoHat = vRho;                    // direction radially away from annular core
			rhoHat.normalize();
			ofVec3f            phiHat = mDirection ^ rhoHat;  // direction along annular core
			vorticity = vortPhi * phiHat;
		}
		else
		{
			vorticity = ofVec3f(0.0f, 0.0f, 0.0f);
		}
	}

	float   mRadiusSlug;   ///< Radius of central region of jet, where velocity is uniform.
	float   mThickness;   ///< Thickness of region outside central jet, where velocity decays gradually
	float   mRadiusOuter;   ///< Radius of jet, including central region and gradial falloff.
	ofVec3f    mDirection;   ///< Direction of jet.
};

/*! \brief Specify a random field of vorticity

The vorticity specified by this class abruptly terminates
at the domain boundaries and therefore violates characteristics
of true vorticity.

*/
class VortexNoise : public VorticityDistribution
{
public:
	/*! \brief Initialize parameters for vortex noise

	\param shape - dimensions of box with noisy vorticity

	*/
	VortexNoise(const ofVec3f & vBox)
		: mBox(vBox)
		, mAmplitude(1.0f, 1.0f, 1.0f)
	{
		if (0.0f == vBox.z)
		{   // Domain is 2D (in XY plane).
			// Make vorticity purely vertical.
			mAmplitude = ofVec3f(0.0f, 0.0f, 1.0f);
		}
	}

	virtual ofVec3f GetDomainSize(void) const
	{
		return mBox;
	}

	virtual void AssignVorticity(ofVec3f & vorticity, const ofVec3f & position, const ofVec3f & vCenter) const
	{
		vorticity = RandomSpread(mAmplitude);
	}

	ofVec3f    mBox;
	ofVec3f    mAmplitude;
};

/*! \brief Specify vorticity in the shape of a vortex tube.

The vorticity specified by this class abruptly terminates
at the domain boundaries and therefore violates characteristics
of true vorticity.

*/
class VortexTube : public VorticityDistribution
{
public:
	/*! \brief Initialize parameters for a vortex tube with spanwise variation

	\param fThickness - characteristic thickness of tube

	\param fVariation - Amplitude of variation of thickness.
	Choose values in [0,0.2].

	\param fWidth - spanwise width of domain

	\param iPeriods - number of spanwise periods in the variation

	*/
	VortexTube(const float & fDiameter, const float & fVariation, const float & fWidth, const int & iPeriods, const int & iLocation)
		: mRadius(0.5f * fDiameter)
		, mVariation(fVariation)
		, mWidth(fWidth)
		, mWavenumber(float(iPeriods))
		, mLocation(iLocation)
	{
	}

	virtual ofVec3f GetDomainSize(void) const
	{
		return ofVec3f(8.0f * mRadius, mWidth, 8.0f * mRadius);
	}

	virtual void AssignVorticity(ofVec3f & vorticity, const ofVec3f & position, const ofVec3f & vCenter) const
	{
		if (0 == mLocation)
		{
			const ofVec3f  posRel = position - vCenter;
			const float rho = sqrtf((posRel.x * posRel.x) + (posRel.z * posRel.z));
			const float modulation = 1.0f - mVariation * (cosf(TWO_PI * mWavenumber * posRel.y / mWidth) - 1.0f);
			const float radiusLocal = mRadius * modulation;
			if (rho < radiusLocal)
			{   // Position is inside vortex tube.
				const float vortY = 0.5f * (cosf(PI * rho / radiusLocal) + 1);
				vorticity = ofVec3f(0.0f, vortY, 0.0f);
			}
			else
			{   // Position is outside vortex tube.
				vorticity = ofVec3f(0.0f, 0.0f, 0.0f);
			}
		}
		else if (1 == mLocation)
		{
			const ofVec3f  posRel = position - vCenter - ofVec3f(0.0f, 0.0f, 1.0f * mRadius);
			const float rho = sqrtf((posRel.x * posRel.x) + (posRel.z * posRel.z));
			const float modulation = 1.0f - mVariation * (cosf(TWO_PI * mWavenumber * posRel.y / mWidth) - 1.0f);
			const float radiusLocal = mRadius * modulation;
			if (rho < radiusLocal)
			{   // Position is inside vortex tube.
				const float vortY = 0.5f * (cosf(PI * rho / radiusLocal) + 1);
				vorticity = ofVec3f(0.0f, vortY, 0.0f);
			}
			else
			{   // Position is outside vortex tube.
				vorticity = ofVec3f(0.0f, 0.0f, 0.0f);
			}
		}
		else if (-1 == mLocation)
		{
			const ofVec3f  posRel = position - vCenter - ofVec3f(0.0f, 0.0f, -1.0f * mRadius);
			const float rho = sqrtf((posRel.y * posRel.y) + (posRel.z * posRel.z));
			const float modulation = 1.0f - mVariation * (cosf(TWO_PI * mWavenumber * posRel.x / mWidth) - 1.0f);
			const float radiusLocal = mRadius * modulation;
			if (rho < radiusLocal)
			{   // Position is inside vortex tube.
				const float vortX = 0.5f * (cosf(PI * rho / radiusLocal) + 1);
				vorticity = ofVec3f(vortX, 0.0f, 0.0f);
			}
			else
			{   // Position is outside vortex tube.
				vorticity = ofVec3f(0.0f, 0.0f, 0.0f);
			}
		}
	}

	float   mRadius;   ///< Maximum radius of vortex tube
	float   mVariation;   ///< Amplitude of radius variation
	float   mWidth;   ///< Spanwise width of domain
	float   mWavenumber;   ///< Number of full periods of spanwise variation to fit in domain
	int     mLocation;   ///< HACK: one of a few hard-coded locations of tube
};

/// \brief Global generic fn to create a vortex field based on a vorticity distribution.
extern void AssignVorticity(std::vector<Vorton> & vortons, float fMagnitude, size_t numVortonsMax, const VorticityDistribution & vorticityDistribution);