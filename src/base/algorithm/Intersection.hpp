#pragma once
#include "base/Geometry.hpp"


namespace Geo
{
    // 判断两线段是否相交并尝试获取交点,共线相交时仅在一个端点相交时获取交点
    bool is_intersected(const Point &point0, const Point &point1, const Point &point2, const Point &point3, Point &output, const bool infinite = false);

    // 判断线段是否与圆相交并尝试获取交点,返回交点数量
    int is_intersected(const Point &point0, const Point &point1, const Circle &circle, Point &output0, Point &output1, const bool infinite = false);

    // 判断线段是否与椭圆相交并尝试获取交点,返回交点数量
    int is_intersected(const Point &point0, const Point &point1, const Ellipse &ellipse, Point &output0, Point &output1, const bool infinite = false);

    // 判断线段是否与圆弧相交并尝试获取交点,返回交点数量
    int is_intersected(const Point &point0, const Point &point1, const Arc &arc, Point &output0, Point &output1, const bool infinite = false);

    // [数值解]计算贝塞尔曲线与直线的交点,<index, t, x, y>
    int is_intersected(const Point &point0, const Point &point1, const Bezier &bezier, std::vector<Point> &intersections,
        const bool infinite = false, std::vector<std::tuple<size_t, double, double, double>> *tvalues = nullptr);

    // [数值解]计算B样条曲线与直线的交点,<t, x, y>
    int is_intersected(const Point &point0, const Point &point1, const BSpline &bspline, const bool is_cubic, std::vector<Point> &intersections,
        const bool infinite = false, std::vector<std::tuple<double, double, double>> *tvalues = nullptr);

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

    // 判断多段线是否与圆弧相机
    bool is_intersected(const Polyline &polyline, const Arc &arc);

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

    // 计算圆与圆弧交点
    int is_intersected(const Circle &circle, const Arc &arc, Point &output0, Point &output1);

    // 计算两椭圆交点
    int is_intersected(const Ellipse &ellipse0, const Ellipse &ellipse1, Point &point0, Point &point1, Point &point2, Point &point3);

    // 计算圆与椭圆的交点
    int is_intersected(const Circle &circle, const Ellipse &ellipse, Point &point0, Point &point1, Point &point2, Point &point3);

    // 计算圆弧与椭圆的交点
    int is_intersected(const Ellipse &ellipse, const Arc &arc, Point &point0, Point &point1, Point &point2, Point &point3);

    // 计算两圆弧交点
    int is_intersected(const Arc &arc0, const Arc &arc1, Point &point0, Point &point1);

    // [数值解]计算圆与贝塞尔曲线交点,<index, t, x, y>
    int is_intersected(const Circle &circle, const Bezier &bezier, std::vector<Point> &intersections, std::vector<std::tuple<size_t, double, double, double>> *tvalues = nullptr);

    // [数值解]计算圆与B样条曲线交点,<t, x, y>
    int is_intersected(const Circle &circle, const BSpline &bspline, const bool is_cubic, std::vector<Point> &intersections, std::vector<std::tuple<double, double, double>> *tvalues = nullptr);

    // [数值解]计算椭圆与贝塞尔曲线交点,<index, t, x, y>
    int is_intersected(const Ellipse &ellipse, const Bezier &bezier, std::vector<Point> &intersections, std::vector<std::tuple<size_t, double, double, double>> *tvalues = nullptr);

    // [数值解]计算椭圆与B样条曲线交点,<t, x, y>
    int is_intersected(const Ellipse &ellipse, const BSpline &bspline, const bool is_cubic, std::vector<Point> &intersections, std::vector<std::tuple<double, double, double>> *tvalues = nullptr);

    // [数值解]计算圆弧与贝塞尔曲线交点,<index, t, x, y>
    int is_intersected(const Arc &arc, const Bezier &bezier, std::vector<Point> &intersections, std::vector<std::tuple<size_t, double, double, double>> *tvalues = nullptr);

    // [数值解]计算圆弧与B样条曲线交点,<t, x, y>
    int is_intersected(const Arc &arc, const BSpline &bspline, const bool is_cubic, std::vector<Point> &intersections, std::vector<std::tuple<double, double, double>> *tvalues = nullptr);

    // [数值解]计算两贝塞尔曲线交点,<index, t, x, y>,<,<index, t, x, y>>
    int is_intersected(const Bezier &bezier0, const Bezier &bezier1, std::vector<Point> &intersections, std::vector<std::tuple<size_t, double, double, double>> *tvalues0 = nullptr,
        std::vector<std::tuple<size_t, double, double, double>> *tvalues1 = nullptr);

