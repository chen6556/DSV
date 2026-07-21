#include "base/Algorithm.hpp"
#include "base/Math.hpp"


bool Geo::fillet(const Geo::Polygon &polygon, const size_t index, const double radius, Geo::Arc &arc, Geo::Polyline &result)
{
    if (radius <= 0 || index >= polygon.size())
    {
        return false;
    }
    const size_t index0 = index > 0 ? index - 1 : polygon.size() - 2;
    const size_t index2 = index < polygon.size() - 1 ? index + 1 : 1;
    if (Geo::angle_to_arc(polygon[index0], polygon[index], polygon[index2], radius, arc))
    {
        result.clear();
        result.append(polygon.begin(), polygon.end() - 1);
        std::rotate(result.begin(), result.begin() + index, result.end());
        result.front() = arc.control_points[2];
        result.append(arc.control_points[0]);
        return true;
    }
    return false;
}

bool Geo::fillet(const Geo::Polyline &polyline, const size_t index, const double radius, Geo::Arc &arc, Geo::Polyline &result0,
                 Geo::Polyline &result1)
{
    if (index == 0 || index >= polyline.size() - 1 || radius <= 0)
    {
        return false;
    }
    if (Geo::angle_to_arc(polyline[index - 1], polyline[index], polyline[index + 1], radius, arc))
    {
        result0.clear();
        result0.append(polyline.begin(), polyline.begin() + index);
        result0.append(arc.control_points[0]);
        result1.clear();
        result1.append(polyline.begin() + index, polyline.end());
        result1.front() = arc.control_points[2];
        return true;
    }
    return false;
}

bool Geo::fillet(const Geo::Polygon &polygon, const size_t index, const double radius0, const double radius1, Geo::CubicBezier &arc,
                 Geo::Polyline &result)
{
    if (radius0 <= 0 || radius1 <= 0 || index >= polygon.size())
    {
        return false;
    }
    const size_t index0 = index > 0 ? index - 1 : polygon.size() - 2;
    const size_t index2 = index < polygon.size() - 1 ? index + 1 : 1;
    if (Geo::angle_to_arc(polygon[index0], polygon[index], polygon[index2], radius0, radius1, arc))
    {
        result.clear();
        result.append(polygon.begin(), polygon.end() - 1);
        std::rotate(result.begin(), result.begin() + index, result.end());
        result.front() = arc.back();
        result.append(arc.front());
        return true;
    }
    return false;
}

bool Geo::fillet(const Geo::Polyline &polyline, const size_t index, const double radius0, const double radius1, Geo::CubicBezier &arc,
                 Geo::Polyline &result0, Geo::Polyline &result1)
{
    if (radius0 <= 0 || radius1 <= 0 || index == 0 || index >= polyline.size() - 1)
    {
        return false;
    }
    if (Geo::angle_to_arc(polyline[index - 1], polyline[index], polyline[index + 1], radius0, radius1, arc))
    {
        result0.clear();
        result1.clear();
        result0.append(polyline.begin(), polyline.begin() + index);
        result0.append(arc.front());
        result1.append(polyline.begin() + index, polyline.end());
        result1.front() = arc.back();
        return true;
    }
    return false;
}

bool Geo::fillet(const Geo::Polyline &polyline0, const Geo::Point &point0, const Geo::Polyline &polyline1, const Geo::Point &point1,
                 const double radius, Geo::Arc &arc, Geo::Polyline &result0, Geo::Polyline &result1, Geo::Polyline &result2,
                 Geo::Polyline &result3)
{
    if (radius <= 0)
    {
        return false;
    }
    result0.clear(), result1.clear(), result2.clear(), result3.clear();
    Geo::Point head0, tail0, head1, tail1;
    if (Geo::distance(point0, polyline0.front(), polyline0[1], false) <
        Geo::distance(point0, polyline0[polyline0.size() - 2], polyline0.back(), false))
    {
        head0 = polyline0.front();
        tail0 = polyline0[1];
        result2.append(polyline0.begin() + 1, polyline0.end());
    }
    else
    {
        tail0 = polyline0.back();
        head0 = polyline0[polyline0.size() - 2];
        result2.append(polyline0.begin(), polyline0.end() - 1);
    }
    if (Geo::distance(point1, polyline1.front(), polyline1[1], false) <
        Geo::distance(point1, polyline1[polyline1.size() - 2], polyline1.back(), false))
    {
        head1 = polyline1.front();
        tail1 = polyline1[1];
        result3.append(polyline1.begin() + 1, polyline1.end());
    }
    else
    {
        tail1 = polyline1.back();
        head1 = polyline1[polyline1.size() - 2];
        result3.append(polyline1.begin(), polyline1.end() - 1);
    }
    if (Geo::Point intersection; Geo::is_intersected(head0, tail0, head1, tail1, intersection, true))
    {
        if ((point0 - intersection) * (head0 - intersection) < 0)
        {
            std::swap(head0, tail0);
        }
        if ((point1 - intersection) * (head1 - intersection) < 0)
        {
            std::swap(head1, tail1);
        }

        Geo::Point offset_head0, offset_tail0, offset_head1, offset_tail1, foot0, foot1;
        if (Geo::is_on_left(point1, head0, tail0))
        {
            const Geo::Vector vec((tail0 - head0).vertical().normalize() * radius);
            offset_head0 = head0 + vec;
            offset_tail0 = intersection + vec;
        }
        else
        {
            const Geo::Vector vec((head0 - tail0).vertical().normalize() * radius);
            offset_head0 = head0 + vec;
            offset_tail0 = intersection + vec;
        }
        if (Geo::is_on_left(point0, head1, tail1))
        {
            const Geo::Vector vec((tail1 - head1).vertical().normalize() * radius);
            offset_head1 = head1 + vec;
            offset_tail1 = intersection + vec;
        }
        else
        {
            const Geo::Vector vec((head1 - tail1).vertical().normalize() * radius);
            offset_head1 = head1 + vec;
            offset_tail1 = intersection + vec;
        }

        if (Geo::is_intersected(offset_head0, offset_tail0, offset_head1, offset_tail1, intersection, true) &&
            Geo::foot_point(head0, tail0, intersection, foot0, true) && Geo::foot_point(head1, tail1, intersection, foot1, true))
        {
            arc = Geo::Arc(foot0, intersection, foot1, true);
            if ((foot0 - head0) * arc.start_direction() < 0)
            {
                arc = Geo::Arc(foot0, intersection, foot1, false);
            }
            result0.append(head0);
            result0.append(foot0);
            result1.append(head1);
            result1.append(foot1);
            return true;
        }
    }
    return false;
}

