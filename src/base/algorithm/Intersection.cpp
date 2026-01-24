#include <algorithm>
#include "base/Algorithm.hpp"
#include "base/Math.hpp"


bool Geo::is_intersected(const Point &point0, const Point &point1, const Point &point2, const Point &point3, Point &output,
                         const bool infinite)
{
    // if (point0 == point1 && point2 == point3)
    // {
    //     return point0 == point2;
    // }
    // else if (point0 == point1)
    // {
    //     return Geo::is_inside(point0, point2, point3);
    // }
    // else if (point2 == point3)
    // {
    //     return Geo::is_inside(point2, point0, point1);
    // }
    assert(point0 != point1 && point2 != point3);
    if (point0 == point2 || point0 == point3)
    {
        output = point0;
        return true;
    }
    else if (point1 == point2 || point1 == point3)
    {
        output = point1;
        return true;
    }

    if (!infinite)
    {
        const double left0 = std::min(point0.x, point1.x), left1 = std::min(point2.x, point3.x);
        const double right0 = std::max(point0.x, point1.x), right1 = std::max(point2.x, point3.x);
        const double top0 = std::max(point0.y, point1.y), top1 = std::max(point2.y, point3.y);
        const double bottom0 = std::min(point0.y, point1.y), bottom1 = std::min(point2.y, point3.y);
        if (left0 > right1 || right0 < left1 || top0 < bottom1 || bottom0 > top1)
        {
            return false;
        }
    }

    const double a0 = point1.y - point0.y, b0 = point0.x - point1.x, c0 = point1.x * point0.y - point0.x * point1.y;
    const double a1 = point3.y - point2.y, b1 = point2.x - point3.x, c1 = point3.x * point2.y - point2.x * point3.y;
    if (std::abs(a0 * b1 - a1 * b0) < Geo::EPSILON)
    {
        if (std::abs(a0 * c1 - a1 * c0) > Geo::EPSILON || std::abs(b0 * c1 - b1 * c0) > Geo::EPSILON)
        {
            return false;
        }
        if (infinite)
        {
            return true;
        }
        else
        {
            const double a = Geo::distance((point0 + point1) / 2, (point2 + point3) / 2) * 2;
            const double b = Geo::distance(point0, point1) + Geo::distance(point2, point3);
            if (a < b)
            {
                return true;
            }
            else if (a == b)
            {
                if (Geo::distance(point0, point2) < Geo::EPSILON || Geo::distance(point0, point3) < Geo::EPSILON)
                {
                    output = point0;
                }
                else
                {
                    output = point1;
                }
                return true;
            }
            else
            {
                return false;
            }
        }
    }

    output.x = (c1 * b0 - c0 * b1) / (a0 * b1 - a1 * b0), output.y = (c0 * a1 - c1 * a0) / (a0 * b1 - a1 * b0);

    if (Geo::is_inside(point0, point2, point3))
    {
        output = point0;
    }
    else if (Geo::is_inside(point1, point2, point3))
    {
        output = point1;
    }
    else if (Geo::is_inside(point2, point0, point1))
    {
        output = point2;
    }
    else if (Geo::is_inside(point3, point0, point1))
    {
        output = point3;
    }

    if (point0.x == point1.x)
    {
        output.x = point0.x;
    }
    else if (point2.x == point3.x)
    {
        output.x = point2.x;
    }

    if (point0.y == point1.y)
    {
        output.y = point0.y;
    }
    else if (point2.y == point3.y)
    {
        output.y = point2.y;
    }

    if (infinite)
    {
        return true;
    }
    else
    {
        const double left = std::max(std::min(point0.x, point1.x), std::min(point2.x, point3.x));
        const double right = std::min(std::max(point0.x, point1.x), std::max(point2.x, point3.x));
        const double top = std::min(std::max(point0.y, point1.y), std::max(point2.y, point3.y));
        const double bottom = std::max(std::min(point0.y, point1.y), std::min(point2.y, point3.y));

        return left - 5e-14 <= output.x && output.x <= right + 5e-14 && bottom - 5e-14 <= output.y && output.y <= top + 5e-14;
    }
}

int Geo::is_intersected(const Point &point0, const Point &point1, const Circle &circle, Point &output0, Point &output1, const bool infinite)
{
    if (Geo::distance_square(circle, point0, point1, infinite) > std::pow(circle.radius, 2) + Geo::EPSILON)
    {
        return 0;
    }
    Geo::Point foot;
    Geo::foot_point(point0, point1, circle, foot, true);
    const double l = std::sqrt(std::abs(std::pow(circle.radius, 2) - Geo::distance_square(circle, point0, point1, true)));
    output0 = foot + (point0 - point1).normalize() * l;
    output1 = foot + (point1 - point0).normalize() * l;
    if (infinite)
    {
        return Geo::distance_square(circle, point0, point1, true) < std::pow(circle.radius, 2) ? 2 : 1;
    }
    if (Geo::distance(output0, point0) < Geo::EPSILON)
    {
        output0 = point0;
    }
    else if (Geo::distance(output0, point1) < Geo::EPSILON)
    {
        output0 = point1;
    }
    if (Geo::distance(output1, point0) < Geo::EPSILON)
    {
        output1 = point0;
    }
    else if (Geo::distance(output1, point1) < Geo::EPSILON)
    {
        output1 = point1;
    }
    if (Geo::is_inside(output0, point0, point1))
    {
        if (Geo::is_inside(output1, point0, point1))
        {
            return 2;
        }
        else
        {
            return 1;
        }
    }
    else
    {
        if (Geo::is_inside(output1, point0, point1))
        {
            output0 = output1;
            return 1;
        }
        else
        {
            return 0;
        }
    }
}

int Geo::is_intersected(const Point &point0, const Point &point1, const Ellipse &ellipse, Point &output0, Point &output1,
                        const bool infinite)
{
    const double eps = Geo::EPSILON;
    const Geo::Point center = ellipse.center();
    const double angle = Geo::angle(ellipse.a0(), ellipse.a1());
    Geo::Point point2 = Geo::to_coord(point0, center.x, center.y, angle);
    Geo::Point point3 = Geo::to_coord(point1, center.x, center.y, angle);
    const double a1 = point3.y - point2.y;
    const double b1 = point2.x - point3.x;
    const double c1 = point3.x * point2.y - point2.x * point3.y;
    const double a0 = Geo::distance_square(ellipse.a0(), ellipse.a1()) / 4;
    const double b0 = Geo::distance_square(ellipse.b0(), ellipse.b1()) / 4;

    if (std::pow(a1, 2) * a0 + std::pow(b1, 2) * b0 > std::pow(c1, 2))
    {
        if (b1 != 0)
        {
            const double t0 = a0 * std::pow(a1, 2) + b0 * std::pow(b1, 2);
            const double t1 = b0 * std::pow(b1, 2) * c1;
            const double t2 = std::sqrt(a0 * b0 * std::pow(b1, 2) * (t0 - std::pow(c1, 2)));
            output0.x = (-a0 * a1 * c1 - t2) / t0;
            output0.y = (a1 * t2 - t1) / (b1 * t0);
            output1.x = (t2 - a0 * a1 * c1) / t0;
            output1.y = (-a1 * t2 - t1) / (b1 * t0);
        }
        else
        {
            output0.x = point2.x;
            output0.y = std::sqrt(b0 - b0 * std::pow(output0.x, 2) / a0);
            output1.x = point2.x;
            output1.y = -std::sqrt(b0 - b0 * std::pow(output1.x, 2) / a0);
        }

        if (infinite)
        {
            const Geo::Point coord = Geo::to_coord(Geo::Point(0, 0), center.x, center.y, angle);
            output0 = Geo::to_coord(output0, coord.x, coord.y, -angle);
            output1 = Geo::to_coord(output1, coord.x, coord.y, -angle);
            if (ellipse.is_arc())
            {
                int result = 0;
                if (Geo::distance(output0, ellipse) < eps)
                {
                    ++result;
                }
                else
                {
                    output0 = output1;
                }
                if (Geo::distance(output1, ellipse) < eps)
                {
                    ++result;
                }
                return result;
            }
            return 2;
        }
        else
        {
            if (Geo::is_inside(output0, point2, point3, false))
            {
                if (Geo::is_inside(output1, point2, point3, false))
                {
                    const Geo::Point coord = Geo::to_coord(Geo::Point(0, 0), center.x, center.y, angle);
                    output0 = Geo::to_coord(output0, coord.x, coord.y, -angle);
                    output1 = Geo::to_coord(output1, coord.x, coord.y, -angle);
                    if (ellipse.is_arc())
                    {
                        int result = 0;
                        if (Geo::distance(output0, ellipse) < eps)
                        {
                            ++result;
                        }
                        else
                        {
                            output0 = output1;
                        }
                        if (Geo::distance(output1, ellipse) < eps)
                        {
                            ++result;
                        }
                        return result;
                    }
                    return 2;
                }
                else
                {
                    const Geo::Point coord = Geo::to_coord(Geo::Point(0, 0), center.x, center.y, angle);
                    output0 = Geo::to_coord(output0, coord.x, coord.y, -angle);
                    if (ellipse.is_arc())
                    {
                        return Geo::distance(output0, ellipse) < eps ? 1 : 0;
                    }
                    return 1;
                }
            }
            else
            {
                if (Geo::is_inside(output1, point2, point3, false))
                {
                    const Geo::Point coord = Geo::to_coord(Geo::Point(0, 0), center.x, center.y, angle);
                    output0 = Geo::to_coord(output1, coord.x, coord.y, -angle);
                    if (ellipse.is_arc())
                    {
                        return Geo::distance(output0, ellipse) < eps ? 1 : 0;
                    }
                    return 1;
                }
                else
                {
                    return 0;
                }
            }
        }
    }
    else if (std::pow(a1, 2) * a0 + std::pow(b1, 2) * b0 == std::pow(c1, 2))
    {
        if (b1 != 0)
        {
            const double t0 = a0 * std::pow(a1, 2) + b0 * std::pow(b1, 2);
            const double t1 = b0 * std::pow(b1, 2) * c1;
            output0.x = (-a0 * a1 * c1) / t0;
            output0.y = t1 / (b1 * t0);
        }
        else
        {
            output0.x = point2.x;
            output0.y = 0;
        }

        if (infinite)
        {
            const Geo::Point coord = Geo::to_coord(Geo::Point(0, 0), center.x, center.y, angle);
            output0 = Geo::to_coord(output0, coord.x, coord.y, -angle);
            if (ellipse.is_arc())
            {
                return Geo::distance(output0, ellipse) < eps ? 1 : 0;
            }
            return 1;
        }
        else
        {
            if (Geo::is_inside(output0, point2, point3))
            {
                const Geo::Point coord = Geo::to_coord(Geo::Point(0, 0), center.x, center.y, angle);
                output0 = Geo::to_coord(output0, coord.x, coord.y, -angle);
                if (ellipse.is_arc())
                {
                    return Geo::distance(output0, ellipse) < eps ? 1 : 0;
                }
                return 1;
            }
            else
            {
                return 0;
            }
        }
    }
    else
    {
        return 0;
    }
}

int Geo::is_intersected(const Point &point0, const Point &point1, const Arc &arc, Point &output0, Point &output1, const bool infinite)
{
    const double eps = Geo::EPSILON;
    switch (Geo::is_intersected(point0, point1, Geo::Circle(arc.x, arc.y, arc.radius), output0, output1, infinite))
    {
    case 1:
        if (Geo::distance(output0, arc) < eps)
        {
            return 1;
        }
        else
        {
            return 0;
        }
        break;
    case 2:
        {
            int count = 0;
            if (Geo::distance(output0, arc) < eps)
            {
                ++count;
            }
            else
            {
                output0 = output1;
            }
            if (Geo::distance(output1, arc) < eps)
            {
                ++count;
            }
            return count;
        }
        break;
    default:
        return 0;
    }
}

int Geo::is_intersected(const Point &point0, const Point &point1, const CubicBezier &bezier, std::vector<Point> &intersections,
                        const bool infinite, std::vector<std::tuple<size_t, double, double, double>> *tvalues)
{
    const int order = 3;
    const int nums[4] = {1, 3, 3, 1};
    std::vector<Geo::Point> result;
    for (size_t i = 0, end = bezier.size() - order; i < end; i += order)
    {
        Geo::Polyline polyline;
        polyline.append(bezier[i]);
        double t = 0;
        while (t <= 1)
        {
            Geo::Point point;
            for (int j = 0; j <= order; ++j)
            {
                point += (bezier[j + i] * (nums[j] * std::pow(1 - t, order - j) * std::pow(t, j)));
            }
            polyline.append(point);
            t += Geo::CubicBezier::default_step;
        }
        polyline.append(bezier[i + order]);
        Geo::down_sampling(polyline, Geo::CubicBezier::default_down_sampling_value);

        if (!infinite && !Geo::is_intersected(polyline.bounding_rect(), point0, point1))
        {
            continue;
        }

        std::vector<Geo::Point> temp;
        for (size_t j = 1, count = polyline.size(); j < count; ++j)
        {
            if (Geo::Point point; Geo::is_intersected(polyline[j - 1], polyline[j], point0, point1, point, infinite) &&
                                  (!infinite || Geo::is_inside(point, polyline[j - 1], polyline[j], false)))
            {
                temp.emplace_back(point);
            }
        }

        for (Geo::Point &point : temp)
        {
            t = 0;
            double step = 1e-3, lower = 0, upper = 1;
            double min_dis = DBL_MAX;
            while (min_dis > 1e-4 && step > 1e-12)
            {
                for (double x = lower; x < upper + step; x += step)
                {
                    x = x < upper ? x : upper;
                    Geo::Point coord;
                    for (int j = 0; j <= order; ++j)
                    {
                        coord += (bezier[j + i] * (nums[j] * std::pow(1 - x, order - j) * std::pow(x, j)));
                    }
                    if (double dis = Geo::distance(point, coord); dis < min_dis)
                    {
                        min_dis = dis;
                        t = x;
                    }
                }
                lower = std::max(0.0, t - step);
                upper = std::min(1.0, t + step);
                step = (upper - lower) / 100;
            }

            lower = std::max(0.0, t - 1e-4), upper = std::min(1.0, t + 1e-4);
            step = (upper - lower) / 100;
            min_dis = DBL_MAX;
            std::vector<double> stored_t;
            while ((upper - lower) * 1e15 > 1)
            {
                int flag = 0;
                for (double x = lower, dis0 = 0; x < upper + step; x += step)
                {
                    x = x < upper ? x : upper;
                    Geo::Point coord;
                    for (int j = 0; j <= order; ++j)
                    {
                        coord += (bezier[j + i] * (nums[j] * std::pow(1 - x, order - j) * std::pow(x, j)));
                    }
                    if (const double dis = Geo::distance(coord, point0, point1, infinite) * 1e9; dis < min_dis)
                    {
                        min_dis = dis;
                        t = x;
                    }
                    else if (dis == min_dis) // 需要扩大搜索范围
                    {
                        flag = -1;
                        break;
                    }
                    else
                    {
                        if (dis == dis0)
                        {
                            if (++flag == 10)
                            {
                                break; // 连续10次相等就退出循环
                            }
                        }
                        else
                        {
                            flag = 0;
                        }
                        dis0 = dis;
                    }
                }
                if (min_dis < 2e-5)
                {
                    break;
                }
                else if (flag == -1) // 需要扩大搜索范围
                {
                    if (t - lower < upper - t)
                    {
                        lower = std::max(0.0, lower - step * 2);
                        if (stored_t.size() > 3 && stored_t[0] == stored_t[2] && stored_t[1] == stored_t[3])
                        {
                            stored_t.clear();
                            lower += (upper - lower) / 4;
                        }
                    }
                    else
                    {
                        upper = std::min(1.0, upper + step * 2);
                        if (stored_t.size() > 3 && stored_t[0] == stored_t[2] && stored_t[1] == stored_t[3])
                        {
                            stored_t.clear();
                            upper -= (upper - lower) / 4;
                        }
                    }
                    stored_t.push_back(t);
                    if (stored_t.size() > 4)
                    {
                        stored_t.erase(stored_t.begin(), stored_t.end() - 4);
                    }
                    step = (upper - lower) / 100;
                }
                else
                {
                    lower = std::max(0.0, t - step * 2);
                    upper = std::min(1.0, t + step * 2);
                    step = (upper - lower) / 100;
                }
            }

            point.clear();
            for (int j = 0; j <= order; ++j)
            {
                point += (bezier[j + i] * (nums[j] * std::pow(1 - t, order - j) * std::pow(t, j)));
            }
            if (std::find(result.begin(), result.end(), point) == result.end() && Geo::is_inside(point, point0, point1, infinite))
            {
                result.emplace_back(point);
                if (tvalues != nullptr)
                {
                    tvalues->emplace_back(i, t, point.x, point.y);
                }
            }
        }
    }

    if (Geo::distance(bezier.front(), point0, point1, infinite) < Geo::EPSILON &&
        std::find(intersections.begin(), intersections.end(), bezier.front()) == intersections.end())
    {
        intersections.emplace_back(bezier.front());
        if (tvalues != nullptr)
        {
            tvalues->emplace_back(0, 0, bezier.front().x, bezier.front().y);
        }
    }
    if (Geo::distance(bezier.back(), point0, point1, infinite) < Geo::EPSILON &&
        std::find(intersections.begin(), intersections.end(), bezier.back()) == intersections.end())
    {
        intersections.emplace_back(bezier.back());
        if (tvalues != nullptr)
        {
            tvalues->emplace_back(bezier.size() / (order + 1) - 1, 1, bezier.back().x, bezier.back().y);
        }
    }

    intersections.insert(intersections.end(), result.begin(), result.end());
    return result.size();
}

