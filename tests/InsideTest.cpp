#include "InsideTest.hpp"
#include "Utils.hpp"
#include "../src/base/Algorithm.hpp"

static bool all_passed = true;

void InsideTest::run_tests()
{
    test_point_line();
    test_point_polyline();
    test_point_polygon();
    test_point_circle();
    test_point_ellipse();
    test_point_triangle();
    test_point_arc();
    test_point_aabbrect();
    PASSED
}

void InsideTest::test_point_line()
{
    TEST(Geo::is_inside(Geo::Point(0.5, 0), Geo::Point(0, 0), Geo::Point(1, 0)), "Point on horizontal line")
    TEST(!Geo::is_inside(Geo::Point(2, 0), Geo::Point(0, 0), Geo::Point(1, 0)), "Point beyond horizontal line")
    TEST(Geo::is_inside(Geo::Point(2, 0), Geo::Point(0, 0), Geo::Point(1, 0), true), "Point on infinite horizontal line")
    TEST(!Geo::is_inside(Geo::Point(2, 1), Geo::Point(0, 0), Geo::Point(1, 0), true), "Point off infinite horizontal line")
    TEST(Geo::is_inside(Geo::Point(0, 0), Geo::Point(0, 0), Geo::Point(1, 0)), "Point at line start")
    TEST(Geo::is_inside(Geo::Point(1, 0), Geo::Point(0, 0), Geo::Point(1, 0)), "Point at line end")
    TEST(Geo::is_inside(Geo::Point(0, 0), Geo::Point(1, 1), Geo::Point(-1, -1)), "Point on diagonal line")
    TEST(!Geo::is_inside(Geo::Point(1, 0), Geo::Point(0, 1), Geo::Point(1, 1)), "Point off diagonal line")
    TEST(Geo::is_inside(Geo::Point(0, 0), Geo::Point(0, 0), Geo::Point(0, 1)), "Point on vertical line start")
    TEST(!Geo::is_inside(Geo::Point(0, 2), Geo::Point(0, 0), Geo::Point(0, 1)), "Point beyond vertical line")
    TEST(Geo::is_inside(Geo::Point(0, 0.5), Geo::Point(0, 0), Geo::Point(0, 1), true), "Point on infinite vertical line")
}

void InsideTest::test_point_polyline()
{
    const Geo::Polyline polyline({Geo::Point(0, 0), Geo::Point(2, 0), Geo::Point(2, 2), Geo::Point(0, 2)});
    TEST(Geo::is_inside(Geo::Point(1, 0), polyline), "Point on bottom segment")
    TEST(Geo::is_inside(Geo::Point(2, 1), polyline), "Point on right segment")
    TEST(Geo::is_inside(Geo::Point(1, 2), polyline), "Point on top segment")
    TEST(!Geo::is_inside(Geo::Point(1, 1), polyline), "Point inside open polyline")
    TEST(Geo::is_inside(Geo::Point(0, 0), polyline), "Point at start vertex")
    TEST(Geo::is_inside(Geo::Point(2, 0), polyline), "Point at corner vertex")
    TEST(Geo::is_inside(Geo::Point(2, 2), polyline), "Point at corner vertex")
    TEST(Geo::is_inside(Geo::Point(0, 2), polyline), "Point at end vertex")

    const Geo::Polyline single({Geo::Point(0, 0), Geo::Point(1, 0)});
    TEST(Geo::is_inside(Geo::Point(0.5, 0), single), "Point on single segment")
    TEST(!Geo::is_inside(Geo::Point(0.5, 1), single), "Point off single segment")
    TEST(Geo::is_inside(Geo::Point(0, 0), single), "Point at single segment start")
    TEST(Geo::is_inside(Geo::Point(1, 0), single), "Point at single segment end")

    const Geo::Polyline diagonal({Geo::Point(0, 0), Geo::Point(1, 1)});
    TEST(Geo::is_inside(Geo::Point(0.5, 0.5), diagonal), "Point on diagonal segment")
    TEST(!Geo::is_inside(Geo::Point(0.5, 0), diagonal), "Point off diagonal segment")
}

