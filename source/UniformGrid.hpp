#pragma once

#include <cinder/Vector.h>
#include "UniformGridGeometry.hpp"
#include <vector>

template <class TypeT>
class UniformGrid: public UniformGridGeometry {
public:
	typedef UniformGridGeometry Parent;

	UniformGrid(): UniformGridGeometry() {}

	/*! \brief Construct a uniform grid container that fits the given geometry.
		\see Initialize
    */
	UniformGrid( unsigned uNumElements , const glm::vec3 & vMin , const glm::vec3 & vMax , bool bPowerOf2 = true)
	: UniformGridGeometry( uNumElements , vMin , vMax , bPowerOf2 ) {}

    /// Copy shape from given uniform grid
	explicit UniformGrid( const UniformGridGeometry & that )
	: UniformGridGeometry( that ){}

    // Delete the copy ctor & assignment, as copying is far too expensive
	UniformGrid(const UniformGrid & other) = delete;
	UniformGrid & operator=(const UniformGrid & other) = delete;

	~UniformGrid() {}

	TypeT & 	operator[]( const size_t & offset)					{ return mContents.at(offset); }
	const TypeT & 	operator[]( const size_t & offset) const		{ return mContents.at(offset); }

	/// Initialize contents to whatever default ctor provides
	void Init() { mContents.resize(GetGridCapacity()); }

	virtual void DefineShape( size_t uNumElements , const glm::vec3 & vMin , const glm::vec3 & vMax , bool bPowerOf2 ) override {
		mContents.clear();
		Parent::DefineShape(uNumElements, vMin, vMax, bPowerOf2);
	}

	size_t Size() const { return mContents.size(); }

    /*! \brief Compute statistics of data in a uniform grid.

        \param min - minimum of all values in grid.  Caller must initialize to large values before calling this routine.

        \param max - maximum of all values in grid.  Caller must initialize to smale values before calling this routine.

    */
	void ComputeStatistics( TypeT & min , TypeT & max ) const {
		max = min = (*this)[ 0 ] ;
		const size_t numCells = GetGridCapacity() ;
		for( size_t offset = 0 ; offset < numCells ; ++ offset )
		{
			const TypeT & rVal = (*this)[ offset ] ;
			min = glm::min( min , rVal ) ;
			max = glm::max( max , rVal ) ;
		}
	}
    /*! \brief Interpolate values from grid to get value at given position

	    \param vPosition - position to sample

	    \return Interpolated value corresponding to value of grid contents at vPosition.

    */
	void Interpolate(TypeT &vResult, const glm::vec3 &vPosition) const {
		//TODO: Replace with std::tuple refs to avoid unnecessary copies.
		size_t        indices[3] ; // Indices of grid cell containing position.
		Parent::IndicesOfPosition( indices , vPosition ) ;
		glm::vec3            vMinCorner ;
		Parent::PositionFromIndices( vMinCorner , indices ) ;
		const unsigned  offsetX0Y0Z0 = OffsetFromIndices( indices ) ;
	            const glm::vec3      vDiff         = vPosition - vMinCorner ; // Relative location of position within its containing grid cell.
	            const glm::vec3      tween         = glm::vec3( vDiff.x * GetCellsPerExtent().x , vDiff.y * GetCellsPerExtent().y , vDiff.z * GetCellsPerExtent().z ) ;
	            const glm::vec3      oneMinusTween = glm::vec3( 1.0f , 1.0f , 1.0f ) - tween ;
	            const unsigned  numXY         = GetNumPoints( 0 ) * GetNumPoints( 1 ) ;
	            const unsigned  offsetX1Y0Z0  = offsetX0Y0Z0 + 1 ;
	            const unsigned  offsetX0Y1Z0  = offsetX0Y0Z0 + GetNumPoints(0) ;
	            const unsigned  offsetX1Y1Z0  = offsetX0Y0Z0 + GetNumPoints(0) + 1 ;
	            const unsigned  offsetX0Y0Z1  = offsetX0Y0Z0 + numXY ;
	            const unsigned  offsetX1Y0Z1  = offsetX0Y0Z0 + numXY + 1 ;
	            const unsigned  offsetX0Y1Z1  = offsetX0Y0Z0 + numXY + GetNumPoints(0) ;
	            const unsigned  offsetX1Y1Z1  = offsetX0Y0Z0 + numXY + GetNumPoints(0) + 1 ;
	            vResult = oneMinusTween.x * oneMinusTween.y * oneMinusTween.z * (*this)[ offsetX0Y0Z0 ]
	            +         tween.x * oneMinusTween.y * oneMinusTween.z * (*this)[ offsetX1Y0Z0 ]
	            + oneMinusTween.x *         tween.y * oneMinusTween.z * (*this)[ offsetX0Y1Z0 ]
	            +         tween.x *         tween.y * oneMinusTween.z * (*this)[ offsetX1Y1Z0 ]
	            + oneMinusTween.x * oneMinusTween.y *         tween.z * (*this)[ offsetX0Y0Z1 ]
	            +         tween.x * oneMinusTween.y *         tween.z * (*this)[ offsetX1Y0Z1 ]
	            + oneMinusTween.x *         tween.y *         tween.z * (*this)[ offsetX0Y1Z1 ]
	            +         tween.x *         tween.y *         tween.z * (*this)[ offsetX1Y1Z1 ] ;
	}

