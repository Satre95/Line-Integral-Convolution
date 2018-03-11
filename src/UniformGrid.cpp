#include "UniformGrid.hpp"
#include "math_helper.hpp"
#include <cassert>

/// Explicit template instantiation for scalar float type.
void UniformGrid<ofVec3f>::Interpolate(ofVec3f & vResult, const ofVec3f & vPosition) const {
	size_t        indices[3]; // Indices of grid cell containing position.
	Parent::IndicesOfPosition(indices, vPosition);
	ofVec3f            vMinCorner;
	Parent::PositionFromIndices(vMinCorner, indices);
	const size_t  offsetX0Y0Z0 = OffsetFromIndices(indices);
	const ofVec3f      vDiff = vPosition - vMinCorner; // Relative location of position within its containing grid cell.
	const ofVec3f      tween = ofVec3f(vDiff.x * GetCellsPerExtent().x, vDiff.y * GetCellsPerExtent().y, vDiff.z * GetCellsPerExtent().z);
	const ofVec3f      oneMinusTween = ofVec3f(1.0f, 1.0f, 1.0f) - tween;
	const size_t  numXY = GetNumPoints(0) * GetNumPoints(1);
	const size_t  offsetX1Y0Z0 = offsetX0Y0Z0 + 1;
	const size_t  offsetX0Y1Z0 = offsetX0Y0Z0 + GetNumPoints(0);
	const size_t  offsetX1Y1Z0 = offsetX0Y0Z0 + GetNumPoints(0) + 1;
	const size_t  offsetX0Y0Z1 = offsetX0Y0Z0 + numXY;
	const size_t  offsetX1Y0Z1 = offsetX0Y0Z0 + numXY + 1;
	const size_t  offsetX0Y1Z1 = offsetX0Y0Z0 + numXY + GetNumPoints(0);
	const size_t  offsetX1Y1Z1 = offsetX0Y0Z0 + numXY + GetNumPoints(0) + 1;

	//vResult =
	//	oneMinusTween.x * oneMinusTween.y * oneMinusTween.z * (*this)[offsetX0Y0Z0]		//0
	//	+ tween.x * oneMinusTween.y * oneMinusTween.z * (*this)[offsetX1Y0Z0]			//1
	//	+ oneMinusTween.x *         tween.y * oneMinusTween.z * (*this)[offsetX0Y1Z0]	//2
	//	+ tween.x *         tween.y * oneMinusTween.z * (*this)[offsetX1Y1Z0]			//3
	//	+ oneMinusTween.x * oneMinusTween.y *         tween.z * (*this)[offsetX0Y0Z1]	//4
	//	+ tween.x * oneMinusTween.y *         tween.z * (*this)[offsetX1Y0Z1]			//5
	//	+ oneMinusTween.x *         tween.y *         tween.z * (*this)[offsetX0Y1Z1]	//6
	//	+ tween.x *         tween.y *         tween.z * (*this)[offsetX1Y1Z1];			//7

	__m128 param0 = LoadVec3(hMultiply(oneMinusTween) * (*this)[offsetX0Y0Z0]);
	__m128 param1 = LoadVec3(tween.x * oneMinusTween.y * oneMinusTween.z * (*this)[offsetX1Y0Z0]);
	__m128 param2 = LoadVec3(oneMinusTween.x * tween.y * oneMinusTween.z * (*this)[offsetX0Y1Z0]);
	__m128 param3 = LoadVec3(tween.x * tween.y * oneMinusTween.z * (*this)[offsetX1Y1Z0]);
	__m128 param4 = LoadVec3(oneMinusTween.x * oneMinusTween.y * tween.z * (*this)[offsetX0Y0Z1]);
	__m128 param5 = LoadVec3(tween.x * oneMinusTween.y * tween.z * (*this)[offsetX1Y0Z1]);
	__m128 param6 = LoadVec3(oneMinusTween.x * tween.y * tween.z * (*this)[offsetX0Y1Z1]);
	__m128 param7 = LoadVec3(tween.x * tween.y * tween.z * (*this)[offsetX1Y1Z1]);

	__m128 result = _mm_setzero_ps();
	result = _mm_add_ps(result, param0);
	result = _mm_add_ps(result, param1);
	result = _mm_add_ps(result, param2);
	result = _mm_add_ps(result, param3);
	result = _mm_add_ps(result, param4);
	result = _mm_add_ps(result, param5);
	result = _mm_add_ps(result, param6);
	result = _mm_add_ps(result, param7);

	vResult = StoreVec3(result);
}