#include "FootTest.hpp"
#include "Utils.hpp"
#include "../src/base/Algorithm.hpp"

static bool all_passed = true;

void FootTest::run_tests()
{
    test_line();
    test_circle();
    test_ellipse();
    test_bezier();
    test_bspline();
    PASSED
}

void FootTest::test_line()
{
    {
        Geo::Point foot;
        bool result = Geo::foot_point(Geo::Point(0, 0), Geo::Point(2, 0), Geo::Point(1, 1), foot);
        TEST(result, "Foot on horizontal line")
        TEST(Geo::Point(1, 0) == foot, "Foot at (1,0)")
    }

    {
        Geo::Point foot;
        bool result = Geo::foot_point(Geo::Point(0, 0), Geo::Point(2, 0), Geo::Point(1, 1), foot, true);
        TEST(result, "Foot on infinite horizontal line")
        TEST(Geo::Point(1, 0) == foot, "Foot at (1,0)")
    }

    {
        Geo::Point foot;
        bool result = Geo::foot_point(Geo::Point(0, 0), Geo::Point(1, 0), Geo::Point(2, 1), foot, true);
        TEST(result, "Foot on infinite line beyond segment")
        TEST(Geo::Point(2, 0) == foot, "Foot at (2,0)")
    }

    {
        Geo::Point foot;
        bool result = Geo::foot_point(Geo::Point(0, 0), Geo::Point(2, 2), Geo::Point(2, 0), foot);
        TEST(result, "Foot on diagonal line")
        TEST(Geo::Point(1, 1) == foot, "Foot at (1,1)")
    }

    {
        Geo::Point foot;
        bool result = Geo::foot_point(Geo::Point(0, 0), Geo::Point(0, 2), Geo::Point(1, 1), foot);
        TEST(result, "Foot on vertical line")
        TEST(Geo::Point(0, 1) == foot, "Foot at (0,1)")
    }

    {
        Geo::Point foot;
        bool result = Geo::foot_point(Geo::Point(0, 0), Geo::Point(2, 0), Geo::Point(1, 0), foot);
        TEST(result, "Point on line gives itself")
        TEST(Geo::Point(1, 0) == foot, "Foot is the point itself")
    }

    {
        Geo::Point foot;
        bool result = Geo::foot_point(Geo::Point(0, 0), Geo::Point(1, 1), Geo::Point(0.5, 0.5), foot);
        TEST(result, "Point on diagonal line")
        TEST(Geo::Point(0.5, 0.5) == foot, "Foot at (0.5,0.5)")
    }

    {
        Geo::Point foot;
        bool result = Geo::foot_point(Geo::Point(0, 0), Geo::Point(1, 0), Geo::Point(-1, 1), foot, true);
        TEST(result, "Foot on infinite line before segment")
        TEST(Geo::Point(-1, 0) == foot, "Foot at (-1,0)")
    }

    {
        Geo::Point foot;
        bool result = Geo::foot_point(Geo::Point(0, 0), Geo::Point(1, 0), Geo::Point(0.5, 0), foot);
        TEST(result, "Point at line midpoint")
        TEST(Geo::Point(0.5, 0) == foot, "Foot is midpoint")
    }
}

