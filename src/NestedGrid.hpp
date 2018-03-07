#pragma once

#include "UniformGrid.hpp"
#include "ofVec3f.h"
#include <vector>
#include <array>
#include <algorithm>

/// Templated nested uniform grid container, a hierarchical, octree-like spatial partition.
template <class TypeT> class NestedGrid {
public:
	typedef UniformGrid<TypeT> Layer;

	NestedGrid();
	explicit NestedGrid(const Layer & src);
	NestedGrid(const NestedGrid & other) = delete;
	~NestedGrid();

	void Initialize(const Layer & src);
	void AddLayer(const UniformGridGeometry & layerTemplate, size_t iDecimation);
	size_t GetDepth() const { return mLayers.size(); }
	Layer & operator[](size_t index) { return mLayers.at(index); }
	const Layer & operator[](size_t index) const { return mLayers.at(index); }
	const size_t * GetDecimations(size_t iParentLayer) const { return mDecimations.at(iParentLayer); }

	/*! \brief Get indices of minimal cell in child layer of cluster represented by specified cell in parent layer.

			Each cell in a parent layer represents a grid cluster of typically 8 cells
			in the child layer.  This routine calculates the index of the "minimal"
			cell in the child layer grid cluster, i.e. the cell in the child layer
			which corresponds to minimum corner cell of the grid cluster represented
			by the cell in the parent layer with the specified index.

			The cells in the child layer that belong to the same grid cluster would
			be visited by this code:

			\verbatim

				int i[3] ; // i is the increment past the minimum corner cell in the grid cluster.
				int j[3] ; // j indexes into the child layer.
				for( i[2] = 0 ; i[2] <= decimations[2] ; ++ i[2] )
				{
					j[2] = i[2] + clusterMinIndices[2] ;
					for( i[1] = 0 ; i[1] <= decimations[1] ; ++ i[1] )
					{
						j[1] = i[1] + clusterMinIndices[1] ;
						for( i[0] = 0 ; i[0] <= decimations[0] ; ++ i[0] )
						{
							j[0] = i[0] + clusterMinIndices[0] ;
							// Use j to index into child layer.
						}
					}
				}

			\endverbatim

			\param clusterMinIndices - (out) index of minimal cell in child layer grid cluster represented by given parent cell

			\param decimations - (in) ratios of dimensions of child layer to its parent, for each axis.
					This must be the same as the result of calling GetDecimations for the intended parent layer.

			\param indicesOfParentCell - (in) index of cell in parent layer.

			\see GetDecimations

		*/
	void GetChildClusterMinCornerIndex(size_t clusterMinIndices[3], const size_t decimations[3], const size_t indicesOfParentCell[3]);

	void Clear();

private:

	/*! \brief Precompute the total number of layers this nested grid will contain

		\param src - UniformGrid upon which this NestedGrid is based.

	*/
	size_t PrecomputeNumLayers(const Layer & src);

	/*! \brief Compute decimations, in each direction, for specified parent layer

		\param decimations - (out) ratio of dimensions between child layer and its parent.

		\param iParentLayer - index of parent layer.
							Child has index iParentLayer-1.
							Layer 0 has no child so providing "0" is invalid.

		This method effectively gives the number of child cells in each
		grid cluster that a parent cell represents.

		Each non-leaf layer in this NestedGrid is a decimation of its child
		layer. Typically that decimation is 2 in each direction, but the
		decimation can also be 1, or, more atypically, any other integer.
		Each child typically has twice as many cells in each direction as
		its parent.

		\note This assumes each parent has an integer decimation of its child.

		\see GetDecimations
	*/
	void ComputeDecimations(size_t decimations[3], size_t iParentLayer) const;

	/*! \brief Precompute decimations for each layer.

	   This provides the number of grid cells per cluster
	   of a child of each layer.

	   \note The child layer has index one less than the parent layer index.
			   That implies there is no such thing as "parent layer 0".
			   Layer 0  has no children. That further implies there is no
			   meaningful value for decimations at iParentLayer==0.

   */
	void PrecomputeDecimations();

	/// Dynamic array of uniform grids
	std::vector<Layer> mLayers;
	/// Cache of cluster sizes.
	std::array<size_t*, 3> mDecimations;
};

