#include <algorithm>
#include "base/Algorithm.hpp"


Geo::Point Geo::to_coord(const Geo::Point &point, const double x, const double y, const double rad)
{
    const double x0 = point.x - x, y0 = point.y - y;
    return Geo::Point(x0 * std::cos(rad) + y0 * std::sin(rad), y0 * std::cos(rad) - x0 * std::sin(rad));
}


bool Geo::angle_to_arc(const Point &point0, const Point &point1, const Point &point2, const double radius, Polyline &arc, const double step,
                       const double down_sampling_value)
{
    if (radius <= 0)
    {
        return false;
    }

    const double rad1 = std::abs(Geo::angle(point0, point1, point2));
    const double len = radius / std::tan(rad1 / 2.0);
    if (Geo::distance_square(point1, point0) < len * len || Geo::distance_square(point2, point1) < len * len)
    {
        return false;
    }

    Geo::Vector vp = ((point0 - point1).normalize() + (point2 - point1).normalize()).normalize();
    Geo::Point center = point1 + vp * std::hypot(len, radius);
    Geo::Point foot0, foot1;
    Geo::foot_point(point0, point1, center, foot0, true);
    Geo::foot_point(point2, point1, center, foot1, true);

    Geo::Vector vec = foot0 - center;
    double vrad = std::atan2(vec.y, vec.x);
    double rad2 = Geo::PI - rad1;
    if (Geo::angle(foot0, center, foot1) < 0)
    {
        rad2 = -rad2;
    }

    arc.clear();
    const double c = std::atan(len / radius) * radius * 2;
    const size_t slice_num = std::max(c / step, 10.0);
    const double d_rad = rad2 / slice_num;
    for (size_t i = 0; i <= slice_num; i++)
    {
        arc.append(center + Geo::Point(radius * std::cos(vrad), radius * std::sin(vrad)));
        vrad += d_rad;
    }
    Geo::down_sampling(arc, down_sampling_value);
    return true;
}

bool Geo::angle_to_arc(const Point &point0, const Point &point1, const Point &point2, const double radius, Arc &arc)
{
    if (radius <= 0)
    {
        return false;
    }

    const double rad1 = std::abs(Geo::angle(point0, point1, point2));
    const double len = radius / std::tan(rad1 / 2.0);
    if (Geo::distance_square(point1, point0) < len * len || Geo::distance_square(point2, point1) < len * len)
    {
        return false;
    }

    const Geo::Vector vp = ((point0 - point1).normalize() + (point2 - point1).normalize()).normalize();
    const Geo::Point center = point1 + vp * std::hypot(len, radius);
    Geo::Point foot0, foot1;
    Geo::foot_point(point0, point1, center, foot0, true);
    Geo::foot_point(point2, point1, center, foot1, true);

    const Geo::Vector vec = foot0 - center;
    const double rad2 = Geo::angle(foot0, center, foot1) < 0 ? rad1 - Geo::PI : Geo::PI - rad1;
    const double vrad = std::atan2(vec.y, vec.x) + rad2 / 2;
    arc = Geo::Arc(foot0, center + Geo::Point(radius * std::cos(vrad), radius * std::sin(vrad)), foot1);
    return true;
}

bool Geo::angle_to_arc(const Point &point0, const Point &point1, const Point &point2, const double radius0, const double radius1,
                       CubicBezier &arc)
{
    if (radius0 <= 0 || radius1 <= 0)
    {
        return false;
    }

    Geo::Point center;
    if (Geo::is_on_left(point0, point2, point1))
    {
        const Geo::Point vec0 = (point0 - point1).vertical().normalize() * radius0;
        const Geo::Point vec1 = (point1 - point2).vertical().normalize() * radius1;
        if (!Geo::is_intersected(point0 + vec0, point1 + vec0, point2 + vec1, point1 + vec1, center, true))
        {
            return false;
        }
        if (!(Geo::is_on_left(center, point2, point1) && Geo::is_on_left(center, point1, point0)))
        {
            return false;
        }
    }
    else
    {
        const Geo::Point vec0 = (point1 - point0).vertical().normalize() * radius0;
        const Geo::Point vec1 = (point2 - point1).vertical().normalize() * radius1;
        if (!Geo::is_intersected(point0 + vec0, point1 + vec0, point2 + vec1, point1 + vec1, center, true))
        {
            return false;
        }
        if (!(Geo::is_on_left(center, point1, point2) && Geo::is_on_left(center, point0, point1)))
        {
            return false;
        }
    }

    Geo::Point array0[3];
    if (!Geo::foot_point(point0, point1, center, array0[0]))
    {
        return false;
    }
    Geo::Point array1[3];
    if (!Geo::foot_point(point2, point1, center, array1[0]))
    {
        return false;
    }
    array0[2] = (array0[0] + point1) / 2;
    array0[1] = (array0[0] + array0[2]) / 2;
    array1[2] = (array1[0] + point1) / 2;
    array1[1] = (array1[0] + array1[2]) / 2;
    std::vector<Geo::Point> controls({array0[0], array0[1], array0[2]});
    controls.emplace_back((array0[2] + array1[2]) / 2);
    controls.emplace_back(array1[2]);
    controls.emplace_back(array1[1]);
    controls.emplace_back(array1[0]);

    arc = Geo::CubicBezier(controls.begin(), controls.end(), false);
    return true;
}