int Geo::is_intersected(const Point &point0, const Point &point1, const BSpline &bspline, const bool is_cubic,
                        std::vector<Point> &intersections, const bool infinite, std::vector<std::tuple<double, double, double>> *tvalues)
{
    const std::vector<double> &knots = bspline.knots();
    const size_t npts = bspline.control_points.size();
    const size_t nplusc = npts + (is_cubic ? 4 : 3);
    const size_t p1 = std::max(npts * 8.0, bspline.shape().length() / Geo::BSpline::default_step);

    double t = knots[0];
    const double init_step = (knots[nplusc - 1] - t) / (p1 - 1);
    std::vector<double> temp;
    double min_dis[2] = {1, 1};
    while (t <= knots[nplusc - 1])
    {
        std::vector<double> nbasis;
        Geo::BSpline::rbasis(is_cubic ? 3 : 2, t, npts, knots, nbasis);
        Geo::Point coord;
        for (size_t i = 0; i < npts; ++i)
        {
            coord += bspline.control_points[i] * nbasis[i];
        }
        const double dis = Geo::distance(coord, point0, point1, infinite);
        if (min_dis[0] > min_dis[1] && dis > min_dis[1])
        {
            temp.push_back(t - init_step);
        }
        min_dis[0] = min_dis[1];
        min_dis[1] = dis;
        t += init_step;
    }

    std::vector<Geo::Point> result;
    std::vector<std::tuple<double, double, double>> temp_tvalues;
    for (size_t n = 0, count = temp.size(); n < count; ++n)
    {
        t = temp[n];
        const double min_lower = n > 0 ? temp[n - 1] : knots[0];
        const double max_upper = n < count - 1 ? temp[n + 1] : knots[nplusc - 1];
        double lower = std::max(min_lower, t - init_step);
        double upper = std::min(max_upper, t + init_step);
        double step = (upper - lower) / 1000;
        min_dis[0] = DBL_MAX, min_dis[1] = 1;
        while (std::abs(min_dis[0] - min_dis[1]) > 1e-4)
        {
            for (double x = lower; x < upper + step; x += step)
            {
                x = x < upper ? x : upper;
                std::vector<double> nbasis;
                Geo::BSpline::rbasis(is_cubic ? 3 : 2, x, npts, knots, nbasis);
                Geo::Point coord;
                for (size_t i = 0; i < npts; ++i)
                {
                    coord += bspline.control_points[i] * nbasis[i];
                }
                if (double dis = Geo::distance(coord, point0, point1, infinite); dis < min_dis[1])
                {
                    min_dis[1] = dis;
                    t = x;
                }
            }
            lower = std::max(min_lower, t - step);
            upper = std::min(max_upper, t + step);
            step = (upper - lower) / 100;
            if (min_dis[0] > min_dis[1])
            {
                min_dis[0] = min_dis[1];
            }
        }

        if (min_dis[0] > 1e-3 && min_dis[1] > 1e-3)
        {
            continue;
        }

        min_dis[0] = 0, min_dis[1] = DBL_MAX;
        step = (upper - lower) / 100;
        std::vector<double> stored_t;
        while ((upper - lower) * 1e15 > 1)
        {
            int flag = 0;
            for (double x = lower, dis0 = 0; x < upper + step; x += step)
            {
                x = x < upper ? x : upper;
                std::vector<double> nbasis;
                Geo::BSpline::rbasis(is_cubic ? 3 : 2, x, npts, knots, nbasis);
                Geo::Point coord;
                for (size_t i = 0; i < npts; ++i)
                {
                    coord += bspline.control_points[i] * nbasis[i];
                }
                if (const double dis = Geo::distance(coord, point0, point1, infinite) * 1e9; dis < min_dis[1])
                {
                    min_dis[1] = dis;
                    t = x;
                }
                else if (dis == min_dis[1])
                {
                    flag = -1; // 需要扩大搜索范围
                    break;
                }
                else
                {
                    if (dis0 == dis)
                    {
                        if (++flag == 10)
                        {
                            break;
                        }
                    }
                    else
                    {
                        flag = 0;
                    }
                    dis0 = dis;
                }
            }
            if (min_dis[1] < 2e-5)
            {
                break;
            }
            else if (flag == -1) // 需要扩大搜索范围
            {
                if (t - lower < upper - t)
                {
                    lower = std::max(min_lower, lower - step * 2);
                    if (stored_t.size() > 3 && stored_t[0] == stored_t[2] && stored_t[1] == stored_t[3])
                    {
                        stored_t.clear();
                        lower += (upper - lower) / 4;
                    }
                }
                else
                {
                    upper = std::min(max_upper, upper + step * 2);
                    if (stored_t.size() > 3 && stored_t[0] == stored_t[2] && stored_t[1] == stored_t[3])
                    {
                        stored_t.clear();
                        upper -= (upper - lower) / 4;
                    }
                }
                stored_t.push_back(t);
                if (stored_t.size() > 4)
                {
                    stored_t.erase(stored_t.begin(), stored_t.end() - 4);
                }
                step = (upper - lower) / 100;
            }
            else
            {
                lower = std::max(min_lower, t - step * 2);
                upper = std::min(max_upper, t + step * 2);
                step = (upper - lower) / 100;
            }
        }

        std::vector<double> nbasis;
        Geo::BSpline::rbasis(is_cubic ? 3 : 2, t, npts, knots, nbasis);
        Geo::Point coord;
        for (size_t i = 0; i < npts; ++i)
        {
            coord += bspline.control_points[i] * nbasis[i];
        }
        if (std::find(result.begin(), result.end(), coord) == result.end() && Geo::is_inside(coord, point0, point1, infinite))
        {
            result.emplace_back(coord);
            if (tvalues != nullptr)
            {
                tvalues->emplace_back(t, coord.x, coord.y);
            }
        }
    }

    if (Geo::distance(bspline.front(), point0, point1, infinite) < Geo::EPSILON &&
        std::find(intersections.begin(), intersections.end(), bspline.front()) == intersections.end())
    {
        intersections.emplace_back(bspline.front());
        if (tvalues != nullptr)
        {
            tvalues->emplace_back(0, bspline.front().x, bspline.front().y);
        }
    }
    if (Geo::distance(bspline.back(), point0, point1, infinite) < Geo::EPSILON &&
        std::find(intersections.begin(), intersections.end(), bspline.back()) == intersections.end())
    {
        intersections.emplace_back(bspline.back());
        if (tvalues != nullptr)
        {
            tvalues->emplace_back(1, bspline.back().x, bspline.back().y);
        }
    }

    intersections.insert(intersections.end(), result.begin(), result.end());
    return result.size();
}

bool Geo::is_intersected(const AABBRect &rect0, const AABBRect &rect1, const bool inside)
{
    if (rect0.empty() || rect1.empty())
    {
        return false;
    }

    if (rect0.right() < rect1.left() || rect0.left() > rect1.right() || rect0.bottom() > rect1.top() || rect0.top() < rect1.bottom())
    {
        return false;
    }

    if (inside)
    {
        return true;
    }
    else
    {
        return !(
            (rect0.top() < rect1.top() && rect0.right() < rect1.right() && rect0.bottom() > rect1.bottom() &&
             rect0.left() > rect1.left()) ||
            (rect1.top() < rect0.top() && rect1.right() < rect0.right() && rect1.bottom() > rect0.bottom() && rect1.left() > rect0.left()));
    }
}

bool Geo::is_intersected(const Polyline &polyline0, const Polyline &polyline1)
{
    if (polyline0.empty() || polyline1.empty() || !Geo::is_intersected(polyline0.bounding_rect(), polyline1.bounding_rect()))
    {
        return false;
    }
    Point point;
    for (size_t i = 1, count0 = polyline0.size(); i < count0; ++i)
    {
        for (size_t j = 1, count1 = polyline1.size(); j < count1; ++j)
        {
            if (polyline0[i - 1] != polyline0[i] && polyline1[j - 1] != polyline1[j] &&
                Geo::is_intersected(polyline0[i - 1], polyline0[i], polyline1[j - 1], polyline1[j], point))
            {
                return true;
            }
        }
    }
    return false;
}

bool Geo::is_intersected(const Polyline &polyline, const Polygon &polygon, const bool inside)
{
    if (polyline.empty() || polygon.empty() || !Geo::is_intersected(polygon.bounding_rect(), polyline.bounding_rect()))
    {
        return false;
    }

    Point point;
    if (inside)
    {
        for (size_t i = 1, count0 = polyline.size(); i < count0; ++i)
        {
            if (Geo::is_inside(polyline[i - 1], polygon))
            {
                return true;
            }
            for (size_t j = 1, count1 = polygon.size(); j < count1; ++j)
            {
                if (polyline[i - 1] != polyline[i] && polygon[j - 1] != polygon[j] &&
                    Geo::is_intersected(polyline[i - 1], polyline[i], polygon[j - 1], polygon[j], point))
                {
                    return true;
                }
            }
        }
        return Geo::is_inside(polyline.back(), polygon);
    }
    else
    {
        for (size_t i = 1, count0 = polyline.size(); i < count0; ++i)
        {
            for (size_t j = 1, count1 = polygon.size(); j < count1; ++j)
            {
                if (polyline[i - 1] != polyline[i] && polygon[j - 1] != polygon[j] &&
                    Geo::is_intersected(polyline[i - 1], polyline[i], polygon[j - 1], polygon[j], point))
                {
                    return true;
                }
            }
        }
        return false;
    }
}

bool Geo::is_intersected(const Polyline &polyline, const Circle &circle)
{
    const double length = circle.radius * circle.radius;
    for (size_t i = 0, count = polyline.size(); i < count; ++i)
    {
        if (polyline[i - 1] != polyline[i] && Geo::distance_square(circle, polyline[i - 1], polyline[i]) < length)
        {
            return true;
        }
    }
    return false;
}

bool Geo::is_intersected(const Polyline &polyline, const Ellipse &ellipse, const bool inside)
{
    if (polyline.empty() || ellipse.empty() || !Geo::is_intersected(polyline.bounding_rect(), ellipse.bounding_rect()))
    {
        return false;
    }

    if (inside)
    {
        for (const Geo::Point &point : polyline)
        {
            if (Geo::is_inside(point, ellipse, true))
            {
                return true;
            }
        }
    }

    Geo::Point output0, output1;
    for (size_t i = 1, count = polyline.size(); i < count; ++i)
    {
        if (polyline[i - 1] != polyline[i] && Geo::is_intersected(polyline[i - 1], polyline[i], ellipse, output0, output1))
        {
            return true;
        }
    }
    return false;
}

bool Geo::is_intersected(const Polyline &polyline, const Arc &arc)
{
    for (size_t i = 1, count = polyline.size(); i < count; ++i)
    {
        if (Geo::Point point0, point1;
            polyline[i - 1] != polyline[i] && Geo::is_intersected(polyline[i - 1], polyline[i], arc, point0, point1))
        {
            return true;
        }
    }
    return false;
}

bool Geo::is_intersected(const Polygon &polygon0, const Polygon &polygon1, const bool inside)
{
    if (polygon0.empty() || polygon1.empty() || !Geo::is_intersected(polygon0.bounding_rect(), polygon1.bounding_rect()))
    {
        return false;
    }
    Point point;
    for (size_t i = 1, count0 = polygon0.size(); i < count0; ++i)
    {
        for (size_t j = 1, count1 = polygon1.size(); j < count1; ++j)
        {
            if (polygon0[i - 1] != polygon0[i] && polygon1[j - 1] != polygon1[j] &&
                Geo::is_intersected(polygon0[i - 1], polygon0[i], polygon1[j - 1], polygon1[j], point))
            {
                return true;
            }
        }
    }
    if (inside)
    {
        for (const Point &point : polygon0)
        {
            if (Geo::is_inside(point, polygon1, true))
            {
                return true;
            }
        }
        for (const Point &point : polygon1)
        {
            if (Geo::is_inside(point, polygon0, true))
            {
                return true;
            }
        }
    }
    return false;
}

bool Geo::is_intersected(const Polygon &polygon, const Circle &circle, const bool inside)
{
    const double length = circle.radius * circle.radius;
    for (size_t i = 1, count = polygon.size(); i < count; ++i)
    {
        if (polygon[i - 1] != polygon[i] && Geo::distance_square(circle, polygon[i - 1], polygon[i]) < length)
        {
            return true;
        }
    }

    if (!inside)
    {
        return false;
    }

    if (Geo::is_inside(circle, polygon, true) ||
        std::any_of(polygon.begin(), polygon.end(), [&](const Geo::Point &point) { return Geo::is_inside(point, circle, true); }))
    {
        return true;
    }

    return false;
}

bool Geo::is_intersected(const Polygon &polygon, const Ellipse &ellipse, const bool inside)
{
    if (polygon.empty() || ellipse.empty() || !Geo::is_intersected(polygon.bounding_rect(), ellipse.bounding_rect()))
    {
        return false;
    }

    if (inside)
    {
        for (const Geo::Point &point : polygon)
        {
            if (Geo::is_inside(point, ellipse, true))
            {
                return true;
            }
        }
        if (Geo::is_inside(ellipse.center(), polygon) || Geo::is_inside(ellipse.a0(), polygon) || Geo::is_inside(ellipse.a1(), polygon) ||
            Geo::is_inside(ellipse.b0(), polygon) || Geo::is_inside(ellipse.b1(), polygon))
        {
            return true;
        }
    }

    Geo::Point output0, output1;
    for (size_t i = 1, count = polygon.size(); i < count; ++i)
    {
        if (polygon[i - 1] != polygon[i] && Geo::is_intersected(polygon[i - 1], polygon[i], ellipse, output0, output1))
        {
            return true;
        }
    }
    return false;
}

bool Geo::is_intersected(const Circle &circle0, const Circle &circle1, const bool inside)
{
    if (inside)
    {
        return Geo::distance(circle0, circle1) <= circle0.radius + circle1.radius;
    }
    else
    {
        const double distance = Geo::distance(circle0, circle1);
        return distance <= circle0.radius + circle1.radius && distance >= std::abs(circle0.radius - circle1.radius);
    }
}

int Geo::is_intersected(const Circle &circle0, const Circle &circle1, Point &point0, Point &point1)
{
    const double distance = Geo::distance(circle0, circle1);
    if (distance <= circle0.radius + circle1.radius && distance >= std::abs(circle0.radius - circle1.radius))
    {
        const double d = Geo::distance(circle0, circle1), dd = Geo::distance_square(circle0, circle1);
        const double a = (std::pow(circle0.radius, 2) - std::pow(circle1.radius, 2) + dd) / (2 * d);
        const double x = circle0.x + (a / d) * (circle1.x - circle0.x);
        const double y = circle0.y + (a / d) * (circle1.y - circle0.y);
        if (distance == circle0.radius + circle1.radius || distance == std::abs(circle0.radius - circle1.radius))
        {
            if (circle0 == circle1)
            {
                return 0;
            }
            point0.x = x;
            point0.y = y;
            return 1;
        }
        else
        {
            const double aa = std::pow(std::pow(circle0.radius, 2) - std::pow(circle1.radius, 2) + dd, 2) / (4 * dd);
            const double h = std::sqrt(std::pow(circle0.radius, 2) - aa);
            point0.x = x - (circle1.y - circle0.y) * h / d;
            point0.y = y - (circle0.x - circle1.x) * h / d;
            point1.x = x + (circle1.y - circle0.y) * h / d;
            point1.y = y + (circle0.x - circle1.x) * h / d;
            return 2;
        }
    }
    else
    {
        return 0;
    }
}

int Geo::is_intersected(const Circle &circle, const Arc &arc, Point &output0, Point &output1)
{
    const double eps = Geo::EPSILON;
    switch (Geo::is_intersected(circle, Geo::Circle(arc.x, arc.y, arc.radius), output0, output1))
    {
    case 1:
        if (Geo::distance(output0, arc) < eps)
        {
            return 1;
        }
        else
        {
            return 0;
        }
        break;
    case 2:
        {
            int count = 0;
            if (double d = Geo::distance(output0, arc); d < eps)
            {
                ++count;
            }
            else
            {
                output0 = output1;
            }
            if (double d = Geo::distance(output1, arc); d < eps)
            {
                ++count;
            }
            return count;
        }
        break;
    default:
        return 0;
    }
}

