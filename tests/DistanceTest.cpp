#include "DistanceTest.hpp"
#include "Utils.hpp"
#include "../src/base/Algorithm.hpp"


static bool all_passed = true;

void DistanceTest::run_tests()
{
    test_point_to_point();
    test_point_to_line();
    test_point_to_polyline();
    test_point_to_polygon();
    test_point_to_bezier();
    test_point_to_bspline();
    test_point_to_arc();
    test_point_to_ellipse();
    test_line_to_line();
    test_distance_square();
    PASSED
}

const Geo::Point zero(0, 0);

void DistanceTest::test_point_to_point()
{
    TEST(0.0 == Geo::distance(zero, zero), "Same point")
    TEST(1.0 == Geo::distance(zero, Geo::Point(1, 0)), "Horizontal unit")
    TEST(1.0 == Geo::distance(zero, Geo::Point(0, 1)), "Vertical unit")
    TEST(fequal(std::sqrt(2.0), Geo::distance(zero, Geo::Point(1, 1))), "Diagonal")
    TEST(fequal(std::sqrt(8.0), Geo::distance(Geo::Point(1, 2), Geo::Point(3, 4))), "Arbitrary points")
    TEST(fequal(5.0, Geo::distance(Geo::Point(-3, -4), Geo::Point(0, 0))), "3-4-5 triangle")
    TEST(1.0 == Geo::distance(Geo::Point(5, 5), Geo::Point(6, 5)), "Horizontal shift")
    TEST(1.0 == Geo::distance(Geo::Point(5, 5), Geo::Point(5, 6)), "Vertical shift")
    TEST(fequal(std::sqrt(5.0), Geo::distance(Geo::Point(1, 1), Geo::Point(2, 3))), "Arbitrary diagonal")
}

void DistanceTest::test_point_to_line()
{
    TEST(1.0 == Geo::distance(zero, Geo::Point(-1, 1), Geo::Point(1, 1), true), "Point to horizontal infinite line")
    TEST(fequal(std::sqrt(2.0), Geo::distance(zero, Geo::Point(0, 2), Geo::Point(2, 0), true)), "Point to diagonal infinite line")
    TEST(0.0 == Geo::distance(zero, Geo::Point(-1, 0), Geo::Point(1, 0), true), "Point on horizontal infinite line")
    TEST(0.0 == Geo::distance(zero, Geo::Point(-1, -1), Geo::Point(1, 1), true), "Point on diagonal infinite line")
    TEST(1.0 == Geo::distance(zero, Geo::Point(1, 0), Geo::Point(2, 0), false), "Point to finite line (perpendicular)")
    TEST(fequal(std::sqrt(2.0), Geo::distance(zero, Geo::Point(1, 1), Geo::Point(2, 1), false)), "Point to finite line (diagonal)")
    TEST(0.0 == Geo::distance(Geo::Point(0.5, 0), Geo::Point(0, 0), Geo::Point(1, 0), false), "Point on finite line")
    TEST(1.0 == Geo::distance(Geo::Point(1, 1), Geo::Point(0, 0), Geo::Point(2, 0), false), "Point to horizontal line")
    TEST(1.0 == Geo::distance(Geo::Point(0, 0), Geo::Point(1, 0), Geo::Point(2, 0), false), "Point beyond finite line")
}

void DistanceTest::test_point_to_polyline()
{
    {
        const Geo::Polyline polyline({Geo::Point(-1, 0), Geo::Point(1, 0)});
        TEST(0.0 == Geo::distance(zero, polyline), "Point on single segment")
        TEST(1.0 == Geo::distance(Geo::Point(0, 1), polyline), "Point above single segment")
        TEST(1.0 == Geo::distance(Geo::Point(0, -1), polyline), "Point below single segment")
    }

    {
        const Geo::Polyline polyline({Geo::Point(0, 0), Geo::Point(2, 0), Geo::Point(2, 2)});
        TEST(0.0 == Geo::distance(zero, polyline), "Point at start vertex")
        TEST(0.0 == Geo::distance(Geo::Point(1, 0), polyline), "Point on first segment")
        TEST(0.0 == Geo::distance(Geo::Point(2, 1), polyline), "Point on second segment")
        TEST(fequal(1.0, Geo::distance(Geo::Point(1, 1), polyline)), "Point inside L-shape")
        TEST(fequal(std::sqrt(2.0), Geo::distance(Geo::Point(3, 3), polyline)), "Point outside L-shape")
        TEST(fequal(1.0, Geo::distance(Geo::Point(-1, 0), polyline)), "Point before start")
    }

    {
        const Geo::Polyline polyline({Geo::Point(0, 0), Geo::Point(1, 0), Geo::Point(1, 1), Geo::Point(0, 1)});
        TEST(fequal(0.5, Geo::distance(Geo::Point(0.5, -0.5), polyline)), "Point below open polyline")
    }
}

