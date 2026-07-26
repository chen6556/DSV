#pragma once


namespace DistanceTest
{
    void run_tests();

    void test_point_to_point();

    void test_point_to_line();

    void test_point_to_polyline();

    void test_point_to_polygon();

    void test_point_to_bezier();

    void test_point_to_bspline();

    void test_point_to_arc();

    void test_point_to_ellipse();

    void test_line_to_line();

    void test_distance_square();
}