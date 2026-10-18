#pragma once


namespace ConversionTest
{
    void run_tests(bool &success);

    void test_to_coord();

    void test_arc_to_polyline();

    void test_circle_to_polygon();

    void test_ellipse_to_polygon();
}
