#pragma once

#include "base/Geometry.hpp"
#include "algorithm/Boolean.hpp"
#include "algorithm/Conversion.hpp"
#include "algorithm/ClosestPoint.hpp"
#include "algorithm/Distance.hpp"
#include "algorithm/EarCut.hpp"
#include "algorithm/Foot.hpp"
#include "algorithm/Split.hpp"
#include "algorithm/Offset.hpp"
#include "algorithm/Inside.hpp"
#include "algorithm/Intersection.hpp"
#include "algorithm/TangencyPoint.hpp"


namespace Geo
{
// 判断两直线是否平行
bool is_parallel(const Point &point0, const Point &point1, const Point &point2, const Point &point3);

// 判断两线段是否有重合,仅有一个端点重合不算两线段重合
bool is_coincide(const Point &start0, const Point &end0, const Point &start1, const Point &end1);

// 判断有限长线段是否与多边形某一边重合
bool is_coincide(const Point &start, const Point &end, const Polygon &polygon);

// 判断一个有限长线段是否是另一个有限长线段的一部分
bool is_part(const Point &start0, const Point &end0, const Point &start1, const Point &end1);

// 判断点是否在直线的左侧
bool is_on_left(const Point &point, const Point &start, const Point &end);


// 判断点在三角形上
bool is_point_on(const Point &point, const Triangle &triangle);

// 判断点在线上
bool is_point_on(const Geo::Point &point, const std::vector<Geo::Point>::const_iterator &begin,
                 const std::vector<Geo::Point>::const_iterator &end);


// 判断多边形是否是矩形
bool is_Rectangle(const Polygon &polygon);


// 计算两向量叉积
double cross(const double x0, const double y0, const double x1, const double y1);

// 计算两向量叉积
double cross(const Vector &vec0, const Vector &vec1);

// 计算两向量叉积
double cross(const Point &start0, const Point &end0, const Point &start1, const Point &end1);


// 计算直线的旋转角度(弧度制,-PI-PI)
double angle(const Point &start, const Point &end);

// 计算角度(弧度制,-PI-PI)
double angle(const Point &point0, const Point &point1, const Point &point2);

// 计算两直线角夹角(弧度制,-PI-PI)
double angle(const Point &start0, const Point &end0, const Point &start1, const Point &end1);

// 弧度转为-PI-PI
double rad_to_PI(double value);

// 弧度转为0-2PI
double rad_to_2PI(double value);

// 弧度转为角度
double rad_to_degree(double value);

// 角度转为-180°-180°
double degree_to_180(double value);

// 角度转为0°-360°
double degree_to_360(double value);

// 角度转为弧度
double degree_to_rad(double value);


void down_sampling(Geo::Polyline &points, const double distance);

} // namespace Geo