void DistanceTest::test_point_to_polygon()
{
    {
        const Geo::Polygon polygon({Geo::Point(0, 0), Geo::Point(2, 0), Geo::Point(2, 2), Geo::Point(0, 2)});
        TEST(fequal(1.0, Geo::distance(Geo::Point(3, 1), polygon)), "Point to right of polygon")
        TEST(fequal(1.0, Geo::distance(Geo::Point(1, 3), polygon)), "Point above polygon")
        TEST(fequal(1.0, Geo::distance(Geo::Point(-1, 1), polygon)), "Point to left of polygon")
        TEST(fequal(1.0, Geo::distance(Geo::Point(1, -1), polygon)), "Point below polygon")
        TEST(fequal(std::sqrt(2.0), Geo::distance(Geo::Point(3, 3), polygon)), "Point to corner of polygon")
    }

    {
        const Geo::Polygon triangle({Geo::Point(0, 0), Geo::Point(4, 0), Geo::Point(2, 3)});
        TEST(fequal(1.0, Geo::distance(Geo::Point(2, -1), triangle)), "Point below triangle")
    }
}

void DistanceTest::test_point_to_bezier()
{
    {
        const Geo::CubicBezier bezier({Geo::Point(1, -1), Geo::Point(1, 1), Geo::Point(1, 2), Geo::Point(1, 3)}, true);
        TEST(1.0 == Geo::distance(zero, bezier), "Point to vertical bezier")
        TEST(1.0 == Geo::distance(Geo::Point(0, 0.5), bezier), "Point to bezier at midpoint")
    }

    {
        const Geo::CubicBezier bezier({Geo::Point(3, -3), Geo::Point(1, 0), Geo::Point(3, 3)}, true);
        TEST(fequal(1.0, Geo::distance(zero, bezier)), "Point to symmetric bezier")
    }

    {
        const Geo::CubicBezier bezier({Geo::Point(1, 1), Geo::Point(2, 2), Geo::Point(3, 0), Geo::Point(4, 1)}, true);
        TEST(fequal(std::sqrt(2.0), Geo::distance(zero, bezier)), "Point to curved bezier")
    }
}

void DistanceTest::test_point_to_bspline()
{
    {
        const Geo::CubicBSpline bspline3({Geo::Point(1, -1), Geo::Point(1, 1), Geo::Point(1, 2), Geo::Point(1, 3)}, true);
        const Geo::QuadBSpline bspline2({Geo::Point(1, -1), Geo::Point(1, 1), Geo::Point(1, 2), Geo::Point(1, 3)}, true);
        TEST(fequal(1.0, Geo::distance(zero, bspline3, true)), "Cubic bspline distance")
        TEST(fequal(1.0, Geo::distance(zero, bspline2, false)), "Quad bspline distance")
    }

    {
        const Geo::CubicBSpline bspline3({Geo::Point(3, -3), Geo::Point(1, 0), Geo::Point(3, 3)}, true);
        const Geo::QuadBSpline bspline2({Geo::Point(3, -3), Geo::Point(1, 0), Geo::Point(3, 3)}, true);
        TEST(1.0 > Geo::distance(zero, bspline3, true), "Cubic bspline distance < 1")
        TEST(1.0 == Geo::distance(zero, bspline2, false), "Quad bspline distance == 1")
    }

    {
        const Geo::CubicBSpline bspline3({Geo::Point(1, 1), Geo::Point(1, 2), Geo::Point(1, 3), Geo::Point(1, 4)}, true);
        const Geo::QuadBSpline bspline2({Geo::Point(1, 1), Geo::Point(1, 2), Geo::Point(1, 3), Geo::Point(1, 4)}, true);
        TEST(fequal(std::sqrt(2.0), Geo::distance(zero, bspline3, true)), "Cubic bspline diagonal distance")
        TEST(fequal(std::sqrt(2.0), Geo::distance(zero, bspline2, false)), "Quad bspline diagonal distance")
    }
}