    // [数值解]计算两B样条曲线交点,<index, t, x, y>,,<t, x, y>
    int is_intersected(const BSpline &bspline0, const bool is_cubic0, const BSpline &bspline1, const bool is_cubic1, std::vector<Point> &intersections,
        std::vector<std::tuple<double, double, double>> *tvalues0 = nullptr, std::vector<std::tuple<double, double, double>> *tvalues1 = nullptr);

    // [数值解]计算贝塞尔曲线与B样条曲线交点,,<index, t, x, y>,<t, x, y>
    int is_intersected(const Bezier &bezier, const BSpline &bspline, const bool is_cubic, std::vector<Point> &intersections,
        std::vector<std::tuple<size_t, double, double, double>> *tvalues0 = nullptr, std::vector<std::tuple<double, double, double>> *tvalues1 = nullptr);


    // 判断AABB矩形是否与有限长线段相交,线段完全在AABB矩形内也算相交
    bool is_intersected(const AABBRect &rect, const Point &point0, const Point &point1);

    // 判断AABB矩形是否与多段线相交,多段线完全在AABB矩形内也算相交
    bool is_intersected(const AABBRect &rect, const Polyline &polyline);

    // 判断AABB矩形是否与多边形相交,多边形完全在AABB矩形内或AABB矩形完全在多边形内也算相交
    bool is_intersected(const AABBRect &rect, const Polygon &polygon);

    // 判断AABB矩形是否与圆相交,圆完全在AABB矩形内或AABB矩形完全在圆内也算相交
    bool is_intersected(const AABBRect &rect, const Circle &circle);

    // 判断AABB矩形是否与椭圆相交,椭圆完全在AABB矩形内或AABB矩形完全在圆内也算相交
    bool is_intersected(const AABBRect &rect, const Ellipse &ellipse);

    // 判断AABB矩形是否与圆弧相交,圆弧完全在AABB矩形内也算相交
    bool is_intersected(const AABBRect &rect, const Arc &arc);


    // 判断有限长线段是否与三角形相交,线段完全在三角形内不算相交
    bool is_intersected(const Point &start, const Point &end, const Triangle &triangle, Point &output0, Point &output1);


    namespace NoAABBTest
    {
        // 判断两多段线是否相交
        bool is_intersected(const Polyline &polyline0, const Polyline &polyline1);

        // 判断多段线是否与多边形相交,inside决定多段线完全在多边形内部是否算相交
        bool is_intersected(const Polyline &polyline, const Polygon &polygon, const bool inside = true);

        // 判断两多边形是否相交,inside决定完全在多边形内部是否算相交
        bool is_intersected(const Polygon &polygon0, const Polygon &polygon1, const bool inside = true);
    }


    // 找到Polyline与Polyline在pos附近的交点
    bool find_intersections(const Polyline &polyline0, const Polyline &polyline1, const Point &pos, const double distance, std::vector<Point> &intersections);

    // [数值解]找到Polyline与Bezier在pos附近的交点
    bool find_intersections(const Polyline &polyline, const Bezier &bezier, const Point &pos, const double distance, std::vector<Point> &intersections);

    // [数值解]找到Polyline与BSpline在pos附近的交点
    bool find_intersections(const Polyline &polyline, const BSpline &bspline, const bool is_cubic, const Point &pos, const double distance, std::vector<Point> &intersections);

    // 找到Polyline与Cirlce在pos附近的交点
    bool find_intersections(const Polyline &polyline, const Circle &circle, const Point &pos, const double distance, std::vector<Point> &intersections);

    // 找到Polyline与Ellipse在pos附近的交点
    bool find_intersections(const Polyline &polyline, const Ellipse &ellipse, const Point &pos, const double distance, std::vector<Point> &intersections);

    // 找到Polyline与Arc在pos附件的交点
    bool find_intersections(const Polyline &polyline, const Arc &arc, const Point &pos, const double distance, std::vector<Point> &intersections);

    // 找到Circle与Circle在pos附近的交点
    bool find_intersections(const Circle &circle0, const Circle &circle1, const Point &pos, const double distance, std::vector<Point> &intersections);

    // 找到Ellipse与Ellipse在pos附近的交点
    bool find_intersections(const Ellipse &ellipse0, const Ellipse &ellipse1, const Point &pos, const double distance, std::vector<Point> &intersections);

    // 找到Ellipse与Circle在pos附近的交点
    bool find_intersections(const Ellipse &ellipse, const Circle &circle, const Point &pos, const double distance, std::vector<Point> &intersections);

    // 找到pos附近的交点
    bool find_intersections(const Geometry *object0, const Geometry *object1, const Point &pos, const double distance, std::vector<Point> &intersections);
}