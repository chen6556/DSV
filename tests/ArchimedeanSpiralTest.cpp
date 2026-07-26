#include "ArchimedeanSpiralTest.hpp"
#include "Utils.hpp"
#include "../src/base/Algorithm.hpp"


static bool all_passed = true;

void ArchimedeanSpiralTest::run_tests()
{
    test_length();
    test_points();
    test_bezier();
    PASSED
}

void ArchimedeanSpiralTest::test_length()
{
    {
        double len = Geo::archimedean_spiral_length(1.0, 0.5, Geo::PI);
        TEST(len > 0, "Spiral length is positive")
    }

    {
        double len1 = Geo::archimedean_spiral_length(1.0, 0.5, Geo::PI);
        double len2 = Geo::archimedean_spiral_length(1.0, 0.5, Geo::PI * 2);
        TEST(len2 > len1, "Larger angle gives longer spiral")
    }

    {
        double len1 = Geo::archimedean_spiral_length(1.0, 0.5, Geo::PI);
        double len2 = Geo::archimedean_spiral_length(2.0, 0.5, Geo::PI);
        TEST(len2 > len1, "Larger a gives longer spiral")
    }

    {
        double len1 = Geo::archimedean_spiral_length(1.0, 0.5, Geo::PI);
        double len2 = Geo::archimedean_spiral_length(1.0, 1.0, Geo::PI);
        TEST(len2 > len1, "Larger b gives longer spiral")
    }

    {
        double len = Geo::archimedean_spiral_length(1.0, 0.1, Geo::PI / 2);
        TEST(len > 0, "Small angle spiral length is positive")
    }
}

void ArchimedeanSpiralTest::test_points()
{
    {
        std::vector<Geo::Point> points = Geo::archimedean_spiral_points(Geo::Point(0, 0), 0.1, 2.0, 0.1, 2, true);
        TEST(points.size() > 2, "Spiral by step has enough points")
    }

    {
        std::vector<Geo::Point> points = Geo::archimedean_spiral_points(Geo::Point(0, 0), 0.1, 2.0, (size_t)10, (size_t)2, true);
        TEST(points.size() >= 5, "Spiral by count has enough points")
    }

    {
        std::vector<Geo::Point> points_cw = Geo::archimedean_spiral_points(Geo::Point(0, 0), 0.1, 2.0, 0.1, 2, true);
        std::vector<Geo::Point> points_ccw = Geo::archimedean_spiral_points(Geo::Point(0, 0), 0.1, 2.0, 0.1, 2, false);
        TEST(points_cw.size() == points_ccw.size(), "CW and CCW have same count")
    }

    {
        std::vector<Geo::Point> points = Geo::archimedean_spiral_points(Geo::Point(0, 0), 0.1, 2.0, 0.05, 3, true);
        TEST(points.size() > 2, "Smaller step gives more points")
    }

    {
        std::vector<Geo::Point> points = Geo::archimedean_spiral_points(Geo::Point(1, 1), 0.1, 1.0, 0.1, 1, true);
        TEST(points.size() > 2, "Non-origin center spiral")
    }
}

void ArchimedeanSpiralTest::test_bezier()
{
    {
        std::vector<Geo::Point> path = Geo::archimedean_spiral_points(Geo::Point(0, 0), 0.1, 2.0, 0.1, 2, true);
        std::vector<Geo::Point> bezier_pts = Geo::archimedean_spiral_bezier(path);
        TEST(bezier_pts.size() >= path.size(), "Bezier has enough points")
    }

    {
        std::vector<Geo::Point> path = Geo::archimedean_spiral_points(Geo::Point(0, 0), 0.1, 1.0, 0.05, 1, true);
        std::vector<Geo::Point> bezier_pts = Geo::archimedean_spiral_bezier(path);
        TEST(bezier_pts.size() >= path.size(), "Short spiral bezier has enough points")
    }
}