void InsideTest::test_point_polygon()
{
    {
        const Geo::Polygon square({Geo::Point(-1, -1), Geo::Point(1, -1), Geo::Point(1, 1), Geo::Point(-1, 1)});
        TEST(Geo::is_inside(Geo::Point(0, 0), square), "Center of square")
        TEST(Geo::is_inside(Geo::Point(0.5, 0.5), square), "Inside square quadrant")
        TEST(Geo::is_inside(Geo::Point(-0.5, -0.5), square), "Inside square quadrant")
        TEST(!Geo::is_inside(Geo::Point(2, 0), square), "Right of square")
        TEST(!Geo::is_inside(Geo::Point(-2, 0), square), "Left of square")
        TEST(!Geo::is_inside(Geo::Point(0, 2), square), "Above square")
        TEST(!Geo::is_inside(Geo::Point(0, -2), square), "Below square")
        TEST(Geo::is_inside(Geo::Point(0, 1), square, true), "On top edge (coincide)")
        TEST(Geo::is_inside(Geo::Point(1, 1), square, true), "At corner (coincide)")
        TEST(!Geo::is_inside(Geo::Point(1, 1), square), "At corner (no coincide)")
        TEST(!Geo::is_inside(Geo::Point(1, 0), square), "On edge (no coincide)")
    }

    {
        const Geo::Polygon triangle({Geo::Point(0, 0), Geo::Point(4, 0), Geo::Point(2, 3)});
        TEST(Geo::is_inside(Geo::Point(2, 1), triangle), "Inside triangle")
        TEST(Geo::is_inside(Geo::Point(2, 0.5), triangle), "Near triangle base")
        TEST(!Geo::is_inside(Geo::Point(5, 0), triangle), "Right of triangle")
        TEST(!Geo::is_inside(Geo::Point(0, 3), triangle), "Above triangle")
        TEST(!Geo::is_inside(Geo::Point(-1, 0), triangle), "Left of triangle")
    }

    {
        const Geo::Polygon pentagon({Geo::Point(0, 2), Geo::Point(2, 0.6), Geo::Point(1.2, -1.6),
                                     Geo::Point(-1.2, -1.6), Geo::Point(-2, 0.6)});
        TEST(Geo::is_inside(Geo::Point(0, 0), pentagon), "Inside pentagon")
        TEST(!Geo::is_inside(Geo::Point(3, 0), pentagon), "Outside pentagon")
        TEST(!Geo::is_inside(Geo::Point(0, 3), pentagon), "Above pentagon")
    }

    {
        const Geo::Polygon hexagon({Geo::Point(2, 0), Geo::Point(1, 1.732), Geo::Point(-1, 1.732),
                                    Geo::Point(-2, 0), Geo::Point(-1, -1.732), Geo::Point(1, -1.732)});
        TEST(Geo::is_inside(Geo::Point(0, 0), hexagon), "Inside hexagon")
        TEST(!Geo::is_inside(Geo::Point(3, 0), hexagon), "Outside hexagon")
    }

    {
        // 凹字形多边形
        const Geo::Polygon polygon({Geo::Point(0, 0), Geo::Point(0, 2), Geo::Point(1, 2), Geo::Point(1, 1),
                                    Geo::Point(2, 1), Geo::Point(2, 2), Geo::Point(3, 2), Geo::Point(3, 0)});
        TEST(Geo::is_inside(Geo::Point(0.5, 1), polygon), "Inside polygon")
        TEST(!Geo::is_inside(Geo::Point(1.5, 1.5), polygon), "Outside polygon")
        TEST(!Geo::is_inside(Geo::Point(-1, 1), polygon), "Outside polygon")
    }

    {
        // 阶梯形多边形
        const Geo::Polygon polygon({Geo::Point(0, 0), Geo::Point(0, 2), Geo::Point(1, 2), Geo::Point(1, 1),
                                    Geo::Point(2, 1), Geo::Point(2, 0), Geo::Point(0, 0)});
        TEST(Geo::is_inside(Geo::Point(0.5, 1), polygon), "Inside polygon")
        TEST(!Geo::is_inside(Geo::Point(-1, 1), polygon), "Outside polygon")
    }

    {
        // 凸字形多边形
        const Geo::Polygon polygon({Geo::Point(0, 0), Geo::Point(0, 1), Geo::Point(1, 1), Geo::Point(1, 2),
                                    Geo::Point(2, 2), Geo::Point(2, 1), Geo::Point(3, 1), Geo::Point(3, 0)});
        TEST(Geo::is_inside(Geo::Point(1.5, 1), polygon), "Inside polygon")
        TEST(Geo::is_inside(Geo::Point(1.5, 1.5), polygon), "Inside polygon")
        TEST(!Geo::is_inside(Geo::Point(-1, 1), polygon), "Outside polygon")
        TEST(!Geo::is_inside(Geo::Point(-1, 1.5), polygon), "Outside polygon")
    }
}

