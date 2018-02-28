#pragma once
#include "ofVectorMath.h"

/*! \brief Base class for uniform grid.

 The shape of this grid is such that the "minimal corner" point
 resides at indices {0,0,0} and the "maximal corner" point
 resides at indices {Nx-1,Ny-1,Nz-1}.

 The number of /points/ in each direction i is N_i.

 A cell is defined by the 8 points that lie at its corners.

 This also implies that the grid must have at least 2 points in
 each direction.

 The number of /cells/ in each direction i is (N_i-1).

 The size of a side i of each cell is therefore
 s_i = (vMax-vMin)_i / (N_i-1) .

 \verbatim

				vMin
				0       1     . . .      Nx-1
				(*)------*-------*-------* 0
				/|       |       |       |
			 . / |       |       |       |
			. *  |       |       |       |
		   . /|  *-------*-------*-------* 1
			/ | /|       |       |       |
	  Nz-1 *  |/ |       | cell  |       |
		   |  *  |       |       |       | .
		   | /|  *-------*-------*-------* .
		   |/ | /|       |       |       | .
		   *  |/ |       |       |       |
		   |  *  |       |       |       |
		   | /|  *-------*-------*-------* Ny-1
		   |/ | /       /       /       /
		   *  |/       /       /       /
		   |  *-------*-------*-------*
		   | /       /       /       /
		   |/       /       /       /
		   *-------*-------*------(*)vMax

 \endverbatim
 */
class UniformGridGeometry {
public:
	/*!
	 \brief Construct an empty UniformGridGeometry.

	 \see Clear
	 */
	UniformGridGeometry() :
		mMinCorner(0.0f, 0.0f, 0.0f), mGridExtent(0.0f, 0.0f, 0.0f),
		mCellExtent(0.0f, 0.0f, 0.0f), mCellsPerExtent(0.0f, 0.0f, 0.0f)
	{
		mNumPoints[0] = mNumPoints[1] = mNumPoints[2] = 0;
	}

	UniformGridGeometry(const UniformGridGeometry & other) {
		Decimate(other, 1);
	}

	virtual ~UniformGridGeometry() {}

	/*!
	 \brief Construct a uniform grid that fits the given geometry.

	 \see Clear, DefineShape
	 */
	UniformGridGeometry(size_t uNumElements, const ofVec3f & vMin, const ofVec3f & vMax, bool bPowerOf2 = true)
	{
		DefineShape(uNumElements, vMin, vMax, bPowerOf2);
	}

	/*! \brief Define the shape a uniform grid such that it fits the given geometry.

	 \param uNumElements - number of elements this container will contain.

	 \param vMin - minimal coordinate of axis-aligned bounding box.

	 \param vMax - maximal coordinate of axis-aligned bounding box.

	 \param bPowerOf2 - whether to make each grid dimension a power of 2.
	 Doing so simplifies grid subdivision, if this grid will be used in a hierarchical grid.

	 This makes a uniform grid of cells, where each cell is the same size
	 and the side of each cell is nearly the same size.  If the cells are
	 3-dimensional then that means each cell is a box, nearly a cube.
	 The number of dimensions of the region depends on the actual size of
	 the region.  If any size component is smaller than a small threshold
	 then this class considers that component to be zero, and reduces the
	 dimensionality of the region.  For example, if the region size is
	 (2,3,0) then this class considers the region to have 2 dimensions
	 (x and y) since the z size is zero.
	 */
	virtual void DefineShape(size_t uNumElements, const ofVec3f & vMin, const ofVec3f & vMax, bool bPowerOf2);

	/*! \brief Create a lower-resolution uniform grid based on another

	\param src - Source uniform grid upon which to base dimensions of this one

	\param iDecimation - amount by which to reduce the number of grid cells in each dimension.
	Typically this would be 2.

	\note The number of cells is decimated.  The number of points is different.

	*/
	virtual void Decimate(const UniformGridGeometry & src, int iDecimation);

	/*! \brief Compute indices into contents array of a point at a given position

	\param vPosition - position of a point.  It must be within the region of this container.

	\param indices - Indices into contents array of a point at vPosition.

	\see IndicesFromOffset, PositionFromOffset, OffsetOfPosition.

	\note Derived class defines the actual contents array.

	*/
	virtual void IndicesOfPosition(size_t indices[3], const ofVec3f & vPosition) const;

