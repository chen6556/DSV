#include "ConversionTest.hpp"
#include "Utils.hpp"
#include "../src/base/Algorithm.hpp"


static bool all_passed = true;

void ConversionTest::run_tests()
{
    test_to_coord();
    test_arc_to_polyline();
    test_circle_to_polygon();
    test_ellipse_to_polygon();
    PASSED
}

void ConversionTest::test_to_coord()
{
    {
        Geo::Point result = Geo::to_coord(Geo::Point(1, 0), 0, 0, 0);
        TEST(fequal(1.0, result.x) && fequal(0.0, result.y), "Identity transform")
    }

    {
        Geo::Point result = Geo::to_coord(Geo::Point(0, 0), 0, 0, 0);
        TEST(fequal(0.0, result.x) && fequal(0.0, result.y), "Origin to origin")
    }

    {
        Geo::Point result = Geo::to_coord(Geo::Point(2, 3), 0, 0, 0);
        TEST(fequal(2.0, result.x) && fequal(3.0, result.y), "No rotation preserves")
    }

    {
        Geo::Point result = Geo::to_coord(Geo::Point(1, 0), 0, 0, Geo::PI);
        TEST(fequal(-1.0, result.x, 1e-10) && fequal(0.0, result.y, 1e-10), "180 degree rotation")
    }

    {
        Geo::Point result = Geo::to_coord(Geo::Point(1, 1), 0, 0, 0);
        TEST(fequal(1.0, result.x) && fequal(1.0, result.y), "Point at (1,1) no rotation")
    }
}

void ConversionTest::test_arc_to_polyline()
{
    {
        Geo::Polyline polyline = Geo::arc_to_polyline(Geo::Point(0, 0), 1, 0, Geo::PI, true);
        TEST(polyline.size() > 2, "Semicircle has enough points")
        TEST(fequal(1.0, polyline.front().x, 1e-6) && fequal(0.0, polyline.front().y, 1e-6), "Start at (1,0)")
        TEST(fequal(-1.0, polyline.back().x, 1e-6) && fequal(0.0, polyline.back().y, 1e-6), "End at (-1,0)")
    }

    {
        Geo::Arc arc(0, 0, 1, 0, Geo::PI, true);
        Geo::Polyline polyline = Geo::arc_to_polyline(arc);
        TEST(polyline.size() > 2, "Arc to polyline has enough points")
    }

    {
        Geo::Arc arc(0, 0, 2, 0, Geo::PI / 2, true);
        Geo::Polyline polyline = Geo::arc_to_polyline(arc);
        TEST(polyline.size() > 2, "Quarter circle has enough points")
        TEST(fequal(2.0, polyline.front().x, 1e-6), "Start at (2,0)")
        TEST(fequal(0.0, polyline.back().x, 1e-6) && fequal(2.0, polyline.back().y, 1e-6), "End at (0,2)")
    }

    {
        Geo::Arc arc(0, 0, 1, 0, Geo::PI * 2, true);
        Geo::Polyline polyline = Geo::arc_to_polyline(arc);
        TEST(polyline.size() > 4, "Full circle has enough points")
    }

    {
        Geo::Arc arc(0, 0, 3, 0, Geo::PI, false);
        Geo::Polyline polyline = Geo::arc_to_polyline(arc);
        TEST(polyline.size() > 2, "CW semicircle has enough points")
    }
}

void ConversionTest::test_circle_to_polygon()
{
    {
        Geo::Polygon polygon = Geo::circle_to_polygon(0, 0, 1);
        TEST(polygon.size() > 8, "Unit circle has enough points")
        for (size_t i = 0; i < polygon.size(); ++i)
        {
            TEST(fequal(1.0, Geo::distance(Geo::Point(0, 0), polygon[i]), 1e-6), "Point on unit circle")
        }
    }

    {
        Geo::Circle circle(0, 0, 2);
        Geo::Polygon polygon = Geo::circle_to_polygon(circle);
        TEST(polygon.size() > 8, "Radius-2 circle has enough points")
        for (size_t i = 0; i < polygon.size(); ++i)
        {
            TEST(fequal(2.0, Geo::distance(Geo::Point(0, 0), polygon[i]), 1e-6), "Point on radius-2 circle")
        }
    }

    {
        Geo::Polygon polygon = Geo::circle_to_polygon(3, 4, 5);
        TEST(polygon.size() > 8, "Non-origin circle has enough points")
        for (size_t i = 0; i < polygon.size(); ++i)
        {
            TEST(fequal(5.0, Geo::distance(Geo::Point(3, 4), polygon[i]), 1e-6), "Point on non-origin circle")
        }
    }

    {
        Geo::Polygon polygon = Geo::circle_to_polygon(0, 0, 0.5);
        TEST(polygon.size() > 4, "Small circle has enough points")
    }
}

void ConversionTest::test_ellipse_to_polygon()
{
    {
        Geo::Polygon polygon = Geo::ellipse_to_polygon(0, 0, 3, 2, 0);
        TEST(polygon.size() > 8, "Ellipse has enough points")
    }

    {
        Geo::Ellipse ellipse(0, 0, 3, 2);
        Geo::Polygon polygon = Geo::ellipse_to_polygon(ellipse);
        TEST(polygon.size() > 8, "Ellipse object has enough points")
    }

    {
        Geo::Polyline polyline = Geo::ellipse_to_polyline(0, 0, 3, 2, 0, 0, Geo::PI);
        TEST(polyline.size() > 4, "Half ellipse has enough points")
    }

    {
        Geo::Polyline polyline = Geo::ellipse_to_polyline(0, 0, 3, 2, 0, 0, Geo::PI * 2);
        TEST(polyline.size() > 8, "Full ellipse has enough points")
    }

    {
        Geo::Ellipse ellipse(0, 0, 2, 2);
        Geo::Polygon polygon = Geo::ellipse_to_polygon(ellipse);
        TEST(polygon.size() > 8, "Circle as ellipse has enough points")
        for (size_t i = 0; i < polygon.size(); ++i)
        {
            TEST(fequal(2.0, Geo::distance(Geo::Point(0, 0), polygon[i]), 1e-6), "Point on circle")
        }
    }

    {
        Geo::Ellipse ellipse(0, 0, 3, 2, 0, Geo::PI * 2, false);
        Geo::Polygon polygon = Geo::ellipse_to_polygon(ellipse);
        TEST(polygon.size() > 8, "Rotated ellipse has enough points")
    }
}
