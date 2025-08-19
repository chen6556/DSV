#pragma once

#include "base/Geometry.hpp"


namespace Geo
{
    // 两点间距离
    double distance(const double x0, const double y0, const double x1, const double y1);

    // 两点间距离
    double distance(const Point &point0, const Point &point1);

    // 点到直线距离,如果为有限长线段且垂足不在线段上,则计算点到线段端点距离
    double distance(const Point &point, const Line &line, const bool infinite = false);

    // 点到直线距离,如果为有限长线段且垂足不在线段上,则计算点到线段端点距离
    double distance(const Point &point, const Point &start, const Point &end, const bool infinite = false);

    // 点到多段线距离,计算点到每一段有限长线段的距离,取最近距离
    double distance(const Point &point, const Polyline &polyline);

    // 点到多边形距离,计算点到每一段有限长线段的距离,取最近距离
    double distance(const Point &point, const Polygon &polygon);

    // [数值解]点到椭圆距离
    double distance(const Point &point, const Ellipse &ellipse);

    // 两有限长线段间的最短距离
    double distance(const Point &start0, const Point &end0, const Point &start1, const Point &end1, Point &point0, Point &point1);

    // 两点间距离的平方
    double distance_square(const double x0, const double y0, const double x1, const double y1);

    // 两点间距离的平方
    double distance_square(const Point &point0, const Point &point1);

    // 点到直线距离的平方,如果为有限长线段且垂足不在线段上,则计算点到线段端点距离的平方
    double distance_square(const Point &point, const Line &line, const bool infinite = false);

    // 点到直线距离的平方,如果为有限长线段且垂足不在线段上,则计算点到线段端点距离的平方
    double distance_square(const Point &point, const Point &start, const Point &end, const bool infinite = false);

    // 点到多段线距离的平方,计算点到每一段有限长线段的距离,取最近距离的平方
    double distance_square(const Point &point, const Polyline &polyline);

    // 点到多边形距离的平方,计算点到每一段有限长线段的距离,取最近距离的平方
    double distance_square(const Point &point, const Polygon &polygon);

    // 判断点是否在有限长线段或直线上
    bool is_inside(const Point &point, const Line &line, const bool infinite = false);

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

    // 判断一个三角形是否完全在另一个三角形内部,与三角形相交或有顶点在三角形上均不算在三角形内部
    bool is_inside(const Triangle &triangle0, const Triangle &triangle1);

    // 判断两直线是否平行
    bool is_parallel(const Point &point0, const Point &point1, const Point &point2, const Point &point3);

    // 判断两直线是否平行
    bool is_parallel(const Line &line0, const Line &line1);

    // 判断两线段是否有重合,仅有一个端点重合不算两线段重合
    bool is_coincide(const Point &start0, const Point &end0, const Point &start1, const Point &end1);

    // 判断有限长线段是否与多边形某一边重合
    bool is_coincide(const Point &start, const Point &end, const Polygon &polygon);

    // 判断一个有限长线段是否是另一个有限长线段的一部分
    bool is_part(const Point &start0, const Point &end0, const Point &start1, const Point &end1);

    // 判断一个有限长线段是否是另一个有限长线段的一部分
    bool is_part(const Line &line0, const Line &line1);

    // 判断两线段是否相交并尝试获取交点,共线相交时仅在一个端点相交时获取交点
    bool is_intersected(const Point &point0, const Point &point1, const Point &point2, const Point &point3, Point &output, const bool infinite = false);

    // 判断两线段是否相交并尝试获取交点,共线相交时仅在一个端点相交时获取交点
    bool is_intersected(const Line &line0, const Line &line1, Point &output, const bool infinite = true);

    // 判断线段是否与圆相交并尝试获取交点,返回交点数量
    int is_intersected(const Point &point0, const Point &point1, const Circle &circle, Point &output0, Point &output1, const bool infinite = false);

