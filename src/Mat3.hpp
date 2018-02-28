#pragma once

#include <array>

class Mat3 {
public:
    Mat3() {}
    ~Mat3() {}
    
    const float & operator()(size_t row, size_t col) const;
    float & operator()(size_t row, size_t col);
    ofVec3f & operator[](size_t i);
    const ofVec3f & operator[](size_t i) const;

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


const float & Mat3::operator()(size_t row, size_t col) const {
    return Get(row, col);
}

float & Mat3::operator()(size_t row, size_t col) {
    return Get(row, col);
}

const float & Mat3::Get(size_t row, size_t col) const {
    switch (col) {
        case 0:
            return mData.at(row).x;
        
        case 1:
            return mData.at(row).y;
            
        default:
            return mData.at(row).z;
    }
}
float & Mat3::Get(size_t row, size_t col) {
    switch (col) {
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