void InsideTest::test_point_circle()
{
    const Geo::Circle circle(0, 0, 1);
    TEST(Geo::is_inside(Geo::Point(0, 0), circle), "Center")
    TEST(Geo::is_inside(Geo::Point(0.5, 0), circle), "Inside on x-axis")
    TEST(Geo::is_inside(Geo::Point(0, 0.5), circle), "Inside on y-axis")
    TEST(Geo::is_inside(Geo::Point(0.7, 0.7), circle), "Inside diagonally")
    TEST(!Geo::is_inside(Geo::Point(2, 0), circle), "Outside on x-axis")
    TEST(!Geo::is_inside(Geo::Point(0, 2), circle), "Outside on y-axis")
    TEST(!Geo::is_inside(Geo::Point(1, 1), circle), "Outside diagonally")
    TEST(Geo::is_inside(Geo::Point(1, 0), circle, true), "On circle (coincide)")
    TEST(!Geo::is_inside(Geo::Point(1, 0), circle), "On circle (no coincide)")

    const Geo::Circle circle2(2, 2, 3);
    TEST(Geo::is_inside(Geo::Point(2, 2), circle2), "Non-origin center")
    TEST(Geo::is_inside(Geo::Point(3, 2), circle2), "Inside non-origin circle")
    TEST(!Geo::is_inside(Geo::Point(6, 2), circle2), "Outside non-origin circle")

    const Geo::Circle circle3(0, 0, 0.5);
    TEST(Geo::is_inside(Geo::Point(0, 0), circle3), "Small circle center")
    TEST(Geo::is_inside(Geo::Point(0.3, 0), circle3), "Inside small circle")
    TEST(!Geo::is_inside(Geo::Point(1, 0), circle3), "Outside small circle")
}

void InsideTest::test_point_ellipse()
{
    const Geo::Ellipse ellipse(0, 0, 3, 2);
    TEST(Geo::is_inside(Geo::Point(0, 0), ellipse), "Center")
    TEST(Geo::is_inside(Geo::Point(1, 0), ellipse), "Inside on a-axis")
    TEST(Geo::is_inside(Geo::Point(0, 1), ellipse), "Inside on b-axis")
    TEST(Geo::is_inside(Geo::Point(2, 1), ellipse), "Inside general")
    TEST(Geo::is_inside(Geo::Point(2.5, 0.5), ellipse), "Inside near boundary")
    TEST(!Geo::is_inside(Geo::Point(5, 0), ellipse), "Outside on a-axis")
    TEST(!Geo::is_inside(Geo::Point(0, 4), ellipse), "Outside on b-axis")
    TEST(!Geo::is_inside(Geo::Point(3, 2), ellipse), "Outside diagonally")
    TEST(Geo::is_inside(Geo::Point(3, 0), ellipse, true), "On ellipse (coincide)")
    TEST(!Geo::is_inside(Geo::Point(3, 0), ellipse), "On ellipse (no coincide)")

    const Geo::Ellipse arc_ellipse(0, 0, 3, 2, 0, Geo::PI, false);
    TEST(!Geo::is_inside(Geo::Point(0, -3), arc_ellipse), "Outside ellipse arc range")

    const Geo::Ellipse circle_ellipse(0, 0, 2, 2);
    TEST(Geo::is_inside(Geo::Point(1, 1), circle_ellipse), "Inside circle as ellipse")
    TEST(!Geo::is_inside(Geo::Point(2, 2), circle_ellipse), "Outside circle as ellipse")
}

