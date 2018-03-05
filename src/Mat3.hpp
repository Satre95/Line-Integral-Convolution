#pragma once

#include <array>
#include "ofVec3f.h"

class Mat3 {
public:
	/// Inits a matrix to the identity.
	Mat3() {
		Get(0, 0) = 1.f; Get(1, 1) = 1.f; Get(2, 2) = 1.f;
	}

	const float & operator()(size_t row, size_t col) const;
	float & operator()(size_t row, size_t col);
	ofVec3f & operator[](size_t i);
	const ofVec3f & operator[](size_t i) const;

	/// \brief Add two matrices
	Mat3 operator+(const Mat3& B) const;

	/// \brief Add matrix to existing matrix
	void operator+=(const Mat3& B);

	/// \brief Subtract two matrices
	Mat3 operator-(const Mat3& B) const;

	/// \brief Subtract matrix from existing matrix
	void operator-=(const Mat3& B);

	/// \brief Multiply a matrix with a scalar
	Mat3 operator*(float scalar) const;

	/// \brief Multiply a matrix by a matrix this = this*B (in that order)
	void operator*=(const Mat3& B);

	/// \brief Multiply a matrix by a scalar (multiples all entries by scalar)
	void operator*=(float scalar);

	/// \brief Multiply this matrix with a vector and return a a vector.
	ofVec3f operator*(const ofVec3f & vec) const;

private:
	const float & Get(size_t row, size_t col) const;
	float & Get(size_t row, size_t col);

	/**
	 \brief The array of matrix values.

	 Values are stored in column major.

	 \verbatim
	 [ a d g ]
	 [ b e h ]
	 [ c f i ]
	 \verbatim
	*/
	std::array<ofVec3f, 3> mData;
};

inline Mat3 operator*(const float & scalar, const Mat3 & mat) {
	Mat3 result;
#pragma omp parallel for
	for (size_t i = 0; i < 3; i++)
		for (size_t j = 0; j < 3; j++)
			result(j, i) = mat(j, i) * scalar;

	return result;
}