bool Geo::angle_to_arc(const Point &start, const Point &center, const Point &end, CubicBezier &arc)
{
    if (center == start || center == end || start == end)
    {
        return false;
    }

    const Geo::Point vec0((start - center).vertical()), vec1((center - end).vertical());
    if (Geo::Point anchor; Geo::is_intersected(start, start + vec0, end, end + vec1, anchor, true))
    {
        const double radius0 = Geo::distance(center, start), radius1 = Geo::distance(center, end);
        Geo::Point array0[3], array1[3];
        array0[0] = start, array1[0] = end;
        array0[2] = (array0[0] + anchor) / 2;
        if (Geo::distance(array0[0], array0[2]) > radius0 * 2.4)
        {
            array0[2] = array0[0] + (array0[2] - array0[0]).normalize() * radius0 * 2.4;
        }
        array0[1] = (array0[0] + array0[2]) / 2;
        array1[2] = (array1[0] + anchor) / 2;
        if (Geo::distance(array1[0], array1[2]) > radius1 * 2.4)
        {
            array1[2] = array1[0] + (array1[2] - array1[0]).normalize() * radius1 * 2.4;
        }
        array1[1] = (array1[0] + array1[2]) / 2;
        std::vector<Geo::Point> controls({array0[0], array0[1], array0[2]});
        controls.emplace_back((array0[2] + array1[2]) / 2);
        controls.emplace_back(array1[2]);
        controls.emplace_back(array1[1]);
        controls.emplace_back(array1[0]);

        arc = Geo::CubicBezier(controls.begin(), controls.end(), false);
        return true;
    }
    else
    {
        return false;
    }
}


Geo::Polyline Geo::arc_to_polyline(const Geo::Point &center, const double radius, double start_angle, double end_angle, const bool is_cw,
                                   const double down_sampling_value)
{
    const double v = std::asin(1 / radius);
    const double step = std::isnan(v) ? Geo::PI / 32 : std::min(v, Geo::PI / 32);
    std::vector<Geo::Point> points;
    if (is_cw)
    {
        if (start_angle < end_angle)
        {
            start_angle += Geo::PI * 2;
        }
        while (start_angle >= end_angle)
        {
            points.emplace_back(radius * std::cos(start_angle) + center.x, radius * std::sin(start_angle) + center.y);
            start_angle -= step;
        }
        points.emplace_back(radius * std::cos(end_angle) + center.x, radius * std::sin(end_angle) + center.y);
    }
    else
    {
        if (start_angle > end_angle)
        {
            end_angle += Geo::PI * 2;
        }
        while (start_angle <= end_angle)
        {
            points.emplace_back(radius * std::cos(start_angle) + center.x, radius * std::sin(start_angle) + center.y);
            start_angle += step;
        }
        points.emplace_back(radius * std::cos(end_angle) + center.x, radius * std::sin(end_angle) + center.y);
    }
    if (points.size() < 2)
    {
        return Geo::Polyline();
    }
    else
    {
        Geo::Polyline shape(points.begin(), points.end());
        if (step < Geo::PI / 32)
        {
            Geo::down_sampling(shape, down_sampling_value);
        }
        return shape;
    }
}

Geo::Polyline Geo::arc_to_polyline(const Geo::Arc &arc, const double down_sampling_value)
{
    const Geo::Point center(arc.x, arc.y);
    double angle0 = Geo::angle(center, arc.control_points[0]);
    double angle1 = Geo::angle(center, arc.control_points[2]);
    return arc_to_polyline(center, arc.radius, angle0, angle1, arc.is_cw(), down_sampling_value);
}

