#include "Vorton.hpp"
#include "math_helper.hpp"

const float  Vorton::sAvoidSingularity = powf(FLT_MIN, 1.0f / 3.0f);

Vorton::Vorton() :
	mPosition(0), mVorticity(1.f), mRadius(1.f), mVelocity(0.f)
{
}

Vorton::Vorton(ofVec3f pos, ofVec3f vorticity, float rad, ofVec2f velocity) :
	mPosition(pos), mVorticity(vorticity), mRadius(rad), mVelocity(velocity)
{
}

Vorton::Vorton(const Vorton & other) :
	mPosition(other.mPosition), mVorticity(other.mVorticity),
	mRadius(other.mRadius), mVelocity(other.mVelocity)
{
}

void Vorton::AccumulateVelocity(ofVec3f & velocityOut, const ofVec3f &posQuery) {
	VORTON_ACCUMULATE_VELOCITY_private(velocityOut, posQuery, mPosition, mVorticity, mRadius);
}

void Vorton::AssignByVelocity(const ofVec3f &queryPosition, const ofVec3f velocity) {
	const ofVec3f  posRelative = queryPosition - mPosition;
	const float dist = posRelative.length();

	mVorticity = (FOUR_PI * dist * posRelative).getCrossed(velocity) / (8.0f * mRadius * mRadius * mRadius);
}