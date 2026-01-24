#pragma once
#include "base/Geometry.hpp"


namespace Geo
{
// 计算多段线到一点的最近点
int closest_point(const Polyline &polyline, const Point &point, std::vector<Point> &output);

// 计算多边形到一点的最近点
int closest_point(const Polygon &polygon, const Point &point, std::vector<Point> &output);

// 计算椭圆到一点的最近点
int closest_point(const Ellipse &ellipse, const Point &point, std::vector<Point> &output);

// [数值解]计算贝塞尔曲线到一点的最近点
int closest_point(const CubicBezier &bezier, const Point &point, std::vector<Point> &output,
                  std::vector<std::tuple<size_t, double, double, double>> *tvalues = nullptr);

// [数值解]计算B样条曲线到一点的最近点
int closest_point(const BSpline &bspline, const bool is_cubic, const Point &point, std::vector<Point> &output,
                  std::vector<std::tuple<double, double, double>> *tvalues = nullptr);
} // namespace Geo