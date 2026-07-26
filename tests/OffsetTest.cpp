#include "OffsetTest.hpp"
#include "Utils.hpp"
#include "../src/base/Algorithm.hpp"


static bool all_passed = true;

void OffsetTest::run_tests()
{
    test_polyline();
    test_polygon();
    test_circle();
    test_aabbrect();
    test_bezier();
    PASSED
}

void OffsetTest::test_polyline()
{
    {
        const Geo::Polyline polyline({Geo::Point(0, 0), Geo::Point(4, 0)});
        Geo::Polyline result;
        bool ok = Geo::offset(polyline, result, 1.0);
        TEST(ok, "Positive offset succeeded")
        TEST(result.size() == 2, "Result has 2 points")
        TEST(fequal(0.0, result[0].x, 1e-6) && fequal(1.0, result[0].y, 1e-6), "Left end at (0,1)")
        TEST(fequal(4.0, result[1].x, 1e-6) && fequal(1.0, result[1].y, 1e-6), "Right end at (4,1)")
    }

    {
        const Geo::Polyline polyline({Geo::Point(0, 0), Geo::Point(4, 0)});
        Geo::Polyline result;
        bool ok = Geo::offset(polyline, result, -1.0);
        TEST(ok, "Negative offset succeeded")
        TEST(result.size() == 2, "Result has 2 points")
        TEST(fequal(0.0, result[0].x, 1e-6) && fequal(-1.0, result[0].y, 1e-6), "Left end at (0,-1)")
        TEST(fequal(4.0, result[1].x, 1e-6) && fequal(-1.0, result[1].y, 1e-6), "Right end at (4,-1)")
    }

    {
        const Geo::Polyline polyline({Geo::Point(0, 0), Geo::Point(2, 0), Geo::Point(2, 2)});
        Geo::Polyline result;
        bool ok = Geo::offset(polyline, result, 0.5);
        TEST(ok, "L-shape offset succeeded")
        TEST(result.size() >= 3, "L-shape result has enough points")
    }

}

void OffsetTest::test_polygon()
{
    {
        const Geo::Polygon polygon({Geo::Point(-1, -1), Geo::Point(1, -1), Geo::Point(1, 1), Geo::Point(-1, 1)});
        Geo::Polygon result;
        bool ok = Geo::offset(polygon, result, 0.2);
        TEST(ok, "Outward offset succeeded")
        TEST(result.size() >= 4, "Result has enough points")
    }

    {
        const Geo::Polygon polygon({Geo::Point(-1, -1), Geo::Point(1, -1), Geo::Point(1, 1), Geo::Point(-1, 1)});
        std::vector<Geo::Polygon> results;
        bool ok = Geo::offset(polygon, results, -0.3);
        TEST(ok, "Inward offset succeeded")
        TEST(results.size() >= 1, "Inward offset has results")
    }

    {
        const Geo::Polygon polygon({Geo::Point(0, 0), Geo::Point(2, 0), Geo::Point(2, 2), Geo::Point(0, 2)});
        Geo::Polygon result;
        bool ok = Geo::offset(polygon, result, 0.1);
        TEST(ok, "Small square offset succeeded")
        TEST(result.size() >= 4, "Result has enough points")
    }
}

void OffsetTest::test_circle()
{
    {
        const Geo::Circle circle(0, 0, 5);
        Geo::Circle result;
        bool ok = Geo::offset(circle, result, 1.0);
        TEST(ok, "Positive offset succeeded")
        TEST(fequal(6.0, result.radius), "Radius increased by 1")
        TEST(fequal(0.0, result.x) && fequal(0.0, result.y), "Center unchanged")
    }

    {
        const Geo::Circle circle(0, 0, 5);
        Geo::Circle result;
        bool ok = Geo::offset(circle, result, -1.0);
        TEST(ok, "Negative offset succeeded")
        TEST(fequal(4.0, result.radius), "Radius decreased by 1")
    }

    {
        const Geo::Circle circle(0, 0, 3);
        Geo::Circle result;
        bool ok = Geo::offset(circle, result, 1.0);
        TEST(ok, "Radius-3 offset succeeded")
        TEST(fequal(4.0, result.radius), "New radius is 4")
    }

    {
        const Geo::Circle circle(2, 3, 2);
        Geo::Circle result;
        bool ok = Geo::offset(circle, result, 0.5);
        TEST(ok, "Non-origin circle offset succeeded")
        TEST(fequal(2.5, result.radius), "Radius increased")
        TEST(fequal(2.0, result.x) && fequal(3.0, result.y), "Center unchanged")
    }
}

void OffsetTest::test_aabbrect()
{
    {
        const Geo::AABBRect rect(-1, -1, 1, 1);
        Geo::AABBRect result;
        bool ok = Geo::offset(rect, result, 0.5);
        TEST(ok, "Outward offset succeeded")
        TEST(fequal(-1.5, result.left), "Left expanded")
        TEST(fequal(1.5, result.right), "Right expanded")
    }

    {
        const Geo::AABBRect rect(-1, -1, 1, 1);
        Geo::AABBRect result;
        bool ok = Geo::offset(rect, result, -0.3);
        TEST(ok, "Inward offset succeeded")
        TEST(fequal(-0.7, result.left), "Left contracted")
        TEST(fequal(0.7, result.right), "Right contracted")
    }

    {
        const Geo::AABBRect rect(0, 0, 4, 3);
        Geo::AABBRect result;
        bool ok = Geo::offset(rect, result, 0.5);
        TEST(ok, "Non-centered rect offset succeeded")
        TEST(fequal(-0.5, result.left), "Left expanded")
        TEST(fequal(4.5, result.right), "Right expanded")
    }
}

void OffsetTest::test_bezier()
{
    {
        const Geo::CubicBezier bezier({Geo::Point(0, 0), Geo::Point(100, 100), Geo::Point(200, 0), Geo::Point(300, 100)}, true);
        std::vector<Geo::CubicBezier> result;
        Geo::offset(bezier, result, 34.0, 1e-9, 50);
        TEST(!result.empty(), "Bezier offset succeeded")
        double max_tolerance = -1;
        for (const Geo::CubicBezier &b : result)
        {
            for (size_t i = 0, count = b.control_points.size() / 3; i < count; ++i)
            {
                for (double t = 0; t <= 1; t += 0.02)
                {
                    std::vector<Geo::Point> points;
                    Geo::foot_point(b.shape_point(i, t), bezier, points);
                    double min_dis = DBL_MAX;
                    for (const Geo::Point &foot : points)
                    {
                        min_dis = std::min(min_dis, Geo::distance(foot, bezier));
                    }
                    max_tolerance = std::max(max_tolerance, std::abs(min_dis - 34.0));
                }
            }
        }
        TEST(max_tolerance <= 1e-8, "Bezier offset is within tolerance")
    }
}