void FootTest::test_circle()
{
    {
        Geo::Circle circle(0, 0, 1);
        Geo::Point output0, output1;
        bool result = Geo::foot_point(circle, Geo::Point(0, 2), output0, output1);
        TEST(result, "Foot from above")
        bool found = false;
        if ((fequal(0.0, output0.x, 1e-6) && fequal(1.0, output0.y, 1e-6)) ||
            (fequal(0.0, output1.x, 1e-6) && fequal(1.0, output1.y, 1e-6)))
            found = true;
        TEST(found, "Foot at (0,1)")
    }

    {
        Geo::Circle circle(0, 0, 1);
        Geo::Point output0, output1;
        bool result = Geo::foot_point(circle, Geo::Point(2, 0), output0, output1);
        TEST(result, "Foot from right")
        bool found = false;
        if ((fequal(1.0, output0.x, 1e-6) && fequal(0.0, output0.y, 1e-6)) ||
            (fequal(1.0, output1.x, 1e-6) && fequal(0.0, output1.y, 1e-6)))
            found = true;
        TEST(found, "Foot at (1,0)")
    }

    {
        Geo::Circle circle(0, 0, 1);
        Geo::Point output0, output1;
        bool result = Geo::foot_point(circle, Geo::Point(-2, 0), output0, output1);
        TEST(result, "Foot from left")
        bool found = false;
        if ((fequal(-1.0, output0.x, 1e-6) && fequal(0.0, output0.y, 1e-6)) ||
            (fequal(-1.0, output1.x, 1e-6) && fequal(0.0, output1.y, 1e-6)))
            found = true;
        TEST(found, "Foot at (-1,0)")
    }

    {
        Geo::Circle circle(0, 0, 1);
        Geo::Point output0, output1;
        bool result = Geo::foot_point(circle, Geo::Point(0, -2), output0, output1);
        TEST(result, "Foot from below")
        bool found = false;
        if ((fequal(0.0, output0.x, 1e-6) && fequal(-1.0, output0.y, 1e-6)) ||
            (fequal(0.0, output1.x, 1e-6) && fequal(-1.0, output1.y, 1e-6)))
            found = true;
        TEST(found, "Foot at (0,-1)")
    }

    {
        Geo::Circle circle(3, 3, 2);
        Geo::Point output0, output1;
        bool result = Geo::foot_point(circle, Geo::Point(3, 7), output0, output1);
        TEST(result, "Foot on non-origin circle")
        bool found = false;
        if ((fequal(3.0, output0.x, 1e-6) && fequal(5.0, output0.y, 1e-6)) ||
            (fequal(3.0, output1.x, 1e-6) && fequal(5.0, output1.y, 1e-6)))
            found = true;
        TEST(found, "Foot at (3,5)")
    }

    {
        Geo::Circle circle(0, 0, 2);
        Geo::Point output0, output1;
        bool result = Geo::foot_point(circle, Geo::Point(0, 5), output0, output1);
        TEST(result, "Foot on larger circle")
        bool found = false;
        if ((fequal(0.0, output0.x, 1e-6) && fequal(2.0, output0.y, 1e-6)) ||
            (fequal(0.0, output1.x, 1e-6) && fequal(2.0, output1.y, 1e-6)))
            found = true;
        TEST(found, "Foot at (0,2)")
    }

    {
        Geo::Circle circle(0, 0, 1);
        Geo::Point output0, output1;
        bool result = Geo::foot_point(circle, Geo::Point(1, 1), output0, output1);
        TEST(result, "Foot from diagonal")
        bool found = false;
        double inv_sqrt2 = 1.0 / std::sqrt(2.0);
        if ((fequal(inv_sqrt2, output0.x, 1e-6) && fequal(inv_sqrt2, output0.y, 1e-6)) ||
            (fequal(inv_sqrt2, output1.x, 1e-6) && fequal(inv_sqrt2, output1.y, 1e-6)))
            found = true;
        TEST(found, "Foot at (1/sqrt2, 1/sqrt2)")
    }
}

void FootTest::test_ellipse()
{
    {
        const Geo::Ellipse ellipse(0, 0, 3, 2);
        std::vector<Geo::Point> output;
        int count = Geo::foot_point(ellipse, Geo::Point(0, 4), output);
        TEST(count >= 1, "Foot above ellipse")
        const double angle = Geo::angle(ellipse.a1(), ellipse.center(), output[0]);
        TEST(fequal((Geo::Point(0, 4) - output[0]) * ellipse.angle_tangency(angle), 0), "The vector is perpendicular to the tangent.")
    }

    {
        const Geo::Ellipse ellipse(0, 0, 3, 2);
        std::vector<Geo::Point> output;
        int count = Geo::foot_point(ellipse, Geo::Point(5, 0), output);
        TEST(count >= 1, "Foot right of ellipse")
        const double angle = Geo::angle(ellipse.a1(), ellipse.center(), output[0]);
        TEST(fequal((Geo::Point(5, 0) - output[0]) * ellipse.angle_tangency(angle), 0), "The vector is perpendicular to the tangent.")
    }

    {
        const Geo::Ellipse ellipse(0, 0, 3, 2);
        std::vector<Geo::Point> output;
        int count = Geo::foot_point(ellipse, Geo::Point(0, -4), output);
        TEST(count >= 1, "Foot below ellipse")
        const double angle = Geo::angle(ellipse.a1(), ellipse.center(), output[0]);
        TEST(fequal((Geo::Point(0, -4) - output[0]) * ellipse.angle_tangency(angle), 0), "The vector is perpendicular to the tangent.")
    }

    {
        const Geo::Ellipse ellipse(0, 0, 3, 2);
        std::vector<Geo::Point> output;
        int count = Geo::foot_point(ellipse, Geo::Point(-5, 0), output);
        TEST(count >= 1, "Foot left of ellipse")
        const double angle = Geo::angle(ellipse.a1(), ellipse.center(), output[0]);
        TEST(fequal((Geo::Point(-5, 0) - output[0]) * ellipse.angle_tangency(angle), 0), "The vector is perpendicular to the tangent.")
    }

    {
        const Geo::Ellipse ellipse(2, 2, 3, 2);
        std::vector<Geo::Point> output;
        int count = Geo::foot_point(ellipse, Geo::Point(2, 6), output);
        TEST(count >= 1, "Foot on non-origin ellipse")
        const double angle = Geo::angle(ellipse.a1(), ellipse.center(), output[0]);
        TEST(fequal((Geo::Point(2, 6) - output[0]) * ellipse.angle_tangency(angle), 0), "The vector is perpendicular to the tangent.")
    }

    {
        const Geo::Ellipse ellipse(0, 0, 2, 2);
        std::vector<Geo::Point> output;
        int count = Geo::foot_point(ellipse, Geo::Point(0, 4), output);
        TEST(count >= 1, "Foot on circle as ellipse")
        const double angle = Geo::angle(ellipse.a1(), ellipse.center(), output[0]);
        TEST(fequal((Geo::Point(0, 4) - output[0]) * ellipse.angle_tangency(angle), 0), "The vector is perpendicular to the tangent.")
    }
}

