#include "Rand.hpp"

std::mt19937 Rand::sBase( time(nullptr) );
std::uniform_real_distribution<float> Rand::sFloatGen;
