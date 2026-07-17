#pragma once
#include "base/Geometry.hpp"


namespace Geo
{
bool fillet(const Geo::Polygon &polygon, const size_t index, const double radius, Geo::Arc &arc, Geo::Polyline &result);

bool fillet(const Geo::Polyline &polyline, const size_t index, const double radius, Geo::Arc &arc, Geo::Polyline &result0,
            Geo::Polyline &result1);

bool fillet(const Geo::Polygon &polygon, const size_t index, const double radius0, const double radius1, Geo::CubicBezier &arc,
            Geo::Polyline &result);

bool fillet(const Geo::Polyline &polyline, const size_t index, const double radius0, const double radius1, Geo::CubicBezier &arc,
            Geo::Polyline &result0, Geo::Polyline &result1);

bool fillet(const Geo::Polyline &polyline0, const Geo::Point &point0, const Geo::Polyline &polyline1, const Geo::Point &point1,
            const double radius, Geo::Arc &arc, Geo::Polyline &result0, Geo::Polyline &result1, Geo::Polyline &result2,
            Geo::Polyline &result3);

bool fillet(const Geo::Polyline &polyline0, const Geo::Point &point0, const Geo::Arc &arc1, const Geo::Point &point1, const double radius,
            Geo::Arc &arc, Geo::Polyline &result0, Geo::Polyline &result1, Geo::Arc &result2);

bool fillet(const Geo::Polyline &polyline0, const Geo::Point &point0, const Geo::CubicBezier &bezier1, const Geo::Point &point1,
            const double radius, Geo::Arc &arc, Geo::Polyline &result0, Geo::Polyline &result1, Geo::CubicBezier &result2);

bool fillet(const Geo::Polyline &polyline0, const Geo::Point &point0, const Geo::BSpline &bspline1, const Geo::Point &point1,
            const bool is_cubic, const double radius, Geo::Arc &arc, Geo::Polyline &result0, Geo::Polyline &result1, Geo::BSpline &result2);

bool fillet(const Geo::Polyline &polyline0, const Geo::Point &point0, const Geo::Polyline &polyline1, const Geo::Point &point1,
            const double radius0, const double radius1, Geo::CubicBezier &arc, Geo::Polyline &result0, Geo::Polyline &result1,
            Geo::Polyline &result2, Geo::Polyline &result3);

bool fillet(const Geo::CubicBezier &bezier0, const Geo::Point &point0, const Geo::CubicBezier &bezier1, const Geo::Point &point1,
            const double radius, Geo::Arc &arc, Geo::CubicBezier &result0, Geo::CubicBezier &result1);

bool fillet(const Geo::Arc &arc0, const Geo::Point &point0, const Geo::Arc &arc1, const Geo::Point &point1,
            const double radius, Geo::Arc &arc, Geo::Arc &result0, Geo::Arc &result1);

}; // namespace Geo