bool Geo::fillet(const Geo::Polyline &polyline0, const Geo::Point &point0, const Geo::Arc &arc1, const Geo::Point &point1,
                 const double radius, Geo::Arc &arc, Geo::Polyline &result0, Geo::Polyline &result1, Geo::Arc &result2)
{
    if (radius <= 0)
    {
        return false;
    }
    result0.clear(), result1.clear();
    Geo::Point head0, tail0, head1, tail1, intersection0, intersection1;
    if (Geo::distance(point0, polyline0.front(), polyline0[1], false) <
        Geo::distance(point0, polyline0[polyline0.size() - 2], polyline0.back(), false))
    {
        head0 = polyline0.front();
        tail0 = polyline0[1];
        result1.append(polyline0.begin() + 1, polyline0.end());
    }
    else
    {
        head0 = polyline0.back();
        tail0 = polyline0[polyline0.size() - 2];
        result1.append(polyline0.begin(), polyline0.end() - 1);
    }
    if (Geo::distance_square(point0, tail0) < Geo::distance_square(point0, head0))
    {
        std::swap(head0, tail0);
    }
    const Geo::Point original_head(head0), original_tail(tail0);

    if (Geo::is_on_left(point1, head0, tail0))
    {
        const Geo::Vector vec((tail0 - head0).vertical().normalize() * radius);
        tail1 = tail0 + vec;
        head1 = head0 + vec;
    }
    else
    {
        const Geo::Vector vec((head0 - tail0).vertical().normalize() * radius);
        tail1 = tail0 + vec;
        head1 = head0 + vec;
    }
    Geo::Circle circle(arc1.x, arc1.y, arc1.radius);
    if (Geo::distance(point0.x, point0.y, arc1.x, arc1.y) < arc1.radius)
    {
        circle.radius -= radius;
        if (Geo::foot_point(original_head, original_tail, circle, intersection0, true))
        {
            head0 = intersection0;
        }
        else
        {
            head0 = circle;
        }
        if (Geo::foot_point(original_head, original_tail, point0, intersection1, true))
        {
            tail0 = intersection1;
        }
        else
        {
            tail0 = point0;
        }
    }
    else
    {
        circle.radius += radius;
    }

    switch (Geo::is_intersected(head1, tail1, circle, intersection0, intersection1, true))
    {
    case 2:
        {
            Geo::Point controls[2];
            if (Geo::distance_square(point0, intersection0) < Geo::distance_square(point0, intersection1))
            {
                controls[0] = circle + (intersection0 - circle).normalize() * arc1.radius;
                Geo::foot_point(original_head, original_tail, intersection0, controls[1], true);
                arc = Geo::Arc(controls[0], intersection0, controls[1], true);
                if (arc.end_direction() * (head0 - tail0) > 0)
                {
                }
                else
                {
                    arc = Geo::Arc(controls[0], intersection0, controls[1], false);
                }
            }
            else
            {
                controls[0] = circle + (intersection1 - circle).normalize() * arc1.radius;
                Geo::foot_point(original_head, original_tail, intersection1, controls[1], true);
                arc = Geo::Arc(controls[0], intersection1, controls[1], true);
                if (arc.end_direction() * (head0 - tail0) > 0)
                {
                }
                else
                {
                    arc = Geo::Arc(controls[0], intersection1, controls[1], false);
                }
            }
        }
        break;
    case 1:
        {
            Geo::Point controls[2];
            controls[0] = circle + (intersection0 - circle).normalize() * arc1.radius;
            Geo::foot_point(original_head, original_tail, intersection0, controls[1], true);
            arc = Geo::Arc(controls[0], intersection0, controls[1], true);
            if (arc.end_direction() * (head0 - tail0) > 0)
            {
            }
            else
            {
                arc = Geo::Arc(controls[0], intersection0, controls[1], false);
            }
        }
        break;
    default:
        return false;
    }

    if (arc1.is_cw())
    {
        const Geo::Point vec0(circle - arc.control_points[0]), vec1(arc.start_direction());
        if (Geo::Point(circle - arc.control_points[0]).vertical() * arc.start_direction() > 0)
        {
            result2 = Geo::Arc(arc1.control_points[0], circle, arc.control_points[0], false);
        }
        else
        {
            result2 = Geo::Arc(arc.control_points[0], circle, arc1.control_points[2], false);
        }
    }
    else
    {
        if ((arc.control_points[0] - circle).vertical() * arc.start_direction() > 0)
        {
            result2 = Geo::Arc(arc1.control_points[0], circle, arc.control_points[0], true);
        }
        else
        {
            result2 = Geo::Arc(arc.control_points[0], circle, arc1.control_points[2], true);
        }
    }

    result0.append(arc.control_points[2]);
    result0.append((original_tail - original_head) * (tail0 - head0) > 0 ? original_head : original_tail);

    return true;
}

