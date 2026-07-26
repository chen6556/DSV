#include "SplitTest.hpp"
#include "Utils.hpp"
#include "../src/base/Algorithm.hpp"


static bool all_passed = true;

void SplitTest::run_tests()
{
    test_polyline();
    test_arc();
    test_bezier();
    test_bspline();
    PASSED
}

void SplitTest::test_polyline()
{
    {
        const Geo::Polyline polyline({Geo::Point(0, 0), Geo::Point(2, 0), Geo::Point(2, 2)});
        Geo::Polyline output0, output1;
        bool result = Geo::split(polyline, Geo::Point(1, 0), output0, output1);
        TEST(result, "Split at first segment midpoint")
        TEST(output0.size() >= 2, "First part has points")
        TEST(output1.size() >= 2, "Second part has points")
        TEST(Geo::Point(0, 0) == output0.front(), "First part starts at origin")
        TEST(Geo::Point(2, 2) == output1.back(), "Second part ends at (2,2)")
    }

    {
        const Geo::Polyline polyline({Geo::Point(0, 0), Geo::Point(2, 0), Geo::Point(2, 2)});
        Geo::Polyline output0, output1;
        bool result = Geo::split(polyline, Geo::Point(2, 1), output0, output1);
        TEST(result, "Split at second segment midpoint")
    }

    {
        const Geo::Polyline polyline({Geo::Point(0, 0), Geo::Point(4, 0)});
        std::vector<std::tuple<size_t, double>> pos;
        bool result = Geo::split(polyline, (size_t)2, pos);
        TEST(result, "Split into 2 equal parts")
        TEST(pos.size() == 1, "One split point")
    }

    {
        const Geo::Polyline polyline({Geo::Point(0, 0), Geo::Point(6, 0)});
        std::vector<std::tuple<size_t, double>> pos;
        bool result = Geo::split(polyline, (size_t)3, pos);
        TEST(result, "Split into 3 equal parts")
        TEST(pos.size() == 2, "Two split points")
    }

    {
        const Geo::Polyline polyline({Geo::Point(0, 0), Geo::Point(4, 0)});
        std::vector<std::tuple<size_t, double>> pos;
        bool result = Geo::split(polyline, 2.0, pos);
        TEST(result, "Split by step distance")
        TEST(pos.size() >= 1, "At least one split point")
    }

    {
        const Geo::Polyline polyline({Geo::Point(0, 0), Geo::Point(1, 0), Geo::Point(1, 1), Geo::Point(0, 1)});
        Geo::Polyline output0, output1;
        bool result = Geo::split(polyline, Geo::Point(0.5, 0), output0, output1);
        TEST(result, "Split closed polyline")
    }
}

void SplitTest::test_arc()
{
    {
        Geo::Arc arc(0, 0, 1, 0, Geo::PI, true);
        Geo::Arc output0, output1;
        bool result = Geo::split(arc, Geo::Point(0, 1), output0, output1);
        TEST(result, "Split semicircle at top")
    }

    {
        Geo::Arc arc(0, 0, 2, 0, Geo::PI, true);
        Geo::Arc output0, output1;
        bool result = Geo::split(arc, Geo::Point(0, 2), output0, output1);
        TEST(result, "Split radius-2 semicircle at top")
    }
}

void SplitTest::test_bezier()
{
    {
        const Geo::CubicBezier bezier({Geo::Point(0, 0), Geo::Point(1, 2), Geo::Point(2, 2), Geo::Point(3, 0)}, true);
        Geo::CubicBezier output0, output1;
        bool result = Geo::split(bezier, (size_t)0, 0.5, output0, output1);
        TEST(result, "Split at t=0.5")
        TEST(output0.control_points.size() >= 2, "First bezier has points")
        TEST(output1.control_points.size() >= 2, "Second bezier has points")
    }

    {
        const Geo::CubicBezier bezier({Geo::Point(0, 0), Geo::Point(1, 2), Geo::Point(2, 2), Geo::Point(3, 0)}, true);
        Geo::CubicBezier output0, output1;
        bool result = Geo::split(bezier, (size_t)0, 0.3, output0, output1);
        TEST(result, "Split at t=0.3")
    }

    {
        const Geo::CubicBezier bezier({Geo::Point(0, 0), Geo::Point(1, 2), Geo::Point(2, 2), Geo::Point(3, 0)}, true);
        Geo::CubicBezier output0, output1;
        bool result = Geo::split(bezier, (size_t)0, 1.0, output0, output1);
        TEST(result, "Split at t=1.0")
    }

    {
        const Geo::CubicBezier bezier({Geo::Point(0, 0), Geo::Point(1, 2), Geo::Point(2, 2), Geo::Point(3, 0)}, true);
        Geo::CubicBezier output0, output1;
        bool result = Geo::split(bezier, (size_t)0, 0.7, output0, output1);
        TEST(result, "Split at t=0.7")
    }
}

void SplitTest::test_bspline()
{
    {
        const Geo::CubicBSpline bspline({Geo::Point(0, 0), Geo::Point(1, 2), Geo::Point(2, 2), Geo::Point(3, 0)}, true);
        Geo::CubicBSpline output0, output1;
        bool result = Geo::split(bspline, true, Geo::Point(1.5, 2), output0, output1);
        TEST(result, "Split cubic bspline at point")
    }

    {
        const Geo::QuadBSpline bspline({Geo::Point(0, 0), Geo::Point(1, 2), Geo::Point(2, 2), Geo::Point(3, 0)}, true);
        Geo::QuadBSpline output0, output1;
        bool result = Geo::split(bspline, false, Geo::Point(1.5, 2), output0, output1);
        TEST(result, "Split quad bspline at point")
    }

    {
        const Geo::CubicBSpline bspline({Geo::Point(0, 0), Geo::Point(1, 2), Geo::Point(2, 2), Geo::Point(3, 0),
                                          Geo::Point(4, 1), Geo::Point(5, 0)}, true);
        Geo::CubicBSpline output0, output1;
        bool result = Geo::split(bspline, true, Geo::Point(2.5, 1.5), output0, output1);
        TEST(result, "Split longer cubic bspline")
    }
}
