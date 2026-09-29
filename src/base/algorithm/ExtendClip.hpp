#pragma once
#include "base/Geometry.hpp"


namespace Geo
{

// 将贝塞尔曲线从首或尾处截短
bool clip(const CubicBezier &bezier, const bool head, const double length, CubicBezier &result);


};