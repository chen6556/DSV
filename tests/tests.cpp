// #define _CRT_SECURE_NO_WARNINGS
#include "Utils.hpp"
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
#include "InsertPointTest.hpp"

int main()
{
    bool success = true;

    DistanceTest::run_tests(success);
    ClosestPointTest::run_tests(success);
    InsideTest::run_tests(success);
    FootTest::run_tests(success);
    ConversionTest::run_tests(success);
    IntersectionTest::run_tests(success);
    SplitTest::run_tests(success);
    OffsetTest::run_tests(success);
    BooleanTest::run_tests(success);
    EarCutTest::run_tests(success);
    TangencyPointTest::run_tests(success);
    ArchimedeanSpiralTest::run_tests(success);
    InsertPointTest::run_tests(success);

    return success ? 0 : 1;
}