	/*! \brief Compute offset into contents array of a point at a given position

	\index vPosition - position of a point.  It must be within the region of this container.

	\return Offset into contents array of a point at vPosition.

	\see IndicesFromOffset, PositionFromOffset.

	\note Derived class defines the actual contents array.

	*/
	virtual size_t OffsetOfPosition(const ofVec3f & vPosition);

	/*! \brief Compute position of minimal corner of grid cell with given indices

	\param position - position of minimal corner of grid cell

	\param indices - grid cell indices.

	\note Rarely if ever would you want to compute position from indices in this way.
	Typically, this kind of computation occurs inside a triply-nested loop,
	in which case the procedure should compute each component
	separately.  Furthermore, such a routine would cache
	GetCellSpacing instead of computing it each iteration.

	*/
	virtual void PositionFromIndices(ofVec3f & vPosition, const size_t indices[3]) const;

	/*! \brief Compute X,Y,Z grid cell indices from offset into contents array.

	\param indices - Individual X,Y,Z component grid cell indices.

	\param offset - Offset into mContents.
	*/
	virtual void    IndicesFromOffset(size_t indices[3], const size_t & offset);

	/*! \brief Get position of grid cell minimum corner.

	\param vPos - position of grid cell minimum corner

	\param offset - offset into contents array

	Each grid cell spans a region (whose size is given by GetCellSpacing)
	starting at a location which this routine returns.  So the grid cell
	with the given offset spans the region from vPos (as this routine
	assigns) to vPos + GetCellSpacing().

	\note Derived class provides actual contents array.

	*/
	virtual void    PositionFromOffset(ofVec3f & vPos, const size_t & offset);

	//Getters and Setters
	ofVec3f & GetExtent() { return mGridExtent; }
	const ofVec3f & GetExtent() const { return mGridExtent; }

	/// Get number of grid cells along the given dimension
	size_t GetNumCells(const size_t & index) const {
		return GetNumPoints(index) - 1;
	}

	/// Get number of grid points along the given dimension
	const size_t & GetNumPoints(const size_t & index) const {
		return mNumPoints[index];
	}

	const ofVec3f & GetMinCorner() const { return mMinCorner; }
	ofVec3f & GetMinCorner() { return mMinCorner; }
	const ofVec3f & GetCellsPerExtent() const { return mCellsPerExtent; }
	ofVec3f & GetCellsPerExtent() { return mCellsPerExtent; }
	size_t GetGridCapacity() const { return GetNumPoints(0) * GetNumPoints(1) * GetNumPoints(2); }
	const ofVec3f & GetCellSpacing() const { return mCellExtent; }
	ofVec3f & GetCellSpacing() { return mCellExtent; }

protected:

	/// Precompute grid spacing, to optimize OffsetOfPosition and other utility routines.
	void PrecomputeSpacing();

	/*! \brief Get offset into contents array given indices

	\param indices - indices specifying a grid cell

	\return offset into contents array

	\note Typically this routine would not be efficient to use, except for special cases.
	Often, one writes a triple-nested loop iterating over each
	component of indices, in which case it is more efficient
	to compute the z and y terms of the offset separately and
	combine them with the x term in the inner-most loop.
	This routine is useful primarily when there is no coherence
	between the indices of this iteration and the previous or next.

	\note Derived class provides actual contents array.
	*/
	size_t OffsetFromIndices(const size_t indices[3]) const
	{
		return indices[0] + GetNumPoints(0) * (indices[1] + GetNumPoints(1) * indices[2]);
	}

	/// Clear out any existing shape information
	virtual void Clear()
	{
		mMinCorner =
			mGridExtent =
			mCellExtent =
			mCellsPerExtent = ofVec3f(0.0f, 0.0f, 0.0f);
		mNumPoints[0] = mNumPoints[1] = mNumPoints[2] = 0;
	}

	ofVec3f        mMinCorner;   ///< Minimum position (in world units) of grid in X, Y and Z directions.
	ofVec3f        mGridExtent;   ///< Size (in world units) of grid in X, Y and Z directions.
	ofVec3f        mCellExtent;   ///< Size (in world units) of a cell.
	ofVec3f        mCellsPerExtent;   ///< Reciprocal of cell size (precomputed once to avoid excess divides).
	size_t              mNumPoints[3];   ///< Number of gridpoints along X, Y and Z directions.
};