Geo::CubicBezier Geo::arc_to_bezier(const Geo::Arc &arc)
{
    std::vector<Geo::Point> points;
    points.emplace_back(arc.control_points[0]);
    if (const double angle = std::abs(arc.angle()); angle <= Geo::PI / 2)
    {
        const double k = 4.0 / 3.0 * (1 - std::cos(angle / 2)) / std::sin(angle / 2) * arc.radius;
        if (arc.is_cw())
        {
            Geo::Vector vec0(arc.control_points[0].x - arc.x, arc.control_points[0].y - arc.y);
            Geo::Vector vec1(arc.control_points[2].x - arc.x, arc.control_points[2].y - arc.y);
            vec0.rotate(0, 0, -Geo::PI / 2), vec1.rotate(0, 0, Geo::PI / 2);
            points.emplace_back(arc.control_points[0] + vec0.normalize() * k);
            points.emplace_back(arc.control_points[2] + vec1.normalize() * k);
        }
        else
        {
            Geo::Vector vec0(arc.control_points[0].x - arc.x, arc.control_points[0].y - arc.y);
            Geo::Vector vec1(arc.control_points[2].x - arc.x, arc.control_points[2].y - arc.y);
            vec0.rotate(0, 0, Geo::PI / 2), vec1.rotate(0, 0, -Geo::PI / 2);
            points.emplace_back(arc.control_points[0] + vec0.normalize() * k);
            points.emplace_back(arc.control_points[2] + vec1.normalize() * k);
        }
        points.emplace_back(arc.control_points[2]);
    }
    else
    {
        const int count = std::ceil(angle / (Geo::PI / 2));
        const double step = angle / count;
        const double k = 4.0 / 3.0 * (1 - std::cos(step / 2)) / std::sin(step / 2) * arc.radius;
        Geo::Point point = arc.control_points[0];
        Geo::Vector vec_start(point.x - arc.x, point.y - arc.y);
        for (int i = 0; i < count; ++i)
        {
            Geo::Vector vec0(point.x - arc.x, point.y - arc.y);
            vec0.rotate(0, 0, arc.is_cw() ? -Geo::PI / 2 : Geo::PI / 2);
            points.emplace_back(point + vec0.normalize() * k);
            vec_start.rotate(0, 0, arc.is_cw() ? -step : step);
            point.x = arc.x + vec_start.x, point.y = arc.y + vec_start.y;
            Geo::Vector vec1(point.x - arc.x, point.y - arc.y);
            vec1.rotate(0, 0, arc.is_cw() ? Geo::PI / 2 : -Geo::PI / 2);
            points.emplace_back(point + vec1.normalize() * k);
            points.emplace_back(point);
        }
    }
    return Geo::CubicBezier(points.begin(), points.end(), false);
}


Geo::Polygon Geo::circle_to_polygon(const double x, const double y, const double r, const double down_sampling_value)
{
    const double v = std::asin(1 / r);
    const double step = std::isnan(v) ? Geo::PI / 32 : std::max(std::min(v, Geo::PI / 64), Geo::PI / 4096);
    double degree = 0;
    std::vector<Geo::Point> points;
    while (degree < Geo::PI * 2)
    {
        points.emplace_back(r * std::cos(degree) + x, r * std::sin(degree) + y);
        degree += step;
    }
    if (points.size() >= 3)
    {
        Geo::Polygon shape(points.cbegin(), points.cend());
        if (step < Geo::PI / 32)
        {
            Geo::down_sampling(shape, down_sampling_value);
        }
        return shape;
    }
    else
    {
        return Geo::Polygon();
    }
}

Geo::Polygon Geo::circle_to_polygon(const Circle &circle, const double down_sampling_value)
{
    return Geo::circle_to_polygon(circle.x, circle.y, circle.radius, down_sampling_value);
}

