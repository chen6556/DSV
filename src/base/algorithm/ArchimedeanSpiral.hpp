#pragma once
#include "base/Geometry.hpp"


namespace Geo
{
double archimedean_spiral_length(const double a, const double b, const double theta);

std::vector<Geo::Point> archimedean_spiral_points(const Geo::Point &center, const double inner_radius, const double outer_radius,
                                                  const double step, const size_t turns, const bool clockwise);

std::vector<Geo::Point> archimedean_spiral_points(const Geo::Point &center, const double inner_radius, const double outer_radius,
                                                  const size_t n, const size_t turns, const bool clockwise);

std::vector<Geo::Point> archimedean_spiral_bezier(const std::vector<Geo::Point> &path);
}