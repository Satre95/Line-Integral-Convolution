#include "Mat3.hpp"

const Mat3 Mat3::sIdentity = Mat3();

const float & Mat3::operator()(size_t row, size_t col) const {
	return Get(row, col);
}

float & Mat3::operator()(size_t row, size_t col) {
	return Get(row, col);
}

const float & Mat3::Get(size_t row, size_t col) const {
	switch (col) { //ofVec3f doesn't return refs with the [] operator :(
	case 0:
		return mData.at(row).x;

	case 1:
		return mData.at(row).y;

	default:
		return mData.at(row).z;
	}
}
float & Mat3::Get(size_t row, size_t col) {
	switch (col) { //ofVec3f doesn't return refs with the [] operator :(
	case 0:
		return mData.at(row).x;

	case 1:
		return mData.at(row).y;

	default:
		return mData.at(row).z;
	}
}

ofVec3f & Mat3::operator[](size_t i) {
	return mData.at(i);
}

const ofVec3f & Mat3::operator[](size_t i) const {
	return mData.at(i);
}

//TODO: Optimize with SIMD intrinsics
Mat3 Mat3::operator+(const Mat3 &B) const {
	Mat3 result;
	for (size_t i = 0; i < 3; i++)
		for (size_t j = 0; j < 3; j++)
			result(j, i) = Get(j, i) + B(j, i);

	return result;
}

void Mat3::operator+=(const Mat3 &B) {
	for (size_t i = 0; i < 3; i++) {
		for (size_t j = 0; j < 3; j++) {
			Get(j, i) += B(j, i);
		}
	}
}

//TODO: Optimize with SIMD intrinsics
Mat3 Mat3::operator-(const Mat3 &B) const {
	Mat3 result;
	for (size_t i = 0; i < 3; i++)
		for (size_t j = 0; j < 3; j++)
			result(j, i) = Get(j, i) - B(j, i);

	return  result;
}

void Mat3::operator-=(const Mat3 &B) {
	for (size_t i = 0; i < 3; i++) {
		for (size_t j = 0; j < 3; j++) {
			Get(j, i) -= B(j, i);
		}
	}
}

//TODO: Optimize with SIMD intrinsics
Mat3 Mat3::operator*(float scalar) const {
	Mat3 result;
#pragma omp parallel for
	for (size_t i = 0; i < 3; i++)
		for (size_t j = 0; j < 3; j++)
			result(j, i) = Get(j, i) * scalar;

	return result;
}

//TODO: Optimize with SIMD intrinsics
void Mat3::operator*=(float scalar) {
	for (size_t i = 0; i < 3; i++)
		for (size_t j = 0; j < 3; j++)
			Get(j, i) *= scalar;
}

ofVec3f Mat3::operator*(const ofVec3f & vec) const {
	ofVec3f result;
	//Iterate over rows
	for (int i = 0; i < 3; i++) {
		ofVec3f row(Get(i, 0), Get(i, 1), Get(i, 2));
		row *= vec;
		result[i] = row.x + row.y + row.z;
	}

	return result;
}