bool Geo::fillet(const Geo::Polyline &polyline0, const Geo::Point &point0, const Geo::CubicBezier &bezier1, const Geo::Point &point1,
                 const double radius, Geo::Arc &arc, Geo::Polyline &result0, Geo::Polyline &result1, Geo::CubicBezier &result2)
{
    result0.clear(), result1.clear(), result2.clear();
    Geo::Point head0, tail0, head1, tail1;
    if (Geo::distance(point0, polyline0[0], polyline0[1], false) <
        Geo::distance(point0, polyline0.back(), polyline0[polyline0.size() - 2], false))
    {
        head0 = polyline0.front();
        tail0 = polyline0[1];
        result1.append(polyline0.begin() + 1, polyline0.end());
    }
    else
    {
        tail0 = polyline0.back();
        head0 = polyline0[polyline0.size() - 2];
        result1.append(polyline0.begin(), polyline0.end() - 1);
    }

    if (Geo::is_on_left(point1, head0, tail0))
    {
        const Geo::Vector vec((tail0 - head0).vertical().normalize() * radius);
        head1 = head0 + vec;
        tail1 = tail0 + vec;
    }
    else
    {
        const Geo::Vector vec((head0 - tail0).vertical().normalize() * radius);
        head1 = head0 + vec;
        tail1 = tail0 + vec;
    }

    double bezier_offset_distance = -radius;
    {
        // 找出贝塞尔曲线偏移方向
        std::vector<std::tuple<size_t, double, double, double>> values;
        if (std::vector<Geo::Point> points; Geo::closest_point(bezier1, point0, points, &values))
        {
            const Geo::Point vec(bezier1.vertical(std::get<0>(values.front()), std::get<1>(values.front())));
            if (vec * (point0 - points.front()) > 0)
            {
                bezier_offset_distance = radius;
            }
        }
        else
        {
            return false;
        }
    }

    size_t mid_i = 0;
    double mid_t = 0;
    {
        std::vector<Geo::CubicBezier> offseted_beziers;
        if (!Geo::offset(bezier1, offseted_beziers, bezier_offset_distance, 0.001, 60))
        {
            return false;
        }
        double min_distance = DBL_MAX, min_x = 0, min_y = 0; // 找出与参考点最近的圆心
        for (size_t i = 0, count = offseted_beziers.size(); i < count; ++i)
        {
            if (std::min(Geo::distance(point0, offseted_beziers[i].shape()), Geo::distance(point1, offseted_beziers[i].shape())) >
                min_distance)
            {
                continue;
            }
            std::vector<Geo::Point> points;
            std::vector<std::tuple<size_t, double, double, double>> values;
            Geo::is_intersected(head1, tail1, offseted_beziers[i], points, true, &values);
            for (const auto [j, t, x, y] : values)
            {
                if (const double dis = Geo::distance(x, y, point0.x, point0.y); dis < min_distance)
                {
                    min_distance = dis;
                    min_x = x;
                    min_y = y;
                }
                if (const double dis = Geo::distance(x, y, point1.x, point1.y); dis < min_distance)
                {
                    min_distance = dis;
                    min_x = x;
                    min_y = y;
                }
            }
        }
        std::vector<Geo::Point> points;
        std::vector<std::tuple<size_t, double, double, double>> values;
        if (min_distance < DBL_MAX && Geo::closest_point(bezier1, Geo::Point(min_x, min_y), points, &values))
        {
            const auto [i, t, x, y] = values.front();
            mid_i = i;
            mid_t = t;
            const Geo::Point vec(bezier1.vertical(mid_i, mid_t).normalize() * radius);
            const Geo::Point center(x + vec.x, y + vec.y);
        }
        else
        {
            return false;
        }
    }
    {
        double left_t = mid_t - 1e-5, right_t = mid_t + 1e-5;
        const std::function<double(const double)> f = [&](const double t)
        {
            const Geo::Point anchor(bezier1.shape_point(mid_i, t));
            const Geo::Point center(anchor + bezier1.vertical(mid_i, t).normalize() * bezier_offset_distance);
            return std::abs(Geo::distance(center, head0, tail0, true) - radius);
        };
        double mid_err = f(mid_t);
        double left_err = f(left_t);
        double right_err = f(right_t);
        while (left_err < mid_err && left_t > 0)
        {
            left_t -= 1e-5;
            left_err = f(left_t);
        }
        while (right_err < mid_err && right_t < 1)
        {
            right_t += 1e-5;
            right_err = f(right_t);
        }
        left_t = std::max(0.0, left_t), right_t = std::min(1.0, right_t);
        mid_t = Math::min_x_trichotomy(f, left_t, right_t);
    }

    Geo::Point controls[2] = {bezier1.shape_point(mid_i, mid_t), Geo::Point()};
    const Geo::Point center(controls[0] + bezier1.vertical(mid_i, mid_t).normalize() * bezier_offset_distance);
    if (!Geo::foot_point(head0, tail0, center, controls[1], true))
    {
        return false;
    }

    Geo::Point intersection;
    double intersection_t = -1;
    {
        std::vector<Geo::Point> points;
        std::vector<std::tuple<size_t, double, double, double>> values;
        if (Geo::is_intersected(head0, tail0, bezier1, points, true, &values))
        {
            intersection_t = std::get<1>(values[0]);
            intersection.x = std::get<2>(values[0]);
            intersection.y = std::get<3>(values[0]);
            double min_dis = std::min(Geo::distance(point0, intersection), Geo::distance(point1, intersection));
            for (size_t j = 1, count = values.size(); j < count; ++j)
            {
                const auto [i, t, x, y] = values[j];
                const double dis = std::min(Geo::distance(point0.x, point0.y, x, y), Geo::distance(point1.x, point1.y, x, y));
                if (dis < min_dis)
                {
                    min_dis = dis;
                    intersection_t = t;
                    intersection.x = x;
                    intersection.y = y;
                }
            }
        }
    }

    if (intersection_t >= 0 && (point0 - intersection) * (head0 - intersection) < 0)
    {
        std::swap(head0, tail0);
    }
    else if (intersection_t < 0 && Geo::distance_square(point0, head0) < Geo::distance_square(point0, tail0))
    {
        std::swap(head0, tail0);
    }
    result0.append(head0);
    result0.append(controls[1]);
    arc = Geo::Arc(controls[0], center, controls[1], true);
    if (arc.end_direction() * (head0 - tail0) < 0)
    {
        arc = Geo::Arc(controls[0], center, controls[1], false);
    }

    if (Geo::CubicBezier temp_bezier; Geo::split(bezier1, mid_i, mid_t, result2, temp_bezier))
    {
        if ((controls[0] == result2.front() && arc.start_direction() * (result2.control_points[0] - result2.control_points[1]) < 0) ||
            (controls[0] == result2.back() &&
             arc.start_direction() * (result2.back() - result2.control_points[result2.control_points.size() - 2]) < 0))
        {
            result2 = temp_bezier;
        }
    }
    else
    {
        result2 = bezier1;
    }
    return true;
}

bool Geo::fillet(const Geo::Polyline &polyline0, const Geo::Point &point0, const Geo::BSpline &bspline1, const Geo::Point &point1,
                 const bool is_cubic, const double radius, Geo::Arc &arc, Geo::Polyline &result0, Geo::Polyline &result1,
                 Geo::BSpline &result2)
{
    if (is_cubic &&
        (dynamic_cast<const Geo::CubicBSpline *>(&bspline1) == nullptr || dynamic_cast<Geo::CubicBSpline *>(&result2) == nullptr))
    {
        return false;
    }
    else if (!is_cubic &&
             (dynamic_cast<const Geo::QuadBSpline *>(&bspline1) == nullptr || dynamic_cast<Geo::QuadBSpline *>(&result2) == nullptr))
    {
        return false;
    }

    Geo::Point bezier_front, bezier_back;
    if (Geo::CubicBezier bezier0(Geo::bspline_to_bezier(bspline1)), bezier1;
        Geo::fillet(polyline0, point0, bezier0, point1, radius, arc, result0, result1, bezier1))
    {
        bezier_front = bezier1.front();
        bezier_back = bezier1.back();
    }
    else
    {
        return false;
    }

    std::vector<Geo::Point> points;
    std::vector<std::tuple<double, double, double>> values;
    if (!Geo::closest_point(bspline1, is_cubic, arc.control_points[0], points, &values))
    {
        return false;
    }

    Geo::Point head0, tail0;
    if (Geo::distance(point0, polyline0[0], polyline0[1], false) <
        Geo::distance(point0, polyline0.back(), polyline0[polyline0.size() - 2], false))
    {
        head0 = polyline0.front();
        tail0 = polyline0[1];
        result1.append(polyline0.begin() + 1, polyline0.end());
    }
    else
    {
        tail0 = polyline0.back();
        head0 = polyline0[polyline0.size() - 2];
        result1.append(polyline0.begin(), polyline0.end() - 1);
    }

    double mid_t = std::get<0>(values.front()), offset_distance = radius;
    {
        const Geo::Point anchor(bspline1.at(mid_t)), vec(bspline1.vertical(mid_t).normalize());
        const Geo::Point center0(anchor + vec * radius), center1(anchor - vec * radius);
        if (Geo::distance_square(center0, Geo::Point(arc.x, arc.y)) > Geo::distance_square(center1, Geo::Point(arc.x, arc.y)))
        {
            offset_distance = -radius;
        }
    }
    {
        double left_t = mid_t - 1e-5, right_t = mid_t + 1e-5;
        const std::function<double(const double)> f = [&](const double t)
        {
            const Geo::Point anchor(bspline1.at(t));
            const Geo::Point center(anchor + bspline1.vertical(mid_t).normalize() * offset_distance);
            return std::abs(Geo::distance(center, head0, tail0, true) - radius);
        };
        double mid_err = f(mid_t);
        double left_err = f(left_t);
        double right_err = f(right_t);
        while (left_err < mid_err && left_t > 0)
        {
            left_t -= 1e-5;
            left_err = f(left_t);
        }
        while (right_err < mid_err && right_t < 1)
        {
            right_t += 1e-5;
            right_err = f(right_t);
        }
        left_t = std::max(0.0, left_t), right_t = std::min(1.0, right_t);
        mid_t = Math::min_x_trichotomy(f, left_t, right_t);
    }

    Geo::Point controls[2] = {bspline1.at(mid_t), Geo::Point()};
    const Geo::Point center(controls[0] + bspline1.vertical(mid_t).normalize() * offset_distance);
    if (!Geo::foot_point(head0, tail0, center, controls[1], true))
    {
        return false;
    }
    result0.back() = controls[1];
    arc = Geo::Arc(controls[0], center, controls[1], !arc.is_cw());

    if (is_cubic)
    {
        if (Geo::CubicBSpline temp; Geo::split(bspline1, true, mid_t, result2, temp))
        {
            const double dis0 = std::min(Geo::distance(result0.front(), bezier_front), Geo::distance(result0.front(), bezier_back)) +
                                std::min(Geo::distance(result0.back(), bezier_front), Geo::distance(result0.back(), bezier_back));
            const double dis1 = std::min(Geo::distance(temp.front(), bezier_front), Geo::distance(temp.front(), bezier_back)) +
                                std::min(Geo::distance(temp.back(), bezier_front), Geo::distance(temp.back(), bezier_back));
            if (dis0 > dis1)
            {
                result2 = temp;
            }
        }
        else
        {
            result2 = bspline1;
        }
    }
    else
    {
        if (Geo::QuadBSpline temp; Geo::split(bspline1, false, mid_t, result2, temp))
        {
            const double dis0 = std::min(Geo::distance(result0.front(), bezier_front), Geo::distance(result0.front(), bezier_back)) +
                                std::min(Geo::distance(result0.back(), bezier_front), Geo::distance(result0.back(), bezier_back));
            const double dis1 = std::min(Geo::distance(temp.front(), bezier_front), Geo::distance(temp.front(), bezier_back)) +
                                std::min(Geo::distance(temp.back(), bezier_front), Geo::distance(temp.back(), bezier_back));
            if (dis0 > dis1)
            {
                result2 = temp;
            }
        }
        else
        {
            result2 = bspline1;
        }
    }
    return true;
}