    /// Insert given value into grid at given position
    void Insert( const glm::vec3 & vPosition , const TypeT & item ) {
        size_t        indices[3] ; // Indices of grid cell containing position.
        Parent::IndicesOfPosition( indices , vPosition ) ;
        glm::vec3            vMinCorner ;
        PositionFromIndices( vMinCorner , indices ) ;
        const unsigned  offsetX0Y0Z0 = OffsetFromIndices( indices ) ;
        const glm::vec3      vDiff         = vPosition - vMinCorner ; // Relative location of position within its containing grid cell.
        const glm::vec3      tween         = glm::vec3( vDiff.x * GetCellsPerExtent().x , vDiff.y * GetCellsPerExtent().y , vDiff.z * GetCellsPerExtent().z ) ;
        const glm::vec3      oneMinusTween = glm::vec3( 1.0f , 1.0f , 1.0f ) - tween ;
        const unsigned  numXY         = GetNumPoints( 0 ) * GetNumPoints( 1 ) ;
        const unsigned  offsetX1Y0Z0  = offsetX0Y0Z0 + 1 ;
        const unsigned  offsetX0Y1Z0  = offsetX0Y0Z0 + GetNumPoints(0) ;
        const unsigned  offsetX1Y1Z0  = offsetX0Y0Z0 + GetNumPoints(0) + 1 ;
        const unsigned  offsetX0Y0Z1  = offsetX0Y0Z0 + numXY ;
        const unsigned  offsetX1Y0Z1  = offsetX0Y0Z0 + numXY + 1 ;
        const unsigned  offsetX0Y1Z1  = offsetX0Y0Z0 + numXY + GetNumPoints(0) ;
        const unsigned  offsetX1Y1Z1  = offsetX0Y0Z0 + numXY + GetNumPoints(0) + 1 ;
        (*this)[ offsetX0Y0Z0 ] += oneMinusTween.x * oneMinusTween.y * oneMinusTween.z * item ;
        (*this)[ offsetX1Y0Z0 ] +=         tween.x * oneMinusTween.y * oneMinusTween.z * item ;
        (*this)[ offsetX0Y1Z0 ] += oneMinusTween.x *         tween.y * oneMinusTween.z * item ;
        (*this)[ offsetX1Y1Z0 ] +=         tween.x *         tween.y * oneMinusTween.z * item ;
        (*this)[ offsetX0Y0Z1 ] += oneMinusTween.x * oneMinusTween.y *         tween.z * item ;
        (*this)[ offsetX1Y0Z1 ] +=         tween.x * oneMinusTween.y *         tween.z * item ;
        (*this)[ offsetX0Y1Z1 ] += oneMinusTween.x *         tween.y *         tween.z * item ;
        (*this)[ offsetX1Y1Z1 ] +=         tween.x *         tween.y *         tween.z * item ;
    }

    void Clear() override {
    	mContents.clear();
    	Parent::Clear();
    }

private:
	std::vector<TypeT> mContents;
};