    // 判断线段是否与椭圆相交并尝试获取交点,返回交点数量
    int is_intersected(const Point &point0, const Point &point1, const Ellipse &ellipse, Point &output0, Point &output1, const bool infinite = false);

    // 判断两个AABB矩形是否相交,inside决定完全在AABB矩形内部是否算相交
    bool is_intersected(const AABBRect &rect0, const AABBRect &rect1, const bool inside = true);

    // 判断两多段线是否相交
    bool is_intersected(const Polyline &polyline0, const Polyline &polyline1);

    // 判断多段线是否与多边形相交,inside决定多段线完全在多边形内部是否算相交
    bool is_intersected(const Polyline &polyline, const Polygon &polygon, const bool inside = true);

    // 判断多段线是否与圆相交
    bool is_intersected(const Polyline &polyline, const Circle &circle);

    // 判断多段线是否与椭圆相交,inside决定多段线完全在多边形内部是否算相交
    bool is_intersected(const Polyline &polyline, const Ellipse &ellipse, const bool inside = true);

    // 判断两多边形是否相交,inside决定完全在多边形内部是否算相交
    bool is_intersected(const Polygon &polygon0, const Polygon &polygon1, const bool inside = true);

    // 判断多边形与圆是否相交,inside决定多边形完全在圆内或圆完全在多边形内是否算相交
    bool is_intersected(const Polygon &polygon, const Circle &circle, const bool inside = true);

    // 判断多边形与椭圆是否相交,inside决定多边形完全在椭圆内或椭圆完全在多边形内是否算相交
    bool is_intersected(const Polygon &polygon, const Ellipse &ellipse, const bool inside = true);

    // 判断两圆是否相交,inside决定完全在圆内是否算相交
    bool is_intersected(const Circle &circle0, const Circle &circle1, const bool inside = true);

    // 计算两圆交点
    int is_intersected(const Circle &circle0, const Circle &circle1, Point &point0, Point &point1);

    // 计算两椭圆交点
    int is_intersected(const Ellipse &ellipse0, const Ellipse &ellipse1, Point &point0, Point &point1, Point &point2, Point &point3);

    // 计算圆与椭圆的交点
    int is_intersected(const Circle &circle, const Ellipse &ellipse, Point &point0, Point &point1, Point &point2, Point &point3);

    // 判断AABB矩形是否与有限长线段相交,线段完全在AABB矩形内也算相交
    bool is_intersected(const AABBRect &rect, const Point &point0, const Point &point1);

    // 判断AABB矩形是否与有限长线段相交,线段完全在AABB矩形内也算相交
    bool is_intersected(const AABBRect &rect, const Line &line);

    // 判断AABB矩形是否与多段线相交,多段线完全在AABB矩形内也算相交
    bool is_intersected(const AABBRect &rect, const Polyline &polyline);

    // 判断AABB矩形是否与多边形相交,多边形完全在AABB矩形内或AABB矩形完全在多边形内也算相交
    bool is_intersected(const AABBRect &rect, const Polygon &polygon);

    // 判断AABB矩形是否与圆相交,圆完全在AABB矩形内或AABB矩形完全在圆内也算相交
    bool is_intersected(const AABBRect &rect, const Circle &circle);

    // 判断AABB矩形是否与椭圆相交,椭圆完全在AABB矩形内或AABB矩形完全在圆内也算相交
    bool is_intersected(const AABBRect &rect, const Ellipse &ellipse);

    // 判断有限长线段是否与三角形相交,线段完全在三角形内不算相交
    bool is_intersected(const Point &start, const Point &end, const Triangle &triangle, Point &output0, Point &output1);

    // 判断有限长线段是否与三角形相交,线段完全在三角形内不算相交
    bool is_intersected(const Line &line, const Triangle &triangle, Point &output0, Point &output1);

    // 判断两Geometry Object是否相交
    bool is_intersected(const Geometry *object0, const Geometry *object1);

    // 判断AABB矩形与Geometry Object是否相交
    bool is_intersected(const AABBRect &rect, const Geometry *object);