int Geo::is_intersected(const Ellipse &ellipse0, const Ellipse &ellipse1, Point &point0, Point &point1, Point &point2, Point &point3)
{
    if (!Geo::is_intersected(ellipse0.bounding_rect(), ellipse1.bounding_rect()))
    {
        return 0;
    }

    const Geo::Polyline &polyline0 = ellipse0.shape();
    const Geo::Polyline &polyline1 = ellipse1.shape();
    std::vector<Geo::Point> points;
    for (size_t i = 1, count0 = polyline0.size(); i < count0; ++i)
    {
        Geo::Point point;
        for (size_t j = 1, count1 = polyline1.size(); j < count1; ++j)
        {
            if (Geo::is_intersected(polyline0[i - 1], polyline0[i], polyline1[j - 1], polyline1[j], point, false) &&
                std::find(points.cbegin(), points.cend(), point) == points.cend())
            {
                points.emplace_back(point);
            }
        }
    }
    {
        const double eps = Geo::EPSILON;
        if (ellipse0.is_arc())
        {
            if (Geo::distance(ellipse0.arc_point0(), ellipse1) < eps)
            {
                points.emplace_back(ellipse0.arc_point0());
            }
            if (Geo::distance(ellipse0.arc_point1(), ellipse1) < eps)
            {
                points.emplace_back(ellipse0.arc_point1());
            }
        }
        if (ellipse1.is_arc())
        {
            if (Geo::distance(ellipse1.arc_point0(), ellipse0) < eps)
            {
                points.emplace_back(ellipse1.arc_point0());
            }
            if (Geo::distance(ellipse1.arc_point1(), ellipse0) < eps)
            {
                points.emplace_back(ellipse1.arc_point1());
            }
        }
    }

    if (points.empty())
    {
        return 0;
    }

    if (points.size() > 4)
    {
        std::vector<std::tuple<size_t, size_t, double>> distance_table;
        std::vector<size_t> remove_index;
        for (size_t i = 0, count = points.size(); i < count; ++i)
        {
            for (size_t j = i + 1; j < count; ++j)
            {
                distance_table.emplace_back(i, j, Geo::distance_square(points[i], points[j]));
            }
        }
        std::sort(distance_table.begin(), distance_table.end(),
                  [](const std::tuple<size_t, size_t, double> &a, const std::tuple<size_t, size_t, double> &b)
                  { return std::get<2>(a) > std::get<2>(b); });
        while (points.size() - remove_index.size() > 4)
        {
            const size_t index = std::get<1>(distance_table.back());
            remove_index.push_back(index);
            distance_table.pop_back();
            for (size_t i = distance_table.size() - 1; i > 0; --i)
            {
                if (std::get<1>(distance_table[i]) == index || std::get<0>(distance_table[i]) == index)
                {
                    distance_table.erase(distance_table.begin() + i);
                }
            }
            if (std::get<1>(distance_table.front()) == index || std::get<0>(distance_table.front()) == index)
            {
                distance_table.erase(distance_table.begin());
            }
        }
        std::sort(remove_index.begin(), remove_index.end(), std::greater<>());
        for (const size_t index : remove_index)
        {
            points.erase(points.begin() + index);
        }
    }

    const Geo::Point center0 = ellipse0.center(), center1 = ellipse1.center();
    const double theta0 = Geo::angle(ellipse0.a0(), ellipse0.a1()), theta1 = Geo::angle(ellipse1.a0(), ellipse1.a1());
    const double a0 = ellipse0.lengtha(), b0 = ellipse0.lengthb(), a1 = ellipse1.lengtha(), b1 = ellipse1.lengthb();
    const double A0 = std::pow(std::sin(theta0) / b0, 2) + std::pow(std::cos(theta0) / a0, 2),
                 A1 = std::pow(std::sin(theta1) / b1, 2) + std::pow(std::cos(theta1) / a1, 2);
    const double B0 = 2 * (std::pow(1 / a0, 2) - std::pow(1 / b0, 2)) * std::sin(theta0) * std::cos(theta0),
                 B1 = 2 * (std::pow(1 / a1, 2) - std::pow(1 / b1, 2)) * std::sin(theta1) * std::cos(theta1);
    const double C0 = std::pow(std::cos(theta0) / b0, 2) + std::pow(std::sin(theta0) / a0, 2),
                 C1 = std::pow(std::cos(theta1) / b1, 2) + std::pow(std::sin(theta1) / a1, 2);
    const double D0 = -(2 * A0 * center0.x + B0 * center0.y), D1 = -(2 * A1 * center1.x + B1 * center1.y);
    const double E0 = -(2 * C0 * center0.y + B0 * center0.x), E1 = -(2 * C1 * center1.y + B1 * center1.x);
    const double F0 = -(D0 * center0.x + E0 * center0.y) / 2 - 1, F1 = -(D1 * center1.x + E1 * center1.y) / 2 - 1;

    Math::EllipseParameter param;
    param.a[0] = A0, param.a[1] = A1;
    param.b[0] = B0, param.b[1] = B1;
    param.c[0] = C0, param.c[1] = C1;
    param.d[0] = D0, param.d[1] = D1;
    param.e[0] = E0, param.e[1] = E1;
    param.f[0] = F0, param.f[1] = F1;
    const double eps = Geo::EPSILON;
    switch (points.size())
    {
    case 4:
        {
            auto [x, y] = Math::solve_ellipse_ellipse_intersection(param, points[3].x, points[3].y);
            points[3].x = x, points[3].y = y;
            if (ellipse0.is_arc() && Geo::distance(points[3], ellipse0) >= eps)
            {
                points.pop_back();
            }
            else if (ellipse1.is_arc() && Geo::distance(points[3], ellipse1) >= eps)
            {
                points.pop_back();
            }
        }
        [[fallthrough]];
    case 3:
        {
            auto [x, y] = Math::solve_ellipse_ellipse_intersection(param, points[2].x, points[2].y);
            points[2].x = x, points[2].y = y;
            if (ellipse0.is_arc() && Geo::distance(points[2], ellipse0) >= eps)
            {
                points.erase(points.begin() + 2);
            }
            else if (ellipse1.is_arc() && Geo::distance(points[2], ellipse1) >= eps)
            {
                points.erase(points.begin() + 2);
            }
        }
        [[fallthrough]];
    case 2:
        {
            auto [x, y] = Math::solve_ellipse_ellipse_intersection(param, points[1].x, points[1].y);
            points[1].x = x, points[1].y = y;
            if (ellipse0.is_arc() && Geo::distance(points[1], ellipse0) >= eps)
            {
                points.erase(points.begin() + 1);
            }
            else if (ellipse1.is_arc() && Geo::distance(points[1], ellipse1) >= eps)
            {
                points.erase(points.begin() + 1);
            }
        }
        [[fallthrough]];
    case 1:
        {
            auto [x, y] = Math::solve_ellipse_ellipse_intersection(param, points[0].x, points[0].y);
            points[0].x = x, points[0].y = y;
            if (ellipse0.is_arc() && Geo::distance(points[0], ellipse0) >= eps)
            {
                points.erase(points.begin());
            }
            else if (ellipse1.is_arc() && Geo::distance(points[0], ellipse1) >= eps)
            {
                points.erase(points.begin());
            }
        }
        break;
    default:
        break;
    }
    switch (points.size())
    {
    case 4:
        point3 = points[3];
    case 3:
        point2 = points[2];
    case 2:
        point1 = points[1];
    case 1:
        point0 = points[0];
        break;
    default:
        break;
    }
    return points.size();
}

int Geo::is_intersected(const Circle &circle, const Ellipse &ellipse, Point &point0, Point &point1, Point &point2, Point &point3)
{
    if (!Geo::is_intersected(circle.bounding_rect(), ellipse.bounding_rect()))
    {
        return 0;
    }

    const Geo::Polygon &polygon = circle.shape();
    const Geo::Polyline &polyline = ellipse.shape();
    std::vector<Geo::Point> points;
    for (size_t i = 1, count0 = polygon.size(); i < count0; ++i)
    {
        Geo::Point point;
        for (size_t j = 1, count1 = polyline.size(); j < count1; ++j)
        {
            if (Geo::is_intersected(polygon[i - 1], polygon[i], polyline[j - 1], polyline[j], point, false) &&
                std::find(points.cbegin(), points.cend(), point) == points.cend())
            {
                points.emplace_back(point);
            }
        }
    }
    if (ellipse.is_arc())
    {
        const double eps = Geo::EPSILON;
        if (std::abs(Geo::distance(ellipse.arc_point0(), circle) - circle.radius) < eps)
        {
            points.emplace_back(ellipse.arc_point0());
        }
        if (std::abs(Geo::distance(ellipse.arc_point1(), circle) - circle.radius) < eps)
        {
            points.emplace_back(ellipse.arc_point1());
        }
    }

    if (points.empty())
    {
        return 0;
    }

    if (points.size() > 4)
    {
        std::vector<std::tuple<size_t, size_t, double>> distance_table;
        std::vector<size_t> remove_index;
        for (size_t i = 0, count = points.size(); i < count; ++i)
        {
            for (size_t j = i + 1; j < count; ++j)
            {
                distance_table.emplace_back(i, j, Geo::distance_square(points[i], points[j]));
            }
        }
        std::sort(distance_table.begin(), distance_table.end(),
                  [](const std::tuple<size_t, size_t, double> &a, const std::tuple<size_t, size_t, double> &b)
                  { return std::get<2>(a) > std::get<2>(b); });
        while (points.size() - remove_index.size() > 4)
        {
            const size_t index = std::get<1>(distance_table.back());
            remove_index.push_back(index);
            distance_table.pop_back();
            for (size_t i = distance_table.size() - 1; i > 0; --i)
            {
                if (std::get<1>(distance_table[i]) == index || std::get<0>(distance_table[i]) == index)
                {
                    distance_table.erase(distance_table.begin() + i);
                }
            }
            if (std::get<1>(distance_table.front()) == index || std::get<0>(distance_table.front()) == index)
            {
                distance_table.erase(distance_table.begin());
            }
        }
        std::sort(remove_index.begin(), remove_index.end(), std::greater<>());
        for (const size_t index : remove_index)
        {
            points.erase(points.begin() + index);
        }
    }

    const Geo::Point &center0 = circle;
    const Geo::Point center1 = ellipse.center();
    const double theta0 = 0, theta1 = Geo::angle(ellipse.a0(), ellipse.a1());
    const double a0 = circle.radius, b0 = circle.radius, a1 = ellipse.lengtha(), b1 = ellipse.lengthb();
    const double A0 = std::pow(1 / a0, 2), A1 = std::pow(std::sin(theta1) / b1, 2) + std::pow(std::cos(theta1) / a1, 2);
    const double B0 = 2 * (std::pow(1 / a0, 2) - std::pow(1 / b0, 2)) * std::sin(theta0) * std::cos(theta0),
                 B1 = 2 * (std::pow(1 / a1, 2) - std::pow(1 / b1, 2)) * std::sin(theta1) * std::cos(theta1);
    const double C0 = std::pow(1 / b0, 2), C1 = std::pow(std::cos(theta1) / b1, 2) + std::pow(std::sin(theta1) / a1, 2);
    const double D0 = -(2 * A0 * center0.x + B0 * center0.y), D1 = -(2 * A1 * center1.x + B1 * center1.y);
    const double E0 = -(2 * C0 * center0.y + B0 * center0.x), E1 = -(2 * C1 * center1.y + B1 * center1.x);
    const double F0 = -(D0 * center0.x + E0 * center0.y) / 2 - 1, F1 = -(D1 * center1.x + E1 * center1.y) / 2 - 1;

    Math::EllipseParameter param;
    param.a[0] = A0, param.a[1] = A1;
    param.b[0] = B0, param.b[1] = B1;
    param.c[0] = C0, param.c[1] = C1;
    param.d[0] = D0, param.d[1] = D1;
    param.e[0] = E0, param.e[1] = E1;
    param.f[0] = F0, param.f[1] = F1;
    const double eps = Geo::EPSILON;
    switch (points.size())
    {
    case 4:
        {
            auto [x, y] = Math::solve_ellipse_ellipse_intersection(param, points[3].x, points[3].y);
            points[3].x = x, points[3].y = y;
            if (ellipse.is_arc() && Geo::distance(points[3], ellipse) >= eps)
            {
                points.pop_back();
            }
        }
        [[fallthrough]];
    case 3:
        {
            auto [x, y] = Math::solve_ellipse_ellipse_intersection(param, points[2].x, points[2].y);
            points[2].x = x, points[2].y = y;
            if (ellipse.is_arc() && Geo::distance(points[2], ellipse) >= eps)
            {
                points.erase(points.begin() + 2);
            }
        }
        [[fallthrough]];
    case 2:
        {
            auto [x, y] = Math::solve_ellipse_ellipse_intersection(param, points[1].x, points[1].y);
            points[1].x = x, points[1].y = y;
            if (ellipse.is_arc() && Geo::distance(points[1], ellipse) >= eps)
            {
                points.erase(points.begin() + 1);
            }
        }
        [[fallthrough]];
    case 1:
        {
            auto [x, y] = Math::solve_ellipse_ellipse_intersection(param, points[0].x, points[0].y);
            points[0].x = x, points[0].y = y;
            if (ellipse.is_arc() && Geo::distance(points[0], ellipse) >= eps)
            {
                points.erase(points.begin());
            }
        }
        break;
    default:
        break;
    }
    switch (points.size())
    {
    case 4:
        point3 = points[3];
    case 3:
        point2 = points[2];
    case 2:
        point1 = points[1];
    case 1:
        point0 = points[0];
        break;
    default:
        break;
    }
    return points.size();
}

int Geo::is_intersected(const Ellipse &ellipse, const Arc &arc, Point &point0, Point &point1, Point &point2, Point &point3)
{
    const double eps = Geo::EPSILON;
    switch (Geo::is_intersected(Geo::Circle(arc.x, arc.y, arc.radius), ellipse, point0, point1, point2, point3))
    {
    case 1:
        if (Geo::distance(point0, arc) < eps)
        {
            return 1;
        }
        break;
    case 2:
        {
            int count = 0;
            if (Geo::distance(point0, arc) < eps)
            {
                ++count;
            }
            else
            {
                point0 = point1;
            }
            if (Geo::distance(point1, arc) < eps)
            {
                ++count;
            }
            return count;
        }
        break;
    case 3:
        {
            std::vector<Geo::Point> result;
            if (Geo::distance(point0, arc) < eps)
            {
                result.emplace_back(point0);
            }
            if (Geo::distance(point1, arc) < eps)
            {
                result.emplace_back(point1);
            }
            if (Geo::distance(point2, arc) < eps)
            {
                result.emplace_back(point2);
            }
            switch (result.size())
            {
            case 2:
                point1 = result[1];
                [[fallthrough]];
            case 1:
                point0 = result[0];
                break;
            default:
                break;
            }
            return result.size();
        }
        break;
    case 4:
        {
            std::vector<Geo::Point> result;
            if (Geo::distance(point0, arc) < eps)
            {
                result.emplace_back(point0);
            }
            if (Geo::distance(point1, arc) < eps)
            {
                result.emplace_back(point1);
            }
            if (Geo::distance(point2, arc) < eps)
            {
                result.emplace_back(point2);
            }
            if (Geo::distance(point3, arc) < eps)
            {
                result.emplace_back(point3);
            }
            switch (result.size())
            {
            case 3:
                point2 = result[2];
                [[fallthrough]];
            case 2:
                point1 = result[1];
                [[fallthrough]];
            case 1:
                point0 = result[0];
                break;
            default:
                break;
            }
            return result.size();
        }
        break;
    default:
        break;
    }
    return 0;
}

int Geo::is_intersected(const Arc &arc0, const Arc &arc1, Point &point0, Point &point1)
{
    switch (Geo::is_intersected(Geo::Circle(arc0.x, arc0.y, arc0.radius), Geo::Circle(arc1.x, arc1.y, arc1.radius), point0, point1))
    {
    case 1:
        if (Geo::distance(point0, arc0) < Geo::EPSILON && Geo::distance(point0, arc1) < Geo::EPSILON)
        {
            return 1;
        }
        else
        {
            return 0;
        }
        break;
    case 2:
        {
            int count = 0;
            if (Geo::distance(point0, arc0) < Geo::EPSILON && Geo::distance(point0, arc1) < Geo::EPSILON)
            {
                ++count;
            }
            else
            {
                point0 = point1;
            }
            if (Geo::distance(point1, arc0) < Geo::EPSILON && Geo::distance(point1, arc1) < Geo::EPSILON)
            {
                ++count;
            }
            return count;
        }
        break;
    default:
        return 0;
    }
}

int Geo::is_intersected(const Circle &circle, const CubicBezier &bezier, std::vector<Point> &intersections,
                        std::vector<std::tuple<size_t, double, double, double>> *tvalues)
{
    const int order = 3;
    const int nums[4] = {1, 3, 3, 1};
    const size_t size = intersections.size();
    std::vector<Geo::Point> result;
    for (size_t i = 0, end = bezier.size() - order; i < end; i += order)
    {
        Geo::Polyline polyline;
        polyline.append(bezier[i]);
        double t = 0;
        while (t <= 1)
        {
            Geo::Point point;
            for (int j = 0; j <= order; ++j)
            {
                point += (bezier[j + i] * (nums[j] * std::pow(1 - t, order - j) * std::pow(t, j)));
            }
            polyline.append(point);
            t += Geo::CubicBezier::default_step;
        }
        polyline.append(bezier[i + order]);
        Geo::down_sampling(polyline, Geo::CubicBezier::default_down_sampling_value);

        if (!Geo::is_intersected(polyline.bounding_rect(), circle.bounding_rect()))
        {
            continue;
        }

        std::vector<Geo::Point> temp;
        for (size_t j = 1, count = polyline.size(); j < count; ++j)
        {
            Geo::Point point0, point1;
            switch (Geo::is_intersected(polyline[j - 1], polyline[j], circle, point0, point1, false))
            {
            case 2:
                if (std::find(temp.begin(), temp.end(), point1) == temp.end())
                {
                    temp.emplace_back(point1);
                }
                [[fallthrough]];
            case 1:
                if (std::find(temp.begin(), temp.end(), point0) == temp.end())
                {
                    temp.emplace_back(point0);
                }
                break;
            default:
                break;
            }
        }

        for (Geo::Point &point : temp)
        {
            t = 0;
            double step = 1e-3, lower = 0, upper = 1;
            double min_dis = DBL_MAX;
            while (min_dis > 1e-4 && step > 1e-12)
            {
                for (double x = lower; x < upper + step; x += step)
                {
                    x = x < upper ? x : upper;
                    Geo::Point coord;
                    for (int j = 0; j <= order; ++j)
                    {
                        coord += (bezier[j + i] * (nums[j] * std::pow(1 - x, order - j) * std::pow(x, j)));
                    }
                    if (double dis = Geo::distance(point, coord); dis < min_dis)
                    {
                        min_dis = dis;
                        t = x;
                    }
                }
                lower = std::max(0.0, t - step);
                upper = std::min(1.0, t + step);
                step = (upper - lower) / 100;
            }

            lower = std::max(0.0, t - 1e-4), upper = std::min(1.0, t + 1e-4);
            step = (upper - lower) / 100;
            min_dis = DBL_MAX;
            std::vector<double> stored_t;
            while ((upper - lower) * 1e15 > 1)
            {
                int flag = 0;
                for (double x = lower, dis0 = 0; x < upper + step; x += step)
                {
                    x = x < upper ? x : upper;
                    Geo::Point coord;
                    for (int j = 0; j <= order; ++j)
                    {
                        coord += (bezier[j + i] * (nums[j] * std::pow(1 - x, order - j) * std::pow(x, j)));
                    }
                    if (const double dis = std::abs(Geo::distance(coord, circle) - circle.radius) * 1e9; dis < min_dis)
                    {
                        min_dis = dis;
                        t = x;
                    }
                    else if (dis == min_dis) // 需要扩大搜索范围
                    {
                        flag = -1;
                        break;
                    }
                    else
                    {
                        if (dis == dis0)
                        {
                            if (++flag == 10)
                            {
                                break; // 连续10次相等就退出循环
                            }
                        }
                        else
                        {
                            flag = 0;
                        }
                        dis0 = dis;
                    }
                }
                if (min_dis < 2e-5)
                {
                    break;
                }
                else if (flag == -1) // 需要扩大搜索范围
                {
                    if (t - lower < upper - t)
                    {
                        lower = std::max(0.0, lower - step * 2);
                        if (stored_t.size() > 3 && stored_t[0] == stored_t[2] && stored_t[1] == stored_t[3])
                        {
                            stored_t.clear();
                            lower += (upper - lower) / 4;
                        }
                    }
                    else
                    {
                        upper = std::min(1.0, upper + step * 2);
                        if (stored_t.size() > 3 && stored_t[0] == stored_t[2] && stored_t[1] == stored_t[3])
                        {
                            stored_t.clear();
                            upper -= (upper - lower) / 4;
                        }
                    }
                    stored_t.push_back(t);
                    if (stored_t.size() > 4)
                    {
                        stored_t.erase(stored_t.begin(), stored_t.end() - 4);
                    }
                    step = (upper - lower) / 100;
                }
                else
                {
                    lower = std::max(0.0, t - step * 2);
                    upper = std::min(1.0, t + step * 2);
                    step = (upper - lower) / 100;
                }
            }

            point.clear();
            for (int j = 0; j <= order; ++j)
            {
                point += (bezier[j + i] * (nums[j] * std::pow(1 - t, order - j) * std::pow(t, j)));
            }
            if (std::find(result.begin(), result.end(), point) == result.end() &&
                std::abs(Geo::distance(point, circle) - circle.radius) < Geo::EPSILON)
            {
                result.emplace_back(point);
                if (tvalues != nullptr)
                {
                    tvalues->emplace_back(i, t, point.x, point.y);
                }
            }
        }
    }
    if (std::abs(Geo::distance(bezier.front(), circle) - circle.radius) < Geo::EPSILON &&
        std::find(result.begin(), result.end(), bezier.front()) == result.end())
    {
        result.emplace_back(bezier.front());
        if (tvalues != nullptr)
        {
            tvalues->emplace_back(0, 0, bezier.front().x, bezier.front().y);
        }
    }
    if (std::abs(Geo::distance(bezier.back(), circle) - circle.radius) < Geo::EPSILON &&
        std::find(result.begin(), result.end(), bezier.back()) == result.end())
    {
        result.emplace_back(bezier.back());
        if (tvalues != nullptr)
        {
            tvalues->emplace_back(bezier.size() / (order + 1) - 1, 1, bezier.back().x, bezier.back().y);
        }
    }


    intersections.insert(intersections.end(), result.begin(), result.end());
    return intersections.size() - size;
}

