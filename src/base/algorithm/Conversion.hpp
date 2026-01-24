#pragma once
#include "base/Geometry.hpp"


namespace Geo
{
// 点在另一坐标系下的坐标
Point to_coord(const Point &point, const double x, const double y, const double rad);


// 倒圆角
bool angle_to_arc(const Point &point0, const Point &point1, const Point &point2, const double radius, Polyline &arc,
                  const double step = 0.8, const double down_sampling_value = 0.02);

// 倒圆角
bool angle_to_arc(const Point &point0, const Point &point1, const Point &point2, const double radius, Arc &arc);

// 非对称圆角
bool angle_to_arc(const Point &point0, const Point &point1, const Point &point2, const double radius0, const double radius1, CubicBezier &arc);

// 自由非对称圆角
bool angle_to_arc(const Point &start, const Point &center, const Point &end, CubicBezier &arc);


Polyline arc_to_polyline(const Point &center, const double radius, double start_angle, double end_angle, const bool is_cw,
                         const double down_sampling_value = 0.02);

Polyline arc_to_polyline(const Arc &arc, const double down_sampling_value = 0.02);

CubicBezier arc_to_bezier(const Arc &arc);


Polygon circle_to_polygon(const double x, const double y, const double r, const double down_sampling_value = 0.02);

Polygon circle_to_polygon(const Circle &circle, const double down_sampling_value = 0.02);

CubicBezier circle_to_bezier(const Circle &circle);


Polygon ellipse_to_polygon(const double x, const double y, const double a, const double b, const double rad,
                           const double down_sampling_value = 0.02);

Polygon ellipse_to_polygon(const Ellipse &ellipse, const double down_sampling_value = 0.02);

// start_angle:参数方程中参数的起点 end_angle:参数方程中参数的终点
Polyline ellipse_to_polyline(const double x, const double y, const double a, const double b, const double rad, const double start_angle,
                             double end_angle, const double down_sampling_value = 0.02);

Polyline ellipse_to_polyline(const Ellipse &ellipse, const double down_sampling_value = 0.02);

CubicBezier ellipse_to_bezier(const Ellipse &ellipse);


CubicBSpline bezier_to_bspline(const CubicBezier &bezier);

CubicBezier *bspline_to_bezier(const BSpline &bspline);


CubicBezier *blend(const Point &pre0, const Point &point0, const Point &point1, const Point &pre1);
} // namespace Geo