bool Geo::fillet(const Geo::Polyline &polyline0, const Geo::Point &point0, const Geo::Polyline &polyline1, const Geo::Point &point1,
                 const double radius0, const double radius1, Geo::CubicBezier &arc, Geo::Polyline &result0, Geo::Polyline &result1,
                 Geo::Polyline &result2, Geo::Polyline &result3)
{
    result0.clear(), result1.clear(), result2.clear(), result3.clear();
    Geo::Point head0, tail0, head1, tail1;
    if (Geo::distance(point0, polyline0.front(), polyline0[1], false) <
        Geo::distance(point0, polyline0[polyline0.size() - 2], polyline0.back(), false))
    {
        head0 = polyline0.front();
        tail0 = polyline0[1];
        result2.append(polyline0.begin() + 1, polyline0.end());
    }
    else
    {
        tail0 = polyline0.back();
        head0 = polyline0[polyline0.size() - 2];
        result2.append(polyline0.begin(), polyline0.end() - 1);
    }
    if (Geo::distance(point1, polyline1.front(), polyline1[1], false) <
        Geo::distance(point1, polyline1[polyline1.size() - 2], polyline1.back(), false))
    {
        head1 = polyline1.front();
        tail1 = polyline1[1];
        result3.append(polyline1.begin() + 1, polyline1.end());
    }
    else
    {
        tail1 = polyline1.back();
        head1 = polyline1[polyline1.size() - 2];
        result3.append(polyline1.begin(), polyline1.end() - 1);
    }
    if (Geo::Point center; Geo::is_intersected(head0, tail0, head1, tail1, center, true))
    {
        bool success = false;
        if ((head0 - center) * (tail0 - center) < 0 && (head1 - center) * (tail1 - center) < 0)
        {
            if ((head0 - center) * (point0 - center) > 0)
            {
                if ((head1 - center) * (point1 - center) > 0)
                {
                    if (Geo::angle_to_arc(head0, center, head1, radius0, radius1, arc))
                    {
                        result0.append(head0);
                        result0.append(arc.front());
                        result1.append(head1);
                        result1.append(arc.back());
                        success = true;
                    }
                }
                else
                {
                    if (Geo::angle_to_arc(head0, center, tail1, radius0, radius1, arc))
                    {
                        result0.append(head0);
                        result0.append(arc.front());
                        result1.append(tail1);
                        result1.append(arc.back());
                        success = true;
                    }
                }
            }
            else
            {
                if ((head1 - center) * (point1 - center) > 0)
                {
                    if (Geo::angle_to_arc(tail0, center, head1, radius0, radius1, arc))
                    {
                        result0.append(tail0);
                        result0.append(arc.front());
                        result1.append(head1);
                        result1.append(arc.back());
                        success = true;
                    }
                }
                else
                {
                    if (Geo::angle_to_arc(tail0, center, tail1, radius0, radius1, arc))
                    {
                        result0.append(tail0);
                        result0.append(arc.front());
                        result1.append(tail1);
                        result1.append(arc.back());
                        success = true;
                    }
                }
            }
        }
        else if ((head0 - center) * (tail0 - center) < 0)
        {
            if ((head0 - center) * (point0 - center) > 0)
            {
                if (Geo::distance_square(center, head1) <= Geo::distance_square(center, tail1))
                {
                    if (Geo::angle_to_arc(head0, center, tail1, radius0, radius1, arc))
                    {
                        result0.append(head0);
                        result0.append(arc.front());
                        result1.append(tail1);
                        result1.append(arc.back());
                        success = true;
                    }
                }
                else
                {
                    if (Geo::angle_to_arc(head0, center, head1, radius0, radius1, arc))
                    {
                        result0.append(head0);
                        result0.append(arc.front());
                        result1.append(head1);
                        result1.append(arc.back());
                        success = true;
                    }
                }
            }
            else
            {
                if (Geo::distance_square(center, head1) <= Geo::distance_square(center, tail1))
                {
                    if (Geo::angle_to_arc(tail0, center, tail1, radius0, radius1, arc))
                    {
                        result0.append(tail0);
                        result0.append(arc.front());
                        result1.append(tail1);
                        result1.append(arc.back());
                        success = true;
                    }
                }
                else
                {
                    if (Geo::angle_to_arc(tail0, center, head1, radius0, radius1, arc))
                    {
                        result0.append(tail0);
                        result0.append(arc.front());
                        result1.append(head1);
                        result1.append(arc.back());
                        success = true;
                    }
                }
            }
        }
        else if ((head1 - center) * (tail1 - center) < 0)
        {
            if ((head1 - center) * (point1 - center) > 0)
            {
                if (Geo::distance_square(center, head0) <= Geo::distance_square(center, tail0))
                {
                    if (Geo::angle_to_arc(tail0, center, head1, radius0, radius1, arc))
                    {
                        result0.append(tail0);
                        result0.append(arc.front());
                        result1.append(head1);
                        result1.append(arc.back());
                        success = true;
                    }
                }
                else
                {
                    if (Geo::angle_to_arc(head0, center, head1, radius0, radius1, arc))
                    {
                        result0.append(head0);
                        result0.append(arc.front());
                        result1.append(head1);
                        result1.append(arc.back());
                        success = true;
                    }
                }
            }
            else
            {
                if (Geo::distance_square(center, head0) <= Geo::distance_square(center, tail0))
                {
                    if (Geo::angle_to_arc(tail0, center, tail1, radius0, radius1, arc))
                    {
                        result0.append(tail0);
                        result0.append(arc.front());
                        result1.append(tail1);
                        result1.append(arc.back());
                        success = true;
                    }
                }
                else
                {
                    if (Geo::angle_to_arc(head0, center, tail1, radius0, radius1, arc))
                    {
                        result0.append(head0);
                        result0.append(arc.front());
                        result1.append(tail1);
                        result1.append(arc.back());
                        success = true;
                    }
                }
            }
        }
        else
        {
            if (Geo::distance_square(center, head0) <= Geo::distance_square(center, tail0))
            {
                if (Geo::distance_square(center, head1) <= Geo::distance_square(center, tail1))
                {
                    if (Geo::angle_to_arc(tail0, center, tail1, radius0, radius1, arc))
                    {
                        result0.append(tail0);
                        result0.append(arc.front());
                        result1.append(tail1);
                        result1.append(arc.back());
                        success = true;
                    }
                }
                else
                {
                    if (Geo::angle_to_arc(tail0, center, head1, radius0, radius1, arc))
                    {
                        result0.append(tail0);
                        result0.append(arc.front());
                        result1.append(head1);
                        result1.append(arc.back());
                        success = true;
                    }
                }
            }
            else
            {
                if (Geo::distance_square(center, head1) <= Geo::distance_square(center, tail1))
                {
                    if (Geo::angle_to_arc(head0, center, tail1, radius0, radius1, arc))
                    {
                        result0.append(head0);
                        result0.append(arc.front());
                        result1.append(tail1);
                        result1.append(arc.back());
                        success = true;
                    }
                }
                else
                {
                    if (Geo::angle_to_arc(head0, center, head1, radius0, radius1, arc))
                    {
                        result0.append(head0);
                        result0.append(arc.front());
                        result1.append(head1);
                        result1.append(arc.back());
                        success = true;
                    }
                }
            }
        }
        return success;
    }
    return false;
}

