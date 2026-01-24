#pragma once
#include "base/Geometry.hpp"


namespace Geo
{
namespace Offset
{
enum class JoinType
{
    Square,
    Bevel,
    Round,
    Miter
};
enum class EndType
{
    Polygon,
    Joined,
    Butt,
    Square,
    Round
};
}; // namespace Offset

bool offset(const Polyline &input, Polyline &result, const double distance);

bool offset(const Polygon &input, Polygon &result, const double distance);

bool offset(const Polygon &input, std::vector<Polygon> &result, const double distance,
            const Offset::JoinType join_type = Offset::JoinType::Round, const Offset::EndType end_type = Offset::EndType::Polygon,
            const double epsilon = 2.0);

bool offset(const Circle &input, Circle &result, const double distance);

bool offset(const AABBRect &input, AABBRect &result, const double distance);

bool offset(const CubicBezier &bezier, std::vector<CubicBezier> &result, const double distance, const double tolerance, const int sample_count);
} // namespace Geo