void DistanceTest::test_point_to_arc()
{
    {
        const Geo::Arc arc(Geo::Point(2, 0), Geo::Point(0, 1), Geo::Point(-2, 0));
        TEST(Geo::distance(Geo::Point(0, 2), arc.control_points[1]) == Geo::distance(zero, arc), "Point to arc midpoint")
        TEST(Geo::distance(Geo::Point(2, -1), arc.control_points[0]) == Geo::distance(Geo::Point(2, -1), arc), "Point to arc start")
        TEST(Geo::distance(Geo::Point(-2, -1), arc.control_points[2]) == Geo::distance(Geo::Point(-2, -1), arc), "Point to arc end")
    }

    {
        const Geo::Arc arc(0, 0, 2, 0, Geo::PI * 4 / 3, true);
        TEST(arc.radius == Geo::distance(zero, arc), "Center to arc = radius")
        TEST(0 == Geo::distance(Geo::Point(-2, 0), arc), "Point on arc")
        TEST(fequal(std::sqrt(2.0), Geo::distance(Geo::Point(1, -1), arc)), "Point to arc")
        TEST(arc.radius - 0.2 == Geo::distance(Geo::Point(-0.2, 0), arc), "Point inside arc")
    }

    {
        const Geo::Arc arc(0, 0, 2, -Geo::PI / 6, Geo::PI * 4 / 3, true);
        TEST(arc.radius == Geo::distance(zero, arc), "Center to arc = radius")
        TEST(0 == Geo::distance(Geo::Point(-2, 0), arc), "Point on arc")
        TEST(arc.radius - 0.2 == Geo::distance(Geo::Point(-0.2, 0), arc), "Point inside arc")
    }

    {
        const Geo::Arc arc(0, 0, 2, Geo::PI / 6, -Geo::PI / 6, true);
        TEST(arc.radius == Geo::distance(zero, arc), "Center to arc = radius")
        TEST(0 == Geo::distance(Geo::Point(-2, 0), arc), "Point on arc")
        TEST(arc.radius - 0.2 == Geo::distance(Geo::Point(-0.2, 0), arc), "Point inside arc")
    }

    {
        const Geo::Arc arc(0, 0, 2, Geo::PI / 6, -Geo::PI / 6, false);
        TEST(arc.radius == Geo::distance(zero, arc), "Center to arc = radius")
        TEST(arc.radius - 1.8 == Geo::distance(Geo::Point(1.8, 0), arc), "Point inside CW arc")
    }
}

void DistanceTest::test_point_to_ellipse()
{
    {
        Geo::Ellipse ellipse(0, 0, 3, 2);
        Geo::Point point0(0, 2), point1(3, 0);
        TEST(2.0 == Geo::distance(zero, ellipse), "Center to ellipse = minor radius")
        TEST(fequal(0, Geo::distance(point0, ellipse)), "Point on b-axis")
        TEST(fequal(0, Geo::distance(point1, ellipse)), "Point on a-axis")
        ellipse.rotate(0, 0, Geo::PI / 3);
        point0.rotate(0, 0, Geo::PI / 3);
        point1.rotate(0, 0, Geo::PI / 3);
        TEST(2.0 == Geo::distance(zero, ellipse), "Rotated ellipse center distance")
        TEST(fequal(0, Geo::distance(point0, ellipse)), "Rotated point on ellipse")
        TEST(fequal(0, Geo::distance(point1, ellipse)), "Rotated point on ellipse")
    }

    {
        Geo::Ellipse ellipse(0, 0, 2, 2, 0, Geo::PI * 4 / 3, false);
        TEST(2.0 == Geo::distance(zero, ellipse), "Circle arc center distance")
        TEST(fequal(0, Geo::distance(Geo::Point(-2, 0), ellipse)), "Point on circle arc")
        TEST(fequal(std::sqrt(2.0), Geo::distance(Geo::Point(1, -1), ellipse)), "Point to circle arc")
        TEST(fequal(1.8, Geo::distance(Geo::Point(-0.2, 0), ellipse)), "Point inside circle arc")
    }

    {
        Geo::Ellipse ellipse(0, 0, 2, 2, -Geo::PI / 6, Geo::PI * 4 / 3, false);
        TEST(2.0 == Geo::distance(zero, ellipse), "Rotated circle arc center distance")
        TEST(fequal(0, Geo::distance(Geo::Point(-2, 0), ellipse)), "Point on rotated circle arc")
    }

    {
        Geo::Ellipse ellipse(0, 0, 2, 2, Geo::PI / 6, -Geo::PI / 6, false);
        TEST(2.0 == Geo::distance(zero, ellipse), "CCW circle arc center distance")
        TEST(fequal(0, Geo::distance(Geo::Point(-2, 0), ellipse)), "Point on CCW circle arc")
    }

    {
        Geo::Ellipse ellipse(0, 0, 2, 2, -Geo::PI / 6, Geo::PI / 6, false);
        TEST(2.0 == Geo::distance(zero, ellipse), "Small arc center distance")
        TEST(fequal(0.2, Geo::distance(Geo::Point(1.8, 0), ellipse)), "Point near small arc")
    }
}

