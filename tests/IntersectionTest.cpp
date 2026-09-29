#include "IntersectionTest.hpp"
#include "Utils.hpp"
#include "../src/base/Algorithm.hpp"


static bool all_passed = true;

void IntersectionTest::run_tests()
{
    test_line_line();
    test_line_circle();
    test_line_ellipse();
    test_line_arc();
    test_circle_circle();
    test_aabbrect();
    PASSED
}

void IntersectionTest::test_line_line()
{
    {
        Geo::Point output;
        bool result = Geo::is_intersected(Geo::Point(0, 0), Geo::Point(2, 2), Geo::Point(0, 2), Geo::Point(2, 0), output);
        TEST(result, "X-shape lines")
        TEST(fequal(1.0, output.x) && fequal(1.0, output.y), "Intersection at center")
    }

    {
        Geo::Point output;
        bool result = Geo::is_intersected(Geo::Point(0, 0), Geo::Point(1, 0), Geo::Point(0, 1), Geo::Point(1, 1), output);
        TEST(!result, "Parallel horizontal lines")
    }

    {
        Geo::Point output;
        bool result = Geo::is_intersected(Geo::Point(0, 0), Geo::Point(2, 0), Geo::Point(1, -1), Geo::Point(1, 1), output);
        TEST(result, "Perpendicular lines")
        TEST(fequal(1.0, output.x) && fequal(0.0, output.y), "Intersection at (1,0)")
    }

    {
        Geo::Point output;
        bool result = Geo::is_intersected(Geo::Point(0, 0), Geo::Point(1, 0), Geo::Point(2, 0), Geo::Point(3, 0), output);
        TEST(!result, "Collinear non-overlapping")
    }

    {
        Geo::Point output;
        bool result = Geo::is_intersected(Geo::Point(0, 0), Geo::Point(2, 0), Geo::Point(1, 0), Geo::Point(3, 0), output);
        TEST(result, "Collinear overlapping")
    }

    {
        Geo::Point output;
        bool result = Geo::is_intersected(Geo::Point(0, 0), Geo::Point(0, 2), Geo::Point(0, 1), Geo::Point(0, 3), output);
        TEST(result, "Vertical collinear overlapping")
    }

    {
        Geo::Point output;
        bool result = Geo::is_intersected(Geo::Point(0, 0), Geo::Point(0, 1), Geo::Point(0, 2), Geo::Point(0, 3), output);
        TEST(!result, "Vertical collinear non-overlapping")
    }

    {
        Geo::Point output;
        bool result = Geo::is_intersected(Geo::Point(0, 0), Geo::Point(2, 2), Geo::Point(0, 0), Geo::Point(-2, -2), output);
        TEST(result, "Lines sharing endpoint")
    }

    {
        Geo::Point output;
        bool result = Geo::is_intersected(Geo::Point(0, 0), Geo::Point(1, 0), Geo::Point(0, 0), Geo::Point(0, 1), output);
        TEST(result, "Lines at right angle sharing origin")
        TEST(fequal(0.0, output.x) && fequal(0.0, output.y), "Intersection at origin")
    }

    {
        Geo::Point output;
        bool result = Geo::is_intersected(Geo::Point(0, 0), Geo::Point(2, 0), Geo::Point(1, 1), Geo::Point(1, -1), output);
        TEST(result, "Vertical line crossing horizontal")
        TEST(fequal(1.0, output.x) && fequal(0.0, output.y), "Intersection at (1,0)")
    }
}