int Geo::is_intersected(const Circle &circle, const BSpline &bspline, const bool is_cubic, std::vector<Point> &intersections,
                        std::vector<std::tuple<double, double, double>> *tvalues)
{
    std::vector<Geo::Point> temp_points;
    for (size_t i = 1, count = bspline.shape().size(); i < count; ++i)
    {
        Geo::Point point0, point1;
        switch (Geo::is_intersected(bspline.shape()[i - 1], bspline.shape()[i], circle, point0, point1, false))
        {
        case 2:
            if (std::find(temp_points.begin(), temp_points.end(), point1) == temp_points.end())
            {
                temp_points.emplace_back(point1);
            }
            [[fallthrough]];
        case 1:
            if (std::find(temp_points.begin(), temp_points.end(), point0) == temp_points.end())
            {
                temp_points.emplace_back(point0);
            }
            break;
        default:
            break;
        }
    }

    std::vector<double> values(temp_points.size(), 0);
    const std::vector<double> &knots = bspline.knots();
    const size_t npts = bspline.control_points.size();
    const size_t nplusc = npts + (is_cubic ? 4 : 3);
    {
        const size_t p = std::max(npts * 8.0, bspline.shape().length() / Geo::BSpline::default_step);
        double t = knots[0];
        const double init_step = (knots[nplusc - 1] - t) / (p - 1);
        std::vector<double> min_dis(temp_points.size(), DBL_MAX);
        while (t <= knots[nplusc - 1])
        {
            std::vector<double> nbasis;
            Geo::BSpline::rbasis(is_cubic ? 3 : 2, t, npts, knots, nbasis);
            Geo::Point coord;
            for (size_t i = 0; i < npts; ++i)
            {
                coord += bspline.control_points[i] * nbasis[i];
            }
            for (size_t i = 0, count = temp_points.size(); i < count; ++i)
            {
                if (const double dis = Geo::distance(coord, temp_points[i]); dis < min_dis[i])
                {
                    min_dis[i] = dis;
                    values[i] = t;
                }
            }
            t += init_step;
        }
    }

    const size_t count = intersections.size();
    std::vector<Geo::Point> result;
    for (size_t n = 0, count = values.size(); n < count; ++n)
    {
        Geo::Point &point = temp_points[n];
        double min_dis = DBL_MAX, t = values[n];
        const double min_lower = n > 0 ? values[n - 1] : knots[0];
        const double max_upper = n < count - 1 ? values[n + 1] : knots[nplusc - 1];
        double lower = std::max(min_lower, t - 2e-3);
        double upper = std::min(max_upper, t + 2e-3);
        double step = (upper - lower) / 1000;
        while (min_dis > 1e-4 && step > 1e-12)
        {
            for (double x = lower; x < upper + step; x += step)
            {
                x = x < upper ? x : upper;
                std::vector<double> nbasis;
                Geo::BSpline::rbasis(is_cubic ? 3 : 2, x, npts, knots, nbasis);
                Geo::Point coord;
                for (size_t i = 0; i < npts; ++i)
                {
                    coord += bspline.control_points[i] * nbasis[i];
                }
                if (double dis = Geo::distance(point, coord); dis < min_dis)
                {
                    min_dis = dis;
                    t = x;
                }
            }
            lower = std::max(0.0, t - step);
            upper = std::min(1.0, t + step);
            step = (upper - lower) / 100;
        }

        if (min_dis > 0.1)
        {
            continue;
        }

        lower = std::max(min_lower, t - 1e-4);
        upper = std::min(max_upper, t + 1e-4);
        step = (upper - lower) / 100;
        min_dis = DBL_MAX;
        std::vector<double> stored_t;
        while ((upper - lower) * 1e15 > 1)
        {
            int flag = 0;
            for (double x = lower, dis0 = 0; x < upper + step; x += step)
            {
                x = x < upper ? x : upper;
                std::vector<double> nbasis;
                Geo::BSpline::rbasis(is_cubic ? 3 : 2, x, npts, knots, nbasis);
                Geo::Point coord;
                for (size_t i = 0; i < npts; ++i)
                {
                    coord += bspline.control_points[i] * nbasis[i];
                }
                if (const double dis = std::abs(Geo::distance(coord, circle) - circle.radius) * 1e9; dis < min_dis)
                {
                    min_dis = dis;
                    t = x;
                }
                else if (dis == min_dis)
                {
                    flag = -1; // 需要扩大搜索范围
                    break;
                }
                else
                {
                    if (dis0 == dis)
                    {
                        if (++flag == 10)
                        {
                            break;
                        }
                    }
                    else
                    {
                        flag = 0;
                    }
                    dis0 = dis;
                }
            }
            if (min_dis < 2e-5)
            {
                break;
            }
            else if (flag == -1) // 需要扩大搜索范围
            {
                if (t - lower < upper - t)
                {
                    lower = std::max(min_lower, lower - step * 2);
                    if (stored_t.size() > 3 && stored_t[0] == stored_t[2] && stored_t[1] == stored_t[3])
                    {
                        stored_t.clear();
                        lower += (upper - lower) / 4;
                    }
                }
                else
                {
                    upper = std::min(max_upper, upper + step * 2);
                    if (stored_t.size() > 3 && stored_t[0] == stored_t[2] && stored_t[1] == stored_t[3])
                    {
                        stored_t.clear();
                        upper -= (upper - lower) / 4;
                    }
                }
                stored_t.push_back(t);
                if (stored_t.size() > 4)
                {
                    stored_t.erase(stored_t.begin(), stored_t.end() - 4);
                }
                step = (upper - lower) / 100;
            }
            else
            {
                lower = std::max(min_lower, t - step * 2);
                upper = std::min(max_upper, t + step * 2);
                step = (upper - lower) / 100;
            }
        }

        std::vector<double> nbasis;
        Geo::BSpline::rbasis(is_cubic ? 3 : 2, t, npts, knots, nbasis);
        point.clear();
        for (size_t i = 0; i < npts; ++i)
        {
            point += bspline.control_points[i] * nbasis[i];
        }
        if (std::find(result.begin(), result.end(), point) == result.end() &&
            std::abs(Geo::distance(point, circle) - circle.radius) < Geo::EPSILON)
        {
            result.emplace_back(point);
            if (tvalues != nullptr)
            {
                tvalues->emplace_back(t, point.x, point.y);
            }
        }
    }
    if (std::abs(Geo::distance(bspline.front(), circle) - circle.radius) < Geo::EPSILON &&
        std::find(result.begin(), result.end(), bspline.front()) == result.end())
    {
        result.emplace_back(bspline.front());
        if (tvalues != nullptr)
        {
            tvalues->emplace_back(0, bspline.front().x, bspline.front().y);
        }
    }
    if (std::abs(Geo::distance(bspline.back(), circle) - circle.radius) < Geo::EPSILON &&
        std::find(result.begin(), result.end(), bspline.back()) == result.end())
    {
        result.emplace_back(bspline.back());
        if (tvalues != nullptr)
        {
            tvalues->emplace_back(1, bspline.back().x, bspline.back().y);
        }
    }

    intersections.insert(intersections.end(), result.begin(), result.end());
    return intersections.size() - count;
}

int Geo::is_intersected(const Ellipse &ellipse, const CubicBezier &bezier, std::vector<Point> &intersections,
                        std::vector<std::tuple<size_t, double, double, double>> *tvalues)
{
    const int order = 3;
    const int nums[4] = {1, 3, 3, 1};
    const double eps = Geo::EPSILON;
    const size_t size = intersections.size();
    std::vector<Geo::Point> result;
    for (size_t i = 0, end = bezier.size() - order; i < end; i += order)
    {
        Geo::Polyline polyline;
        polyline.append(bezier[i]);
        double t = 0;
        while (t <= 1)
        {
            Geo::Point point;
            for (int j = 0; j <= order; ++j)
            {
                point += (bezier[j + i] * (nums[j] * std::pow(1 - t, order - j) * std::pow(t, j)));
            }
            polyline.append(point);
            t += Geo::CubicBezier::default_step;
        }
        polyline.append(bezier[i + order]);
        Geo::down_sampling(polyline, Geo::CubicBezier::default_down_sampling_value);

        std::vector<Geo::Point> temp;
        for (size_t j = 1, count = polyline.size(); j < count; ++j)
        {
            Geo::Point point0, point1;
            switch (Geo::is_intersected(polyline[j - 1], polyline[j], ellipse, point0, point1, false))
            {
            case 2:
                if (std::find(temp.begin(), temp.end(), point1) == temp.end())
                {
                    temp.emplace_back(point1);
                }
                [[fallthrough]];
            case 1:
                if (std::find(temp.begin(), temp.end(), point0) == temp.end())
                {
                    temp.emplace_back(point0);
                }
                break;
            default:
                break;
            }
        }

        for (Geo::Point &point : temp)
        {
            t = 0;
            double step = 1e-3, lower = 0, upper = 1;
            double min_dis = DBL_MAX;
            while (min_dis > 1e-4 && step > 1e-12)
            {
                for (double x = lower; x < upper + step; x += step)
                {
                    x = x < upper ? x : upper;
                    Geo::Point coord;
                    for (int j = 0; j <= order; ++j)
                    {
                        coord += (bezier[j + i] * (nums[j] * std::pow(1 - x, order - j) * std::pow(x, j)));
                    }
                    if (double dis = Geo::distance(point, coord); dis < min_dis)
                    {
                        min_dis = dis;
                        t = x;
                    }
                }
                lower = std::max(0.0, t - step);
                upper = std::min(1.0, t + step);
                step = (upper - lower) / 100;
            }

            lower = std::max(0.0, t - 1e-4), upper = std::min(1.0, t + 1e-4);
            step = (upper - lower) / 100;
            min_dis = DBL_MAX;
            std::vector<double> stored_t;
            while ((upper - lower) * 1e15 > 1)
            {
                int flag = 0;
                for (double x = lower, dis0 = 0; x < upper + step; x += step)
                {
                    x = x < upper ? x : upper;
                    Geo::Point coord;
                    for (int j = 0; j <= order; ++j)
                    {
                        coord += (bezier[j + i] * (nums[j] * std::pow(1 - x, order - j) * std::pow(x, j)));
                    }
                    if (const double dis = Geo::distance(coord, ellipse) * 1e18; dis < min_dis)
                    {
                        min_dis = dis;
                        t = x;
                    }
                    else if (dis == min_dis) // 需要扩大搜索范围
                    {
                        flag = -1;
                        break;
                    }
                    else
                    {
                        if (dis == dis0)
                        {
                            if (++flag == 10)
                            {
                                break; // 连续10次相等就退出循环
                            }
                        }
                        else
                        {
                            flag = 0;
                        }
                        dis0 = dis;
                    }
                }
                if (min_dis < 2e-5)
                {
                    break;
                }
                else if (flag == -1) // 需要扩大搜索范围
                {
                    if (t - lower < upper - t)
                    {
                        lower = std::max(0.0, lower - step * 2);
                        if (stored_t.size() > 3 && stored_t[0] == stored_t[2] && stored_t[1] == stored_t[3])
                        {
                            stored_t.clear();
                            lower += (upper - lower) / 4;
                        }
                    }
                    else
                    {
                        upper = std::min(1.0, upper + step * 2);
                        if (stored_t.size() > 3 && stored_t[0] == stored_t[2] && stored_t[1] == stored_t[3])
                        {
                            stored_t.clear();
                            upper -= (upper - lower) / 4;
                        }
                    }
                    stored_t.push_back(t);
                    if (stored_t.size() > 4)
                    {
                        stored_t.erase(stored_t.begin(), stored_t.end() - 4);
                    }
                    step = (upper - lower) / 100;
                }
                else
                {
                    lower = std::max(0.0, t - step * 2);
                    upper = std::min(1.0, t + step * 2);
                    step = (upper - lower) / 100;
                }
            }

            point.clear();
            for (int j = 0; j <= order; ++j)
            {
                point += (bezier[j + i] * (nums[j] * std::pow(1 - t, order - j) * std::pow(t, j)));
            }
            if (std::find(result.begin(), result.end(), point) == result.end() && Geo::distance(point, ellipse) < eps)
            {
                result.emplace_back(point);
                if (tvalues != nullptr)
                {
                    tvalues->emplace_back(i, t, point.x, point.y);
                }
            }
        }
    }
    if (Geo::distance(bezier.front(), ellipse) < eps && std::find(result.begin(), result.end(), bezier.front()) == result.end())
    {
        result.emplace_back(bezier.front());
        if (tvalues != nullptr)
        {
            tvalues->emplace_back(0, 0, bezier.front().x, bezier.front().y);
        }
    }
    if (Geo::distance(bezier.back(), ellipse) < eps && std::find(result.begin(), result.end(), bezier.back()) == result.end())
    {
        result.emplace_back(bezier.back());
        if (tvalues != nullptr)
        {
            tvalues->emplace_back(bezier.size() / (order + 1) - 1, 1, bezier.back().x, bezier.back().y);
        }
    }

    intersections.insert(intersections.end(), result.begin(), result.end());
    return intersections.size() - size;
}

int Geo::is_intersected(const Ellipse &ellipse, const BSpline &bspline, const bool is_cubic, std::vector<Point> &intersections,
                        std::vector<std::tuple<double, double, double>> *tvalues)
{
    std::vector<Geo::Point> temp_points;
    for (size_t i = 1, count = bspline.shape().size(); i < count; ++i)
    {
        Geo::Point point0, point1;
        switch (Geo::is_intersected(bspline.shape()[i - 1], bspline.shape()[i], ellipse, point0, point1, false))
        {
        case 2:
            if (std::find(temp_points.begin(), temp_points.end(), point1) == temp_points.end())
            {
                temp_points.emplace_back(point1);
            }
            [[fallthrough]];
        case 1:
            if (std::find(temp_points.begin(), temp_points.end(), point0) == temp_points.end())
            {
                temp_points.emplace_back(point0);
            }
            break;
        default:
            break;
        }
    }

    std::vector<double> values(temp_points.size(), 0);
    const std::vector<double> &knots = bspline.knots();
    const size_t npts = bspline.control_points.size();
    const size_t nplusc = npts + (is_cubic ? 4 : 3);
    {
        const size_t p = std::max(npts * 8.0, bspline.shape().length() / Geo::BSpline::default_step);
        double t = knots[0];
        const double init_step = (knots[nplusc - 1] - t) / (p - 1);
        std::vector<double> min_dis(temp_points.size(), DBL_MAX);
        while (t <= knots[nplusc - 1])
        {
            std::vector<double> nbasis;
            Geo::BSpline::rbasis(is_cubic ? 3 : 2, t, npts, knots, nbasis);
            Geo::Point coord;
            for (size_t i = 0; i < npts; ++i)
            {
                coord += bspline.control_points[i] * nbasis[i];
            }
            for (size_t i = 0, count = temp_points.size(); i < count; ++i)
            {
                if (const double dis = Geo::distance(coord, temp_points[i]); dis < min_dis[i])
                {
                    min_dis[i] = dis;
                    values[i] = t;
                }
            }
            t += init_step;
        }
    }

    const double eps = Geo::EPSILON;
    const size_t count = intersections.size();
    std::vector<Geo::Point> result;
    for (size_t n = 0, count = values.size(); n < count; ++n)
    {
        Geo::Point &point = temp_points[n];
        double min_dis = DBL_MAX, t = values[n];
        const double min_lower = n > 0 ? values[n - 1] : knots[0];
        const double max_upper = n < count - 1 ? values[n + 1] : knots[nplusc - 1];
        double lower = std::max(min_lower, t - 2e-3);
        double upper = std::min(max_upper, t + 2e-3);
        double step = (upper - lower) / 1000;
        while (min_dis > 1e-4 && step > 1e-12)
        {
            for (double x = lower; x < upper + step; x += step)
            {
                x = x < upper ? x : upper;
                std::vector<double> nbasis;
                Geo::BSpline::rbasis(is_cubic ? 3 : 2, x, npts, knots, nbasis);
                Geo::Point coord;
                for (size_t i = 0; i < npts; ++i)
                {
                    coord += bspline.control_points[i] * nbasis[i];
                }
                if (double dis = Geo::distance(point, coord); dis < min_dis)
                {
                    min_dis = dis;
                    t = x;
                }
            }
            lower = std::max(0.0, t - step);
            upper = std::min(1.0, t + step);
            step = (upper - lower) / 100;
        }

        if (min_dis > 0.1)
        {
            continue;
        }

        lower = std::max(min_lower, t - 1e-4);
        upper = std::min(max_upper, t + 1e-4);
        step = (upper - lower) / 100;
        min_dis = DBL_MAX;
        std::vector<double> stored_t;
        while ((upper - lower) * 1e15 > 1)
        {
            int flag = 0;
            for (double x = lower, dis0 = 0; x < upper + step; x += step)
            {
                x = x < upper ? x : upper;
                std::vector<double> nbasis;
                Geo::BSpline::rbasis(is_cubic ? 3 : 2, x, npts, knots, nbasis);
                Geo::Point coord;
                for (size_t i = 0; i < npts; ++i)
                {
                    coord += bspline.control_points[i] * nbasis[i];
                }
                if (const double dis = Geo::distance(coord, ellipse) * 1e9; dis < min_dis)
                {
                    min_dis = dis;
                    t = x;
                }
                else if (dis == min_dis)
                {
                    flag = -1; // 需要扩大搜索范围
                    break;
                }
                else
                {
                    if (dis0 == dis)
                    {
                        if (++flag == 10)
                        {
                            break;
                        }
                    }
                    else
                    {
                        flag = 0;
                    }
                    dis0 = dis;
                }
            }
            if (min_dis < 2e-5)
            {
                break;
            }
            else if (flag == -1) // 需要扩大搜索范围
            {
                if (t - lower < upper - t)
                {
                    lower = std::max(min_lower, lower - step * 2);
                    if (stored_t.size() > 3 && stored_t[0] == stored_t[2] && stored_t[1] == stored_t[3])
                    {
                        stored_t.clear();
                        lower += (upper - lower) / 4;
                    }
                }
                else
                {
                    upper = std::min(max_upper, upper + step * 2);
                    if (stored_t.size() > 3 && stored_t[0] == stored_t[2] && stored_t[1] == stored_t[3])
                    {
                        stored_t.clear();
                        upper -= (upper - lower) / 4;
                    }
                }
                stored_t.push_back(t);
                if (stored_t.size() > 4)
                {
                    stored_t.erase(stored_t.begin(), stored_t.end() - 4);
                }
                step = (upper - lower) / 100;
            }
            else
            {
                lower = std::max(min_lower, t - step * 2);
                upper = std::min(max_upper, t + step * 2);
                step = (upper - lower) / 100;
            }
        }

        std::vector<double> nbasis;
        Geo::BSpline::rbasis(is_cubic ? 3 : 2, t, npts, knots, nbasis);
        point.clear();
        for (size_t i = 0; i < npts; ++i)
        {
            point += bspline.control_points[i] * nbasis[i];
        }
        if (std::find(result.begin(), result.end(), point) == result.end() && Geo::distance(point, ellipse) < eps)
        {
            result.emplace_back(point);
            if (tvalues != nullptr)
            {
                tvalues->emplace_back(t, point.x, point.y);
            }
        }
    }
    if (Geo::distance(bspline.front(), ellipse) < eps && std::find(result.begin(), result.end(), bspline.front()) == result.end())
    {
        result.emplace_back(bspline.front());
        if (tvalues != nullptr)
        {
            tvalues->emplace_back(0, bspline.front().x, bspline.front().y);
        }
    }
    if (Geo::distance(bspline.back(), ellipse) < eps && std::find(result.begin(), result.end(), bspline.back()) == result.end())
    {
        result.emplace_back(bspline.back());
        if (tvalues != nullptr)
        {
            tvalues->emplace_back(1, bspline.back().x, bspline.back().y);
        }
    }

    intersections.insert(intersections.end(), result.begin(), result.end());
    return intersections.size() - count;
}

