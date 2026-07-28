#include <numeric>
#include <algorithm>
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
        Geo::offset(bezier, result, 34.0, 1e-4, 50);
        TEST(!result.empty(), "Bezier offset succeeded")
        std::vector<double> tolerances;
        for (size_t i = 0, count = bezier.control_points.size() / 3; i < count; ++i)
        {
            for (double t = 0; t <= 1; t += 0.01)
            {
                const Geo::Point point0 = bezier.shape_point(i, t);
                const Geo::Point point1 = point0 + bezier.vertical(i, t).normalize() * 70.0;
                double min_err = DBL_MAX;
                for (const Geo::CubicBezier &b : result)
                {
                    std::vector<Geo::Point> points;
                    Geo::is_intersected(point0, point1, b, points, false);
                    for (const Geo::Point &point : points)
                    {
                        min_err = std::min(min_err, std::abs(Geo::distance(point, point0) - 34.0));
                    }
                    if (!points.empty())
                    {
                        break;
                    }
                }
                if (min_err < DBL_MAX)
                {
                    tolerances.push_back(min_err);
                }
            }
        }
        std::sort(tolerances.begin(), tolerances.end());
        const size_t count = tolerances.size();
        const double mid_value = count % 2 == 0 ? (tolerances[count / 2 - 1] + tolerances[count / 2]) / 2 : tolerances[count / 2];
        const double avg_value = std::accumulate(tolerances.begin(), tolerances.end(), 0.0) / count;
        TEST(mid_value / 34.0 <= 1e-4 && avg_value / 34.0 <= 1e-4, "Bezier offset is within tolerance")
    }

    {
        const Geo::CubicBezier bezier({Geo::Point(-100, -100), Geo::Point(-100, 100), Geo::Point(100, 100), Geo::Point(100, -100)}, true);
        std::vector<Geo::CubicBezier> result;
        Geo::offset(bezier, result, 34.0, 1e-4, 50);
        TEST(!result.empty(), "Bezier offset succeeded")
        std::vector<double> tolerances;
        for (size_t i = 0, count = bezier.control_points.size() / 3; i < count; ++i)
        {
            for (double t = 0; t <= 1; t += 0.01)
            {
                const Geo::Point point0 = bezier.shape_point(i, t);
                const Geo::Point point1 = point0 + bezier.vertical(i, t).normalize() * 70.0;
                double min_err = DBL_MAX;
                for (const Geo::CubicBezier &b : result)
                {
                    std::vector<Geo::Point> points;
                    Geo::is_intersected(point0, point1, b, points, false);
                    for (const Geo::Point &point : points)
                    {
                        min_err = std::min(min_err, std::abs(Geo::distance(point, point0) - 34.0));
                    }
                    if (!points.empty())
                    {
                        break;
                    }
                }
                if (min_err < DBL_MAX)
                {
                    tolerances.push_back(min_err);
                }
            }
        }
        std::sort(tolerances.begin(), tolerances.end());
        const size_t count = tolerances.size();
        const double mid_value = count % 2 == 0 ? (tolerances[count / 2 - 1] + tolerances[count / 2]) / 2 : tolerances[count / 2];
        const double avg_value = std::accumulate(tolerances.begin(), tolerances.end(), 0.0) / count;
        TEST(mid_value / 34.0 <= 1e-4 && avg_value / 34.0 <= 1e-4, "Bezier offset is within tolerance")
    }

    {
        const Geo::CubicBezier bezier({Geo::Point(-200, 100), Geo::Point(-100, 100), Geo::Point(100, 100), Geo::Point(200, 100)}, true);
        std::vector<Geo::CubicBezier> result;
        Geo::offset(bezier, result, 34.0, 1e-4, 50);
        TEST(!result.empty(), "Bezier offset succeeded")
        std::vector<double> tolerances;
        for (size_t i = 0, count = bezier.control_points.size() / 3; i < count; ++i)
        {
            for (double t = 0; t <= 1; t += 0.01)
            {
                const Geo::Point point0 = bezier.shape_point(i, t);
                const Geo::Point point1 = point0 + bezier.vertical(i, t).normalize() * 70.0;
                double min_err = DBL_MAX;
                for (const Geo::CubicBezier &b : result)
                {
                    std::vector<Geo::Point> points;
                    Geo::is_intersected(point0, point1, b, points, false);
                    for (const Geo::Point &point : points)
                    {
                        min_err = std::min(min_err, std::abs(Geo::distance(point, point0) - 34.0));
                    }
                    if (!points.empty())
                    {
                        break;
                    }
                }
                if (min_err < DBL_MAX)
                {
                    tolerances.push_back(min_err);
                }
            }
        }
        std::sort(tolerances.begin(), tolerances.end());
        const size_t count = tolerances.size();
        const double mid_value = count % 2 == 0 ? (tolerances[count / 2 - 1] + tolerances[count / 2]) / 2 : tolerances[count / 2];
        const double avg_value = std::accumulate(tolerances.begin(), tolerances.end(), 0.0) / count;
        TEST(mid_value / 34.0 <= 1e-4 && avg_value / 34.0 <= 1e-4, "Bezier offset is within tolerance")
    }
}