void IntersectionTest::test_line_circle()
{
    {
        Geo::Circle circle(0, 0, 1);
        Geo::Point output0, output1;
        int count = Geo::is_intersected(Geo::Point(0, 2), Geo::Point(0, -2), circle, output0, output1);
        TEST(count == 2, "Vertical line through circle")
        TEST(fequal(0.0, output0.x, 1e-6) && fequal(1.0, std::abs(output0.y), 1e-6), "Intersection on circle")
    }

    {
        Geo::Circle circle(0, 0, 1);
        Geo::Point output0, output1;
        int count = Geo::is_intersected(Geo::Point(-2, 0), Geo::Point(2, 0), circle, output0, output1);
        TEST(count == 2, "Horizontal line through circle")
    }

    {
        Geo::Circle circle(0, 0, 1);
        Geo::Point output0, output1;
        int count = Geo::is_intersected(Geo::Point(3, 0), Geo::Point(3, 1), circle, output0, output1);
        TEST(count == 0, "Line outside circle")
    }

    {
        Geo::Circle circle(0, 0, 1);
        Geo::Point output0, output1;
        int count = Geo::is_intersected(Geo::Point(0, 1), Geo::Point(2, 1), circle, output0, output1);
        TEST(count <= 2, "Tangent line")
    }

    {
        Geo::Circle circle(0, 0, 1);
        Geo::Point output0, output1;
        int count = Geo::is_intersected(Geo::Point(0, 2), Geo::Point(0, -2), circle, output0, output1, true);
        TEST(count == 2, "Infinite line through circle")
    }

    {
        Geo::Circle circle(3, 3, 2);
        Geo::Point output0, output1;
        int count = Geo::is_intersected(Geo::Point(3, 0), Geo::Point(3, 6), circle, output0, output1);
        TEST(count == 2, "Line through non-origin circle")
    }

    {
        Geo::Circle circle(0, 0, 1);
        Geo::Point output0, output1;
        int count = Geo::is_intersected(Geo::Point(-1, -1), Geo::Point(1, 1), circle, output0, output1);
        TEST(count == 2, "Diagonal line through circle")
    }
}

void IntersectionTest::test_line_ellipse()
{
    {
        Geo::Ellipse ellipse(0, 0, 3, 2);
        Geo::Point output0, output1;
        int count = Geo::is_intersected(Geo::Point(0, 4), Geo::Point(0, -4), ellipse, output0, output1);
        TEST(count == 2, "Vertical line through ellipse")
        TEST(fequal(0.0, output0.x, 1e-6), "Intersection x is 0")
    }

    {
        Geo::Ellipse ellipse(0, 0, 3, 2);
        Geo::Point output0, output1;
        int count = Geo::is_intersected(Geo::Point(-5, 0), Geo::Point(5, 0), ellipse, output0, output1);
        TEST(count == 2, "Horizontal line through ellipse")
    }

    {
        Geo::Ellipse ellipse(0, 0, 3, 2);
        Geo::Point output0, output1;
        int count = Geo::is_intersected(Geo::Point(5, 0), Geo::Point(5, 1), ellipse, output0, output1);
        TEST(count == 0, "Line outside ellipse")
    }

    {
        Geo::Ellipse ellipse(0, 0, 3, 2);
        Geo::Point output0, output1;
        int count = Geo::is_intersected(Geo::Point(0, 4), Geo::Point(0, -4), ellipse, output0, output1, true);
        TEST(count == 2, "Infinite line through ellipse")
    }

    {
        Geo::Ellipse ellipse(0, 0, 3, 2);
        Geo::Point output0, output1;
        int count = Geo::is_intersected(Geo::Point(-3, -2), Geo::Point(3, 2), ellipse, output0, output1);
        TEST(count == 2, "Diagonal line through ellipse")
    }
}

void IntersectionTest::test_line_arc()
{
    {
        Geo::Arc arc(0, 0, 1, 0, Geo::PI, true);
        Geo::Point output0, output1;
        int count = Geo::is_intersected(Geo::Point(0, 2), Geo::Point(0, -2), arc, output0, output1);
        TEST(count >= 1, "Line intersects upper semicircle")
    }

    {
        Geo::Arc arc(0, 0, 1, 0, Geo::PI, true);
        Geo::Point output0, output1;
        int count = Geo::is_intersected(Geo::Point(0, -2), Geo::Point(0, -3), arc, output0, output1);
        TEST(count == 0, "Line misses upper semicircle")
    }

    {
        Geo::Arc arc(0, 0, 1, -Geo::PI / 2, Geo::PI / 2, true);
        Geo::Point output0, output1;
        int count = Geo::is_intersected(Geo::Point(0, 2), Geo::Point(0, -2), arc, output0, output1);
        TEST(count >= 1, "Line intersects right semicircle")
    }
}