int Geo::is_intersected(const Arc &arc, const CubicBezier &bezier, std::vector<Point> &intersections,
                        std::vector<std::tuple<size_t, double, double, double>> *tvalues)
{
    std::vector<Point> temp_intersections;
    std::vector<std::tuple<size_t, double, double, double>> temp_tvalues;
    Geo::is_intersected(Geo::Circle(arc.x, arc.y, arc.radius), bezier, temp_intersections, tvalues == nullptr ? nullptr : &temp_tvalues);
    for (size_t i = 0, count = temp_intersections.size(); i < count; ++i)
    {
        if (Geo::distance(temp_intersections[i], arc) > Geo::EPSILON)
        {
            auto it =
                std::find_if(temp_tvalues.begin(), temp_tvalues.end(), [&](const std::tuple<size_t, double, double, double> &item)
                             { return std::get<2>(item) == temp_intersections[i].x && std::get<3>(item) == temp_intersections[i].y; });
            while (it != temp_tvalues.end())
            {
                temp_tvalues.erase(it);
                it = std::find_if(temp_tvalues.begin(), temp_tvalues.end(), [&](const std::tuple<size_t, double, double, double> &item)
                                  { return std::get<2>(item) == temp_intersections[i].x && std::get<3>(item) == temp_intersections[i].y; });
            }
            temp_intersections.erase(temp_intersections.begin() + i--);
            --count;
        }
    }
    intersections.insert(intersections.end(), temp_intersections.begin(), temp_intersections.end());
    if (tvalues != nullptr)
    {
        tvalues->insert(tvalues->end(), temp_tvalues.begin(), temp_tvalues.end());
    }
    return temp_intersections.size();
}

int Geo::is_intersected(const Arc &arc, const BSpline &bspline, const bool is_cubic, std::vector<Point> &intersections,
                        std::vector<std::tuple<double, double, double>> *tvalues)
{
    std::vector<Point> temp_intersections;
    std::vector<std::tuple<double, double, double>> temp_tvalues;
    Geo::is_intersected(Geo::Circle(arc.x, arc.y, arc.radius), bspline, is_cubic, temp_intersections,
                        tvalues == nullptr ? nullptr : &temp_tvalues);
    for (size_t i = 0, count = temp_intersections.size(); i < count; ++i)
    {
        if (Geo::distance(temp_intersections[i], arc) > Geo::EPSILON)
        {
            auto it =
                std::find_if(temp_tvalues.begin(), temp_tvalues.end(), [&](const std::tuple<double, double, double> &item)
                             { return std::get<1>(item) == temp_intersections[i].x && std::get<2>(item) == temp_intersections[i].y; });
            while (it != tvalues->end())
            {
                temp_tvalues.erase(it);
                it = std::find_if(temp_tvalues.begin(), temp_tvalues.end(), [&](const std::tuple<double, double, double> &item)
                                  { return std::get<1>(item) == temp_intersections[i].x && std::get<2>(item) == temp_intersections[i].y; });
            }
            temp_intersections.erase(temp_intersections.begin() + i--);
            --count;
        }
    }
    intersections.insert(intersections.end(), temp_intersections.begin(), temp_intersections.end());
    if (tvalues != nullptr)
    {
        tvalues->insert(tvalues->end(), temp_tvalues.begin(), temp_tvalues.end());
    }
    return temp_intersections.size();
}

int Geo::is_intersected(const CubicBezier &bezier0, const CubicBezier &bezier1, std::vector<Point> &intersections,
                        std::vector<std::tuple<size_t, double, double, double>> *tvalues0,
                        std::vector<std::tuple<size_t, double, double, double>> *tvalues1)
{
    const int order = 3;
    const int nums[4] = {1, 3, 3, 1};
    const size_t count = intersections.size();
    for (size_t p = 0, end0 = bezier0.size() - order; p < end0; p += order)
    {
        Geo::Polyline polyline0;
        for (double t = 0; t <= 1; t += Geo::CubicBezier::default_step)
        {
            Geo::Point point;
            for (int j = 0; j <= order; ++j)
            {
                point += (bezier0[j + p] * (nums[j] * std::pow(1 - t, order - j) * std::pow(t, j)));
            }
            polyline0.append(point);
        }
        polyline0.append(bezier0[p + order]);
        Geo::down_sampling(polyline0, Geo::CubicBezier::default_down_sampling_value);

        for (size_t q = 0, end1 = bezier1.size() - order; q < end1; q += order)
        {
            Geo::Polyline polyline1;
            for (double t = 0; t <= 1; t += Geo::CubicBezier::default_step)
            {
                Geo::Point point;
                for (int j = 0; j <= order; ++j)
                {
                    point += (bezier1[j + q] * (nums[j] * std::pow(1 - t, order - j) * std::pow(t, j)));
                }
                polyline1.append(point);
            }
            polyline1.append(bezier1[q + order]);
            Geo::down_sampling(polyline1, Geo::CubicBezier::default_down_sampling_value);

            if (!Geo::is_intersected(polyline0.bounding_rect(), polyline1.bounding_rect(), true))
            {
                continue;
            }

            for (size_t a = 1, count0 = polyline0.size(); a < count0; ++a)
            {
                for (size_t b = 1, count1 = polyline1.size(); b < count1; ++b)
                {
                    Geo::Point point;
                    if (!Geo::is_intersected(polyline0[a - 1], polyline0[a], polyline1[b - 1], polyline1[b], point, false))
                    {
                        continue;
                    }

                    double step = 1e-3, lower0 = 0, upper0 = 1;
                    double min_dis = DBL_MAX, t0 = 0;
                    while (min_dis > 1e-8 && step > 1e-15)
                    {
                        for (double x = lower0; x < upper0 + step; x += step)
                        {
                            x = x < upper0 ? x : upper0;
                            Geo::Point coord;
                            for (int j = 0; j <= order; ++j)
                            {
                                coord += (bezier0[j + p] * (nums[j] * std::pow(1 - x, order - j) * std::pow(x, j)));
                            }
                            if (double dis = Geo::distance(point, coord); dis < min_dis)
                            {
                                min_dis = dis;
                                t0 = x;
                            }
                        }
                        lower0 = std::max(0.0, t0 - step);
                        upper0 = std::min(1.0, t0 + step);
                        step = (upper0 - lower0) / 100;
                    }

                    double lower1 = 0, upper1 = 1, t1 = 0;
                    step = 1e-3, min_dis = DBL_MAX;
                    while (min_dis > 1e-8 && step > 1e-15)
                    {
                        for (double x = lower1; x < upper1 + step; x += step)
                        {
                            x = x < upper1 ? x : upper1;
                            Geo::Point coord;
                            for (int j = 0; j <= order; ++j)
                            {
                                coord += (bezier1[j + q] * (nums[j] * std::pow(1 - x, order - j) * std::pow(x, j)));
                            }
                            if (double dis = Geo::distance(point, coord); dis < min_dis)
                            {
                                min_dis = dis;
                                t1 = x;
                            }
                        }
                        lower1 = std::max(0.0, t1 - step);
                        upper1 = std::min(1.0, t1 + step);
                        step = (upper1 - lower1) / 100;
                    }

                    Math::BezierParameter params[2];
                    params[0].order = order;
                    params[0].values = nums;
                    double *points0 = new double[order * 2 + 2];
                    params[0].points = points0;
                    for (int k = 0; k <= order; ++k)
                    {
                        points0[k * 2] = bezier0[k + p].x;
                        points0[k * 2 + 1] = bezier0[k + p].y;
                    }
                    params[1].order = order;
                    params[1].values = nums;
                    double *points1 = new double[order * 2 + 2];
                    params[1].points = points1;
                    for (int k = 0; k <= order; ++k)
                    {
                        points1[k * 2] = bezier1[k + q].x;
                        points1[k * 2 + 1] = bezier1[k + q].y;
                    }
                    std::tuple<double, double> res = Math::solve_curve_intersection(params, Math::CurveIntersectType::BezierBezier, t0, t1);
                    t0 = std::get<0>(res), t1 = std::get<1>(res);
                    delete[] points0;
                    delete[] points1;

                    if (t0 < 0 || t0 > 1 || t1 < 0 || t1 > 1)
                    {
                        continue;
                    }

                    Geo::Point point0, point1;
                    for (int j = 0; j <= order; ++j)
                    {
                        point0 += (bezier0[j + p] * (nums[j] * std::pow(1 - t0, order - j) * std::pow(t0, j)));
                        point1 += (bezier1[j + q] * (nums[j] * std::pow(1 - t1, order - j) * std::pow(t1, j)));
                    }
                    intersections.emplace_back((point0 + point1) / 2);
                    if (tvalues0 != nullptr)
                    {
                        tvalues0->emplace_back(p, t0, point0.x, point0.y);
                    }
                    if (tvalues1 != nullptr)
                    {
                        tvalues1->emplace_back(q, t1, point1.x, point1.y);
                    }
                }
            }
        }
    }
    if (bezier0.front() == bezier1.front())
    {
        if (std::find(intersections.begin(), intersections.end(), bezier0.front()) == intersections.end())
        {
            intersections.emplace_back(bezier0.front());
            if (tvalues0 != nullptr)
            {
                tvalues0->emplace_back(0, 0, bezier0.front().x, bezier0.front().y);
            }
            if (tvalues1 != nullptr)
            {
                tvalues1->emplace_back(0, 0, bezier1.front().x, bezier1.front().y);
            }
        }
    }
    else if (bezier0.front() == bezier1.back())
    {
        if (std::find(intersections.begin(), intersections.end(), bezier0.front()) == intersections.end())
        {
            intersections.emplace_back(bezier0.front());
            if (tvalues0 != nullptr)
            {
                tvalues0->emplace_back(0, 0, bezier0.front().x, bezier0.front().y);
            }
            if (tvalues1 != nullptr)
            {
                tvalues1->emplace_back(bezier1.size() / (order + 1) - 1, 1, bezier1.back().x, bezier1.back().y);
            }
        }
    }
    else if (bezier0.back() == bezier1.front())
    {
        if (std::find(intersections.begin(), intersections.end(), bezier0.back()) == intersections.end())
        {
            intersections.emplace_back(bezier0.back());
            if (tvalues0 != nullptr)
            {
                tvalues0->emplace_back(bezier0.size() / (order + 1) - 1, 1, bezier0.back().x, bezier0.back().y);
            }
            if (tvalues1 != nullptr)
            {
                tvalues1->emplace_back(0, 0, bezier1.front().x, bezier1.front().y);
            }
        }
    }
    else if (bezier0.back() == bezier1.back())
    {
        if (std::find(intersections.begin(), intersections.end(), bezier0.back()) == intersections.end())
        {
            intersections.emplace_back(bezier0.back());
            if (tvalues0 != nullptr)
            {
                tvalues0->emplace_back(bezier0.size() / (order + 1) - 1, 1, bezier0.back().x, bezier0.back().y);
            }
            if (tvalues1 != nullptr)
            {
                tvalues1->emplace_back(bezier1.size() / (order + 1) - 1, 1, bezier1.back().x, bezier1.back().y);
            }
        }
    }

    return intersections.size() - count;
}

int Geo::is_intersected(const BSpline &bspline0, const bool is_cubic0, const BSpline &bspline1, const bool is_cubic1,
                        std::vector<Point> &intersections, std::vector<std::tuple<double, double, double>> *tvalues0,
                        std::vector<std::tuple<double, double, double>> *tvalues1)
{
    std::vector<Geo::Point> temp_points;
    for (size_t i = 1, count0 = bspline0.shape().size(); i < count0; ++i)
    {
        for (size_t j = 1, count1 = bspline1.shape().size(); j < count1; ++j)
        {
            if (Geo::Point point; Geo::is_intersected(bspline0.shape()[i - 1], bspline0.shape()[i], bspline1.shape()[j - 1],
                                                      bspline1.shape()[j], point, false) &&
                                  std::find(temp_points.begin(), temp_points.end(), point) == temp_points.end())
            {
                temp_points.emplace_back(point);
            }
        }
    }

    std::vector<double> values0(temp_points.size(), 0), values1(temp_points.size(), 0);
    const std::vector<double> &knots0 = bspline0.knots(), &knots1 = bspline1.knots();
    const size_t npts0 = bspline0.control_points.size(), npts1 = bspline1.control_points.size();
    const size_t nplusc0 = npts0 + (is_cubic0 ? 4 : 3), nplusc1 = npts1 + (is_cubic1 ? 4 : 3);

    {
        const size_t p = std::max(npts0 * 8.0, bspline0.shape().length() / Geo::BSpline::default_step);
        double t = knots0[0];
        const double init_step = (knots0[nplusc0 - 1] - t) / (p - 1);
        std::vector<double> min_dis(temp_points.size(), DBL_MAX);
        while (t <= knots0[nplusc0 - 1])
        {
            std::vector<double> nbasis;
            Geo::BSpline::rbasis(is_cubic0 ? 3 : 2, t, npts0, knots0, nbasis);
            Geo::Point coord;
            for (size_t i = 0; i < npts0; ++i)
            {
                coord += bspline0.control_points[i] * nbasis[i];
            }
            for (size_t i = 0, count = temp_points.size(); i < count; ++i)
            {
                if (const double dis = Geo::distance(coord, temp_points[i]); dis < min_dis[i])
                {
                    min_dis[i] = dis;
                    values0[i] = t;
                }
            }
            t += init_step;
        }
        /*for (size_t i = 0, count = values0.size(); i < count; ++i)
        {
            t = values0[i];
            double lower = std::max(knots0[0], t - init_step * 2), upper = std::min(knots0[nplusc0 - 1], t + init_step * 2);
            double step = (upper - lower) / 100, min_dis = DBL_MAX, t0 = DBL_MAX;
            while (std::abs(t0 - t) > 1e-15 && step > 1e-15 && upper - lower > 1e-15)
            {
                t = t0;
                for (double x = lower; x < upper + step; x += step)
                {
                    x = x < upper ? x : upper;
                    std::vector<double> nbasis;
                    if (is_cubic0)
                    {
                        Geo::CubicBSpline::rbasis(x, npts0, knots0, nbasis);
                    }
                    else
                    {
                        Geo::QuadBSpline::rbasis(x, npts0, knots0, nbasis);
                    }
                    Geo::Point coord;
                    for (size_t j = 0; j < npts0; ++j)
                    {
                        coord += bspline0.control_points[j] * nbasis[j];
                    }
                    if (double dis = Geo::distance(temp_points[i], coord); dis < min_dis)
                    {
                        min_dis = dis;
                        t0 = x;
                    }
                }
                lower = std::max(knots0[0], t0 - step);
                upper = std::min(knots0[nplusc0 - 1], t0 + step);
                step = (upper - lower) / 100;
            }
            values0[i] = t0;
        }*/
    }
    {
        const size_t p = std::max(npts1 * 8.0, bspline1.shape().length() / Geo::BSpline::default_step);
        double t = knots1[0];
        const double init_step = (knots1[nplusc1 - 1] - t) / (p - 1);
        std::vector<double> min_dis(temp_points.size(), DBL_MAX);
        while (t <= knots1[nplusc1 - 1])
        {
            std::vector<double> nbasis;
            Geo::BSpline::rbasis(is_cubic1 ? 3 : 2, t, npts1, knots1, nbasis);
            Geo::Point coord;
            for (size_t i = 0; i < npts1; ++i)
            {
                coord += bspline1.control_points[i] * nbasis[i];
            }
            for (size_t i = 0, count = temp_points.size(); i < count; ++i)
            {
                if (const double dis = Geo::distance(coord, temp_points[i]); dis < min_dis[i])
                {
                    min_dis[i] = dis;
                    values1[i] = t;
                }
            }
            t += init_step;
        }
        /*for (size_t i = 0, count = values1.size(); i < count; ++i)
        {
            t = values1[i];
            double lower = std::max(knots1[0], t - init_step * 2), upper = std::min(knots1[nplusc1 - 1], t + init_step * 2);
            double step = (upper - lower) / 100, min_dis = DBL_MAX, t0 = DBL_MAX;
            while (std::abs(t0 - t) > 1e-15 && step > 1e-15 && upper - lower > 1e-15)
            {
                t = t0;
                for (double x = lower; x < upper + step; x += step)
                {
                    x = x < upper ? x : upper;
                    std::vector<double> nbasis;
                    if (is_cubic1)
                    {
                        Geo::CubicBSpline::rbasis(x, npts1, knots1, nbasis);
                    }
                    else
                    {
                        Geo::QuadBSpline::rbasis(x, npts1, knots1, nbasis);
                    }
                    Geo::Point coord;
                    for (size_t j = 0; j < npts1; ++j)
                    {
                        coord += bspline1.control_points[j] * nbasis[j];
                    }
                    if (double dis = Geo::distance(temp_points[i], coord); dis < min_dis)
                    {
                        min_dis = dis;
                        t0 = x;
                    }
                }
                lower = std::max(knots1[0], t0 - step);
                upper = std::min(knots1[nplusc1 - 1], t0 + step);
                step = (upper - lower) / 100;
            }
            values1[i] = t0;
        }*/
    }

    const size_t count = intersections.size();
    Math::BSplineParameter params[2];
    params[0].is_cubic = is_cubic0;
    params[0].npts = npts0;
    std::vector<double> points0;
    for (const Geo::Point &point : bspline0.control_points)
    {
        points0.push_back(point.x);
        points0.push_back(point.y);
    }
    params[0].points = points0.data();
    params[0].values = knots0.data();
    params[1].is_cubic = is_cubic1;
    params[1].npts = npts1;
    std::vector<double> points1;
    for (const Geo::Point &point : bspline1.control_points)
    {
        points1.push_back(point.x);
        points1.push_back(point.y);
    }
    params[1].points = points1.data();
    params[1].values = knots1.data();
    for (size_t i = 0, count = values0.size(); i < count; ++i)
    {
        auto [t0, t1] = Math::solve_curve_intersection(params, Math::CurveIntersectType::BSplineBSpline, values0[i], values1[i]);
        std::vector<double> nbasis0;
        Geo::BSpline::rbasis(is_cubic0 ? 3 : 2, t0, npts0, knots0, nbasis0);
        Geo::Point point0;
        for (size_t j = 0; j < npts0; ++j)
        {
            point0 += bspline0.control_points[j] * nbasis0[j];
        }
        std::vector<double> nbasis1;
        Geo::BSpline::rbasis(is_cubic1 ? 3 : 2, t1, npts1, knots1, nbasis1);
        Geo::Point point1;
        for (size_t j = 0; j < npts1; ++j)
        {
            point1 += bspline1.control_points[j] * nbasis1[j];
        }
        if (Geo::Point point((point0 + point1) / 2); std::find(intersections.begin(), intersections.end(), point) == intersections.end())
        {
            intersections.emplace_back(point);
            if (tvalues0 != nullptr)
            {
                tvalues0->emplace_back(t0, point0.x, point0.y);
            }
            if (tvalues1 != nullptr)
            {
                tvalues1->emplace_back(t1, point1.x, point1.y);
            }
        }
    }

    if (bspline0.front() == bspline1.front())
    {
        if (std::find(intersections.begin(), intersections.end(), bspline0.front()) == intersections.end())
        {
            intersections.emplace_back(bspline0.front());
            if (tvalues0 != nullptr)
            {
                tvalues0->emplace_back(0, bspline0.front().x, bspline0.front().y);
            }
            if (tvalues1 != nullptr)
            {
                tvalues1->emplace_back(0, bspline1.front().x, bspline1.front().y);
            }
        }
    }
    else if (bspline0.front() == bspline1.back())
    {
        if (std::find(intersections.begin(), intersections.end(), bspline0.front()) == intersections.end())
        {
            intersections.emplace_back(bspline0.front());
            if (tvalues0 != nullptr)
            {
                tvalues0->emplace_back(0, bspline0.front().x, bspline0.front().y);
            }
            if (tvalues1 != nullptr)
            {
                tvalues1->emplace_back(1, bspline1.back().x, bspline1.back().y);
            }
        }
    }
    else if (bspline0.back() == bspline1.front())
    {
        if (std::find(intersections.begin(), intersections.end(), bspline0.back()) == intersections.end())
        {
            intersections.emplace_back(bspline0.back());
            if (tvalues0 != nullptr)
            {
                tvalues0->emplace_back(1, bspline0.back().x, bspline0.back().y);
            }
            if (tvalues1 != nullptr)
            {
                tvalues1->emplace_back(0, bspline1.front().x, bspline1.front().y);
            }
        }
    }
    else if (bspline0.back() == bspline1.back())
    {
        if (std::find(intersections.begin(), intersections.end(), bspline0.back()) == intersections.end())
        {
            intersections.emplace_back(bspline0.back());
            if (tvalues0 != nullptr)
            {
                tvalues0->emplace_back(1, bspline0.back().x, bspline0.back().y);
            }
            if (tvalues1 != nullptr)
            {
                tvalues1->emplace_back(1, bspline1.back().x, bspline1.back().y);
            }
        }
    }

    return intersections.size() - count;
}

