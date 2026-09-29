#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <chrono>
#include "DistanceTest.hpp"
#include "ClosestPointTest.hpp"
#include "InsideTest.hpp"
#include "FootTest.hpp"
#include "ConversionTest.hpp"
#include "IntersectionTest.hpp"
#include "SplitTest.hpp"
#include "OffsetTest.hpp"
#include "BooleanTest.hpp"
#include "EarCutTest.hpp"
#include "TangencyPointTest.hpp"
#include "ArchimedeanSpiralTest.hpp"

int main()
{
    {
        const std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
        const std::time_t now_c = std::chrono::system_clock::to_time_t(now);
        const std::tm *ltm = std::localtime(&now_c);
        std::cout << "Test start: "
                  << 1900 + ltm->tm_year << '-' << 1 + ltm->tm_mon << '-'
                  << ltm->tm_mday << ' ' << ltm->tm_hour << ':'
                  << ltm->tm_min << ":" << ltm->tm_sec << std::endl;
    }

    DistanceTest::run_tests();
    ClosestPointTest::run_tests();
    InsideTest::run_tests();
    FootTest::run_tests();
    ConversionTest::run_tests();
    IntersectionTest::run_tests();
    SplitTest::run_tests();
    OffsetTest::run_tests();
    BooleanTest::run_tests();
    EarCutTest::run_tests();
    TangencyPointTest::run_tests();
    ArchimedeanSpiralTest::run_tests();

    {
        const std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
        const std::time_t now_c = std::chrono::system_clock::to_time_t(now);
        const std::tm *ltm = std::localtime(&now_c);
        std::cout << "Test end: "
                  << 1900 + ltm->tm_year << '-' << 1 + ltm->tm_mon << '-'
                  << ltm->tm_mday << ' ' << ltm->tm_hour << ':'
                  << ltm->tm_min << ":" << ltm->tm_sec << std::endl;
    }
    return 0;
}
