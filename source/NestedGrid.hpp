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
	~NestedGrid();

private:
	/// Dynamic array of uniform grids
	std::vector<Layer> mLayers;
	/// Cache of cluster sizes.
	std::array<size_t, 3> mDecimations;
	
};