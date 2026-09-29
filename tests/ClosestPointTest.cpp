#include "ClosestPointTest.hpp"
#include "Utils.hpp"
#include "../src/base/Algorithm.hpp"


static bool all_passed = true;

void ClosestPointTest::run_tests()
{
    test_polyline();
    test_polygon();
    test_ellipse();
    test_bezier();
    test_bspline();
    PASSED
}

const Geo::Point zero(0, 0);

void ClosestPointTest::test_polyline()
{
    {
        const Geo::Polyline polyline({Geo::Point(-1, 0), Geo::Point(1, 0)});
        std::vector<Geo::Point> output;
        int count = Geo::closest_point(polyline, zero, output);
        TEST(count == 1, "Single segment count")
        TEST(output.size() == 1, "Single segment output size")
        TEST(zero == output[0], "Point on segment")
    }

    {
        const Geo::Polyline polyline({Geo::Point(-1, 0), Geo::Point(1, 0)});
        std::vector<Geo::Point> output;
        int count = Geo::closest_point(polyline, Geo::Point(0, 1), output);
        TEST(count == 1, "Above segment count")
        TEST(fequal(0.0, output[0].x) && fequal(0.0, output[0].y), "Closest at midpoint")
    }

    {
        const Geo::Polyline polyline({Geo::Point(0, 0), Geo::Point(2, 0), Geo::Point(2, 2)});
        std::vector<Geo::Point> output;
        int count = Geo::closest_point(polyline, Geo::Point(3, 1), output);
        TEST(count == 1, "L-shape count")
        TEST(Geo::Point(2, 1) == output[0], "Closest on vertical segment")
    }

    {
        const Geo::Polyline polyline({Geo::Point(0, 0), Geo::Point(2, 0), Geo::Point(2, 2)});
        std::vector<Geo::Point> output;
        int count = Geo::closest_point(polyline, Geo::Point(1, 1), output);
        TEST(count == 2, "Equidistant segments count")
        bool found_seg1 = false, found_seg2 = false;
        for (const Geo::Point &p : output)
        {
            if (fequal(1.0, p.x) && fequal(0.0, p.y))
                found_seg1 = true;
            if (fequal(2.0, p.x) && fequal(1.0, p.y))
                found_seg2 = true;
        }
        TEST(found_seg1 && found_seg2, "Points on both segments")
    }

    {
        const Geo::Polyline polyline({Geo::Point(0, 0), Geo::Point(1, 0)});
        std::vector<Geo::Point> output;
        int count = Geo::closest_point(polyline, Geo::Point(0, 0), output);
        TEST(count == 1, "Point at start vertex")
        TEST(Geo::Point(0, 0) == output[0], "Closest is start vertex")
    }

    {
        const Geo::Polyline polyline({Geo::Point(0, 0), Geo::Point(1, 0)});
        std::vector<Geo::Point> output;
        int count = Geo::closest_point(polyline, Geo::Point(-1, 0), output);
        TEST(count == 1, "Point before start")
        TEST(Geo::Point(0, 0) == output[0], "Closest is start vertex")
    }

    {
        const Geo::Polyline polyline({Geo::Point(0, 0), Geo::Point(1, 0)});
        std::vector<Geo::Point> output;
        int count = Geo::closest_point(polyline, Geo::Point(2, 0), output);
        TEST(count == 1, "Point after end")
        TEST(Geo::Point(1, 0) == output[0], "Closest is end vertex")
    }
}