int Geo::is_intersected(const CubicBezier &bezier, const BSpline &bspline, const bool is_cubic, std::vector<Point> &intersections,
                        std::vector<std::tuple<size_t, double, double, double>> *tvalues0,
                        std::vector<std::tuple<double, double, double>> *tvalues1)
{
    const int order = 3;
    const int nums[4] = {1, 3, 3, 1};
    std::vector<Geo::Point> temp_points;
    for (size_t i = 1, count0 = bezier.shape().size(); i < count0; ++i)
    {
        for (size_t j = 1, count1 = bspline.shape().size(); j < count1; ++j)
        {
            if (Geo::Point point;
                Geo::is_intersected(bezier.shape()[i - 1], bezier.shape()[i], bspline.shape()[j - 1], bspline.shape()[j], point, false) &&
                std::find(temp_points.begin(), temp_points.end(), point) == temp_points.end())
            {
                temp_points.emplace_back(point);
            }
        }
    }

    std::vector<std::tuple<size_t, double>> bezier_values(temp_points.size(), std::make_tuple(0, 0.0));
    {
        std::vector<double> min_dis(temp_points.size(), DBL_MAX);
        for (size_t i = 0, end = bezier.size() - order; i < end; i += order)
        {
            for (double t = 0; t <= 1; t += 1e-4)
            {
                Geo::Point coord;
                for (int j = 0; j <= order; ++j)
                {
                    coord += (bezier[j + i] * (nums[j] * std::pow(1 - t, order - j) * std::pow(t, j)));
                }
                for (size_t j = 0, count = temp_points.size(); j < count; ++j)
                {
                    if (double dis = Geo::distance(temp_points[j], coord); dis < min_dis[j])
                    {
                        min_dis[j] = dis;
                        bezier_values[j] = std::make_tuple(i, t);
                    }
                }
            }
        }
        for (size_t k = 0, count = bezier_values.size(); k < count; ++k)
        {
            const size_t i = std::get<0>(bezier_values[k]);
            double t = std::get<1>(bezier_values[k]);
            double lower = std::max(0.0, t - 2e-4), upper = std::min(1.0, t + 2e-4);
            double step = (upper - lower) / 100, min_dis = DBL_MAX;
            while (min_dis > 1e-8 && step > 1e-15)
            {
                for (double x = lower; x < upper + step; x += step)
                {
                    x = x < upper ? x : upper;
                    Geo::Point coord;
                    for (int j = 0; j <= order; ++j)
                    {
                        coord += (bezier[j + i] * (nums[j] * std::pow(1 - x, order - j) * std::pow(x, j)));
                    }
                    if (double dis = Geo::distance(temp_points[k], coord); dis < min_dis)
                    {
                        min_dis = dis;
                        t = x;
                    }
                }
                lower = std::max(0.0, t - step);
                upper = std::min(1.0, t + step);
                step = (upper - lower) / 100;
            }
            bezier_values[k] = std::make_tuple(i, t);
        }
    }

    const std::vector<double> &knots = bspline.knots();
    const size_t npts = bspline.control_points.size();
    const size_t nplusc = npts + (is_cubic ? 4 : 3);
    std::vector<double> bspline_values(temp_points.size(), 0);
    {
        const size_t p = std::max(npts * 8.0, bspline.shape().length() / Geo::BSpline::default_step);
        double t = knots[0];
        const double init_step = (knots[nplusc - 1] - t) / (p - 1);
        std::vector<double> min_dis(temp_points.size(), DBL_MAX);
        while (t <= knots[nplusc - 1])
        {
            std::vector<double> nbasis;
            Geo::BSpline::rbasis(is_cubic ? 3 : 2, t, npts, knots, nbasis);
            Geo::Point coord;
            for (size_t i = 0; i < npts; ++i)
            {
                coord += bspline.control_points[i] * nbasis[i];
            }
            for (size_t i = 0, count = temp_points.size(); i < count; ++i)
            {
                if (const double dis = Geo::distance(coord, temp_points[i]); dis < min_dis[i])
                {
                    min_dis[i] = dis;
                    bspline_values[i] = t;
                }
            }
            t += init_step;
        }
    }

    const size_t count = intersections.size();
    Math::BezierBSplineParameter param;
    param.bezier.order = order;
    param.bezier.values = nums;
    param.bspline.is_cubic = is_cubic;
    param.bspline.npts = npts;
    std::vector<double> bspline_points;
    for (const Geo::Point &point : bspline.control_points)
    {
        bspline_points.push_back(point.x);
        bspline_points.push_back(point.y);
    }
    param.bspline.points = bspline_points.data();
    param.bspline.values = knots.data();
    for (size_t i = 0, count = bezier_values.size(); i < count; ++i)
    {
        std::vector<double> bezier_points;
        for (size_t j = 0, k = std::get<0>(bezier_values[i]); j <= order; ++j)
        {
            bezier_points.push_back(bezier[j + k].x);
            bezier_points.push_back(bezier[j + k].y);
        }
        param.bezier.points = bezier_points.data();
        auto [t0, t1] = Math::solve_curve_intersection(&param, Math::CurveIntersectType::BezierBSpline, std::get<1>(bezier_values[i]),
                                                       bspline_values[i]);
        Geo::Point point0;
        for (size_t j = 0, k = std::get<0>(bezier_values[i]); j <= order; ++j)
        {
            point0 += (bezier[j + k] * param.bezier.values[j] * std::pow(1 - t0, order - j) * std::pow(t0, j));
        }
        std::vector<double> nbasis;
        Geo::BSpline::rbasis(is_cubic ? 3 : 2, t1, npts, knots, nbasis);
        Geo::Point point1;
        for (size_t j = 0; j < npts; ++j)
        {
            point1 += bspline.control_points[j] * nbasis[j];
        }
        if (Geo::Point point((point0 + point1) / 2); std::find(intersections.begin(), intersections.end(), point) == intersections.end())
        {
            intersections.emplace_back(point);
            if (tvalues0 != nullptr)
            {
                tvalues0->emplace_back(std::get<0>(bezier_values[i]), t0, point0.x, point0.y);
            }
            if (tvalues1 != nullptr)
            {
                tvalues1->emplace_back(t1, point1.x, point1.y);
            }
        }
    }

    if (bezier.front() == bspline.front())
    {
        if (std::find(intersections.begin(), intersections.end(), bezier.front()) == intersections.end())
        {
            intersections.emplace_back(bezier.front());
            if (tvalues0 != nullptr)
            {
                tvalues0->emplace_back(0, 0, bezier.front().x, bezier.front().y);
            }
            if (tvalues1 != nullptr)
            {
                tvalues1->emplace_back(0, bspline.front().x, bspline.front().y);
            }
        }
    }
    else if (bezier.back() == bspline.front())
    {
        if (std::find(intersections.begin(), intersections.end(), bezier.back()) == intersections.end())
        {
            intersections.emplace_back(bezier.back());
            if (tvalues0 != nullptr)
            {
                tvalues0->emplace_back(bezier.size() / (order + 1) - 1, 1, bezier.back().x, bezier.back().y);
            }
            if (tvalues1 != nullptr)
            {
                tvalues1->emplace_back(0, bspline.front().x, bspline.front().y);
            }
        }
    }
    else if (bezier.front() == bspline.back())
    {
        if (std::find(intersections.begin(), intersections.end(), bezier.front()) == intersections.end())
        {
            intersections.emplace_back(bezier.front());
            if (tvalues0 != nullptr)
            {
                tvalues0->emplace_back(0, 0, bezier.front().x, bezier.front().y);
            }
            if (tvalues1 != nullptr)
            {
                tvalues1->emplace_back(1, bspline.back().x, bspline.back().y);
            }
        }
    }
    else if (bezier.back() == bspline.back())
    {
        if (std::find(intersections.begin(), intersections.end(), bezier.back()) == intersections.end())
        {
            intersections.emplace_back(bezier.back());
            if (tvalues0 != nullptr)
            {
                tvalues0->emplace_back(bezier.size() / (order + 1) - 1, 1, bezier.back().x, bezier.back().y);
            }
            if (tvalues1 != nullptr)
            {
                tvalues1->emplace_back(1, bspline.back().x, bspline.back().y);
            }
        }
    }

    return intersections.size() - count;
}


bool Geo::is_intersected(const AABBRect &rect, const Point &point0, const Point &point1)
{
    if (Geo::is_inside(point0, rect) || Geo::is_inside(point1, rect))
    {
        return true;
    }

    const double x_max = std::max(point0.x, point1.x);
    const double x_min = std::min(point0.x, point1.x);
    const double y_max = std::max(point0.y, point1.y);
    const double y_min = std::min(point0.y, point1.y);

    if (x_max < rect.left() || x_min > rect.right() || y_max < rect.bottom() || y_min > rect.top())
    {
        return false;
    }
    else
    {
        if ((x_min >= rect.left() && x_max <= rect.right()) || (y_min >= rect.bottom() && y_max <= rect.top()))
        {
            return true;
        }
        else
        {
            Geo::Point point;
            return Geo::is_intersected(rect[0], rect[2], point0, point1, point) ||
                   Geo::is_intersected(rect[1], rect[3], point0, point1, point);
        }
    }
}

bool Geo::is_intersected(const AABBRect &rect, const Polyline &polyline)
{
    if (polyline.empty() || !Geo::is_intersected(rect, polyline.bounding_rect()))
    {
        return false;
    }

    for (const Geo::Point &point : polyline)
    {
        if (Geo::is_inside(point, rect))
        {
            return true;
        }
    }
    for (size_t i = 1, count = polyline.size(); i < count; ++i)
    {
        if (polyline[i - 1] != polyline[i] && Geo::is_intersected(rect, polyline[i - 1], polyline[i]))
        {
            return true;
        }
    }
    return false;
}

bool Geo::is_intersected(const AABBRect &rect, const Polygon &polygon)
{
    if (polygon.empty() || !Geo::is_intersected(rect, polygon.bounding_rect()))
    {
        return false;
    }

    for (const Geo::Point &point : polygon)
    {
        if (Geo::is_inside(point, rect))
        {
            return true;
        }
    }
    for (size_t i = 1, count = polygon.size(); i < count; ++i)
    {
        if (polygon[i - 1] != polygon[i] && Geo::is_intersected(rect, polygon[i - 1], polygon[i]))
        {
            return true;
        }
    }
    return false;
}

bool Geo::is_intersected(const AABBRect &rect, const Circle &circle)
{
    if (circle.empty() || !Geo::is_intersected(rect, circle.bounding_rect()))
    {
        return false;
    }

    for (const Geo::Point &point : circle.shape())
    {
        if (Geo::is_inside(point, rect))
        {
            return true;
        }
    }
    Geo::Point point0, point1;
    for (size_t i = 1; i < 5; ++i)
    {
        if (Geo::is_intersected(rect[i - 1], rect[i], circle, point0, point1))
        {
            return true;
        }
    }
    return false;
}

bool Geo::is_intersected(const AABBRect &rect, const Ellipse &ellipse)
{
    if (ellipse.empty() || !Geo::is_intersected(rect, ellipse.bounding_rect()))
    {
        return false;
    }

    for (const Geo::Point &point : ellipse.shape())
    {
        if (Geo::is_inside(point, rect))
        {
            return true;
        }
    }
    if (ellipse.is_arc())
    {
        Geo::Point point0, point1;
        for (int i = 0; i < 4; ++i)
        {
            if (Geo::is_intersected(rect[i + 1], rect[i], ellipse, point0, point1))
            {
                return true;
            }
        }
    }
    else
    {
        if (Geo::is_inside(ellipse.a0(), rect) || Geo::is_inside(ellipse.a1(), rect) || Geo::is_inside(ellipse.b0(), rect) ||
            Geo::is_inside(ellipse.b1(), rect))
        {
            return true;
        }
        Geo::Point point0, point1;
        for (int i = 0; i < 4; ++i)
        {
            if (Geo::is_intersected(rect[i + 1], rect[i], ellipse, point0, point1))
            {
                return true;
            }
        }
    }
    return false;
}

bool Geo::is_intersected(const AABBRect &rect, const Arc &arc)
{
    if (!Geo::is_intersected(rect, arc.bounding_rect()))
    {
        return false;
    }
    for (const Geo::Point &point : arc.shape())
    {
        if (Geo::is_inside(point, rect, true))
        {
            return true;
        }
    }
    for (int i = 1; i < 5; ++i)
    {
        if (Geo::Point point0, point1; Geo::is_intersected(rect[i - 1], rect[i], arc, point0, point1, false))
        {
            return true;
        }
    }
    return false;
}


bool Geo::is_intersected(const Point &start, const Point &end, const Triangle &triangle, Point &output0, Point &output1)
{
    if (Geo::is_inside(start, end, triangle) || !Geo::is_intersected(triangle.bounding_rect(), start, end))
    {
        return false;
    }

    const bool a = Geo::is_intersected(start, end, triangle[0], triangle[1], output0) ||
                   Geo::is_intersected(start, end, triangle[1], triangle[2], output0) ||
                   Geo::is_intersected(start, end, triangle[0], triangle[2], output0);
    const bool b = Geo::is_intersected(start, end, triangle[0], triangle[2], output1) ||
                   Geo::is_intersected(start, end, triangle[1], triangle[2], output1) ||
                   Geo::is_intersected(start, end, triangle[0], triangle[1], output1);
    return a || b;
}


bool Geo::NoAABBTest::is_intersected(const Geo::Polyline &polyline0, const Geo::Polyline &polyline1)
{
    Geo::Point point;
    for (size_t i = 1, count0 = polyline0.size(); i < count0; ++i)
    {
        for (size_t j = 1, count1 = polyline1.size(); j < count1; ++j)
        {
            if (polyline0[i - 1] != polyline0[i] && polyline1[j - 1] != polyline1[j] &&
                Geo::is_intersected(polyline0[i - 1], polyline0[i], polyline1[j - 1], polyline1[j], point))
            {
                return true;
            }
        }
    }
    return false;
}

