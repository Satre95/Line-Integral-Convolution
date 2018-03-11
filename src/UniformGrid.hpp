#pragma once

#include "ofVec3f.h"
#include "UniformGridGeometry.hpp"
#include <vector>

template <class TypeT>
class UniformGrid : public UniformGridGeometry {
public:
	typedef UniformGridGeometry Parent;
	friend class UniformGridMath;

	UniformGrid() : UniformGridGeometry() {}

	/*! \brief Construct a uniform grid container that fits the given geometry.
		\see Initialize
	*/
	UniformGrid(size_t uNumElements, const ofVec3f & vMin, const ofVec3f & vMax, bool bPowerOf2 = true)
		: UniformGridGeometry(uNumElements, vMin, vMax, bPowerOf2) {}

	/// Copy shape from given uniform grid
	explicit UniformGrid(const UniformGridGeometry & that)
		: UniformGridGeometry(that) {}

	/*! \brief Copy constructor for empty uniform grids

	This copy constructor does not copy contained contents.  A proper
	copy ctor should deep-copy all of its data in order to operate with
	containers, in particular STL vector) when it reallocates arrays.
	But reallocating these containers would entail massive memory moves,
	which although possible, would be inefficient.  Meanwhile, NestedGrid
	also pushes UniformGrids onto a vector, which requires using a copy
	ctor to initialize the new element.  So that code makes empty elements
	to push onto the vector, and arrange for this copy ctor to handle those
	empties properly.

	Meanwhile we want to catch any unintentional copies of actual data,
	so this method catches any attempt to copy populated UniformGrid objects.
	*/
	UniformGrid(const UniformGrid & that)
		: UniformGridGeometry(that) {}
	UniformGrid & operator=(const UniformGrid & other) = delete;

	~UniformGrid() {}

	TypeT & 	operator[](const size_t & offset) { return mContents.at(offset); }
	const TypeT & 	operator[](const size_t & offset) const { return mContents.at(offset); }
	TypeT &			operator[](const ofVec3f & pos) { return mContents.at(OffsetOfPosition(pos)); }

	/// Initialize contents to whatever default ctor provides
	void Init() { mContents.resize(GetGridCapacity()); }

	virtual void DefineShape(size_t uNumElements, const ofVec3f & vMin, const ofVec3f & vMax, bool bPowerOf2) override {
		mContents.clear();
		Parent::DefineShape(uNumElements, vMin, vMax, bPowerOf2);
	}

	size_t Size() const { return mContents.size(); }

	/*! \brief Compute statistics of data in a uniform grid.

		\param min - minimum of all values in grid.  Caller must initialize to large values before calling this routine.

		\param max - maximum of all values in grid.  Caller must initialize to smale values before calling this routine.

	*/
	void ComputeStatistics(TypeT & min, TypeT & max) const {
		max = min = (*this)[0];
		const size_t numCells = GetGridCapacity();
		for (size_t offset = 0; offset < numCells; ++offset)
		{
			const TypeT & rVal = (*this)[offset];
			min = std::min(min, rVal);
			max = std::max(max, rVal);
		}
	}
	/*! \brief Interpolate values from grid to get value at given position

		\param vPosition - position to sample

		\return Interpolated value corresponding to value of grid contents at vPosition.

	*/
	void Interpolate(TypeT &vResult, const ofVec3f &vPosition) const;

	/// Insert given value into grid at given position
	void Insert(const ofVec3f & vPosition, const TypeT & item) {
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
		(*this)[offsetX0Y0Z0] += oneMinusTween.x * oneMinusTween.y * oneMinusTween.z * item;
		(*this)[offsetX1Y0Z0] += tween.x * oneMinusTween.y * oneMinusTween.z * item;
		(*this)[offsetX0Y1Z0] += oneMinusTween.x *         tween.y * oneMinusTween.z * item;
		(*this)[offsetX1Y1Z0] += tween.x *         tween.y * oneMinusTween.z * item;
		(*this)[offsetX0Y0Z1] += oneMinusTween.x * oneMinusTween.y *         tween.z * item;
		(*this)[offsetX1Y0Z1] += tween.x * oneMinusTween.y *         tween.z * item;
		(*this)[offsetX0Y1Z1] += oneMinusTween.x *         tween.y *         tween.z * item;
		(*this)[offsetX1Y1Z1] += tween.x *         tween.y *         tween.z * item;
	}

	void Clear() override {
		mContents.clear();
		Parent::Clear();
	}

private:
	/// 3D array of items.
	std::vector<TypeT> mContents;
};

/// Explicit template instantiation for ofVec3f.
/// \see UniformGrid.cpp
extern template void UniformGrid<ofVec3f>::Interpolate(ofVec3f & result, const ofVec3f & vPosition) const;

template <class TypeT>
void UniformGrid<TypeT>::Interpolate(TypeT &vResult, const ofVec3f &vPosition) const {
	//TODO: Replace with std::tuple refs to avoid unnecessary copies.
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
	vResult = oneMinusTween.x * oneMinusTween.y * oneMinusTween.z * (*this)[offsetX0Y0Z0]
		+ tween.x * oneMinusTween.y * oneMinusTween.z * (*this)[offsetX1Y0Z0]
		+ oneMinusTween.x *         tween.y * oneMinusTween.z * (*this)[offsetX0Y1Z0]
		+ tween.x *         tween.y * oneMinusTween.z * (*this)[offsetX1Y1Z0]
		+ oneMinusTween.x * oneMinusTween.y *         tween.z * (*this)[offsetX0Y0Z1]
		+ tween.x * oneMinusTween.y *         tween.z * (*this)[offsetX1Y0Z1]
		+ oneMinusTween.x *         tween.y *         tween.z * (*this)[offsetX0Y1Z1]
		+ tween.x *         tween.y *         tween.z * (*this)[offsetX1Y1Z1];
}