void ClosestPointTest::test_polygon()
{
    {
        const Geo::Polygon polygon({Geo::Point(-1, -1), Geo::Point(1, -1), Geo::Point(1, 1), Geo::Point(-1, 1)});
        std::vector<Geo::Point> output;
        int count = Geo::closest_point(polygon, Geo::Point(0, 2), output);
        TEST(count == 1, "Above square count")
        TEST(fequal(0.0, output[0].x) && fequal(1.0, output[0].y), "Closest at top edge")
    }

    {
        const Geo::Polygon polygon({Geo::Point(-1, -1), Geo::Point(1, -1), Geo::Point(1, 1), Geo::Point(-1, 1)});
        std::vector<Geo::Point> output;
        int count = Geo::closest_point(polygon, Geo::Point(2, 0), output);
        TEST(count == 1, "Right of square count")
        TEST(Geo::Point(1, 0) == output[0], "Closest at right edge")
    }

    {
        const Geo::Polygon polygon({Geo::Point(-1, -1), Geo::Point(1, -1), Geo::Point(1, 1), Geo::Point(-1, 1)});
        std::vector<Geo::Point> output;
        int count = Geo::closest_point(polygon, Geo::Point(2, 2), output);
        TEST(count == 1, "Corner of square count")
        TEST(Geo::Point(1, 1) == output[0], "Closest at corner")
    }

    {
        const Geo::Polygon polygon({Geo::Point(-1, -1), Geo::Point(1, -1), Geo::Point(1, 1), Geo::Point(-1, 1)});
        std::vector<Geo::Point> output;
        int count = Geo::closest_point(polygon, Geo::Point(0, 0), output);
        TEST(count >= 1, "Inside polygon count")
    }

    {
        const Geo::Polygon triangle({Geo::Point(0, 0), Geo::Point(4, 0), Geo::Point(2, 3)});
        std::vector<Geo::Point> output;
        int count = Geo::closest_point(triangle, Geo::Point(2, -1), output);
        TEST(count == 1, "Below triangle count")
        TEST(Geo::Point(2, 0) == output[0], "Closest at base")
    }
}

void ClosestPointTest::test_ellipse()
{
    {
        const Geo::Ellipse ellipse(0, 0, 3, 2);
        const Geo::Point point(0, 4);
        std::vector<Geo::Point> output;
        int count = Geo::closest_point(ellipse, point, output);
        TEST(count > 0, "Above ellipse count")
        TEST(fequal(Geo::distance(point, Geo::Point(0, 2)), Geo::distance(point, output[0])), "Closest at (0,2)")
        TEST(fequal(Geo::distance(point, Geo::Point(0, 2)), Geo::distance(point, ellipse)), "Distance is correct")
    }

    {
        const Geo::Ellipse ellipse(0, 0, 3, 2);
        const Geo::Point point(5, 0);
        std::vector<Geo::Point> output;
        int count = Geo::closest_point(ellipse, point, output);
        TEST(count > 0, "Right of ellipse count")
        TEST(fequal(Geo::distance(point, Geo::Point(3, 0)), Geo::distance(point, output[0])), "Closest at (3,0)")
        TEST(fequal(Geo::distance(point, Geo::Point(3, 0)), Geo::distance(point, ellipse)), "Distance is correct")
    }

    {
        const Geo::Ellipse ellipse(0, 0, 2, 2);
        std::vector<Geo::Point> output;
        int count = Geo::closest_point(ellipse, zero, output);
        TEST(count == 2, "Center of circle count")
        TEST(fequal(Geo::distance(zero, Geo::Point(0, 2)), Geo::distance(zero, output[output[0].y > 0 ? 0 : 1])), "Closest at (0,2)");
        TEST(fequal(Geo::distance(zero, Geo::Point(0, -2)), Geo::distance(zero, output[output[0].y < 0 ? 0 : 1])), "Closest at (0,-2)");
    }

    {
        const Geo::Ellipse ellipse(0, 0, 3, 2, 0, Geo::PI * 4 / 3, false);
        std::vector<Geo::Point> output;
        int count = Geo::closest_point(ellipse, Geo::Point(0, 4), output);
        TEST(count > 0, "Above ellipse arc count")
        TEST(fequal(Geo::distance(Geo::Point(0, 4), Geo::Point(0, 2)), Geo::distance(Geo::Point(0, 4), output[0])), "Closest on arc at (0,2)")
    }

    {
        const Geo::Ellipse ellipse(0, 0, 3, 2);
        const Geo::Point point(0, -4);
        std::vector<Geo::Point> output;
        int count = Geo::closest_point(ellipse, point, output);
        TEST(count > 0, "Below ellipse count")
        TEST(fequal(Geo::distance(point, Geo::Point(0, -2)), Geo::distance(point, output[0])), "Closest at (0,-2)")
    }
}

