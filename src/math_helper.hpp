#pragma once
#include "Rand.hpp"
#include "ofVec3f.h"

// A set of helper math functions such as fast inverse sqrt, etc.

/*!
 \brief Fast reciprocal square root
 
 \note This assumes "float" uses IEEE 754 format.

 \see Paul Hsieh's Square Root page: http://www.azillionmonkeys.com/qed/sqroot.html

 \see Charles McEniry (2007): The mathematics behind the fast inverse square root function code

 \see Chris Lomont: Fast inverse square root
 */
inline float finvsqrtf(const float & val ) {
    long    i   = (long&) val ;             // Exploit IEEE 754 inner workings.
    i  = 0x5f3759df - ( i >> 1 ) ;          // From Taylor's theorem and IEEE 754 format.
    float   y   = (float&) i ;              // Estimate of 1/sqrt(val) close enough for convergence using Newton's method.
    static const float  f   = 1.5f ;        // Derived from Newton's method.
    const float         x   = val * 0.5f ;  // Derived from Newton's method.
    y  = y * ( f - ( x * y * y ) ) ;        // Newton's method for 1/sqrt(val)
    y  = y * ( f - ( x * y * y ) ) ;        // Another iteration of Newton's method
    return y ;
}

/*!
 \brief Fast square root
 
 This computes val/sqrt(val) (which is sqrt(val)) so uses the 1/sqrt formula of finvsqrtf.

 \note This assumes "float" uses IEEE 754 format.

 \see Paul Hsieh's Square Root page: http://www.azillionmonkeys.com/qed/sqroot.html

 \see Charles McEniry (2007): The mathematics behind the fast inverse square root function code

 \see Chris Lomont: Fast inverse square root
 */
inline float fsqrtf( const float & val )
{
    long    i   = (long&) val ;             // Exploit IEEE 754 inner workings.
    i  = 0x5f3759df - ( i >> 1 ) ;          // From Taylor's theorem and IEEE 754 format.
    float   y   = (float&) i ;              // Estimate of 1/sqrt(val) close enough for convergence using Newton's method.
    static const float  f   = 1.5f ;        // Derived from Newton's method.
    const float         x   = val * 0.5f ;  // Derived from Newton's method.
    y  = y * ( f - ( x * y * y ) ) ;        // Newton's method for 1/sqrt(val)
    y  = y * ( f - ( x * y * y ) ) ;        // Another iteration of Newton's method
    return val * y ;                        // Return val / sqrt(val) which is sqrt(val)
}

/*! \brief compute nearest power of 2 greater than or equal to the given value
 */
inline unsigned int NearestPowerOfTwoExponent( unsigned int iVal )
{
    if( 0 == iVal ) return 0 ;
    --iVal ; // exact powers of two otherwise produce the wrong result below
    unsigned int shift = 0 ;
    while( ( iVal >> shift ) != 0 ) ++ shift ;
    return shift ;
}

/*! \brief Compute nearest power of 2 greater than or equal to the given value
 */
inline unsigned int NearestPowerOfTwo( unsigned int iVal )
{
    return 1 << NearestPowerOfTwoExponent( iVal ) ;
}

/*! \brief Generate a random 3D vector in the given range.
    \param range Each component will be in [-range/2, range/2).
 
    \note range components must be positive.
 */
inline ofVec3f RandomSpread(ofVec3f range) {
    static bool firstRun = true;
    if(firstRun) { Rand::randomize(); firstRun = false;}
    
    return ofVec3f(
    
    );
}