bool Geo::fillet(const Geo::CubicBezier &bezier0, const Geo::Point &point0, const Geo::CubicBezier &bezier1, const Geo::Point &point1,
                 const double radius, Geo::Arc &arc, Geo::CubicBezier &result0, Geo::CubicBezier &result1)
{
    double offset_distance0 = -radius, offset_distance1 = -radius;
    {
        std::vector<std::tuple<size_t, double, double, double>> values;
        if (std::vector<Geo::Point> points; Geo::closest_point(bezier0, point1, points, &values))
        {
            const Geo::Point vec(bezier0.vertical(std::get<0>(values.front()), std::get<1>(values.front())));
            if (vec * (point1 - points.front()) > 0)
            {
                offset_distance0 = radius;
            }
        }
        else
        {
            return false;
        }
        values.clear();
        if (std::vector<Geo::Point> points; Geo::closest_point(bezier1, point0, points, &values))
        {
            const Geo::Point vec(bezier1.vertical(std::get<0>(values.front()), std::get<1>(values.front())));
            if (vec * (point0 - points.front()) > 0)
            {
                offset_distance1 = radius;
            }
        }
        else
        {
            return false;
        }
    }

    Geo::Point init_center;
    {
        std::vector<Geo::CubicBezier> offseted0, offseted1;
        if (!Geo::offset(bezier0, offseted0, offset_distance0, 0.001, 60) || !Geo::offset(bezier1, offseted1, offset_distance1, 0.001, 60))
        {
            return false;
        }
        std::vector<Geo::Point> points;
        for (size_t i = 0, count0 = offseted0.size(); i < count0; ++i)
        {
            for (size_t j = 0, count1 = offseted1.size(); j < count1; ++j)
            {
                if (Geo::is_intersected(offseted0[i].aabbrect(), offseted1[j].aabbrect()))
                {
                    Geo::is_intersected(offseted0[i], offseted1[j], points);
                }
            }
        }
        if (points.empty())
        {
            return false;
        }
        double min_dis = DBL_MAX;
        for (const Geo::Point &point : points)
        {
            if (const double dis = std::min(Geo::distance(point, point0), Geo::distance(point, point1)); dis < min_dis)
            {
                min_dis = dis;
                init_center = point;
            }
        }
    }

    size_t index0 = 0, index1 = 0;
    double t0 = 0, t1 = 0;
    {
        std::vector<std::tuple<size_t, double, double, double>> values0, values1;
        if (std::vector<Geo::Point> points; Geo::closest_point(bezier0, init_center, points, &values0))
        {
            index0 = std::get<0>(values0.front());
            t0 = std::get<1>(values0.front());
        }
        if (std::vector<Geo::Point> points; Geo::closest_point(bezier1, init_center, points, &values1))
        {
            index1 = std::get<0>(values1.front());
            t1 = std::get<1>(values1.front());
        }
    }

    {
        double left_t0 = t0 - 1e-5, right_t0 = t0 + 1e-5;
        const std::function<double(const double)> f = [&](const double t)
        {
            const Geo::Point anchor(bezier0.shape_point(index0, t));
            const Geo::Point center(anchor + bezier0.vertical(index0, t).normalize() * offset_distance0);
            return std::abs(Geo::distance(center, bezier1) - radius);
        };
        double mid_err = f(t0);
        double left_err = f(left_t0);
        double right_err = f(right_t0);
        while (left_err < mid_err && left_t0 > 0)
        {
            left_t0 -= 1e-5;
            left_err = f(left_t0);
        }
        while (right_err < mid_err && right_t0 < 1)
        {
            right_t0 += 1e-5;
            right_err = f(right_t0);
        }
        left_t0 = std::max(0.0, left_t0), right_t0 = std::min(1.0, right_t0);
        t0 = Math::min_x_trichotomy(f, left_t0, right_t0);
    }
    {
        double left_t1 = t1 - 1e-5, right_t1 = t1 + 1e-5;
        const std::function<double(const double)> f = [&](const double t)
        {
            const Geo::Point anchor(bezier1.shape_point(index1, t));
            const Geo::Point center(anchor + bezier1.vertical(index1, t).normalize() * offset_distance1);
            return std::abs(Geo::distance(center, bezier0) - radius);
        };
        double mid_err = f(t1);
        double left_err = f(left_t1);
        double right_err = f(right_t1);
        while (left_err < mid_err && left_t1 > 0)
        {
            left_t1 -= 1e-5;
            left_err = f(left_t1);
        }
        while (right_err < mid_err && right_t1 < 1)
        {
            right_t1 += 1e-5;
            right_err = f(right_t1);
        }
        left_t1 = std::max(0.0, left_t1), right_t1 = std::min(1.0, right_t1);
        t1 = Math::min_x_trichotomy(f, left_t1, right_t1);
    }

    Geo::Point intersection;
    size_t intersection_index0 = 0, intersection_index1 = 0;
    double intersection_t0 = -1, intersection_t1 = -1;
    {
        std::vector<std::tuple<size_t, double, double, double>> values0, values1;
        if (std::vector<Geo::Point> points; Geo::is_intersected(bezier0, bezier1, points, &values0, &values1) > 0)
        {
            Geo::Point intersection0, intersection1;
            size_t min_item0 = 0, min_item1 = 0;
            double min_distance0 = DBL_MAX, min_distance1 = DBL_MAX;
            for (size_t i = 0, count = values0.size(); i < count; ++i)
            {
                {
                    const auto [index, t, x, y] = values0[i];
                    if (const double distance = Geo::distance(x, y, point0.x, point0.y); distance < min_distance0)
                    {
                        min_distance0 = distance;
                        intersection_index0 = index;
                        intersection_t0 = t;
                        intersection0.x = x;
                        intersection0.y = y;
                        min_item0 = i;
                    }
                    {
                        const auto [index, t, x, y] = values1[i];
                        if (const double distance = Geo::distance(x, y, point1.x, point1.y); distance < min_distance1)
                        {
                            min_distance1 = distance;
                            intersection_index1 = index;
                            intersection_t1 = t;
                            intersection1.x = x;
                            intersection1.y = y;
                            min_item1 = i;
                        }
                    }
                }
            }

            if (min_distance0 < min_distance1)
            {
                min_item1 = min_item0;
                auto [index, t, x, y] = values1[min_item1];
                intersection_index1 = index;
                intersection_t1 = t;
                intersection0.x = x;
                intersection0.y = y;
            }
            else
            {
                min_item0 = min_item1;
                auto [index, t, x, y] = values0[min_item0];
                intersection_index0 = index;
                intersection_t0 = t;
                intersection0.x = x;
                intersection0.y = y;
            }
        }
    }

    size_t point0_index = 0, point1_index = 0;
    double point0_t = 0, point1_t = 0;
    {
        std::vector<std::tuple<size_t, double, double, double>> values0, values1;
        if (std::vector<Geo::Point> points; Geo::closest_point(bezier0, point0, points, &values0))
        {
            point0_index = std::get<0>(values0.front());
            point0_t = std::get<1>(values0.front());
        }
        if (std::vector<Geo::Point> points; Geo::closest_point(bezier1, point1, points, &values1))
        {
            point1_index = std::get<0>(values1.front());
            point1_t = std::get<1>(values1.front());
        }
    }

    if (Geo::CubicBezier part0, part1; Geo::split(bezier0, index0, t0, part0, part1))
    {
        if (intersection_t0 < 0)
        {
            result0 = Geo::distance(point0, part0) < Geo::distance(point0, part1) ? part0 : part1;
        }
        else
        {
            if (point0_index > intersection_index0 || (point0_index == intersection_index0 && point0_t > intersection_t0))
            {
                result0 = part1;
            }
            else
            {
                result0 = part0;
            }
        }
    }
    else
    {
        result0 = bezier0;
    }

    const Geo::Point controls[2] = {bezier0.shape_point(index0, t0), bezier1.shape_point(index1, t1)};
    const Geo::Point center0(controls[0] + bezier0.vertical(index0, t0).normalize() * offset_distance0);
    const Geo::Point center1(controls[1] + bezier1.vertical(index1, t1).normalize() * offset_distance1);
    init_center = (center0 + center1) / 2;
    arc = Geo::Arc(controls[0], init_center, controls[1], true);
    if (Geo::distance(arc.control_points[0], result0.front()) < Geo::distance(arc.control_points[0], result0.back()) &&
        arc.start_direction() * (result0.control_points[0] - result0.control_points[1]) < 0)
    {
        arc = Geo::Arc(controls[0], init_center, controls[1], false);
    }
    else if (Geo::distance(arc.control_points[0], result0.back()) < Geo::distance(arc.control_points[0], result0.front()) &&
             arc.start_direction() * (result0.back() - result0.control_points[result0.control_points.size() - 2]) < 0)
    {
        arc = Geo::Arc(controls[0], init_center, controls[1], false);
    }

    if (Geo::CubicBezier part0, part1; Geo::split(bezier1, index1, t1, part0, part1))
    {
        if (intersection_t1 < 0)
        {
            if (Geo::distance(part0.front(), arc.control_points[2]) < Geo::distance(part0.back(), arc.control_points[2]) &&
                (part0.control_points[1] - part0.control_points[0]) * arc.start_direction() > 0)
            {
                result1 = part0;
            }
            else if (Geo::distance(part1.front(), arc.control_points[2]) < Geo::distance(part1.back(), arc.control_points[2]) &&
                     (part1.control_points[1] - part1.control_points[0]) * arc.start_direction() > 0)
            {
                result1 = part1;
            }
            else if (Geo::distance(part0.back(), arc.control_points[2]) < Geo::distance(part0.front(), arc.control_points[2]) &&
                     (part0.back() - part0.front()) * arc.start_direction() > 0)
            {
                result1 = part0;
            }
            else
            {
                result1 = part1;
            }
        }
        else
        {
            if (point1_index > intersection_index1 || (point1_index == intersection_index1 && point1_t > intersection_t1))
            {
                result1 = part1;
            }
            else
            {
                result1 = part0;
            }
        }
    }
    else
    {
        result1 = bezier1;
    }

    return true;
}

