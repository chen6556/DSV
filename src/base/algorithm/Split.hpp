#pragma once
#include "base/Geometry.hpp"


namespace Geo
{
// 将Polyline从pos处拆分为两段Polyline
bool split(const Polyline &polyline, const Point &pos, Polyline &output0, Polyline &output1);

// 将Bezier从pos处拆分为两段Bezier
bool split(const CubicBezier &bezier, const Point &pos, CubicBezier &output0, CubicBezier &output1);

// 将Bezier从i段t值处处拆分为两段Bezier
bool split(const CubicBezier &bezier, const size_t i, const double t, CubicBezier &output0, CubicBezier &output1);

// 将BSpline曲线从pos处拆分为两段BSpline
bool split(const BSpline &bspline, const bool is_cubic, const Point &pos, BSpline &output0, BSpline &output1);

// 将BSpline曲线从t值处拆分为两段BSpline
bool split(const BSpline &bspline, const bool is_cubic, const double t, BSpline &output0, BSpline &output1);

// 将Arc从pos处拆分为两段Arc
bool split(const Arc &arc, const Point &pos, Arc &output0, Arc &output1);

// 将椭圆弧从pos处拆分为两段椭圆弧
bool split(const Ellipse &ellipse, const Point &pos, Ellipse &output0, Ellipse &output1);

// 将多段线n等分
bool split(const Polyline &polyline, const size_t n, std::vector<std::tuple<size_t, double>> &pos);

// 将贝塞尔曲线n等分
bool split(const CubicBezier &bezier, const size_t n, std::vector<std::tuple<size_t, double>> &pos);

// 将B样条曲线n等分
bool split(const BSpline &bspline, const size_t n, std::vector<double> &pos);

// 将椭圆弧n等分
bool split(const Ellipse &ellipse, const size_t n, std::vector<double> &pos);

// 将多段线按step距离等分
bool split(const Polyline &polyline, const double step, std::vector<std::tuple<size_t, double>> &pos);

// 将贝塞尔曲线按step距离等分
bool split(const CubicBezier &bezier, const double step, std::vector<std::tuple<size_t, double>> &pos);

// 将B样条曲线按step距离等分
bool split(const BSpline &bspline, const double step, std::vector<double> &pos);

// 将圆弧按step距离等分
bool split(const Arc &arc, const double step, std::vector<double> &pos);

// 将椭圆弧按step距离等分
bool split(const Ellipse &ellipse, const double step, std::vector<double> &pos);

} // namespace Geo