    namespace NoAABBTest
    {
        // 判断两多段线是否相交
        bool is_intersected(const Polyline &polyline0, const Polyline &polyline1);

        // 判断多段线是否与多边形相交,inside决定多段线完全在多边形内部是否算相交
        bool is_intersected(const Polyline &polyline, const Polygon &polygon, const bool inside = true);

        // 判断两多边形是否相交,inside决定完全在多边形内部是否算相交
        bool is_intersected(const Polygon &polygon0, const Polygon &polygon1, const bool inside = true);

        // 判断AABB矩形是否与多段线相交,多段线完全在AABB矩形内也算相交
        bool is_intersected(const AABBRect &rect, const Polyline &polyline);

        // 判断AABB矩形是否与多边形相交,多边形完全在AABB矩形内或AABB矩形完全在多边形内也算相交
        bool is_intersected(const AABBRect &rect, const Polygon &polygon);

        // 判断AABB矩形是否与椭圆相交,椭圆完全在AABB矩形内或AABB矩形完全在多边形内也算相交
        bool is_intersected(const AABBRect &rect, const Ellipse &ellipse);

        // 判断两Geometry Object是否相交
        bool is_intersected(const Geometry *object0, const Geometry *object1);

        // 判断AABB矩形与Geometry Object是否相交
        bool is_intersected(const AABBRect &rect, const Geometry *object);
    }

    // 找到Polyline与Polyline在pos附近的交点
    bool find_intersections(const Polyline &polyline0, const Polyline &polyline1, const Point &pos, const double distance, std::vector<Point> &intersections);

    // 找到Polyline与Polygon在pos附近的交点
    bool find_intersections(const Polyline &polyline, const Polygon &polygon, const Point &pos, const double distance, std::vector<Point> &intersections);

    // 找到Polyline与Cirlce在pos附近的交点
    bool find_intersections(const Polyline &polyline, const Circle &circle, const Point &pos, const double distance, std::vector<Point> &intersections);

    // 找到Polyline与Ellipse在pos附近的交点
    bool find_intersections(const Polyline &polyline, const Ellipse &ellipse, const Point &pos, const double distance, std::vector<Point> &intersections);

    // 找到Polygon与Polygon在pos附近的交点
    bool find_intersections(const Polygon &polygon0, const Polygon &polygon1, const Point &pos, const double distance, std::vector<Point> &intersections);

    // 找到Polygon与Cirlce在pos附近的交点
    bool find_intersections(const Polygon &polygon, const Circle &circle, const Point &pos, const double distance, std::vector<Point> &intersections);

    // 找到Polygon与Ellipse在pos附近的交点
    bool find_intersections(const Polygon &polygon, const Ellipse &ellipse, const Point &pos, const double distance, std::vector<Point> &intersections);
    
    // 找到Circle与Circle在pos附近的交点
    bool find_intersections(const Circle &circle0, const Circle &circle1, const Point &pos, const double distance, std::vector<Point> &intersections);

    // 找到Ellipse与Ellipse在pos附近的交点
    bool find_intersections(const Ellipse &ellipse0, const Ellipse &ellipse1, const Point &pos, const double distance, std::vector<Point> &intersections);

    // 找到Ellipse与Circle在pos附近的交点
    bool find_intersections(const Ellipse &ellipse, const Circle &circle, const Point &pos, const double distance, std::vector<Point> &intersections);

    // 找到pos附近的交点
    bool find_intersections(const Geometry *object0, const Geometry *object1, const Point &pos, const double distance, std::vector<Point> &intersections);

    // 判断点是否在直线的左侧
    bool is_on_left(const Point &point, const Point &start, const Point &end);

    // 判断点在三角形上
    bool is_point_on(const Point &point, const Triangle &triangle);

    // 判断点在线上
    bool is_point_on(const Geo::Point &point, std::vector<Geo::Point>::const_iterator begin, std::vector<Geo::Point>::const_iterator end);