void DistanceTest::test_line_to_line()
{
    {
        Geo::Point p0, p1;
        TEST(1.0 == Geo::distance(Geo::Point(0, 1), Geo::Point(0, 2), Geo::Point(0, 0), Geo::Point(1, 0), p0, p1), "Parallel lines")
        TEST(Geo::Point(0, 1) == p0, "Closest point on first line")
        TEST(Geo::Point(0, 0) == p1, "Closest point on second line")
    }

    {
        Geo::Point p0, p1;
        TEST(0.0 == Geo::distance(Geo::Point(0, 0), Geo::Point(1, 0), Geo::Point(0, 0), Geo::Point(0, 1), p0, p1), "Intersecting lines")
    }

    {
        Geo::Point p0, p1;
        TEST(fequal(std::sqrt(2.0), Geo::distance(Geo::Point(0, 0), Geo::Point(1, 0), Geo::Point(2, 1), Geo::Point(2, 2), p0, p1)), "Skew lines")
    }

    {
        Geo::Point p0, p1;
        TEST(0.0 == Geo::distance(Geo::Point(0, 0), Geo::Point(2, 0), Geo::Point(1, 0), Geo::Point(1, 1), p0, p1), "Perpendicular lines intersect")
    }

    {
        Geo::Point p0, p1;
        TEST(2.0 == Geo::distance(Geo::Point(0, 0), Geo::Point(1, 0), Geo::Point(0, 2), Geo::Point(1, 2), p0, p1), "Parallel horizontal lines")
    }
}

void DistanceTest::test_distance_square()
{
    TEST(0.0 == Geo::distance_square(zero, zero), "Same point square")
    TEST(1.0 == Geo::distance_square(zero, Geo::Point(1, 0)), "Horizontal unit square")
    TEST(2.0 == Geo::distance_square(zero, Geo::Point(1, 1)), "Diagonal square")
    TEST(8.0 == Geo::distance_square(Geo::Point(1, 2), Geo::Point(3, 4)), "Arbitrary points square")
    TEST(25.0 == Geo::distance_square(Geo::Point(-3, -4), Geo::Point(0, 0)), "3-4-5 triangle square")

    TEST(0.0 == Geo::distance_square(zero, Geo::Point(-1, 0), Geo::Point(1, 0), true), "Point on infinite line square")
    TEST(1.0 == Geo::distance_square(zero, Geo::Point(-1, 1), Geo::Point(1, 1), true), "Point to infinite line square")
    TEST(1.0 == Geo::distance_square(zero, Geo::Point(1, 0), Geo::Point(2, 0), false), "Point to finite line square")
    TEST(0.0 == Geo::distance_square(Geo::Point(0.5, 0), Geo::Point(0, 0), Geo::Point(1, 0), false), "Point on finite line square")

    {
        const Geo::Polyline polyline({Geo::Point(-1, 0), Geo::Point(1, 0)});
        TEST(0.0 == Geo::distance_square(zero, polyline), "Point on polyline square")
        TEST(1.0 == Geo::distance_square(Geo::Point(0, 1), polyline), "Point above polyline square")
    }

    {
        const Geo::Polygon polygon({Geo::Point(0, 0), Geo::Point(2, 0), Geo::Point(2, 2), Geo::Point(0, 2)});
        TEST(1.0 == Geo::distance_square(Geo::Point(3, 1), polygon), "Point to polygon square")
        TEST(1.0 == Geo::distance_square(Geo::Point(1, 3), polygon), "Point above polygon square")
        TEST(2.0 == Geo::distance_square(Geo::Point(3, 3), polygon), "Point to corner square")
    }
}