void InsideTest::test_point_triangle()
{
    const Geo::Triangle triangle(Geo::Point(0, 0), Geo::Point(4, 0), Geo::Point(2, 3));
    TEST(Geo::is_inside(Geo::Point(2, 1), triangle), "Inside triangle")
    TEST(Geo::is_inside(Geo::Point(2, 0.5), triangle), "Near base")
    TEST(Geo::is_inside(Geo::Point(1, 1), triangle), "Left of center")
    TEST(Geo::is_inside(Geo::Point(3, 1), triangle), "Right of center")
    TEST(!Geo::is_inside(Geo::Point(5, 0), triangle), "Right of triangle")
    TEST(!Geo::is_inside(Geo::Point(0, 3), triangle), "Above triangle")
    TEST(!Geo::is_inside(Geo::Point(-1, 0), triangle), "Left of triangle")
    TEST(!Geo::is_inside(Geo::Point(2, -1), triangle), "Below triangle")

    const Geo::Triangle right(Geo::Point(0, 0), Geo::Point(3, 0), Geo::Point(0, 4));
    TEST(Geo::is_inside(Geo::Point(1, 1), right), "Inside right triangle")
    TEST(!Geo::is_inside(Geo::Point(2, 2), right), "Outside right triangle")

    const Geo::Triangle equilateral(Geo::Point(0, 0), Geo::Point(2, 0), Geo::Point(1, std::sqrt(3.0)));
    TEST(Geo::is_inside(Geo::Point(1, 0.5), equilateral), "Inside equilateral")
    TEST(!Geo::is_inside(Geo::Point(1, 2), equilateral), "Above equilateral")
}

void InsideTest::test_point_arc()
{
    const Geo::Arc arc(0, 0, 2, 0, Geo::PI, true);
    TEST(Geo::is_inside(Geo::Point(2, 0), arc), "At arc start")
    TEST(Geo::is_inside(Geo::Point(-2, 0), arc), "At arc end")
    TEST(Geo::is_inside(Geo::Point(0, 2), arc), "At arc midpoint")
    TEST(!Geo::is_inside(Geo::Point(0, -2), arc), "Opposite side of arc")

    const Geo::Arc arc2(0, 0, 1, 0, Geo::PI / 2, true);
    TEST(Geo::is_inside(Geo::Point(1, 0), arc2), "At quarter arc start")
    TEST(Geo::is_inside(Geo::Point(0, 1), arc2), "At quarter arc end")
    TEST(!Geo::is_inside(Geo::Point(-1, 0), arc2), "Outside quarter arc")
    TEST(!Geo::is_inside(Geo::Point(0, -1), arc2), "Below quarter arc")

    const Geo::Arc arc3(0, 0, 1, 0, Geo::PI * 2, true);
    TEST(Geo::is_inside(Geo::Point(1, 0), arc3), "Full circle arc start")
}

void InsideTest::test_point_aabbrect()
{
    const Geo::AABBRect rect(-1, -1, 1, 1);
    TEST(Geo::is_inside(Geo::Point(0, 0), rect), "Center")
    TEST(Geo::is_inside(Geo::Point(-0.5, 0.5), rect), "Inside quadrant")
    TEST(Geo::is_inside(Geo::Point(0.5, -0.5), rect), "Inside quadrant")
    TEST(!Geo::is_inside(Geo::Point(2, 0), rect), "Right of rect")
    TEST(!Geo::is_inside(Geo::Point(-2, 0), rect), "Left of rect")
    TEST(!Geo::is_inside(Geo::Point(0, 2), rect), "Above rect")
    TEST(!Geo::is_inside(Geo::Point(0, -2), rect), "Below rect")
    TEST(Geo::is_inside(Geo::Point(1, 0), rect, true), "On right edge (coincide)")
    TEST(Geo::is_inside(Geo::Point(1, 1), rect, true), "At corner (coincide)")
    TEST(!Geo::is_inside(Geo::Point(1, 0), rect), "On edge (no coincide)")

    const Geo::AABBRect rect2(0, 0, 3, 4);
    TEST(Geo::is_inside(Geo::Point(1.5, 2), rect2), "Inside non-centered rect")
    TEST(!Geo::is_inside(Geo::Point(-1, 0), rect2), "Outside non-centered rect")
    TEST(Geo::is_inside(Geo::Point(0.1, 0.1), rect2), "Near corner inside")

    const Geo::AABBRect rect3(-5, -5, 5, 5);
    TEST(Geo::is_inside(Geo::Point(0, 0), rect3), "Large rect center")
    TEST(Geo::is_inside(Geo::Point(4, 4), rect3), "Large rect near corner")
    TEST(!Geo::is_inside(Geo::Point(6, 0), rect3), "Outside large rect")
}