    // 判断多边形是否是矩形
    bool is_Rectangle(const Polygon &polygon);

    // 计算两向量叉积
    double cross(const double x0, const double y0, const double x1, const double y1);

    // 计算两向量叉积
    double cross(const Vector &vec0, const Vector &vec1);

    // 计算两向量叉积
    double cross(const Point &start0, const Point &end0, const Point &start1, const Point &end1);

    // 计算线段或直线外一点的垂足
    bool foot_point(const Point &start, const Point &end, const Point &point, Point &foot, const bool infinite = false);

    // 计算线段或直线外一点的垂足
    bool foot_point(const Line &line, const Point &point, Point &foot, const bool infinite = false);

    // 计算圆外一点与圆的切点
    bool tangency_point(const Point &point, const Circle &circle, Point &output0, Point &output1);

    // 计算椭圆外一点与椭圆的切点
    bool tangency_point(const Point &point, const Ellipse &ellipse, Point &output0, Point &output1);

    // 计算直线的旋转角度(弧度制,-PI-PI)
    double angle(const Point &start, const Point &end);

    // 计算角度(弧度制,-PI-PI)
    double angle(const Point &point0, const Point &point1, const Point &point2);

    // 计算两直线角夹角(弧度制,-PI-PI)
    double angle(const Point &start0, const Point &end0, const Point &start1, const Point &end1);

    // 计算两直线角夹角(弧度制,-PI-PI)
    double angle(const Line &line0, const Line &line1);

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

    // 点在另一坐标系下的坐标
    Point to_coord(const Point &point, const double x, const double y, const double rad);

    // 倒圆角
    bool angle_to_arc(const Point &point0, const Point &point1, const Point &point2, const double radius, Polyline &arc, const double step = 0.8, const double down_sampling_value = 0.02);

    Polyline arc_to_polyline(const Point &center, const double radius, double start_angle, double end_angle, const bool is_cw, const double down_sampling_value = 0.02);

    Polygon circle_to_polygon(const double x, const double y, const double r, const double down_sampling_value = 0.02);

    Polygon circle_to_polygon(const Circle &circle, const double down_sampling_value = 0.02);

    Polygon ellipse_to_polygon(const double x, const double y, const double a, const double b, const double rad, const double down_sampling_value = 0.02);

    Polygon ellipse_to_polygon(const Ellipse &ellipse, const double down_sampling_value = 0.02);

    std::vector<size_t> ear_cut_to_indexs(const Polygon &polygon);

    std::vector<MarkedPoint> ear_cut_to_coords(const Polygon &polygon);

    std::vector<Point> ear_cut_to_points(const Polygon &polygon);

    std::vector<Triangle> ear_cut_to_triangles(const Polygon &polygon);

    namespace Offset
    {
        enum class JoinType { Square, Bevel, Round, Miter };
        enum class EndType { Polygon, Joined, Butt, Square, Round };
    };

    bool offset(const Polyline &input, Polyline &result, const double distance);

    bool offset(const Polygon &input, Polygon &result, const double distance);

    bool offset(const Polygon &input, std::vector<Polygon> &result, const double distance,
        const Offset::JoinType join_type = Offset::JoinType::Round,
        const Offset::EndType end_type = Offset::EndType::Polygon, const double epsilon = 2.0);

    bool offset(const Circle &input, Circle &result, const double distance);

    bool offset(const AABBRect &input, AABBRect &result, const double distance);

    bool polygon_union(const Polygon &polygon0, const Polygon &polygon1, std::vector<Polygon> &output);

    bool polygon_intersection(const Polygon &polygon0, const Polygon &polygon1, std::vector<Polygon> &output);

    bool polygon_difference(const Polygon &polygon0, const Polygon &polygon1, std::vector<Polygon> &output);

    bool merge_ear_cut_triangles(const std::vector<Triangle> &triangles, std::vector<Polygon> &polygons);

    void down_sampling(Geo::Polyline &points, const double distance);
}