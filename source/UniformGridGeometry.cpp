#include "UniformGridGeometry.hpp"
#include "math_helper.hpp"
#include <algorithm>
using namespace cinder;

void UniformGridGeometry::DefineShape(size_t uNumElements, const vec3 & vMin, const vec3 & vMax, bool bPowerOf2)
{
	mMinCorner = vMin;
	static const float Nudge = 1.0f + FLT_EPSILON;  // slightly expand size to ensure robust containment even with roundoff
	mGridExtent = (vMax - vMin) * Nudge;

	vec3 vSizeEffective(GetExtent());
	int numDims = 3;   // Number of dimensions to region.
	if (0.0f == vSizeEffective.x)
	{   // X size is zero so reduce dimensionality
		vSizeEffective.x = 1.0f; // This component will not contribute to the total region volume/area/length.
		mGridExtent.x = 0.0f;
		--numDims;
	}
	if (0.0f == vSizeEffective.y)
	{   // Y size is zero so reduce dimensionality
		vSizeEffective.y = 1.0f; // This component will not contribute to the total region volume/area/length.
		mGridExtent.y = 0.0f;
		--numDims;
	}
	if (0.0f == vSizeEffective.z)
	{   // Z size is zero so reduce dimensionality
		vSizeEffective.z = 1.0f; // This component will not contribute to the total region volume/area/length.
		mGridExtent.z = 0.0f;
		--numDims;
	}

	// Compute region volume, area or length (depending on dimensionality).
	const float volume = vSizeEffective.x * vSizeEffective.y * vSizeEffective.z;
	const float cellVolumeCubeRoot = powf(volume / float(uNumElements), -1.0f / float(numDims)); // Approximate size of each cell in grid.
	// Compute number of cells in each direction of uniform grid.
	// Choose grid dimensions to fit as well as possible, so that the total number
	// of grid cells is nearly the total number of elements in the contents.
	unsigned int numCells[3] = { std::max(unsigned(1) , unsigned(GetExtent().x * cellVolumeCubeRoot + 0.5f)) ,
		std::max(unsigned(1) , unsigned(GetExtent().y * cellVolumeCubeRoot + 0.5f)) ,
		std::max(unsigned(1) , unsigned(GetExtent().z * cellVolumeCubeRoot + 0.5f)) };

	if (bPowerOf2)
	{   // Choose number of gridcells to be powers of 2.
		// This will simplify subdivision in a NestedGrid.
		numCells[0] = NearestPowerOfTwo(numCells[0]);
		numCells[1] = NearestPowerOfTwo(numCells[1]);
		numCells[2] = NearestPowerOfTwo(numCells[2]);
	}

	while (numCells[0] * numCells[1] * numCells[2] >= uNumElements * 8)
	{   // Grid capacity is excessive.
		// This can occur when the trial numCells is below 0.5 in which case the integer arithmetic loses the subtlety.
		numCells[0] = std::max(unsigned(1), numCells[0] / 2);
		numCells[1] = std::max(unsigned(1), numCells[1] / 2);
		numCells[2] = std::max(unsigned(1), numCells[2] / 2);
	}
	mNumPoints[0] = numCells[0] + 1; // Increment to obtain number of points.
	mNumPoints[1] = numCells[1] + 1; // Increment to obtain number of points.
	mNumPoints[2] = numCells[2] + 1; // Increment to obtain number of points.

	PrecomputeSpacing();
}

void UniformGridGeometry::PrecomputeSpacing() {
	mCellExtent.x = GetExtent().x / float(GetNumCells(0));
	mCellExtent.y = GetExtent().y / float(GetNumCells(1));
	mCellExtent.z = GetExtent().z / float(GetNumCells(2));
	mCellsPerExtent.x = float(GetNumCells(0)) / GetExtent().x;
	mCellsPerExtent.y = float(GetNumCells(1)) / GetExtent().y;
	if (0.0f == GetExtent().z)
	{   // Avoid divide-by-zero for 2D domains that lie in the XY plane.
		mCellsPerExtent.z = 1.0f / FLT_MIN;
	}
	else
	{
		mCellsPerExtent.z = float(GetNumCells(2)) / GetExtent().z;
	}
}

void UniformGridGeometry::Decimate(const UniformGridGeometry & src, int iDecimation) {
	mGridExtent = src.mGridExtent;
	mMinCorner = src.mMinCorner;
	mNumPoints[0] = src.GetNumCells(0) / iDecimation + 1;
	mNumPoints[1] = src.GetNumCells(1) / iDecimation + 1;
	mNumPoints[2] = src.GetNumCells(2) / iDecimation + 1;
	if (iDecimation > 1)
	{   // Decimation could reduce dimension and integer arithmetic could make value be 0, which is useless if src contained any data.
		mNumPoints[0] = std::max(size_t(2), GetNumPoints(0));
		mNumPoints[1] = std::max(size_t(2), GetNumPoints(1));
		mNumPoints[2] = std::max(size_t(2), GetNumPoints(2));
	}
	PrecomputeSpacing();
}

void UniformGridGeometry::IndicesOfPosition(size_t indices[3], const vec3 & vPosition) const
{
	// Notice the pecular test here.  vPosition may lie slightly outside of the extent give by vMax.
	// Review the geometry described in the class header comment.
	vec3 vPosRel(vPosition - GetMinCorner());   // position of given point relative to container region
	vec3 vIdx(vPosRel.x * GetCellsPerExtent().x, vPosRel.y * GetCellsPerExtent().y, vPosRel.z * GetCellsPerExtent().z);
	indices[0] = unsigned(vIdx.x);
	indices[1] = unsigned(vIdx.y);
	indices[2] = unsigned(vIdx.z);
}

size_t UniformGridGeometry::OffsetOfPosition(const vec3 & vPosition) {
	size_t indices[3];
	IndicesOfPosition(indices, vPosition);
	const size_t offset = indices[0] + GetNumPoints(0) * (indices[1] + GetNumPoints(1) * indices[2]);
	return offset;
}

void UniformGridGeometry::PositionFromIndices(vec3 & vPosition, const size_t indices[3]) const
{
	vPosition.x = GetMinCorner().x + float(indices[0]) * GetCellSpacing().x;
	vPosition.y = GetMinCorner().y + float(indices[1]) * GetCellSpacing().y;
	vPosition.z = GetMinCorner().z + float(indices[2]) * GetCellSpacing().z;
}

void UniformGridGeometry::IndicesFromOffset(size_t indices[3], const size_t & offset)
{
	indices[2] = offset / (GetNumPoints(0) * GetNumPoints(1));
	indices[1] = (offset - indices[2] * GetNumPoints(0) * GetNumPoints(1)) / GetNumPoints(0);
	indices[0] = offset - GetNumPoints(0) * (indices[1] + GetNumPoints(1) * indices[2]);
}

void UniformGridGeometry::PositionFromOffset(vec3 & vPos, const size_t & offset)
{
	size_t indices[3];
	IndicesFromOffset(indices, offset);
	vPos.x = GetMinCorner().x + float(indices[0]) * GetCellSpacing().x;
	vPos.y = GetMinCorner().y + float(indices[1]) * GetCellSpacing().y;
	vPos.z = GetMinCorner().z + float(indices[2]) * GetCellSpacing().z;
}