template <class TypeT>
NestedGrid<TypeT>::NestedGrid() { mDecimations.fill(nullptr); }

template <class TypeT>
NestedGrid<TypeT>::NestedGrid(const Layer & src) {
}

template <class TypeT>
NestedGrid<TypeT>::~NestedGrid() {
	for (size_t *& aDecimation : mDecimations) delete[] aDecimation;
}

template <class TypeT>
void NestedGrid<TypeT>::Initialize(const Layer & src) {
	mLayers.clear();
	const size_t numLayers = PrecomputeNumLayers(src);
	mLayers.reserve(numLayers);  // Preallocate number of layers to avoid reallocation during PushBack.
	AddLayer(src, 1);
	size_t index = 1;
	while (mLayers.at(index - 1).GetGridCapacity() > 8 /* a cell has 8 corners */)
	{   // Layer to decimate has more than 1 cell.
		AddLayer(mLayers.at(index - 1), 2); // Initialize child layer based on decimation of its parent grid.
		++index;
	}

	PrecomputeDecimations();
}

template <class TypeT>
void NestedGrid<TypeT>::AddLayer(const UniformGridGeometry & layerTemplate, size_t iDecimation) {
	mLayers.emplace_back();
	mLayers.back().Decimate(layerTemplate, (int)iDecimation);
	mLayers.back().Init();
}

template <class TypeT>
void NestedGrid<TypeT>::GetChildClusterMinCornerIndex(size_t clusterMinIndices[3], const size_t decimations[3], const size_t indicesOfParentCell[3]) {
	clusterMinIndices[0] = indicesOfParentCell[0] * decimations[0];
	clusterMinIndices[1] = indicesOfParentCell[1] * decimations[1];
	clusterMinIndices[2] = indicesOfParentCell[2] * decimations[2];
}

template <class TypeT>
void NestedGrid<TypeT>::Clear() {
	for (size_t iLayer = 0; iLayer < GetDepth(); ++iLayer)
		mLayers.at(iLayer).Clear();
	mLayers.clear();
}

template <class TypeT>
size_t NestedGrid<TypeT>::PrecomputeNumLayers(const Layer & src)
{
	size_t numLayers = 1;    // Tally src layer.
	size_t numPoints[3] = { src.GetNumPoints(0) , src.GetNumPoints(1) , src.GetNumPoints(2) };
	size_t size = numPoints[0] * numPoints[1] * numPoints[2];
	while (size > 8 /* a cell has 8 corners */)
	{   // Layer has more than 1 cell.
		++numLayers;
		// Decimate number of cells (where #cells = #points-1):
		numPoints[0] = std::max((numPoints[0] - 1) / 2, size_t(1)) + 1;
		numPoints[1] = std::max((numPoints[1] - 1) / 2, size_t(1)) + 1;
		numPoints[2] = std::max((numPoints[2] - 1) / 2, size_t(1)) + 1;
		size = numPoints[0] * numPoints[1] * numPoints[2];
	}
	return numLayers;
}

//TODO: replace with std::tuple refs
template <class TypeT>
void NestedGrid<TypeT>::ComputeDecimations(size_t decimations[3], size_t iParentLayer) const {
	const Layer & parent = (*this)[iParentLayer];
	const Layer & child = (*this)[iParentLayer - 1];
	decimations[0] = child.GetNumCells(0) / parent.GetNumCells(0);
	decimations[1] = child.GetNumCells(1) / parent.GetNumCells(1);
	decimations[2] = child.GetNumCells(2) / parent.GetNumCells(2);
}

template <class TypeT>
void NestedGrid<TypeT>::PrecomputeDecimations() {
	const size_t numLayers = GetDepth();

	for (auto *& aDec : mDecimations) {
		delete[] aDec;
		aDec = new size_t[numLayers];
	}

	// Precompute decimations for each layer.
	for (size_t iLayer = 1; iLayer < numLayers; ++iLayer)
	{   // For each parent layer...
		ComputeDecimations(mDecimations.at(iLayer), iLayer);
	}
	// Layer 0 is strictly a child (i.e. has no children), so has no decimations.
	// Assign the values with useless nonsense to make this more obvious.
	mDecimations.at(0)[0] = mDecimations.at(0)[1] = mDecimations.at(0)[2] = 0;
}
