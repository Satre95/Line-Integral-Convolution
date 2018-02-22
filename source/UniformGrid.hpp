#pragma once

#include <cinder/Vector.h>
#include "UniformGridGeometry.hpp"

template <class T>
class UniformGrid: public UniformGridGeometry {
public:
	typedef UniformGridGeometry Parent;

	UniformGrid(): UniformGridGeometry() {}

	/*! \brief Construct a uniform grid container that fits the given geometry.
		\see Initialize
    */
    UniformGrid( unsigned uNumElements , const cinder::vec3 & vMin , const cinder::vec3 & vMax , bool bPowerOf2 = true)
        : UniformGridGeometry( uNumElements , vMin , vMax , bPowerOf2 ) {}

    /// Copy shape from given uniform grid
    explicit UniformGrid( const UniformGridGeometry & that )
        : UniformGridGeometry( that ){}

	~UniformGrid() {}
};