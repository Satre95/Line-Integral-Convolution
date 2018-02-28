#pragma once

#include <array>

class Mat3 {
public:
    Mat3() {}
    ~Mat3() {}
    
    const float & operator()(size_t row, size_t col) const;
    float & operator()(size_t row, size_t col);
    ofVec3f operator[](size_t i);
    const ofVec3f operator[](size_t i) const;

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
    std::array<float, 9> mData;
};


const float & Mat3::operator()(size_t row, size_t col) const {
    return Get(row, col);
}

float & Mat3::operator()(size_t row, size_t col) {
    return Get(row, col);
}

const float & Mat3::Get(size_t row, size_t col) const {
    return mData.at(col * 3 + row);
}
float & Mat3::Get(size_t row, size_t col) {
    return mData.at(col * 3 + row);
}

ofVec3f Mat3::operator[](size_t i) {
    return ofVec3f(Get(0, i), Get(1, i), Get(2, i));
}

const ofVec3f Mat3::operator[](size_t i) const {
    return ofVec3f(Get(0, i), Get(1, i), Get(2, i));
}
