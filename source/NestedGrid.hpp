#pragma once

#include "UniformGrid.hpp"
#include <cinder/Vector.h>
#include <vector>
#include <array>

/// Templated nested uniform grid container, a hierarchical, octree-like spatial partition.
template <class TypeT> class NestedGrid {
public:
	typedef UniformGrid<TypeT> Layer;

	NestedGrid();
	NestedGrid(const Layer & src);
	~NestedGrid();

	void Initialize(const Layer & src);
	void AddLayer(const UniformGridGeometry & layerTemplate, size_t iDecimation);
	size_t GetDepth() const { return mLayers.size(); }
	Layer & operator[](size_t index) { return mLayers.at(index); }
	const Layer & operator[](size_t index) const { return mLayers.at(index); }
	const size_t * GetDecimations(size_t iParentLayer) const { return mDecimations.at(iParentLayer); }

private:
	/// Dynamic array of uniform grids
	std::vector<Layer> mLayers;
	/// Cache of cluster sizes.
	std::array<size_t*, 3> mDecimations;
};

template <class TypeT>
NestedGrid<TypeT>::NestedGrid() {}

template <class TypeT>
NestedGrid<TypeT>::NestedGrid(const Layer & src) {
}

template <class TypeT>
NestedGrid<TypeT>::~NestedGrid() {
	for (size_t *& aDecimation : mDecimations) delete[] aDecimation;
}

template <class TypeT>
void NestedGrid<TypeT>::Initialize(const Layer & src) {
	mLayers.Clear();
	const unsigned numLayers = PrecomputeNumLayers(src);
	mLayers.Reserve(numLayers);  // Preallocate number of layers to avoid reallocation during PushBack.
	AddLayer(src, 1);
	unsigned index = 1;
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
	mLayers.back().Decimate(layerTemplate, iDecimation);
	mLayers.back().Init();
}