Geo::CubicBezier Geo::circle_to_bezier(const Geo::Circle &circle)
{
    std::vector<Geo::Point> points;
    const double k = 4 * (-1 + std::sqrt(2)) / 3 * circle.radius;
    const Geo::Point point0(circle.x + circle.radius, circle.y), point1(circle.x, circle.y + circle.radius),
        point2(circle.x - circle.radius, circle.y), point3(circle.x, circle.y - circle.radius);
    // 第一象限
    points.emplace_back(point0);
    points.emplace_back(point0.x, point0.y + k);
    points.emplace_back(point1.x + k, point1.y);
    points.emplace_back(point1);
    // 第二象限
    points.emplace_back(point1.x - k, point1.y);
    points.emplace_back(point2.x, point2.y + k);
    points.emplace_back(point2);
    // 第三象限
    points.emplace_back(point2.x, point2.y - k);
    points.emplace_back(point3.x - k, point3.y);
    points.emplace_back(point3);
    // 第四象限
    points.emplace_back(point3.x + k, point3.y);
    points.emplace_back(point0.x, point0.y - k);
    points.emplace_back(point0);
    return Geo::CubicBezier(points.begin(), points.end(), false);
}


Geo::Polygon Geo::ellipse_to_polygon(const double x, const double y, const double a, const double b, const double rad,
                                     const double down_sampling_value)
{
    const double v = std::asin(1 / std::max(a, b));
    const double step = std::isnan(v) ? Geo::PI / 32 : std::min(v, Geo::PI / 32);
    double degree = 0;
    std::vector<Geo::Point> points;
    while (degree < Geo::PI * 2)
    {
        points.emplace_back(x + a * std::cos(rad) * std::cos(degree) - b * std::sin(rad) * std::sin(degree),
                            y + a * std::sin(rad) * std::cos(degree) + b * std::cos(rad) * std::sin(degree));
        degree += step;
    }
    if (points.size() >= 3)
    {
        Geo::Polygon shape(points.cbegin(), points.cend());
        if (step < Geo::PI / 32)
        {
            Geo::down_sampling(shape, down_sampling_value);
        }
        return shape;
    }
    else
    {
        return Geo::Polygon();
    }
}

Geo::Polygon Geo::ellipse_to_polygon(const Ellipse &ellipse, const double down_sampling_value)
{
    return Geo::ellipse_to_polygon(ellipse.center().x, ellipse.center().y, ellipse.lengtha(), ellipse.lengthb(), ellipse.angle(),
                                   down_sampling_value);
}

Geo::Polyline Geo::ellipse_to_polyline(const double x, const double y, const double a, const double b, const double rad,
                                       const double start_angle, double end_angle, const double down_sampling_value)
{
    const double v = std::asin(1 / std::max(a, b));
    const double step = std::isnan(v) ? Geo::PI / 32 : std::min(v, Geo::PI / 32);
    double degree = start_angle;
    if (end_angle < degree)
    {
        end_angle += Geo::PI * 2;
    }
    std::vector<Geo::Point> points;
    while (degree < end_angle)
    {
        points.emplace_back(x + a * std::cos(rad) * std::cos(degree) - b * std::sin(rad) * std::sin(degree),
                            y + a * std::sin(rad) * std::cos(degree) + b * std::cos(rad) * std::sin(degree));
        degree += step;
    }
    points.emplace_back(x + a * std::cos(rad) * std::cos(end_angle) - b * std::sin(rad) * std::sin(end_angle),
                        y + a * std::sin(rad) * std::cos(end_angle) + b * std::cos(rad) * std::sin(end_angle));
    if (points.size() >= 2)
    {
        Geo::Polyline shape(points.cbegin(), points.cend());
        if (step < Geo::PI / 32)
        {
            Geo::down_sampling(shape, down_sampling_value);
        }
        return shape;
    }
    else
    {
        return Geo::Polyline();
    }
}

Geo::Polyline Geo::ellipse_to_polyline(const Ellipse &ellipse, const double down_sampling_value)
{
    return Geo::ellipse_to_polyline(ellipse.center().x, ellipse.center().y, ellipse.lengtha(), ellipse.lengthb(), ellipse.angle(),
                                    ellipse.arc_param0(), ellipse.arc_param1(), down_sampling_value);
}