void ClosestPointTest::test_bezier()
{
    {
        const Geo::CubicBezier bezier({Geo::Point(0, 0), Geo::Point(1, 2), Geo::Point(2, 2), Geo::Point(3, 0)}, true);
        std::vector<Geo::Point> output;
        int count = Geo::closest_point(bezier, Geo::Point(1.5, 3), output);
        TEST(count > 0, "Above bezier count")
        TEST(fequal(Geo::distance(Geo::Point(1.5, 3), Geo::Point(1.5, 2.375)), Geo::distance(Geo::Point(1.5, 3), output[0])), "Closest near top")
    }

    {
        const Geo::CubicBezier bezier({Geo::Point(0, 0), Geo::Point(0, 2), Geo::Point(3, 2), Geo::Point(3, 0)}, true);
        std::vector<Geo::Point> output;
        int count = Geo::closest_point(bezier, Geo::Point(1.5, 1), output);
        TEST(count > 0, "Inside bezier arch count")
        TEST(fequal(Geo::distance(Geo::Point(1.5, 1), Geo::Point(1.5, 2.375)), Geo::distance(Geo::Point(1.5, 1), output[0])), "Closest at arch top")
    }

    {
        const Geo::CubicBezier bezier({Geo::Point(0, 0), Geo::Point(1, 0), Geo::Point(2, 0), Geo::Point(3, 0)}, true);
        std::vector<Geo::Point> output;
        int count = Geo::closest_point(bezier, Geo::Point(1, 1), output);
        TEST(count > 0, "Linear bezier count")
        TEST(fequal(Geo::distance(Geo::Point(1, 1), Geo::Point(1, 0)), Geo::distance(Geo::Point(1, 1), output[0])), "Closest on line")
    }
}

void ClosestPointTest::test_bspline()
{
    {
        const Geo::CubicBSpline bspline3({Geo::Point(0, 0), Geo::Point(1, 2), Geo::Point(2, 2), Geo::Point(3, 0)}, true);
        std::vector<Geo::Point> output;
        std::vector<std::tuple<double, double, double>> values;
        int count = Geo::closest_point(bspline3, true, Geo::Point(1.5, 3), output, &values);
        TEST(count > 0, "Above cubic bspline count")
        const double dis0 = Geo::distance(Geo::Point(1.5, 3), output[0]);
        const double dis1 = Geo::distance(Geo::Point(1.5, 3), bspline3.at(std::get<0>(values[0]) - 1e-9));
        const double dis2 = Geo::distance(Geo::Point(1.5, 3), bspline3.at(std::get<0>(values[0]) + 1e-9));
        TEST(dis1 >= dis0 && dis0 <= dis2, "Closest near top")
        TEST(fequal(dis0, Geo::distance(Geo::Point(1.5, 3), bspline3, true)), "Distance is correct")
    }

    {
        const Geo::QuadBSpline bspline2({Geo::Point(0, 0), Geo::Point(1, 2), Geo::Point(2, 2), Geo::Point(3, 0)}, true);
        std::vector<Geo::Point> output;
        std::vector<std::tuple<double, double, double>> values;
        int count = Geo::closest_point(bspline2, false, Geo::Point(1.5, 3), output, &values);
        TEST(count > 0, "Above quad bspline count")
        const double dis0 = Geo::distance(Geo::Point(1.5, 3), output[0]);
        const double dis1 = Geo::distance(Geo::Point(1.5, 3), bspline2.at(std::get<0>(values[0]) - 1e-9));
        const double dis2 = Geo::distance(Geo::Point(1.5, 3), bspline2.at(std::get<0>(values[0]) + 1e-9));
        TEST(dis1 >= dis0 && dis0 <= dis2, "Closest near top")
        TEST(fequal(dis0, Geo::distance(Geo::Point(1.5, 3), bspline2, false)), "Distance is correct")
    }

    {
        const Geo::CubicBSpline bspline({Geo::Point(0, 0), Geo::Point(1, 0), Geo::Point(2, 0), Geo::Point(3, 0)}, true);
        std::vector<Geo::Point> output;
        std::vector<std::tuple<double, double, double>> values;
        int count = Geo::closest_point(bspline, true, Geo::Point(1, 1), output, &values);
        TEST(count > 0, "Linear cubic bspline count")
        const double dis0 = Geo::distance(Geo::Point(1, 1), output[0]);
        const double dis1 = Geo::distance(Geo::Point(1, 1), bspline.at(std::get<0>(values[0]) - 1e-9));
        const double dis2 = Geo::distance(Geo::Point(1, 1), bspline.at(std::get<0>(values[0]) + 1e-9));
        TEST(dis1 >= dis0 && dis0 <= dis2, "Closest near line")
        TEST(fequal(dis0, Geo::distance(Geo::Point(1, 1), bspline, true)), "Distance is correct")
    }
}