void FootTest::test_bezier()
{
    {
        const Geo::CubicBezier bezier({Geo::Point(-1, -1), Geo::Point(-1, 1),
                                       Geo::Point(1, 1), Geo::Point(1, -1)},
                                      true);
        const Geo::Point point0(-0.5, 0), point1(0.5, 0), point2(0, 0.5), point3(0, -0.5), zero(0, 0);
        {
            std::vector<Geo::Point> output;
            std::vector<std::tuple<size_t, double, double, double>> values;
            const int count = Geo::foot_point(point0, bezier, output, &values);
            TEST(count >= 1, "Foot on cubic Bézier curve")
            const Geo::Point vec(bezier.tangent(std::get<0>(values[0]), std::get<1>(values[0])));
            const Geo::Point vec2(point0 - output[0]);
            const double v = vec2 * vec;
            TEST(fequal((point0 - output[0]) * vec, 0), "The vector is perpendicular to the tangent.")
        }
        {
            std::vector<Geo::Point> output;
            std::vector<std::tuple<size_t, double, double, double>> values;
            const int count = Geo::foot_point(point1, bezier, output, &values);
            TEST(count >= 1, "Foot on cubic Bézier curve")
            const Geo::Point vec(bezier.tangent(std::get<0>(values[0]), std::get<1>(values[0])));
            TEST(fequal((point1 - output[0]) * vec, 0), "The vector is perpendicular to the tangent.")
        }
        {
            std::vector<Geo::Point> output;
            std::vector<std::tuple<size_t, double, double, double>> values;
            const int count = Geo::foot_point(point2, bezier, output, &values);
            TEST(count >= 1, "Foot on cubic Bézier curve")
            const Geo::Point vec(bezier.tangent(std::get<0>(values[0]), std::get<1>(values[0])));
            TEST(fequal((point2 - output[0]) * vec, 0), "The vector is perpendicular to the tangent.")
        }
        {
            std::vector<Geo::Point> output;
            std::vector<std::tuple<size_t, double, double, double>> values;
            const int count = Geo::foot_point(point3, bezier, output, &values);
            TEST(count >= 1, "Foot on cubic Bézier curve")
            const Geo::Point vec(bezier.tangent(std::get<0>(values[0]), std::get<1>(values[0])));
            TEST(fequal((point3 - output[0]) * vec, 0), "The vector is perpendicular to the tangent.")
        }
        {
            std::vector<Geo::Point> output;
            std::vector<std::tuple<size_t, double, double, double>> values;
            const int count = Geo::foot_point(zero, bezier, output, &values);
            TEST(count >= 1, "Foot on cubic Bézier curve")
            const Geo::Point vec(bezier.tangent(std::get<0>(values[0]), std::get<1>(values[0])));
            TEST(fequal((zero - output[0]) * vec, 0), "The vector is perpendicular to the tangent.")
        }
    }
}

