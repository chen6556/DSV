#pragma once
#include "base/Geometry.hpp"


namespace Geo
{
// 将Polyline从pos处拆分为两段Polyline
bool split(const Polyline &polyline, const Point &pos, Polyline &output0, Polyline &output1);

// 将Bezier从pos处拆分为两段Bezier
bool split(const Bezier &bezier, const Point &pos, Bezier &output0, Bezier &output1);

// 将Bezier从i段t值处处拆分为两段Bezier
bool split(const Bezier &bezier, const size_t i, const double t, Bezier &output0, Bezier &output1);

// 将BSpline曲线从pos处拆分为两段BSpline
bool split(const BSpline &bspline, const bool is_cubic, const Point &pos, BSpline &output0, BSpline &output1);

// 将BSpline曲线从t值处拆分为两段BSpline
bool split(const BSpline &bspline, const bool is_cubic, const double t, BSpline &output0, BSpline &output1);

// 将Arc从pos处拆分为两段Arc
bool split(const Arc &arc, const Point &pos, Arc &output0, Arc &output1);

// 将椭圆弧从pos处拆分为两段椭圆弧
bool split(const Ellipse &ellipse, const Point &pos, Ellipse &output0, Ellipse &output1);
} // namespace Geo