bool Geo::NoAABBTest::is_intersected(const Geo::Polyline &polyline, const Geo::Polygon &polygon, const bool inside)
{
    Geo::Point point;
    for (size_t i = 1, count0 = polyline.size(); i < count0; ++i)
    {
        for (size_t j = 1, count1 = polygon.size(); j < count1; ++j)
        {
            if (polyline[i - 1] != polyline[i] && polygon[j - 1] != polygon[j] &&
                Geo::is_intersected(polyline[i - 1], polyline[i], polygon[j - 1], polygon[j], point))
            {
                return true;
            }
            else if (inside && Geo::NoAABBTest::is_inside(polyline[i - 1], polygon))
            {
                return true;
            }
        }
    }
    if (inside)
    {
        return Geo::NoAABBTest::is_inside(polyline.back(), polygon);
    }
    else
    {
        return false;
    }
}

bool Geo::NoAABBTest::is_intersected(const Geo::Polygon &polygon0, const Geo::Polygon &polygon1, const bool inside)
{
    Geo::Point point;
    for (size_t i = 1, count0 = polygon0.size(); i < count0; ++i)
    {
        for (size_t j = 1, count1 = polygon1.size(); j < count1; ++j)
        {
            if (polygon0[i - 1] != polygon0[i] && polygon1[j - 1] != polygon1[j] &&
                Geo::is_intersected(polygon0[i - 1], polygon0[i], polygon1[j - 1], polygon1[j], point))
            {
                return true;
            }
        }
    }
    if (inside)
    {
        for (const Geo::Point &point : polygon0)
        {
            if (Geo::NoAABBTest::is_inside(point, polygon1, true))
            {
                return true;
            }
        }
        for (const Geo::Point &point : polygon1)
        {
            if (Geo::NoAABBTest::is_inside(point, polygon0, true))
            {
                return true;
            }
        }
    }
    return false;
}


bool Geo::find_intersections(const Geo::Polyline &polyline0, const Geo::Polyline &polyline1, const Geo::Point &pos, const double distance,
                             std::vector<Geo::Point> &intersections)
{
    std::vector<size_t> index0, index1;
    for (size_t i = 1, count = polyline0.size(); i < count; ++i)
    {
        if (polyline0[i - 1] != polyline0[i] && Geo::distance(pos, polyline0[i - 1], polyline0[i], false) <= distance)
        {
            index0.push_back(i);
        }
    }
    for (size_t i = 1, count = polyline1.size(); i < count; ++i)
    {
        if (polyline1[i - 1] != polyline1[i] && Geo::distance(pos, polyline1[i - 1], polyline1[i], false) <= distance)
        {
            index1.push_back(i);
        }
    }

    const size_t count = intersections.size();
    for (const size_t i : index0)
    {
        for (const size_t j : index1)
        {
            if (Geo::Point point; Geo::is_intersected(polyline0[i - 1], polyline0[i], polyline1[j - 1], polyline1[j], point, false) &&
                                  Geo::distance(pos, point) <= distance &&
                                  std::find(intersections.begin(), intersections.end(), point) == intersections.end())
            {
                intersections.emplace_back(point);
            }
        }
    }
    return intersections.size() > count;
}

bool Geo::find_intersections(const Geo::Polyline &polyline, const Geo::CubicBezier &bezier, const Geo::Point &pos, const double distance,
                             std::vector<Geo::Point> &intersections)
{
    const size_t size = intersections.size();
    for (size_t i = 1, count = polyline.size(); i < count; ++i)
    {
        if (polyline[i - 1] == polyline[i] || Geo::distance(pos, polyline[i - 1], polyline[i], false) > distance)
        {
            continue;
        }

        std::vector<Geo::Point> temp;
        Geo::is_intersected(polyline[i - 1], polyline[i], bezier, temp, false);
        for (const Geo::Point &point : temp)
        {
            if (Geo::distance(point, pos) <= distance &&
                std::find(intersections.begin(), intersections.end(), point) == intersections.end())
            {
                intersections.emplace_back(point);
            }
        }
    }
    return intersections.size() > size;
}

bool Geo::find_intersections(const Geo::Polyline &polyline, const Geo::BSpline &bspline, const bool is_cubic, const Geo::Point &pos,
                             const double distance, std::vector<Geo::Point> &intersections)
{
    const size_t size = intersections.size();
    for (size_t i = 1, count = polyline.size(); i < count; ++i)
    {
        if (polyline[i - 1] == polyline[i] || Geo::distance(pos, polyline[i - 1], polyline[i], false) > distance)
        {
            continue;
        }

        std::vector<Geo::Point> temp;
        Geo::is_intersected(polyline[i - 1], polyline[i], bspline, is_cubic, temp, false);
        for (const Geo::Point &point : temp)
        {
            if (Geo::distance(point, pos) <= distance &&
                std::find(intersections.begin(), intersections.end(), point) == intersections.end())
            {
                intersections.emplace_back(point);
            }
        }
    }
    return intersections.size() > size;
}

bool Geo::find_intersections(const Geo::Polyline &polyline, const Geo::Circle &circle, const Geo::Point &pos, const double distance,
                             std::vector<Geo::Point> &intersections)
{
    std::vector<size_t> index;
    for (size_t i = 1, count = polyline.size(); i < count; ++i)
    {
        if (polyline[i - 1] != polyline[i] && Geo::distance(pos, polyline[i - 1], polyline[i], false) <= distance)
        {
            index.push_back(i);
        }
    }

    const size_t count = intersections.size();
    for (const size_t i : index)
    {
        Geo::Point point0, point1;
        switch (Geo::is_intersected(polyline[i - 1], polyline[i], circle, point0, point1, false))
        {
        case 2:
            if (Geo::distance(pos, point1) <= distance)
            {
                intersections.emplace_back(point1);
            }
            [[fallthrough]];
        case 1:
            if (Geo::distance(pos, point0) <= distance)
            {
                intersections.emplace_back(point0);
            }
            break;
        default:
            break;
        }
    }
    return intersections.size() > count;
}

bool Geo::find_intersections(const Geo::Polyline &polyline, const Geo::Ellipse &ellipse, const Geo::Point &pos, const double distance,
                             std::vector<Geo::Point> &intersections)
{
    std::vector<size_t> index;
    for (size_t i = 1, count = polyline.size(); i < count; ++i)
    {
        if (polyline[i - 1] != polyline[i] && Geo::distance(pos, polyline[i - 1], polyline[i], false) <= distance)
        {
            index.push_back(i);
        }
    }

    const size_t count = intersections.size();
    for (const size_t i : index)
    {
        Geo::Point point0, point1;
        switch (Geo::is_intersected(polyline[i - 1], polyline[i], ellipse, point0, point1, true))
        {
        case 2:
            if (Geo::distance(pos, point1) <= distance)
            {
                intersections.emplace_back(point1);
            }
            [[fallthrough]];
        case 1:
            if (Geo::distance(pos, point0) <= distance)
            {
                intersections.emplace_back(point0);
            }
            break;
        default:
            break;
        }
    }
    return intersections.size() > count;
}

bool Geo::find_intersections(const Geo::Polyline &polyline, const Geo::Arc &arc, const Geo::Point &pos, const double distance,
                             std::vector<Geo::Point> &intersections)
{
    std::vector<size_t> index;
    for (size_t i = 1, count = polyline.size(); i < count; ++i)
    {
        if (polyline[i - 1] != polyline[i] && Geo::distance(pos, polyline[i - 1], polyline[i], false) <= distance)
        {
            index.push_back(i);
        }
    }

    const size_t count = intersections.size();
    for (const size_t i : index)
    {
        Geo::Point point0, point1;
        switch (Geo::is_intersected(polyline[i - 1], polyline[i], arc, point0, point1, false))
        {
        case 2:
            if (Geo::distance(pos, point1) <= distance &&
                std::find(intersections.begin(), intersections.end(), point1) == intersections.end())
            {
                intersections.emplace_back(point1);
            }
            [[fallthrough]];
        case 1:
            if (Geo::distance(pos, point0) <= distance &&
                std::find(intersections.begin(), intersections.end(), point0) == intersections.end())
            {
                intersections.emplace_back(point0);
            }
            break;
        default:
            break;
        }
    }
    return intersections.size() > count;
}

bool Geo::find_intersections(const Geo::Circle &circle0, const Geo::Circle &circle1, const Geo::Point &pos, const double distance,
                             std::vector<Geo::Point> &intersections)
{
    Geo::Point point0, point1;
    switch (Geo::is_intersected(circle0, circle1, point0, point1))
    {
    case 2:
        if (Geo::distance(pos, point1) <= distance)
        {
            intersections.emplace_back(point1);
        }
        [[fallthrough]];
    case 1:
        if (Geo::distance(pos, point0) <= distance)
        {
            intersections.emplace_back(point0);
        }
        return true;
    default:
        return false;
    }
}

bool Geo::find_intersections(const Geo::Ellipse &ellipse0, const Geo::Ellipse &ellipse1, const Geo::Point &pos, const double distance,
                             std::vector<Geo::Point> &intersections)
{
    std::vector<Geo::Point> points;
    Geo::find_intersections(ellipse0.shape(), ellipse1.shape(), pos, distance, points);
    {
        const double eps = Geo::EPSILON;
        if (ellipse0.is_arc())
        {
            if (const Geo::Point point(ellipse0.arc_point0());
                Geo::distance(point, pos) <= distance && Geo::distance(point, ellipse1) < eps)
            {
                points.emplace_back(ellipse0.arc_point0());
            }
            if (const Geo::Point point(ellipse0.arc_point1());
                Geo::distance(point, pos) <= distance && Geo::distance(point, ellipse1) < eps)
            {
                points.emplace_back(ellipse0.arc_point1());
            }
        }
        if (ellipse1.is_arc())
        {
            if (const Geo::Point point(ellipse1.arc_point0());
                Geo::distance(point, pos) <= distance && Geo::distance(point, ellipse0) < eps)
            {
                points.emplace_back(ellipse1.arc_point0());
            }
            if (const Geo::Point point(ellipse1.arc_point1());
                Geo::distance(point, pos) <= distance && Geo::distance(point, ellipse0) < eps)
            {
                points.emplace_back(ellipse1.arc_point1());
            }
        }
    }
    if (points.empty())
    {
        return false;
    }

    for (size_t i = 0, count = points.size(); i < count; ++i)
    {
        for (size_t j = i + 1; j < count; ++j)
        {
            if (points[i] == points[j])
            {
                points.erase(points.begin() + j);
                --count;
                --j;
            }
        }
    }

    const Geo::Point center0 = ellipse0.center(), center1 = ellipse1.center();
    double theta0 = Geo::angle(ellipse0.a0(), ellipse0.a1()), theta1 = Geo::angle(ellipse1.a0(), ellipse1.a1());
    double a0 = ellipse0.lengtha(), b0 = ellipse0.lengthb(), a1 = ellipse1.lengtha(), b1 = ellipse1.lengthb();
    double A0 = (std::sin(theta0) / b0) * (std::sin(theta0) / b0) + (std::cos(theta0) / a0) * (std::cos(theta0) / a0),
           A1 = (std::sin(theta1) / b1) * (std::sin(theta1) / b1) + (std::cos(theta1) / a1) * (std::cos(theta1) / a1);
    double B0 = 2 * (std::pow(1 / a0, 2) - std::pow(1 / b0, 2)) * std::sin(theta0) * std::cos(theta0),
           B1 = 2 * (std::pow(1 / a1, 2) - std::pow(1 / b1, 2)) * std::sin(theta1) * std::cos(theta1);
    double C0 = (std::cos(theta0) / b0) * (std::cos(theta0) / b0) + (std::sin(theta0) / a0) * (std::sin(theta0) / a0),
           C1 = (std::cos(theta1) / b1) * (std::cos(theta1) / b1) + (std::sin(theta1) / a1) * (std::sin(theta1) / a1);
    double D0 = -(2 * A0 * center0.x + B0 * center0.y), D1 = -(2 * A1 * center1.x + B1 * center1.y);
    double E0 = -(2 * C0 * center0.y + B0 * center0.x), E1 = -(2 * C1 * center1.y + B1 * center1.x);
    double F0 = -(D0 * center0.x + E0 * center0.y) / 2 - 1, F1 = -(D1 * center1.x + E1 * center1.y) / 2 - 1;

    Math::EllipseParameter param;
    param.a[0] = A0, param.a[1] = A1;
    param.b[0] = B0, param.b[1] = B1;
    param.c[0] = C0, param.c[1] = C1;
    param.d[0] = D0, param.d[1] = D1;
    param.e[0] = E0, param.e[1] = E1;
    param.f[0] = F0, param.f[1] = F1;

    for (Geo::Point &point : points)
    {
        auto [x, y] = Math::solve_ellipse_ellipse_intersection(param, point.x, point.y);
        point.x = x, point.y = y;
    }
    intersections.insert(intersections.end(), points.begin(), points.end());
    return true;
}

bool Geo::find_intersections(const Geo::Ellipse &ellipse, const Geo::Circle &circle, const Geo::Point &pos, const double distance,
                             std::vector<Geo::Point> &intersections)
{
    const size_t count0 = intersections.size();
    if (std::vector<Geo::Point> points; Geo::find_intersections(ellipse.shape(), circle.shape(), pos, distance, points))
    {
        intersections.insert(intersections.end(), points.begin(), points.end());
    }
    else
    {
        return false;
    }

    if (intersections.size() > count0)
    {
        const Geo::Point &center0 = circle;
        const Geo::Point center1 = ellipse.center();
        const double theta0 = 0, theta1 = Geo::angle(ellipse.a0(), ellipse.a1());
        const double a0 = circle.radius, b0 = circle.radius, a1 = ellipse.lengtha(), b1 = ellipse.lengthb();
        const double A0 = std::pow(1 / a0, 2), A1 = std::pow(std::sin(theta1) / b1, 2) + std::pow(std::cos(theta1) / a1, 2);
        const double B0 = 2 * (std::pow(1 / a0, 2) - std::pow(1 / b0, 2)) * std::sin(theta0) * std::cos(theta0),
                     B1 = 2 * (std::pow(1 / a1, 2) - std::pow(1 / b1, 2)) * std::sin(theta1) * std::cos(theta1);
        const double C0 = std::pow(1 / b0, 2), C1 = std::pow(std::cos(theta1) / b1, 2) + std::pow(std::sin(theta1) / a1, 2);
        const double D0 = -(2 * A0 * center0.x + B0 * center0.y), D1 = -(2 * A1 * center1.x + B1 * center1.y);
        const double E0 = -(2 * C0 * center0.y + B0 * center0.x), E1 = -(2 * C1 * center1.y + B1 * center1.x);
        const double F0 = -(D0 * center0.x + E0 * center0.y) / 2 - 1, F1 = -(D1 * center1.x + E1 * center1.y) / 2 - 1;

        Math::EllipseParameter param;
        param.a[0] = A0, param.a[1] = A1;
        param.b[0] = B0, param.b[1] = B1;
        param.c[0] = C0, param.c[1] = C1;
        param.d[0] = D0, param.d[1] = D1;
        param.e[0] = E0, param.e[1] = E1;
        param.f[0] = F0, param.f[1] = F1;
        for (size_t i = count0, count1 = intersections.size(); i < count1; ++i)
        {
            auto [x, y] = Math::solve_ellipse_ellipse_intersection(param, intersections[i].x, intersections[i].y);
            intersections[i].x = x, intersections[i].y = y;
        }
        return true;
    }
    else
    {
        return false;
    }
}

