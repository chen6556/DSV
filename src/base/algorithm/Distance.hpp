#pragma once
#include "base/Geometry.hpp"


namespace Geo
{
// 两点间距离
double distance(const double x0, const double y0, const double x1, const double y1);

// 两点间距离
double distance(const Point &point0, const Point &point1);

// 点到直线距离,如果为有限长线段且垂足不在线段上,则计算点到线段端点距离
double distance(const Point &point, const Point &start, const Point &end, const bool infinite = false);

// 点到多段线距离,计算点到每一段有限长线段的距离,取最近距离
double distance(const Point &point, const Polyline &polyline);

// [数值解]点到贝塞尔曲线距离,计算点到每一段曲线的距离,取最近距离
double distance(const Point &point, const CubicBezier &bezier);

// [数值解]点到B样条曲线的距离
double distance(const Point &point, const BSpline &bspline, const bool is_cubic);

// 点到多边形距离,计算点到每一段有限长线段的距离,取最近距离
double distance(const Point &point, const Polygon &polygon);

// [数值解]点到椭圆距离
double distance(const Point &point, const Ellipse &ellipse);

// 点到圆弧距离
double distance(const Point &point, const Arc &arc);

// 两有限长线段间的最短距离
double distance(const Point &start0, const Point &end0, const Point &start1, const Point &end1, Point &point0, Point &point1);


// 两点间距离的平方
double distance_square(const double x0, const double y0, const double x1, const double y1);

// 两点间距离的平方
double distance_square(const Point &point0, const Point &point1);

// 点到直线距离的平方,如果为有限长线段且垂足不在线段上,则计算点到线段端点距离的平方
double distance_square(const Point &point, const Point &start, const Point &end, const bool infinite = false);

// 点到多段线距离的平方,计算点到每一段有限长线段的距离,取最近距离的平方
double distance_square(const Point &point, const Polyline &polyline);

// 点到多边形距离的平方,计算点到每一段有限长线段的距离,取最近距离的平方
double distance_square(const Point &point, const Polygon &polygon);
} // namespace Geo