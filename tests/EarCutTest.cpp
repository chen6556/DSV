#include "EarCutTest.hpp"
#include "Utils.hpp"
#include "../src/base/Algorithm.hpp"


static bool all_passed = true;

void EarCutTest::run_tests()
{
    test_triangulate();
    test_merge();
    PASSED
}

void EarCutTest::test_triangulate()
{
    {
        const Geo::Polygon polygon({Geo::Point(0, 0), Geo::Point(2, 0), Geo::Point(2, 2), Geo::Point(0, 2)});
        std::vector<Geo::Triangle> triangles = Geo::ear_cut_to_triangles(polygon);
        TEST(triangles.size() == 2, "Quad -> 2 triangles")
    }

    {
        const Geo::Polygon polygon({Geo::Point(0, 0), Geo::Point(2, 0), Geo::Point(1, 2)});
        std::vector<Geo::Triangle> triangles = Geo::ear_cut_to_triangles(polygon);
        TEST(triangles.size() == 1, "Triangle -> 1 triangle")
    }

    {
        const Geo::Polygon polygon({Geo::Point(0, 0), Geo::Point(3, 0), Geo::Point(3, 1), Geo::Point(2, 1),
                                    Geo::Point(2, 2), Geo::Point(1, 2), Geo::Point(1, 1), Geo::Point(0, 1)});
        std::vector<Geo::Point> points = Geo::ear_cut_to_points(polygon);
        TEST(points.size() >= 3, "L-shape produces points")
    }

    {
        const Geo::Polygon polygon({Geo::Point(0, 0), Geo::Point(2, 0), Geo::Point(2, 2), Geo::Point(0, 2)});
        std::vector<unsigned int> indexs = Geo::ear_cut_to_indexs(polygon);
        TEST(indexs.size() == 6, "Quad -> 6 indices")
    }

    {
        const Geo::Polygon polygon({Geo::Point(0, 0), Geo::Point(2, 0), Geo::Point(2, 2), Geo::Point(0, 2)});
        std::vector<Geo::Point> points = Geo::ear_cut_to_points(polygon);
        TEST(points.size() == 6, "Quad -> 6 points")
    }

    {
        const Geo::Polygon polygon({Geo::Point(0, 0), Geo::Point(2, 0), Geo::Point(2, 2), Geo::Point(0, 2)});
        std::vector<Geo::MarkedPoint> coords = Geo::ear_cut_to_coords(polygon);
        TEST(coords.size() == 6, "Quad -> 6 marked points")
    }

    {
        const Geo::Polygon polygon({Geo::Point(0, 0), Geo::Point(3, 0), Geo::Point(3, 3), Geo::Point(0, 3)});
        std::vector<Geo::Triangle> triangles = Geo::ear_cut_to_triangles(polygon);
        TEST(triangles.size() == 2, "Square -> 2 triangles")
    }

    {
        const Geo::Polygon polygon({Geo::Point(0, 0), Geo::Point(4, 0), Geo::Point(4, 4), Geo::Point(0, 4)});
        std::vector<Geo::Triangle> triangles = Geo::ear_cut_to_triangles(polygon);
        TEST(triangles.size() == 2, "Large square -> 2 triangles")
    }
}

void EarCutTest::test_merge()
{
    {
        std::vector<Geo::Triangle> triangles = {
            Geo::Triangle(Geo::Point(0, 0), Geo::Point(1, 0), Geo::Point(1, 1)),
            Geo::Triangle(Geo::Point(0, 0), Geo::Point(1, 1), Geo::Point(0, 1))};
        std::vector<Geo::Polygon> polygons;
        bool ok = Geo::merge_ear_cut_triangles(triangles, polygons);
        TEST(ok, "Merge adjacent triangles succeeded")
        TEST(polygons.size() >= 1, "Merge produces polygons")
    }

    {
        std::vector<Geo::Triangle> triangles = {
            Geo::Triangle(Geo::Point(0, 0), Geo::Point(2, 0), Geo::Point(2, 2)),
            Geo::Triangle(Geo::Point(0, 0), Geo::Point(2, 2), Geo::Point(0, 2))};
        std::vector<Geo::Polygon> polygons;
        bool ok = Geo::merge_ear_cut_triangles(triangles, polygons);
        TEST(ok, "Merge square triangles succeeded")
        TEST(polygons.size() >= 1, "Merge produces polygon")
    }
}