void IntersectionTest::test_circle_circle()
{
    {
        Geo::Circle c0(0, 0, 1);
        Geo::Circle c1(1.5, 0, 1);
        bool result = Geo::is_intersected(c0, c1);
        TEST(result, "Overlapping circles")
    }

    {
        Geo::Circle c0(0, 0, 1);
        Geo::Circle c1(5, 0, 1);
        bool result = Geo::is_intersected(c0, c1);
        TEST(!result, "Distant circles")
    }

    {
        Geo::Circle c0(0, 0, 1);
        Geo::Circle c1(0.5, 0, 1);
        bool result = Geo::is_intersected(c0, c1);
        TEST(result, "Heavily overlapping circles")
    }

    {
        Geo::Circle c0(0, 0, 1);
        Geo::Circle c1(2, 0, 1);
        bool result = Geo::is_intersected(c0, c1);
        TEST(result, "Tangent circles")
    }

    {
        Geo::Circle c0(0, 0, 2);
        Geo::Circle c1(0, 0, 2);
        bool result = Geo::is_intersected(c0, c1);
        TEST(result, "Identical circles")
    }

    {
        Geo::Circle c0(0, 0, 3);
        Geo::Circle c1(0, 0, 1);
        bool result = Geo::is_intersected(c0, c1);
        TEST(result, "One circle inside another")
    }

    {
        Geo::Circle c0(0, 0, 1);
        Geo::Circle c1(0, 3, 1);
        bool result = Geo::is_intersected(c0, c1);
        TEST(!result, "Circles just touching externally")
    }
}

void IntersectionTest::test_aabbrect()
{
    {
        Geo::AABBRect r0(-1, -1, 1, 1);
        Geo::AABBRect r1(0, 0, 2, 2);
        TEST(Geo::is_intersected(r0, r1), "Overlapping rects")
    }

    {
        Geo::AABBRect r0(-1, -1, 1, 1);
        Geo::AABBRect r1(3, 3, 5, 5);
        TEST(!Geo::is_intersected(r0, r1), "Distant rects")
    }

    {
        Geo::AABBRect r0(-1, -1, 1, 1);
        Geo::AABBRect r1(1, 0, 2, 2);
        TEST(Geo::is_intersected(r0, r1), "Touching rects")
    }

    {
        Geo::AABBRect r0(-1, -1, 1, 1);
        Geo::AABBRect r1(-0.5, -0.5, 0.5, 0.5);
        TEST(Geo::is_intersected(r0, r1), "One rect inside another")
    }

    {
        Geo::AABBRect rect(-1, -1, 1, 1);
        TEST(Geo::is_intersected(rect, Geo::Point(0, 0), Geo::Point(2, 0)), "Line through rect")
        TEST(!Geo::is_intersected(rect, Geo::Point(3, 0), Geo::Point(5, 0)), "Line outside rect")
        TEST(Geo::is_intersected(rect, Geo::Point(-2, 0), Geo::Point(2, 0)), "Long line through rect")
        TEST(Geo::is_intersected(rect, Geo::Point(0, 0), Geo::Point(0, 0)), "Point inside rect")
        TEST(!Geo::is_intersected(rect, Geo::Point(3, 0), Geo::Point(3, 0)), "Point outside rect")
    }

    {
        Geo::AABBRect r0(-2, -2, 2, 2);
        Geo::AABBRect r1(-1, -1, 1, 1);
        TEST(Geo::is_intersected(r0, r1), "Large rect containing small")
    }
}
