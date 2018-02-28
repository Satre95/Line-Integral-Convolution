#pragma once

#include <array>

class Mat3 {
public:
    /// Inits a matrix to the identity.
    Mat3() {
        Get(0,0) = 1.f; Get(1,1) = 1.f; Get(2,2) = 1.f;
    }
    
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
