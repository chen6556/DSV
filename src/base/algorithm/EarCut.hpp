#pragma once
#include "base/Geometry.hpp"


namespace Geo
{
std::vector<unsigned int> ear_cut_to_indexs(const Polygon &polygon);

std::vector<MarkedPoint> ear_cut_to_coords(const Polygon &polygon);

std::vector<Point> ear_cut_to_points(const Polygon &polygon);

std::vector<Triangle> ear_cut_to_triangles(const Polygon &polygon);


bool merge_ear_cut_triangles(const std::vector<Triangle> &triangles, std::vector<Polygon> &polygons);
} // namespace Geo