#pragma once

#include <random>
#include <cstdint>

class RandomNumberGenerator {

public:

    template <class INT_T>
    static INT_T GetRandomIntegerBetween(INT_T min, INT_T max)
    {
        std::random_device rd;
        std::mt19937 mt(rd());
        std::uniform_int_distribution<> dis(min, max);

        return (INT_T)dis(mt);
    }


};


