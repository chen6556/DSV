#pragma once
#include "base/Geometry.hpp"


namespace Geo
{
// 判断点是否在有限长线段或直线上
bool is_inside(const Point &point, const Point &start, const Point &end, const bool infinite = false);

// 判断点是否在多段线上
bool is_inside(const Point &point, const Polyline &polyline);

// 判断点是否在多边形内,coincide决定是否包含点在多边形上的情况
bool is_inside(const Point &point, const Polygon &polygon, const bool coincide = false);

// 判断点是否在AABB矩形内,coincide决定是否包含点在AABB矩形上的情况
bool is_inside(const Point &point, const AABBRect &rect, const bool coincide = false);

// 判断点是否在圆内,coincide决定是否包含点在圆上的情况
bool is_inside(const Point &point, const Circle &circle, const bool coincide = false);

// 判断点是否在椭圆内,coincide决定是否包含点在椭圆上的情况
bool is_inside(const Point &point, const Ellipse &ellipse, const bool coincide = false);

// 判断点是否在三角形内,coincide决定是否包含点在三角形上的情况
bool is_inside(const Point &point, const Point &point0, const Point &point1, const Point &point2, const bool coincide = false);

// 判断点是否在三角形内,coincide决定是否包含点在三角形上的情况
bool is_inside(const Point &point, const Triangle &triangle, const bool coincide = false);

// 判断有限长线段是否完全在三角形内,线段与三角形相交或有端点在三角形上均不算在三角形内部
bool is_inside(const Point &start, const Point &end, const Triangle &triangle);

// 判断点是否在圆弧上
bool is_inside(const Point &point, const Arc &arc);

// 判断一个三角形是否完全在另一个三角形内部,与三角形相交或有顶点在三角形上均不算在三角形内部
bool is_inside(const Triangle &triangle0, const Triangle &triangle1);


namespace NoAABBTest
{
// 判断点是否在多边形内,coincide决定是否包含点在多边形上的情况
bool is_inside(const Point &point, const Polygon &polygon, const bool coincide = false);
} // namespace NoAABBTest
} // namespace Geo