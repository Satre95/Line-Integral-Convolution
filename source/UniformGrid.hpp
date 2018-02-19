#pragma once
#include <cinder/Vector.h>

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
    mMinCorner( 0.0f , 0.0f , 0.0f ), mGridExtent( 0.0f , 0.0f , 0.0f ),
    mCellExtent( 0.0f , 0.0f , 0.0f ), mCellsPerExtent( 0.0f , 0.0f , 0.0f )
    {
        mNumPoints[ 0 ] = mNumPoints[ 1 ] = mNumPoints[ 2 ] = 0 ;
    }
    
    ~UniformGridGeometry() {}
    
    /*!
     \brief Construct a uniform grid that fits the given geometry.
     
     \see Clear, DefineShape
     */
    UniformGridGeometry( unsigned uNumElements , const cinder::vec3 & vMin , const cinder::vec3 & vMax , bool bPowerOf2 = true)
    {
        DefineShape( uNumElements , vMin , vMax , bPowerOf2 ) ;
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
    void DefineShape( size_t uNumElements , const cinder::vec3 & vMin , const cinder::vec3 & vMax , bool bPowerOf2);
    
    //Getters and Setters
    cinder::vec3 & GetExtent() { return mGridExtent; }
    const cinder::vec3 & GetExtent() const { return mGridExtent; }
    
protected:
    cinder::vec3        mMinCorner      ;   ///< Minimum position (in world units) of grid in X, Y and Z directions.
    cinder::vec3        mGridExtent     ;   ///< Size (in world units) of grid in X, Y and Z directions.
    cinder::vec3        mCellExtent     ;   ///< Size (in world units) of a cell.
    cinder::vec3        mCellsPerExtent ;   ///< Reciprocal of cell size (precomputed once to avoid excess divides).
    unsigned            mNumPoints[ 3 ] ;   ///< Number of gridpoints along X, Y and Z directions.
    
};

class UniformGrid {
    public:
    UniformGrid(){}
    ~UniformGrid(){}
};


