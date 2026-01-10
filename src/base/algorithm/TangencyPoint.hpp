#pragma once
#include "base/Geometry.hpp"


namespace Geo
{
    // 计算圆外一点与圆的切点
    bool tangency_point(const Point &point, const Circle &circle, Point &output0, Point &output1);

    // 计算椭圆外一点与椭圆的切点
    bool tangency_point(const Point &point, const Ellipse &ellipse, Point &output0, Point &output1);

    // 计算贝塞尔曲线外一点与贝塞尔曲线的切点<index, t, x, y>
    int tangency_point(const Point &point, const Bezier &bezier, std::vector<Point> &output,
        std::vector<std::tuple<size_t, double, double, double>> *tvalues = nullptr);

    // 计算B样条曲线外一点与B样条曲线的切点<t, x, y>
    int tangency_point(const Point &point, const BSpline &bspline, std::vector<Point> &output,
        std::vector<std::tuple<double, double, double>> *tvalues = nullptr);
}