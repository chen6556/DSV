#include "TangencyPointTest.hpp"
#include "Utils.hpp"
#include "../src/base/Algorithm.hpp"


static bool all_passed = true;

void TangencyPointTest::run_tests()
{
    test_circle();
    test_ellipse();
    test_bezier();
    test_bspline();
    PASSED
}

void TangencyPointTest::test_circle()
{
    {
        Geo::Circle circle(0, 0, 1);
        Geo::Point output0, output1;
        bool result = Geo::tangency_point(Geo::Point(0, 2), circle, output0, output1);
        TEST(result, "Tangent from above")
    }

    {
        Geo::Circle circle(0, 0, 1);
        Geo::Point output0, output1;
        bool result = Geo::tangency_point(Geo::Point(2, 0), circle, output0, output1);
        TEST(result, "Tangent from right")
    }

    {
        Geo::Circle circle(0, 0, 1);
        Geo::Point output0, output1;
        bool result = Geo::tangency_point(Geo::Point(0, -2), circle, output0, output1);
        TEST(result, "Tangent from below")
    }

    {
        Geo::Circle circle(0, 0, 1);
        Geo::Point output0, output1;
        bool result = Geo::tangency_point(Geo::Point(-2, 0), circle, output0, output1);
        TEST(result, "Tangent from left")
    }

    {
        Geo::Circle circle(0, 0, 2);
        Geo::Point output0, output1;
        bool result = Geo::tangency_point(Geo::Point(0, 5), circle, output0, output1);
        TEST(result, "Tangent from above larger circle")
    }

    {
        Geo::Circle circle(3, 3, 1);
        Geo::Point output0, output1;
        bool result = Geo::tangency_point(Geo::Point(3, 6), circle, output0, output1);
        TEST(result, "Tangent from non-origin circle")
    }
}

void TangencyPointTest::test_ellipse()
{
    {
        Geo::Ellipse ellipse(0, 0, 3, 2);
        Geo::Point output0, output1;
        bool result = Geo::tangency_point(Geo::Point(0, 4), ellipse, output0, output1);
        TEST(result, "Tangent from above ellipse")
    }

    {
        Geo::Ellipse ellipse(0, 0, 3, 2);
        Geo::Point output0, output1;
        bool result = Geo::tangency_point(Geo::Point(5, 0), ellipse, output0, output1);
        TEST(result, "Tangent from right of ellipse")
    }

    {
        Geo::Ellipse ellipse(0, 0, 3, 2);
        Geo::Point output0, output1;
        bool result = Geo::tangency_point(Geo::Point(0, -4), ellipse, output0, output1);
        TEST(result, "Tangent from below ellipse")
    }

    {
        Geo::Ellipse ellipse(0, 0, 2, 2);
        Geo::Point output0, output1;
        bool result = Geo::tangency_point(Geo::Point(0, 4), ellipse, output0, output1);
        TEST(result, "Tangent from circle as ellipse")
    }
}

void TangencyPointTest::test_bezier()
{
    {
        const Geo::CubicBezier bezier({Geo::Point(0, 0), Geo::Point(1, 2), Geo::Point(2, 2), Geo::Point(3, 0)}, true);
        std::vector<Geo::Point> output;
        int count = Geo::tangency_point(Geo::Point(1.5, 4), bezier, output);
        TEST(count >= 1, "Tangent from above bezier")
    }

    {
        const Geo::CubicBezier bezier({Geo::Point(0, 0), Geo::Point(0, 2), Geo::Point(3, 2), Geo::Point(3, 0)}, true);
        std::vector<Geo::Point> output;
        int count = Geo::tangency_point(Geo::Point(1.5, 4), bezier, output);
        TEST(count >= 1, "Tangent from above arch bezier")
    }
}

void TangencyPointTest::test_bspline()
{
    {
        const Geo::CubicBSpline bspline({Geo::Point(0, 0), Geo::Point(1, 2), Geo::Point(2, 2), Geo::Point(3, 0)}, true);
        std::vector<Geo::Point> output;
        int count = Geo::tangency_point(Geo::Point(1.5, 4), bspline, output);
        TEST(count >= 1, "Tangent from above cubic bspline")
    }

    {
        const Geo::QuadBSpline bspline({Geo::Point(0, 0), Geo::Point(1, 2), Geo::Point(2, 2), Geo::Point(3, 0)}, true);
        std::vector<Geo::Point> output;
        int count = Geo::tangency_point(Geo::Point(1.5, 4), bspline, output);
        TEST(count >= 1, "Tangent from above quad bspline")
    }
}