bool Geo::find_intersections(const Geo::Geometry *object0, const Geo::Geometry *object1, const Geo::Point &pos, const double distance,
                             std::vector<Geo::Point> &intersections)
{
    switch (object0->type())
    {
    case Geo::Type::POLYLINE:
        switch (object1->type())
        {
        case Geo::Type::POLYLINE:
            return Geo::find_intersections(*static_cast<const Geo::Polyline *>(object0), *static_cast<const Geo::Polyline *>(object1), pos,
                                           distance, intersections);
        case Geo::Type::POLYGON:
            return Geo::find_intersections(*static_cast<const Geo::Polyline *>(object0), *static_cast<const Geo::Polygon *>(object1), pos,
                                           distance, intersections);
        case Geo::Type::CIRCLE:
            return Geo::find_intersections(*static_cast<const Geo::Polyline *>(object0), *static_cast<const Geo::Circle *>(object1), pos,
                                           distance, intersections);
        case Geo::Type::ELLIPSE:
            return Geo::find_intersections(*static_cast<const Geo::Polyline *>(object0), *static_cast<const Geo::Ellipse *>(object1), pos,
                                           distance, intersections);
        case Geo::Type::BEZIER:
            return Geo::find_intersections(*static_cast<const Geo::Polyline *>(object0), *static_cast<const Geo::CubicBezier *>(object1),
                                           pos, distance, intersections);
        case Geo::Type::BSPLINE:
            return Geo::find_intersections(*static_cast<const Geo::Polyline *>(object0), *static_cast<const Geo::BSpline *>(object1),
                                           dynamic_cast<const Geo::CubicBSpline *>(object1), pos, distance, intersections);
        case Geo::Type::ARC:
            return Geo::find_intersections(*static_cast<const Geo::Polyline *>(object0), *static_cast<const Geo::Arc *>(object1), pos,
                                           distance, intersections);
        default:
            break;
        }
        break;
    case Geo::Type::POLYGON:
        switch (object1->type())
        {
        case Geo::Type::POLYLINE:
            return Geo::find_intersections(*static_cast<const Geo::Polyline *>(object1), *static_cast<const Geo::Polygon *>(object0), pos,
                                           distance, intersections);
        case Geo::Type::POLYGON:
            return Geo::find_intersections(*static_cast<const Geo::Polygon *>(object0), *static_cast<const Geo::Polygon *>(object1), pos,
                                           distance, intersections);
        case Geo::Type::CIRCLE:
            return Geo::find_intersections(*static_cast<const Geo::Polygon *>(object0), *static_cast<const Geo::Circle *>(object1), pos,
                                           distance, intersections);
        case Geo::Type::ELLIPSE:
            return Geo::find_intersections(*static_cast<const Geo::Polygon *>(object0), *static_cast<const Geo::Ellipse *>(object1), pos,
                                           distance, intersections);
        case Geo::Type::BEZIER:
            return Geo::find_intersections(*static_cast<const Geo::Polygon *>(object0), *static_cast<const Geo::CubicBezier *>(object1),
                                           pos, distance, intersections);
        case Geo::Type::BSPLINE:
            return Geo::find_intersections(*static_cast<const Geo::Polygon *>(object0), *static_cast<const Geo::BSpline *>(object1),
                                           dynamic_cast<const Geo::CubicBSpline *>(object1), pos, distance, intersections);
        case Geo::Type::ARC:
            return Geo::find_intersections(*static_cast<const Geo::Polygon *>(object0), *static_cast<const Geo::Arc *>(object1), pos,
                                           distance, intersections);
        default:
            break;
        }
        break;
    case Geo::Type::CIRCLE:
        switch (object1->type())
        {
        case Geo::Type::POLYLINE:
            return Geo::find_intersections(*static_cast<const Geo::Polyline *>(object1), *static_cast<const Geo::Circle *>(object0), pos,
                                           distance, intersections);
        case Geo::Type::POLYGON:
            return Geo::find_intersections(*static_cast<const Geo::Polygon *>(object1), *static_cast<const Geo::Circle *>(object0), pos,
                                           distance, intersections);
        case Geo::Type::CIRCLE:
            return Geo::find_intersections(*static_cast<const Geo::Circle *>(object0), *static_cast<const Geo::Circle *>(object1), pos,
                                           distance, intersections);
        case Geo::Type::ELLIPSE:
            return Geo::find_intersections(*static_cast<const Geo::Ellipse *>(object1), *static_cast<const Geo::Circle *>(object0), pos,
                                           distance, intersections);
        case Geo::Type::BEZIER:
            if (std::vector<Geo::Point> temp;
                Geo::is_intersected(*static_cast<const Geo::Circle *>(object0), *static_cast<const Geo::CubicBezier *>(object1), temp))
            {
                const size_t count = intersections.size();
                for (const Geo::Point &point : temp)
                {
                    if (std::find(intersections.begin(), intersections.end(), point) == intersections.end() &&
                        Geo::distance(point, pos) < distance)
                    {
                        intersections.emplace_back(point);
                    }
                }
                return intersections.size() > count;
            }
            else
            {
                return false;
            }
        case Geo::Type::BSPLINE:
            if (std::vector<Geo::Point> temp;
                Geo::is_intersected(*static_cast<const Geo::Circle *>(object0), *static_cast<const Geo::BSpline *>(object1),
                                    dynamic_cast<const Geo::CubicBSpline *>(object1), temp))
            {
                const size_t count = intersections.size();
                for (const Geo::Point &point : temp)
                {
                    if (std::find(intersections.begin(), intersections.end(), point) == intersections.end() &&
                        Geo::distance(point, pos) < distance)
                    {
                        intersections.emplace_back(point);
                    }
                }
                return intersections.size() > count;
            }
            else
            {
                return false;
            }
        case Geo::Type::ARC:
            {
                Geo::Point points[2];
                switch (Geo::is_intersected(*static_cast<const Geo::Circle *>(object0), *static_cast<const Geo::Arc *>(object1), points[0],
                                            points[1]))
                {
                case 2:
                    if (Geo::distance(pos, points[1]) < distance &&
                        std::find(intersections.begin(), intersections.end(), points[1]) == intersections.end())
                    {
                        intersections.emplace_back(points[1]);
                    }
                    [[fallthrough]];
                case 1:
                    if (Geo::distance(pos, points[0]) < distance &&
                        std::find(intersections.begin(), intersections.end(), points[0]) == intersections.end())
                    {
                        intersections.emplace_back(points[0]);
                    }
                    return true;
                default:
                    return false;
                }
            }
        default:
            break;
        }
        break;
    case Geo::Type::ELLIPSE:
        switch (object1->type())
        {
        case Geo::Type::POLYLINE:
            return Geo::find_intersections(*static_cast<const Geo::Polyline *>(object1), *static_cast<const Geo::Ellipse *>(object0), pos,
                                           distance, intersections);
        case Geo::Type::POLYGON:
            return Geo::find_intersections(*static_cast<const Geo::Polygon *>(object1), *static_cast<const Geo::Ellipse *>(object0), pos,
                                           distance, intersections);
        case Geo::Type::CIRCLE:
            return Geo::find_intersections(*static_cast<const Geo::Ellipse *>(object0), *static_cast<const Geo::Circle *>(object1), pos,
                                           distance, intersections);
        case Geo::Type::ELLIPSE:
            return Geo::find_intersections(*static_cast<const Geo::Ellipse *>(object0), *static_cast<const Geo::Ellipse *>(object1), pos,
                                           distance, intersections);
        case Geo::Type::BEZIER:
            if (std::vector<Geo::Point> temp;
                Geo::is_intersected(*static_cast<const Geo::Ellipse *>(object0), *static_cast<const Geo::CubicBezier *>(object1), temp))
            {
                const size_t count = intersections.size();
                for (const Geo::Point &point : temp)
                {
                    if (std::find(intersections.begin(), intersections.end(), point) == intersections.end() &&
                        Geo::distance(point, pos) < distance)
                    {
                        intersections.emplace_back(point);
                    }
                }
                return intersections.size() > count;
            }
            else
            {
                return false;
            }
        case Geo::Type::BSPLINE:
            if (std::vector<Geo::Point> temp;
                Geo::is_intersected(*static_cast<const Geo::Ellipse *>(object0), *static_cast<const Geo::BSpline *>(object1),
                                    dynamic_cast<const Geo::CubicBSpline *>(object1), temp))
            {
                const size_t count = intersections.size();
                for (const Geo::Point &point : temp)
                {
                    if (std::find(intersections.begin(), intersections.end(), point) == intersections.end() &&
                        Geo::distance(point, pos) < distance)
                    {
                        intersections.emplace_back(point);
                    }
                }
                return intersections.size() > count;
            }
            else
            {
                return false;
            }
        case Geo::Type::ARC:
            {
                Geo::Point points[4];
                switch (Geo::is_intersected(*static_cast<const Geo::Ellipse *>(object0), *static_cast<const Geo::Arc *>(object1), points[0],
                                            points[1], points[2], points[3]))
                {
                case 4:
                    if (Geo::distance(points[3], pos) < distance &&
                        std::find(intersections.begin(), intersections.end(), points[3]) == intersections.end())
                    {
                        intersections.emplace_back(points[3]);
                    }
                    [[fallthrough]];
                case 3:
                    if (Geo::distance(points[2], pos) < distance &&
                        std::find(intersections.begin(), intersections.end(), points[2]) == intersections.end())
                    {
                        intersections.emplace_back(points[2]);
                    }
                    [[fallthrough]];
                case 2:
                    if (Geo::distance(points[1], pos) < distance &&
                        std::find(intersections.begin(), intersections.end(), points[1]) == intersections.end())
                    {
                        intersections.emplace_back(points[1]);
                    }
                    [[fallthrough]];
                case 1:
                    if (Geo::distance(points[0], pos) < distance &&
                        std::find(intersections.begin(), intersections.end(), points[0]) == intersections.end())
                    {
                        intersections.emplace_back(points[0]);
                    }
                    return true;
                default:
                    return false;
                }
            }
            break;
        default:
            break;
        }
        break;
    case Geo::Type::BEZIER:
        switch (object1->type())
        {
        case Geo::Type::POLYLINE:
            return Geo::find_intersections(*static_cast<const Geo::Polyline *>(object1), *static_cast<const Geo::CubicBezier *>(object0),
                                           pos, distance, intersections);
        case Geo::Type::POLYGON:
            return Geo::find_intersections(*static_cast<const Geo::Polygon *>(object1), *static_cast<const Geo::CubicBezier *>(object0),
                                           pos, distance, intersections);
        case Geo::Type::CIRCLE:
            if (std::vector<Geo::Point> temp;
                Geo::is_intersected(*static_cast<const Geo::Circle *>(object1), *static_cast<const Geo::CubicBezier *>(object0), temp))
            {
                const size_t count = intersections.size();
                for (const Geo::Point &point : temp)
                {
                    if (std::find(intersections.begin(), intersections.end(), point) == intersections.end() &&
                        Geo::distance(point, pos) < distance)
                    {
                        intersections.emplace_back(point);
                    }
                }
                return intersections.size() > count;
            }
            else
            {
                return false;
            }
        case Geo::Type::ELLIPSE:
            if (std::vector<Geo::Point> temp;
                Geo::is_intersected(*static_cast<const Geo::Ellipse *>(object1), *static_cast<const Geo::CubicBezier *>(object0), temp))
            {
                const size_t count = intersections.size();
                for (const Geo::Point &point : temp)
                {
                    if (std::find(intersections.begin(), intersections.end(), point) == intersections.end() &&
                        Geo::distance(point, pos) < distance)
                    {
                        intersections.emplace_back(point);
                    }
                }
                return intersections.size() > count;
            }
            else
            {
                return false;
            }
        case Geo::Type::BEZIER:
            if (std::vector<Geo::Point> temp;
                Geo::is_intersected(*static_cast<const Geo::CubicBezier *>(object0), *static_cast<const Geo::CubicBezier *>(object1), temp))
            {
                const size_t count = intersections.size();
                for (const Geo::Point &point : temp)
                {
                    if (std::find(intersections.begin(), intersections.end(), point) == intersections.end() &&
                        Geo::distance(point, pos) < distance)
                    {
                        intersections.emplace_back(point);
                    }
                }
                return intersections.size() > count;
            }
            else
            {
                return false;
            }
        case Geo::Type::BSPLINE:
            if (std::vector<Geo::Point> temp;
                Geo::is_intersected(*static_cast<const Geo::CubicBezier *>(object0), *static_cast<const Geo::BSpline *>(object1),
                                    dynamic_cast<const Geo::CubicBSpline *>(object1), temp))
            {
                const size_t count = intersections.size();
                for (const Geo::Point &point : temp)
                {
                    if (std::find(intersections.begin(), intersections.end(), point) == intersections.end() &&
                        Geo::distance(point, pos) < distance)
                    {
                        intersections.emplace_back(point);
                    }
                }
                return intersections.size() > count;
            }
            else
            {
                return false;
            }
        case Geo::Type::ARC:
            if (std::vector<Geo::Point> temp;
                Geo::is_intersected(*static_cast<const Geo::Arc *>(object1), *static_cast<const Geo::CubicBezier *>(object0), temp))
            {
                const size_t count = intersections.size();
                for (const Geo::Point &point : temp)
                {
                    if (std::find(intersections.begin(), intersections.end(), point) == intersections.end() &&
                        Geo::distance(point, pos) < distance)
                    {
                        intersections.emplace_back(point);
                    }
                }
                return intersections.size() > count;
            }
            else
            {
                return false;
            }
        default:
            break;
        }
        break;
    case Geo::Type::BSPLINE:
        switch (object1->type())
        {
        case Geo::Type::POLYLINE:
            return Geo::find_intersections(*static_cast<const Geo::Polyline *>(object1), *static_cast<const Geo::BSpline *>(object0),
                                           dynamic_cast<const Geo::CubicBSpline *>(object0), pos, distance, intersections);
        case Geo::Type::POLYGON:
            return Geo::find_intersections(*static_cast<const Geo::Polygon *>(object1), *static_cast<const Geo::BSpline *>(object0),
                                           dynamic_cast<const Geo::CubicBSpline *>(object0), pos, distance, intersections);
        case Geo::Type::CIRCLE:
            if (std::vector<Geo::Point> temp;
                Geo::is_intersected(*static_cast<const Geo::Circle *>(object1), *static_cast<const Geo::BSpline *>(object0),
                                    dynamic_cast<const Geo::CubicBSpline *>(object0), temp))
            {
                const size_t count = intersections.size();
                for (const Geo::Point &point : temp)
                {
                    if (std::find(intersections.begin(), intersections.end(), point) == intersections.end() &&
                        Geo::distance(point, pos) < distance)
                    {
                        intersections.emplace_back(point);
                    }
                }
                return intersections.size() > count;
            }
            else
            {
                return false;
            }
        case Geo::Type::ELLIPSE:
            if (std::vector<Geo::Point> temp;
                Geo::is_intersected(*static_cast<const Geo::Ellipse *>(object1), *static_cast<const Geo::BSpline *>(object0),
                                    dynamic_cast<const Geo::CubicBSpline *>(object0), temp))
            {
                const size_t count = intersections.size();
                for (const Geo::Point &point : temp)
                {
                    if (std::find(intersections.begin(), intersections.end(), point) == intersections.end() &&
                        Geo::distance(point, pos) < distance)
                    {
                        intersections.emplace_back(point);
                    }
                }
                return intersections.size() > count;
            }
            else
            {
                return false;
            }
        case Geo::Type::BEZIER:
            if (std::vector<Geo::Point> temp;
                Geo::is_intersected(*static_cast<const Geo::CubicBezier *>(object1), *static_cast<const Geo::BSpline *>(object0),
                                    dynamic_cast<const Geo::CubicBSpline *>(object0), temp))
            {
                const size_t count = intersections.size();
                for (const Geo::Point &point : temp)
                {
                    if (std::find(intersections.begin(), intersections.end(), point) == intersections.end() &&
                        Geo::distance(point, pos) < distance)
                    {
                        intersections.emplace_back(point);
                    }
                }
                return intersections.size() > count;
            }
            else
            {
                return false;
            }
        case Geo::Type::BSPLINE:
            if (std::vector<Geo::Point> temp;
                Geo::is_intersected(*static_cast<const Geo::BSpline *>(object0), dynamic_cast<const Geo::CubicBSpline *>(object0),
                                    *static_cast<const Geo::BSpline *>(object1), dynamic_cast<const Geo::CubicBSpline *>(object1), temp))
            {
                const size_t count = intersections.size();
                for (const Geo::Point &point : temp)
                {
                    if (std::find(intersections.begin(), intersections.end(), point) == intersections.end() &&
                        Geo::distance(point, pos) < distance)
                    {
                        intersections.emplace_back(point);
                    }
                }
                return intersections.size() > count;
            }
            else
            {
                return false;
            }
        case Geo::Type::ARC:
            if (std::vector<Geo::Point> temp;
                Geo::is_intersected(*static_cast<const Geo::Arc *>(object1), *static_cast<const Geo::BSpline *>(object0),
                                    dynamic_cast<const Geo::CubicBSpline *>(object0), temp))
            {
                const size_t count = intersections.size();
                for (const Geo::Point &point : temp)
                {
                    if (std::find(intersections.begin(), intersections.end(), point) == intersections.end() &&
                        Geo::distance(point, pos) < distance)
                    {
                        intersections.emplace_back(point);
                    }
                }
                return intersections.size() > count;
            }
            else
            {
                return false;
            }
        default:
            break;
        }
        break;
    case Geo::Type::ARC:
        switch (object1->type())
        {
        case Geo::Type::POLYLINE:
            return Geo::find_intersections(*static_cast<const Geo::Polyline *>(object1), *static_cast<const Geo::Arc *>(object0), pos,
                                           distance, intersections);
        case Geo::Type::POLYGON:
            return Geo::find_intersections(*static_cast<const Geo::Polygon *>(object1), *static_cast<const Geo::Arc *>(object0), pos,
                                           distance, intersections);
        case Geo::Type::CIRCLE:
            {
                Geo::Point points[2];
                switch (Geo::is_intersected(*static_cast<const Geo::Circle *>(object1), *static_cast<const Geo::Arc *>(object0), points[0],
                                            points[1]))
                {
                case 2:
                    if (Geo::distance(pos, points[1]) < distance &&
                        std::find(intersections.begin(), intersections.end(), points[1]) == intersections.end())
                    {
                        intersections.emplace_back(points[1]);
                    }
                    [[fallthrough]];
                case 1:
                    if (Geo::distance(pos, points[0]) < distance &&
                        std::find(intersections.begin(), intersections.end(), points[0]) == intersections.end())
                    {
                        intersections.emplace_back(points[0]);
                    }
                    return true;
                default:
                    return false;
                }
            }
        case Geo::Type::ELLIPSE:
            {
                Geo::Point points[4];
                switch (Geo::is_intersected(*static_cast<const Geo::Ellipse *>(object1), *static_cast<const Geo::Arc *>(object0), points[0],
                                            points[1], points[2], points[3]))
                {
                case 4:
                    if (Geo::distance(points[3], pos) < distance &&
                        std::find(intersections.begin(), intersections.end(), points[3]) == intersections.end())
                    {
                        intersections.emplace_back(points[3]);
                    }
                    [[fallthrough]];
                case 3:
                    if (Geo::distance(points[2], pos) < distance &&
                        std::find(intersections.begin(), intersections.end(), points[2]) == intersections.end())
                    {
                        intersections.emplace_back(points[2]);
                    }
                    [[fallthrough]];
                case 2:
                    if (Geo::distance(points[1], pos) < distance &&
                        std::find(intersections.begin(), intersections.end(), points[1]) == intersections.end())
                    {
                        intersections.emplace_back(points[1]);
                    }
                    [[fallthrough]];
                case 1:
                    if (Geo::distance(points[0], pos) < distance &&
                        std::find(intersections.begin(), intersections.end(), points[0]) == intersections.end())
                    {
                        intersections.emplace_back(points[0]);
                    }
                    return true;
                default:
                    return false;
                }
            }
        case Geo::Type::BEZIER:
            if (std::vector<Geo::Point> temp;
                Geo::is_intersected(*static_cast<const Geo::Arc *>(object0), *static_cast<const Geo::CubicBezier *>(object1), temp))
            {
                const size_t count = intersections.size();
                for (const Geo::Point &point : temp)
                {
                    if (std::find(intersections.begin(), intersections.end(), point) == intersections.end() &&
                        Geo::distance(point, pos) < distance)
                    {
                        intersections.emplace_back(point);
                    }
                }
                return intersections.size() > count;
            }
            else
            {
                return false;
            }
        case Geo::Type::BSPLINE:
            if (std::vector<Geo::Point> temp;
                Geo::is_intersected(*static_cast<const Geo::Arc *>(object0), *static_cast<const Geo::BSpline *>(object1),
                                    dynamic_cast<const Geo::CubicBSpline *>(object1), temp))
            {
                const size_t count = intersections.size();
                for (const Geo::Point &point : temp)
                {
                    if (std::find(intersections.begin(), intersections.end(), point) == intersections.end() &&
                        Geo::distance(point, pos) < distance)
                    {
                        intersections.emplace_back(point);
                    }
                }
                return intersections.size() > count;
            }
            else
            {
                return false;
            }
        case Geo::Type::ARC:
            {
                Geo::Point points[2];
                switch (Geo::is_intersected(*static_cast<const Geo::Arc *>(object0), *static_cast<const Geo::Arc *>(object1), points[0],
                                            points[1]))
                {
                case 2:
                    if (Geo::distance(pos, points[1]) < distance &&
                        std::find(intersections.begin(), intersections.end(), points[1]) == intersections.end())
                    {
                        intersections.emplace_back(points[1]);
                    }
                    [[fallthrough]];
                case 1:
                    if (Geo::distance(pos, points[0]) < distance &&
                        std::find(intersections.begin(), intersections.end(), points[0]) == intersections.end())
                    {
                        intersections.emplace_back(points[0]);
                    }
                    return true;
                default:
                    return false;
                }
            }
        default:
            break;
        }
    default:
        break;
    }
    return false;
}
