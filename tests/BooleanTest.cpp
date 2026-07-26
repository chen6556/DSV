#include "BooleanTest.hpp"
#include "Utils.hpp"
#include "../src/base/Algorithm.hpp"


static bool all_passed = true;

void BooleanTest::run_tests()
{
    test_polygon_boolean();
    test_circle_boolean();
    PASSED
}

void BooleanTest::test_polygon_boolean()
{
    {
        const Geo::Polygon p0({Geo::Point(0, 0), Geo::Point(2, 0), Geo::Point(2, 2), Geo::Point(0, 2)});
        const Geo::Polygon p1({Geo::Point(1, 1), Geo::Point(3, 1), Geo::Point(3, 3), Geo::Point(1, 3)});
        std::vector<Geo::Polygon> output;
        bool ok = Geo::polygon_union(p0, p1, output);
        TEST(ok, "Union succeeded")
        TEST(output.size() >= 1, "Union has results")
    }

    {
        const Geo::Polygon p0({Geo::Point(0, 0), Geo::Point(2, 0), Geo::Point(2, 2), Geo::Point(0, 2)});
        const Geo::Polygon p1({Geo::Point(1, 1), Geo::Point(3, 1), Geo::Point(3, 3), Geo::Point(1, 3)});
        std::vector<Geo::Polygon> output;
        bool ok = Geo::polygon_intersection(p0, p1, output);
        TEST(ok, "Intersection succeeded")
        TEST(output.size() >= 1, "Intersection has results")
    }

    {
        const Geo::Polygon p0({Geo::Point(0, 0), Geo::Point(2, 0), Geo::Point(2, 2), Geo::Point(0, 2)});
        const Geo::Polygon p1({Geo::Point(1, 1), Geo::Point(3, 1), Geo::Point(3, 3), Geo::Point(1, 3)});
        std::vector<Geo::Polygon> output;
        bool ok = Geo::polygon_difference(p0, p1, output);
        TEST(ok, "Difference succeeded")
        TEST(output.size() >= 1, "Difference has results")
    }

    {
        const Geo::Polygon p0({Geo::Point(0, 0), Geo::Point(2, 0), Geo::Point(2, 2), Geo::Point(0, 2)});
        const Geo::Polygon p1({Geo::Point(1, 1), Geo::Point(3, 1), Geo::Point(3, 3), Geo::Point(1, 3)});
        std::vector<Geo::Polygon> output;
        bool ok = Geo::polygon_xor(p0, p1, output);
        TEST(ok, "XOR succeeded")
        TEST(output.size() >= 1, "XOR has results")
    }

    {
        const Geo::Polygon p0({Geo::Point(0, 0), Geo::Point(4, 0), Geo::Point(4, 4), Geo::Point(0, 4)});
        const Geo::Polygon p1({Geo::Point(1, 1), Geo::Point(3, 1), Geo::Point(3, 3), Geo::Point(1, 3)});
        std::vector<Geo::Polygon> output;
        bool ok = Geo::polygon_intersection(p0, p1, output);
        TEST(ok, "Inner square intersection succeeded")
        TEST(output.size() >= 1, "Inner square intersection has results")
    }
}

void BooleanTest::test_circle_boolean()
{
    {
        const Geo::Circle c0(0, 0, 1);
        const Geo::Circle c1(1, 0, 1);
        std::vector<Geo::Arc> output;
        bool ok = Geo::circle_union(c0, c1, output);
        TEST(ok, "Circle union succeeded")
        TEST(output.size() >= 1, "Circle union has results")
    }

    {
        const Geo::Circle c0(0, 0, 1);
        const Geo::Circle c1(1, 0, 1);
        std::vector<Geo::Arc> output;
        bool ok = Geo::circle_intersection(c0, c1, output);
        TEST(ok, "Circle intersection succeeded")
        TEST(output.size() >= 1, "Circle intersection has results")
    }

    {
        const Geo::Circle c0(0, 0, 1);
        const Geo::Circle c1(1, 0, 1);
        std::vector<Geo::Arc> output;
        bool ok = Geo::circle_difference(c0, c1, output);
        TEST(ok, "Circle difference succeeded")
        TEST(output.size() >= 1, "Circle difference has results")
    }

    {
        const Geo::Circle c0(0, 0, 1);
        const Geo::Circle c1(1, 0, 1);
        std::vector<Geo::Arc> output;
        bool ok = Geo::circle_xor(c0, c1, output);
        TEST(ok, "Circle XOR succeeded")
        TEST(output.size() >= 1, "Circle XOR has results")
    }
}
