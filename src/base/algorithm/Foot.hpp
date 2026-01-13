#pragma once
#include "base/Geometry.hpp"


namespace Geo
{
// 计算线段或直线外一点的垂足
bool foot_point(const Point &start, const Point &end, const Point &point, Point &foot, const bool infinite = false);

// 计算圆外一点到圆的垂足
bool foot_point(const Circle &circle, const Point &point, Point &output0, Point &output1);

// 计算椭圆外一点到椭圆的垂足
int foot_point(const Ellipse &ellipse, const Point &point, std::vector<Point> &output);

// 计算贝塞尔曲线外一点到贝塞尔曲线的垂足<index, t, x, y>
int foot_point(const Point &point, const Bezier &bezier, std::vector<Point> &output,
               std::vector<std::tuple<size_t, double, double, double>> *tvalues = nullptr);

// 计算B样条曲线外一点到B样条曲线的垂足<t, x, y>
int foot_point(const Point &point, const BSpline &bspline, std::vector<Point> &output,
               std::vector<std::tuple<double, double, double>> *tvalues = nullptr);
} // namespace Geo