bool Geo::fillet(const Geo::Arc &arc0, const Geo::Point &point0, const Geo::Arc &arc1, const Geo::Point &point1, const double radius,
                 Geo::Arc &arc, Geo::Arc &result0, Geo::Arc &result1)
{
    if (Geo::distance(arc0.x, arc0.y, arc1.x, arc1.y) > arc0.radius + arc1.radius) // 两圆相离
    {
        const Geo::Circle circle0(arc0.x, arc0.y, arc0.radius + radius), circle1(arc1.x, arc1.y, arc1.radius + radius);
        Geo::Point controls[2], center;
        switch (Geo::is_intersected(circle0, circle1, controls[0], controls[1]))
        {
        case 2:
            if (std::min(Geo::distance(point0, controls[0]), Geo::distance(point1, controls[0])) <
                std::min(Geo::distance(point0, controls[1]), Geo::distance(point1, controls[1])))
            {
                center = controls[0];
            }
            else
            {
                center = controls[1];
            }
            break;
        case 1:
            center = controls[0];
            break;
        default:
            return false;
        }
        controls[0] = center + Geo::Point(circle0 - center).normalize() * radius;
        controls[1] = center + Geo::Point(circle1 - center).normalize() * radius;
        arc = Geo::Arc(controls[0], center, controls[1], true);

        if (Geo::Arc temp0, temp1; Geo::split(arc0, arc.control_points[0], temp0, temp1))
        {
            const Geo::Vector vec(circle1 - circle0);
            if (Geo::distance(temp0.control_points[0], arc.control_points[0]) <
                Geo::distance(temp0.control_points[2], arc.control_points[0]))
            {
                result0 = temp0.start_direction() * vec < 0 ? temp0 : temp1;
            }
            else
            {
                result0 = temp0.end_direction() * vec > 0 ? temp0 : temp1;
            }
        }
        else
        {
            temp0 = Geo::Arc(arc0.control_points[0], arc0.control_points[1], arc.control_points[0]);
            temp1 = Geo::Arc(arc.control_points[0], arc0.control_points[1], arc0.control_points[2]);
            const Geo::Vector vec(circle1 - circle0);
            if (Geo::distance(temp0.control_points[0], arc.control_points[0]) <
                Geo::distance(temp0.control_points[2], arc.control_points[0]))
            {
                result0 = temp0.start_direction() * vec < 0 ? temp0 : temp1;
            }
            else
            {
                result0 = temp0.end_direction() * vec > 0 ? temp0 : temp1;
            }
        }
        if (Geo::Arc temp0, temp1; Geo::split(arc1, arc.control_points[2], temp0, temp1))
        {
            const Geo::Vector vec(circle0 - circle1);
            if (Geo::distance(temp0.control_points[0], arc.control_points[2]) <
                Geo::distance(temp0.control_points[2], arc.control_points[2]))
            {
                result1 = temp0.start_direction() * vec < 0 ? temp0 : temp1;
            }
            else
            {
                result1 = temp0.end_direction() * vec > 0 ? temp0 : temp1;
            }
        }
        else
        {
            temp0 = Geo::Arc(arc1.control_points[0], arc1.control_points[1], arc.control_points[2]);
            temp1 = Geo::Arc(arc.control_points[2], arc1.control_points[1], arc1.control_points[2]);
            const Geo::Vector vec(circle0 - circle1);
            if (Geo::distance(temp0.control_points[0], arc.control_points[2]) <
                Geo::distance(temp0.control_points[2], arc.control_points[2]))
            {
                result1 = temp0.start_direction() * vec < 0 ? temp0 : temp1;
            }
            else
            {
                result1 = temp0.end_direction() * vec > 0 ? temp0 : temp1;
            }
        }

        if (Geo::distance(arc.control_points[0], result0.control_points[0]) <
            Geo::distance(arc.control_points[0], result0.control_points[2]))
        {
            if (result0.start_direction() * arc.start_direction() > 0)
            {
                arc = Geo::Arc(controls[0], center, controls[1], false);
            }
        }
        else
        {
            if (result0.end_direction() * arc.start_direction() < 0)
            {
                arc = Geo::Arc(controls[0], center, controls[1], false);
            }
        }
    }
    else if (Geo::distance(arc0.x, arc0.y, arc1.x, arc1.y) < std::abs(arc0.radius - arc1.radius)) // 两圆内含
    {
        const Geo::Circle circle0(arc0.x, arc0.y, arc0.radius > arc1.radius ? arc0.radius - radius : arc0.radius + radius);
        const Geo::Circle circle1(arc1.x, arc1.y, arc1.radius > arc0.radius ? arc1.radius - radius : arc1.radius + radius);
        Geo::Point controls[2], center;
        switch (Geo::is_intersected(circle0, circle1, controls[0], controls[1]))
        {
        case 2:
            center = controls[Geo::distance(point0, controls[0]) < Geo::distance(point0, controls[1]) ? 0 : 1];
            break;
        case 1:
            center = controls[0];
            break;
        default:
            return false;
        }
        controls[0] = Geo::Point(circle0) + (center - circle0).normalize() * arc0.radius;
        controls[1] = Geo::Point(circle1) + (center - circle1).normalize() * arc1.radius;
        arc = Geo::Arc(controls[0], center, controls[1], true);
        if (Geo::Arc temp0, temp1; Geo::split(arc0, controls[0], temp0, temp1))
        {
            if (Geo::distance(point0, arc0.control_points[0]) < Geo::distance(point0, arc0.control_points[2]))
            {
                double angle0 = 0, angle1 = 0;
                if (arc0.is_cw())
                {
                    angle0 = Geo::angle(arc0.control_points[2], circle0, temp0.control_points[1]);
                    if (angle0 < 0)
                    {
                        angle0 += Geo::PI * 2;
                    }
                    angle1 = Geo::angle(arc0.control_points[2], circle0, temp1.control_points[1]);
                    if (angle1 < 0)
                    {
                        angle0 += Geo::PI * 2;
                    }
                }
                else
                {
                    angle0 = Geo::angle(arc0.control_points[2], circle0, temp0.control_points[1]);
                    if (angle0 > 0)
                    {
                        angle0 -= Geo::PI * 2;
                    }
                    angle1 = Geo::angle(arc0.control_points[2], circle0, temp1.control_points[1]);
                    if (angle1 > 0)
                    {
                        angle1 -= Geo::PI * 2;
                    }
                }
                result0 = std::abs(angle0) < std::abs(angle1) ? temp0 : temp1;
                if (arc.start_direction() * result0.start_direction() > 0)
                {
                    arc = Geo::Arc(controls[0], center, controls[1], false);
                }
            }
            else
            {
                double angle0 = 0, angle1 = 0;
                if (arc0.is_cw())
                {
                    angle0 = Geo::angle(arc0.control_points[0], circle0, temp0.control_points[1]);
                    if (angle0 > 0)
                    {
                        angle0 -= Geo::PI * 2;
                    }
                    angle1 = Geo::angle(arc0.control_points[0], circle0, temp1.control_points[1]);
                    if (angle1 > 0)
                    {
                        angle1 -= Geo::PI * 2;
                    }
                }
                else
                {
                    angle0 = Geo::angle(arc0.control_points[0], circle0, temp0.control_points[1]);
                    if (angle0 < 0)
                    {
                        angle0 += Geo::PI * 2;
                    }
                    angle1 = Geo::angle(arc0.control_points[0], circle0, temp1.control_points[1]);
                    if (angle1 < 0)
                    {
                        angle1 += Geo::PI * 2;
                    }
                }
                result0 = std::abs(angle0) < std::abs(angle1) ? temp0 : temp1;
                if (arc.start_direction() * result0.end_direction() < 0)
                {
                    arc = Geo::Arc(controls[0], center, controls[1], false);
                }
            }
        }
        else
        {
            if (Geo::distance(point0, arc0.control_points[0]) < Geo::distance(point0, arc0.control_points[2]))
            {
                result0 = Geo::Arc(controls[0], arc0.control_points[1], arc0.control_points[2]);
                if (arc.start_direction() * result0.start_direction() > 0)
                {
                    arc = Geo::Arc(controls[0], center, controls[1], false);
                }
            }
            else
            {
                result0 = Geo::Arc(arc0.control_points[0], arc0.control_points[1], controls[0]);
                if (arc.start_direction() * result0.end_direction() < 0)
                {
                    arc = Geo::Arc(controls[0], center, controls[1], false);
                }
            }
        }
        if (Geo::Arc temp0, temp1; Geo::split(arc1, controls[1], temp0, temp1))
        {
            if (Geo::distance(controls[1], temp0.control_points[0]) < Geo::distance(controls[1], temp0.control_points[2]) &&
                arc.end_direction() * temp0.start_direction() > 0)
            {
                result1 = temp0;
            }
            else if (Geo::distance(controls[1], temp0.control_points[2]) < Geo::distance(controls[1], temp0.control_points[0]) &&
                     arc.end_direction() * temp0.end_direction() < 0)
            {
                result1 = temp0;
            }
            else if (Geo::distance(controls[1], temp1.control_points[0]) < Geo::distance(controls[1], temp1.control_points[2]) &&
                     arc.end_direction() * temp1.start_direction() > 0)
            {
                result1 = temp1;
            }
            else if (Geo::distance(controls[1], temp1.control_points[2]) < Geo::distance(controls[1], temp1.control_points[0]) &&
                     arc.end_direction() * temp1.end_direction() < 0)
            {
                result1 = temp1;
            }
        }
        else
        {
            result1 = Geo::Arc(controls[1], arc1.control_points[1], arc1.control_points[2]);
            if (arc.end_direction() * result1.start_direction() < 0)
            {
                result1 = Geo::Arc(arc1.control_points[0], arc1.control_points[1], controls[1]);
            }
        }
    }
    else // 两圆相交
    {
        const Geo::Circle circle0(
            arc0.x, arc0.y, Geo::distance(point1, Geo::Point(arc0.x, arc0.y)) > arc0.radius ? arc0.radius + radius : arc0.radius - radius);
        const Geo::Circle circle1(
            arc1.x, arc1.y, Geo::distance(point0, Geo::Point(arc1.x, arc1.y)) > arc1.radius ? arc1.radius + radius : arc1.radius - radius);
        Geo::Point controls[2], center, intersection;
        switch (Geo::is_intersected(Geo::Circle(arc0.x, arc0.y, arc0.radius), Geo::Circle(arc1.x, arc1.y, arc1.radius), controls[0],
                                    controls[1]))
        {
        case 2:
            if (std::min(Geo::distance(point0, controls[0]), Geo::distance(point1, controls[0])) <
                std::min(Geo::distance(point0, controls[1]), Geo::distance(point1, controls[1])))
            {
                intersection = controls[0];
            }
            else
            {
                intersection = controls[1];
            }
            break;
        case 1:
            intersection = controls[0];
            break;
        default:
            return false;
        }
        switch (Geo::is_intersected(circle0, circle1, controls[0], controls[1]))
        {
        case 2:
            if (std::min(Geo::distance(point0, controls[0]), Geo::distance(point1, controls[0])) <
                std::min(Geo::distance(point0, controls[1]), Geo::distance(point1, controls[1])))
            {
                center = controls[0];
            }
            else
            {
                center = controls[1];
            }
            break;
        case 1:
            center = controls[0];
            break;
        default:
            return false;
        }
        controls[0] = Geo::Point(circle0) + (center - circle0).normalized() * arc0.radius;
        controls[1] = Geo::Point(circle1) + (center - circle1).normalize() * arc1.radius;

        bool is_small_part0 = true, is_small_part1 = true;
        {
            double angle0 = Geo::angle(arc0.control_points[0], circle0, intersection);
            double angle1 = Geo::angle(arc0.control_points[0], circle0, point0);
            if (arc0.is_cw())
            {
                if (angle0 > 0)
                {
                    angle0 -= Geo::PI * 2;
                }
                if (angle1 > 0)
                {
                    angle1 -= Geo::PI * 2;
                }
            }
            else
            {
                if (angle0 < 0)
                {
                    angle0 += Geo::PI * 2;
                }
                if (angle1 < 0)
                {
                    angle1 += Geo::PI * 2;
                }
            }
            is_small_part0 = std::abs(angle1) < std::abs(angle0);
        }
        {
            double angle0 = Geo::angle(arc1.control_points[0], circle1, intersection);
            double angle1 = Geo::angle(arc1.control_points[0], circle1, point1);
            if (arc1.is_cw())
            {
                if (angle0 > 0)
                {
                    angle0 -= Geo::PI * 2;
                }
                if (angle1 > 0)
                {
                    angle1 -= Geo::PI * 2;
                }
            }
            else
            {
                if (angle0 < 0)
                {
                    angle0 += Geo::PI * 2;
                }
                if (angle1 < 0)
                {
                    angle1 += Geo::PI * 2;
                }
            }
            is_small_part1 = std::abs(angle1) < std::abs(angle0);
        }

        if (Geo::Arc temp0, temp1; Geo::split(arc0, controls[0], temp0, temp1))
        {
            double angle0 = Geo::angle(arc0.control_points[0], circle0, temp0.control_points[1]);
            double angle1 = Geo::angle(arc0.control_points[0], circle0, temp1.control_points[1]);
            if (arc0.is_cw())
            {
                if (angle0 > 0)
                {
                    angle0 -= Geo::PI * 2;
                }
                if (angle1 > 0)
                {
                    angle1 -= Geo::PI * 2;
                }
            }
            else
            {
                if (angle0 < 0)
                {
                    angle0 += Geo::PI * 2;
                }
                if (angle1 < 0)
                {
                    angle1 += Geo::PI * 2;
                }
            }
            if (is_small_part0)
            {
                result0 = std::abs(angle0) < std::abs(angle1) ? temp0 : temp1;
            }
            else
            {
                result0 = std::abs(angle0) > std::abs(angle1) ? temp0 : temp1;
            }
        }
        else
        {
            return false;
        }

        if (Geo::Arc temp0, temp1; Geo::split(arc1, controls[1], temp0, temp1))
        {
            double angle0 = Geo::angle(arc1.control_points[0], circle1, temp0.control_points[1]);
            double angle1 = Geo::angle(arc1.control_points[0], circle1, temp1.control_points[1]);
            if (arc1.is_cw())
            {
                if (angle0 > 0)
                {
                    angle0 -= Geo::PI * 2;
                }
                if (angle1 > 0)
                {
                    angle1 -= Geo::PI * 2;
                }
            }
            else
            {
                if (angle0 < 0)
                {
                    angle0 += Geo::PI * 2;
                }
                if (angle1 < 0)
                {
                    angle1 += Geo::PI * 2;
                }
            }
            if (is_small_part1)
            {
                result1 = std::abs(angle0) < std::abs(angle1) ? temp0 : temp1;
            }
            else
            {
                result1 = std::abs(angle0) > std::abs(angle1) ? temp0 : temp1;
            }
        }
        else
        {
            return false;
        }

        arc = Geo::Arc(controls[0], center, controls[1], true);
        if (Geo::distance(controls[0], result0.control_points[0]) < Geo::distance(controls[0], result0.control_points[2]))
        {
            if (result0.start_direction() * arc.start_direction() > 0)
            {
                arc = Geo::Arc(controls[0], center, controls[1], false);
            }
        }
        else
        {
            if (result0.end_direction() * arc.start_direction() < 0)
            {
                arc = Geo::Arc(controls[0], center, controls[1], false);
            }
        }
    }
    return true;
}