void FootTest::test_bspline()
{
    {
        const Geo::CubicBSpline bspline({Geo::Point(-1, -1), Geo::Point(-1, 1), Geo::Point(1, 1), Geo::Point(1, -1)}, true);
        const Geo::Point point0(-0.5, 0), point1(0.5, 0), point2(0, 0.5), point3(0, -0.5), zero(0, 0);
        {
            std::vector<Geo::Point> output;
            std::vector<std::tuple<double, double, double>> values;
            const int count = Geo::foot_point(point0, bspline, output, &values);
            TEST(count >= 1, "Foot on cubic BSpline curve")
            const Geo::Point vec(bspline.tangent(std::get<0>(values[0])));
            TEST(fequal((point0 - output[0]) * vec, 0), "The vector is perpendicular to the tangent.")
        }
        {
            std::vector<Geo::Point> output;
            std::vector<std::tuple<double, double, double>> values;
            const int count = Geo::foot_point(point1, bspline, output, &values);
            TEST(count >= 1, "Foot on cubic BSpline curve")
            const Geo::Point vec(bspline.tangent(std::get<0>(values[0])));
            TEST(fequal((point1 - output[0]) * vec, 0), "The vector is perpendicular to the tangent.")
        }
        {
            std::vector<Geo::Point> output;
            std::vector<std::tuple<double, double, double>> values;
            const int count = Geo::foot_point(point2, bspline, output, &values);
            TEST(count >= 1, "Foot on cubic BSpline curve")
            const Geo::Point vec(bspline.tangent(std::get<0>(values[0])));
            TEST(fequal((point2 - output[0]) * vec, 0), "The vector is perpendicular to the tangent.")
        }
        {
            std::vector<Geo::Point> output;
            std::vector<std::tuple<double, double, double>> values;
            const int count = Geo::foot_point(point3, bspline, output, &values);
            TEST(count >= 1, "Foot on cubic BSpline curve")
            const Geo::Point vec(bspline.tangent(std::get<0>(values[0])));
            TEST(fequal((point3 - output[0]) * vec, 0), "The vector is perpendicular to the tangent.")
        }
        {
            std::vector<Geo::Point> output;
            std::vector<std::tuple<double, double, double>> values;
            const int count = Geo::foot_point(zero, bspline, output, &values);
            TEST(count >= 1, "Foot on cubic BSpline curve")
            const Geo::Point vec(bspline.tangent(std::get<0>(values[0])));
            TEST(fequal((zero - output[0]) * vec, 0), "The vector is perpendicular to the tangent.")
        }
    }

    {
        const Geo::QuadBSpline bspline({Geo::Point(-1, -1), Geo::Point(-1, 1), Geo::Point(1, 1), Geo::Point(1, -1)}, true);
        const Geo::Point point0(-0.5, 0), point1(0.5, 0), point2(0, 0.5), point3(0, -0.5), zero(0, 0);
        {
            std::vector<Geo::Point> output;
            std::vector<std::tuple<double, double, double>> values;
            const int count = Geo::foot_point(point0, bspline, output, &values);
            TEST(count >= 1, "Foot on quadratic BSpline curve")
            const Geo::Point vec(bspline.tangent(std::get<0>(values[0])));
            TEST(fequal((point0 - output[0]) * vec, 0), "The vector is perpendicular to the tangent.")
        }
        {
            std::vector<Geo::Point> output;
            std::vector<std::tuple<double, double, double>> values;
            const int count = Geo::foot_point(point1, bspline, output, &values);
            TEST(count >= 1, "Foot on quadratic BSpline curve")
            const Geo::Point vec(bspline.tangent(std::get<0>(values[0])));
            TEST(fequal((point1 - output[0]) * vec, 0), "The vector is perpendicular to the tangent.")
        }
        {
            std::vector<Geo::Point> output;
            std::vector<std::tuple<double, double, double>> values;
            const int count = Geo::foot_point(point2, bspline, output, &values);
            TEST(count >= 1, "Foot on quadratic BSpline curve")
            const Geo::Point vec(bspline.tangent(std::get<0>(values[0])));
            TEST(fequal((point2 - output[0]) * vec, 0), "The vector is perpendicular to the tangent.")
        }
        {
            std::vector<Geo::Point> output;
            std::vector<std::tuple<double, double, double>> values;
            const int count = Geo::foot_point(point3, bspline, output, &values);
            TEST(count >= 1, "Foot on quadratic BSpline curve")
            const Geo::Point vec(bspline.tangent(std::get<0>(values[0])));
            TEST(fequal((point3 - output[0]) * vec, 0), "The vector is perpendicular to the tangent.")
        }
        {
            std::vector<Geo::Point> output;
            std::vector<std::tuple<double, double, double>> values;
            const int count = Geo::foot_point(zero, bspline, output, &values);
            TEST(count >= 1, "Foot on quadratic BSpline curve")
            const Geo::Point vec(bspline.tangent(std::get<0>(values[0])));
            TEST(fequal((zero - output[0]) * vec, 0), "The vector is perpendicular to the tangent.")
        }
    }
}
