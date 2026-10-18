#include "InsertPointTest.hpp"
#include "Utils.hpp"
#include "../src/base/Algorithm.hpp"


static bool all_passed = true;

void InsertPointTest::run_tests(bool &success)
{
    test_polyline();
    test_polygon();
    test_bezier();
    PASSED
}

void InsertPointTest::test_polyline()
{
    {
        Geo::Polyline polyline({Geo::Point(0, 0), Geo::Point(2, 0), Geo::Point(2, 2)});
        const bool ok = Geo::insert_point(polyline, 0, 0.5);
        TEST(ok, "Insert polyline point by segment parameter succeeded")
        TEST(polyline.size() == 4, "Polyline has one extra point")
        TEST(Geo::Point(1, 0) == polyline[1], "Inserted point is at midpoint of first segment")
    }

    {
        Geo::Polyline polyline({Geo::Point(0, 0), Geo::Point(2, 0), Geo::Point(2, 2)});
        const Geo::Point point(1, 1);
        const bool ok = Geo::insert_point(polyline, point);
        TEST(ok, "Insert polyline point by nearest segment succeeded")
        TEST(polyline.size() == 4, "Polyline insertion increased vertex count")
        bool found = false;
        for (const Geo::Point &p : polyline)
        {
            if (p == Geo::Point(1, 0))
            {
                found = true;
                break;
            }
        }
        TEST(found, "Inserted point matches nearest segment foot")
    }
}

void InsertPointTest::test_polygon()
{
    {
        Geo::Polygon polygon({Geo::Point(0, 0), Geo::Point(3, 0), Geo::Point(3, 3), Geo::Point(0, 3)});
        const size_t before = polygon.size();
        const bool ok = Geo::insert_point(polygon, 1, 0.5);
        TEST(ok, "Insert polygon point by segment parameter succeeded")
        TEST(polygon.size() == before + 1, "Polygon adds one vertex")
        TEST(Geo::Point(3, 1.5) == polygon[2], "Inserted point matches segment midpoint")
    }

    {
        Geo::Polygon polygon({Geo::Point(0, 0), Geo::Point(3, 0), Geo::Point(3, 3), Geo::Point(0, 3)});
        const size_t before = polygon.size();
        const Geo::Point point(2, 2);
        const bool ok = Geo::insert_point(polygon, point);
        TEST(ok, "Insert polygon point by nearest segment succeeded")
        TEST(polygon.size() == before + 1, "Polygon insertion increased vertex count")
        bool found = false;
        for (const Geo::Point &p : polygon)
        {
            if (p == Geo::Point(3, 2))
            {
                found = true;
                break;
            }
        }
        TEST(found, "Inserted point lies on the closest edge")
    }
}

void InsertPointTest::test_bezier()
{
    {
        Geo::CubicBezier bezier({Geo::Point(0, 0), Geo::Point(1, 2), Geo::Point(2, 2), Geo::Point(3, 0)}, true);
        const size_t before = bezier.control_points.size();
        const bool ok = Geo::insert_point(bezier, 0, 0.5);
        TEST(ok, "Insert cubic bezier point by segment parameter succeeded")
        TEST(bezier.control_points.size() == before + 3, "Bezier segment split inserts 3 control points")
        TEST(bezier.front() == Geo::Point(0, 0), "Bezier start point remains unchanged")
        TEST(bezier.back() == Geo::Point(3, 0), "Bezier end point remains unchanged")
    }
}
