#pragma once
#include <iostream>


#define TEST(condition, info) \
    if (!(condition)) { \
        std::cout << __FILE__ << "-" << __LINE__ << ": Test failed: " << info << std::endl; \
        all_passed = false; \
    }

#define PASSED \
    if (all_passed) { \
        std::cout << __FILE__ << " passed." << std::endl; \
    } \
    else { \
        all_passed = true; \
    }


inline bool fequal(const double a, const double b, const double epsilon = 1e-12)
{
    return std::abs(a - b) < epsilon;
}