Geo::CubicBezier Geo::ellipse_to_bezier(const Geo::Ellipse &ellipse)
{
    std::vector<Geo::Point> points;
    {
        const double k0 = 4 * (-1 + std::sqrt(2)) / 3 * ellipse.lengtha();
        const double k1 = 4 * (-1 + std::sqrt(2)) / 3 * ellipse.lengthb();
        const Geo::Point center = ellipse.center();
        const Geo::Point point0(center.x + ellipse.lengtha(), center.y), point1(center.x, center.y + ellipse.lengthb()),
            point2(center.x - ellipse.lengtha(), center.y), point3(center.x, center.y - ellipse.lengthb());
        // 第一象限
        points.emplace_back(point0);
        points.emplace_back(point0.x, point0.y + k1);
        points.emplace_back(point1.x + k0, point1.y);
        points.emplace_back(point1);
        // 第二象限
        points.emplace_back(point1.x - k0, point1.y);
        points.emplace_back(point2.x, point2.y + k1);
        points.emplace_back(point2);
        // 第三象限
        points.emplace_back(point2.x, point2.y - k1);
        points.emplace_back(point3.x - k0, point3.y);
        points.emplace_back(point3);
        // 第四象限
        points.emplace_back(point3.x + k0, point3.y);
        points.emplace_back(point0.x, point0.y - k1);
        points.emplace_back(point0);

        if (const double angle = ellipse.angle(); angle != 0)
        {
            for (Geo::Point &point : points)
            {
                point.rotate(center.x, center.y, angle);
            }
        }
    }
    if (ellipse.is_arc())
    {
        Geo::CubicBezier bezier(points.begin(), points.end(), false);
        if (std::vector<Geo::Point> output; ellipse.arc_angle0() < ellipse.arc_angle1())
        {
            if (Geo::CubicBezier b0, b1;
                Geo::closest_point(bezier, ellipse.arc_point0(), output) && Geo::split(bezier, output.front(), b0, b1))
            {
                output.clear();
                if (Geo::CubicBezier b2, b3;
                    Geo::closest_point(b1, ellipse.arc_point1(), output) && Geo::split(b1, output.front(), b2, b3))
                {
                    points.assign(b2.begin(), b2.end());
                }
            }
        }
        else
        {
            if (Geo::CubicBezier b0, b1;
                Geo::closest_point(bezier, ellipse.arc_point0(), output) && Geo::split(bezier, output.front(), b0, b1))
            {
                output.clear();
                if (Geo::CubicBezier b2, b3;
                    Geo::closest_point(b0, ellipse.arc_point1(), output) && Geo::split(b0, output.front(), b2, b3))
                {
                    points.assign(b1.begin(), b1.end());
                    points.insert(points.end(), b2.begin(), b2.end());
                }
            }
        }
    }
    return Geo::CubicBezier(points.begin(), points.end(), false);
}


Geo::CubicBSpline Geo::bezier_to_bspline(const Geo::CubicBezier &bezier)
{
    std::vector<double> knots(4, 0);
    int value = 1;
    for (size_t i = 1, count = bezier.size() / 3; i < count; ++i, ++value)
    {
        knots.push_back(value);
        knots.push_back(value);
        knots.push_back(value);
    }
    knots.push_back(value);
    knots.push_back(value);
    knots.push_back(value);
    knots.push_back(value);
    Geo::CubicBSpline bspline(bezier.begin(), bezier.end(), knots, false);
    bspline.controls_model = true;
    return bspline;
}

Geo::CubicBezier *Geo::bspline_to_bezier(const Geo::BSpline &bspline)
{
    if (dynamic_cast<const Geo::CubicBSpline *>(&bspline) != nullptr)
    {
        Geo::CubicBSpline temp(*static_cast<const Geo::CubicBSpline *>(&bspline));
        for (size_t j = 0, i = temp.knots().size() - 1; i > 0; --i)
        {
            if (j = std::count(temp.knots().begin(), temp.knots().end(), temp.knots()[i]); j < 4)
            {
                const double t = temp.knots()[i];
                while (j++ <= 4)
                {
                    temp.insert(t);
                }
            }
        }
        return new Geo::CubicBezier(temp.control_points.begin(), temp.control_points.end(), false);
    }
    else
    {
        return nullptr;
    }
}


Geo::CubicBezier *Geo::blend(const Geo::Point &pre0, const Geo::Point &point0, const Geo::Point &point1, const Geo::Point &pre1)
{
    if (pre0 == point0 || point1 == pre1)
    {
        return nullptr;
    }

    if (const double dis = Geo::distance(point0, point1) / 3; dis > 0)
    {
        std::vector<Geo::Point> points(7);
        points[0] = point0, points[6] = point1;
        points[2] = point0 + (point0 - pre0).normalize() * dis;
        points[4] = point1 + (point1 - pre1).normalize() * dis;
        points[1] = (points[0] + points[2]) / 2;
        points[5] = (points[4] + points[6]) / 2;
        points[3] = (points[2] + points[4]) / 2;
        return new Geo::CubicBezier(points.begin(), points.end(), false);
    }
    else
    {
        return nullptr;
    }
}