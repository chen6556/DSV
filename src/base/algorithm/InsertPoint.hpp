#pragma once
#include "base/Geometry.hpp"


namespace Geo
{

bool insert_point(Geo::Polyline &polyline, const size_t i, const double t);

bool insert_point(Geo::Polygon &polygon, const size_t i, const double t);

bool insert_point(Geo::CubicBezier &bezier, const size_t i, const double t);

bool insert_point(Geo::Polyline &polyline, const Geo::Point &point);

bool insert_point(Geo::Polygon &polygon, const Geo::Point &point);

}