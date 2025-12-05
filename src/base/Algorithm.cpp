#include <cassert>
#include <algorithm>
#include <array>
#include <functional>

#include <EarCut/EarCut.hpp>
#include <clipper2/clipper.h>
#include "base/Algorithm.hpp"
#include "base/Math.hpp"


double Geo::distance(const double x0, const double y0, const double x1, const double y1)
{
    return std::hypot(x0 - x1, y0 - y1);
}

double Geo::distance(const Point &point0, const Point &point1)
{
    return std::hypot(point0.x - point1.x, point0.y - point1.y);
}

double Geo::distance(const Point &point, const Point &start, const Point &end, const bool infinite)
{
    if (start.x == end.x)
    {
        if (infinite)
        {
            return std::abs(point.x - start.x);
        }
        else
        {
            if ((point.y >= start.y && point.y <= end.y) ||
                (point.y <= start.y && point.y >= end.y))
            {
                return std::abs(point.x - start.x);
            }
            else
            {
                return std::min(Geo::distance(point, start), Geo::distance(point, end));
            }
        }
    }
    else if (start.y == end.y)
    {
        if (infinite)
        {
            return std::abs(point.y - start.y);
        }
        else
        {
            if ((point.x >= start.x && point.x <= end.x) ||
                (point.x <= start.x && point.x >= end.x))
            {
                return std::abs(point.y - start.y);
            }
            else
            {
                return std::min(Geo::distance(point, start), Geo::distance(point, end));
            }
        }
    }
    
    const double a = end.y - start.y, 
                b = start.x - end.x,
                c = end.x * start.y - start.x * end.y;
    if (infinite)
    {
        return std::abs(a * point.x + b * point.y + c) / std::hypot(a, b);
    }
    else
    {
        const double k = ((point.x - start.x) * (end.x - start.x) +
            (point.y - start.y) * (end.y - start.y)) /
            (std::pow(end.x - start.x, 2) + std::pow(end.y - start.y, 2)); 
        const double x = start.x + k * (end.x - start.x);

        if ((x >= start.x && x <= end.x) || (x <= start.x && x >= end.x))
        {
            return std::abs(a * point.x + b * point.y + c) / std::hypot(a, b);
        }
        else
        {
            return std::min(Geo::distance(point, start), Geo::distance(point, end));
        }
    }
}

double Geo::distance(const Point &point, const Polyline &polyline)
{
    double dis = Geo::distance(point, polyline.front(), polyline[1]);
    for (size_t i = 2, count = polyline.size(); i < count; ++i)
    {
        dis = std::min(dis, Geo::distance(point, polyline[i - 1], polyline[i]));
    }
    return dis;
}

double Geo::distance(const Point &point, const Bezier &bezier)
{
    const size_t order = bezier.order();
    std::vector<int> nums(order + 1, 1);
    switch (order)
    {
    case 2:
        nums[1] = 2;
        break;
    case 3:
        nums[1] = nums[2] = 3;
        break;
    default:
        {
            std::vector<int> temp(1, 1);
            for (size_t i = 1; i <= order; ++i)
            {
                for (size_t j = 1; j < i; ++j)
                {
                    nums[j] = temp[j - 1] + temp[j];
                }
                temp.assign(nums.begin(), nums.begin() + i + 1);
            }
        }
        break;
    }

    // index, points, distance
    std::vector<std::tuple<size_t, std::vector<Geo::Point>, double>> temp;
    for (size_t i = 0, end = bezier.size() - order; i < end; i += order)
    {
        Geo::Polyline polyline;
        polyline.append(bezier[i]);
        double t = 0;
        while (t <= 1)
        {
            Geo::Point point;
            for (size_t j = 0; j <= order; ++j)
            {
                point += (bezier[j + i] * (nums[j] * std::pow(1 - t, order - j) * std::pow(t, j))); 
            }
            polyline.append(point);
            t += Geo::Bezier::default_step;
        }
        polyline.append(bezier[i + order]);
        Geo::down_sampling(polyline, Geo::Bezier::default_down_sampling_value);

        std::vector<Geo::Point> points;
        Geo::closest_point(polyline, point, points);
        temp.emplace_back(i, points, Geo::distance(point, points.front()));
    }
    std::sort(temp.begin(), temp.end(), [](const auto &a, const auto &b)
        { return std::get<2>(a) < std::get<2>(b); });
    while (temp.size() > 1)
    {
        if (std::get<2>(temp.back()) - std::get<2>(temp.front()) > 1.0)
        {
            temp.pop_back();
        }
        else
        {
            break;
        }
    }

    double result = DBL_MAX;
    for (const auto &[i, points, dis] : temp)
    {
        double t = 0;
        double step = 1e-3, lower = 0, upper = 1;
        double min_dis[2] = {DBL_MAX, DBL_MAX};
        do
        {
            for (double x = lower; x < upper + step; x += step)
            {
                x = x < upper ? x : upper;
                Geo::Point coord;
                for (size_t j = 0; j <= order; ++j)
                {
                    coord += (bezier[j + i] * (nums[j] * std::pow(1 - x, order - j) * std::pow(x, j))); 
                }
                if (double dis = Geo::distance(point, coord); dis < min_dis[1])
                {
                    min_dis[1] = dis;
                    t = x;
                }
            }
            lower = std::max(0.0, t - step);
            upper = std::min(1.0, t + step);
            step = (upper - lower) / 100;
            if (min_dis[0] > min_dis[1])
            {
                min_dis[0] = min_dis[1];
            }
        }
        while (std::abs(min_dis[0] - min_dis[1]) > 1e-4 && step > 1e-12);

        lower = std::max(0.0, t - 0.1), upper = std::min(1.0, t + 0.1);
        step = (upper - lower) / 100;
        min_dis[0] = min_dis[1] = DBL_MAX;
        std::vector<double> stored_t;
        while ((upper - lower) * 1e15 > 1)
        {
            int flag = 0;
            for (double x = lower, dis0 = 0; x < upper + step; x += step)
            {
                x = x < upper ? x : upper;
                Geo::Point coord;
                for (size_t j = 0; j <= order; ++j)
                {
                    coord += (bezier[j + i] * (nums[j] * std::pow(1 - x, order - j) * std::pow(x, j)));
                }
                if (const double dis = Geo::distance(coord, point) * 1e9; dis < min_dis[1])
                {
                    min_dis[1] = dis;
                    t = x;
                }
                else if (dis == min_dis[1]) // 需要扩大搜索范围
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
            if (min_dis[1] < 2e-5)
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

        result = std::min(result, std::min(min_dis[0], min_dis[1]));
    }
    return result;
}

double Geo::distance(const Point &point, const BSpline &bspline, const bool is_cubic)
{
    const std::vector<double> &knots = bspline.knots();
    const size_t npts = bspline.control_points.size();
    const size_t nplusc = npts + (is_cubic ? 4 : 3);
    const size_t p1 = std::max(npts * 8.0, bspline.shape().length() / Geo::BSpline::default_step);

    double t = knots[0];
    const double init_step = (knots[nplusc - 1] - t) / (p1 - 1);
    std::vector<double> temp;
    double min_dis[2] = {DBL_MAX, DBL_MAX};
    while (t <= knots[nplusc - 1])
    {
        std::vector<double> nbasis;
        Geo::BSpline::rbasis(is_cubic ? 3 : 2, t, npts, knots, nbasis);
        Geo::Point coord;
        for (size_t i = 0; i < npts; ++i)
        {
            coord += bspline.control_points[i] * nbasis[i];
        }
        if (double dis = Geo::distance(coord, point); dis < min_dis[0])
        {
            temp.clear();
            min_dis[0] = dis;
            temp.push_back(t);
        }
        else if (dis == min_dis[0])
        {
            temp.push_back(t);
        }
        t += init_step;
    }

    double result = DBL_MAX;
    for (size_t n = 0, count = temp.size(); n < count; ++n)
    {
        t = temp[n];
        const double min_lower = n > 0 ? temp[n - 1] : knots[0];
        const double max_upper = n < count - 1 ? temp[n + 1] : knots[nplusc - 1];
        double lower = std::max(min_lower, t - init_step);
        double upper = std::min(max_upper, t + init_step);
        double step = (upper - lower) / 1000;
        min_dis[0] = min_dis[1] = DBL_MAX;
        do
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
                if (double dis = Geo::distance(point, coord); dis < min_dis[1])
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
        while (std::abs(min_dis[0] - min_dis[1]) > 1e-4 && step > 1e-12);

        min_dis[0] = min_dis[1] = DBL_MAX;
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
                if (double dis = Geo::distance(coord, point); dis < min_dis[1])
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
            if (flag == -1) // 需要扩大搜索范围
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

        result = std::min(result, std::min(min_dis[0], min_dis[1]));
    }
    return result;
}

double Geo::distance(const Point &point, const Polygon &polygon)
{
    double dis = Geo::distance(point, polygon.front(), polygon[1]);
    for (size_t i = 2, count = polygon.size(); i < count; ++i)
    {
        dis = std::min(dis, Geo::distance(point, polygon[i - 1], polygon[i]));
    }
    return dis;
}

double Geo::distance(const Point &point, const Ellipse &ellipse)
{
    if (const Geo::Point center = ellipse.center(); point == ellipse.center())
    {
        if (ellipse.is_arc())
        {
            const Geo::Point start(ellipse.arc_point0()), end(ellipse.arc_point1());
            double angles[4] = { Geo::angle(start, center, ellipse.a0()),
                Geo::angle(start, center, ellipse.a1()),
                Geo::angle(start, center, ellipse.b0()),
                Geo::angle(start, center, ellipse.b1()) };
            bool mask[2] = { false, false }; // a, b
            for (int i = 0; i < 4; ++i)
            {
                if (angles[i] < 0)
                {
                    angles[i] += Geo::PI * 2;
                }
                if (ellipse.arc_angle0() <= angles[i] && angles[i] <= ellipse.arc_angle1())
                {
                    mask[i < 2 ? 0 : 1] = true; // 判断椭圆弧是否经过轴端点
                }
            }
            if (mask[0] && mask[1]) // 椭圆弧经过a轴和b轴端点
            {
                return std::min(ellipse.lengtha(), ellipse.lengthb());
            }
            else if (mask[0]) // 椭圆弧经过a轴端点
            {
                return std::min(ellipse.lengtha(), std::min(Geo::distance(point, ellipse.arc_point0()),
                    Geo::distance(point, ellipse.arc_point1())));
            }
            else if (mask[1]) // 椭圆弧经过b轴端点
            {
                return std::min(ellipse.lengthb(), std::min(Geo::distance(point, ellipse.arc_point0()),
                    Geo::distance(point, ellipse.arc_point1())));
            }
            else // 椭圆弧不经过轴端点
            {
                return std::min(Geo::distance(point, ellipse.arc_point0()), Geo::distance(point, ellipse.arc_point1()));
            }
        }
        else
        {
            return std::min(ellipse.lengtha(), ellipse.lengthb());
        }
    }
    else
    {
        const Geo::Point coord = Geo::to_coord(point, center.x, center.y, Geo::angle(ellipse.a0(), ellipse.a1()));
        const double a = ellipse.lengtha(), b = ellipse.lengthb();
        double degree0 = Geo::angle(Geo::Point(0, 0), coord) - Geo::PI / 2,
            degree1 = Geo::angle(Geo::Point(0, 0), coord) + Geo::PI / 2;
        double last_degree0 = degree0 - 1, last_degree1 = degree1 - 1;
        double m0 = (degree1 - degree0) / 3 + degree0, m1 = degree1 - (degree1 - degree0) / 3;
        double x0, y0, x1, y1;
        while (degree1 * 1e16 - degree0 * 1e16 > 1 && (last_degree0 != degree0 || last_degree1 != degree1))
        {
            last_degree0 = degree0, last_degree1 = degree1;
            m0 = (degree1 - degree0) / 3 + degree0, m1 = degree1 - (degree1 - degree0) / 3;
            x0 = a * std::cos(m0), y0 = b * std::sin(m0);
            x1 = a * std::cos(m1), y1 = b * std::sin(m1);
            if (Geo::distance_square(x0, y0, coord.x, coord.y) > Geo::distance_square(x1, y1, coord.x, coord.y))
            {
                degree0 = m0;
            }
            else
            {
                degree1 = m1;
            }
        }
        if (ellipse.is_arc())
        {
            double angle0 = Geo::rad_to_2PI(ellipse.arc_angle0());
            double angle1 = Geo::rad_to_2PI(ellipse.arc_angle1());
            angle0 = angle0 < angle1 ? angle1 - angle0 : Geo::PI * 2 - angle0 + angle1;
            angle1 = Geo::angle(ellipse.arc_point0(), center, point);
            if (angle1 < 0)
            {
                angle1 += Geo::PI * 2;
            }
            if (angle1 <= angle0)
            {
                return std::min(Geo::distance(x0, y0, coord.x, coord.y), Geo::distance(x1, y1, coord.x, coord.y));
            }
            else
            {
                return std::min(Geo::distance(point, ellipse.arc_point0()), Geo::distance(point, ellipse.arc_point1()));
            }
        }
        else
        {
            return std::min(Geo::distance(x0, y0, coord.x, coord.y), Geo::distance(x1, y1, coord.x, coord.y));
        }
    }
}

double Geo::distance(const Point &point, const Arc &arc)
{
    const Geo::Point center(arc.x, arc.y);
    if (point == center)
    {
        return arc.radius;
    }
    double angle0 = Geo::angle(arc.control_points[0], center, arc.control_points[2]);
    double angle1 = Geo::angle(arc.control_points[0], center, point);
    if (arc.is_cw())
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
    if (std::abs(angle1) <= std::abs(angle0))
    {
        return std::abs(Geo::distance(point, center) - arc.radius);
    }
    else
    {
        return std::min(Geo::distance(point, arc.control_points[0]),
            Geo::distance(point, arc.control_points[2]));
    }
}

double Geo::distance(const Geo::Point &start0, const Geo::Point &end0, const Geo::Point &start1, const Geo::Point &end1, Geo::Point &point0, Geo::Point &point1)
{
    if (Geo::is_parallel(start0, end0, start1, end1))
    {
        if (Geo::foot_point(start1, end1, start0, point1) && Geo::foot_point(start1, end1, end0, point1))
        {
            point0 = (start0 + end0) / 2;
            Geo::foot_point(start1, end1, point0, point1, true);
        }
        else if (Geo::foot_point(start0, end0, start1, point0) && Geo::foot_point(start0, end0, end1, point0))
        {
            point1 = (start1 + end1) / 2;
            Geo::foot_point(start0, end0, point1, point0, true);
        }
        else
        {
            if (Geo::foot_point(start1, end1, start0, point0))
            {
                if ((end0 - start0) * (end1 - start1) >= 0)
                {
                    point1 = (point0 + end1) / 2;
                }
                else
                {
                    point1 = (point0 + start1) / 2;
                }
                Geo::foot_point(start0, end0, point1, point0, true);
            }
            else if (Geo::foot_point(start1, end1, end0, point0))
            {
                if ((end0 - start0) * (end1 - start1) >= 0)
                {
                    point1 = (point0 + start1) / 2;
                }
                else
                {
                    point1 = (point0 + end1) / 2;
                }
                Geo::foot_point(start0, end0, point1, point0, true);
            }
            else
            {
                double distance[5] = {Geo::distance_square(start0, start1), Geo::distance_square(start0, end1),
                    Geo::distance_square(end0, start1), Geo::distance(end0, end1), DBL_MAX};
                int index = 0;
                for (int i = 0; i < 4; ++i)
                {
                    if (distance[i] < distance[4])
                    {
                        index = i;
                        distance[4] = distance[i];
                    }
                }
                switch (index)
                {
                case 0:
                    point0 = start0;
                    point1 = start1;
                    break;
                case 1:
                    point0 = start0;
                    point1 = end1;
                    break;
                case 2:
                    point0 = end0;
                    point1 = start1;
                    break;
                case 3:
                    point0 = end0;
                    point1 = end1;
                    break;
                }
            }
        }
        return Geo::distance(start0, start1, end1);
    }
    else
    {
        double distance[5] = {Geo::distance(start0, start1, end1), Geo::distance(end0, start1, end1),
            Geo::distance(start1, start0, end0), Geo::distance(end1, start0, end0), DBL_MAX};
        int index = 0;
        for (int i = 0; i < 4; ++i)
        {
            if (distance[i] < distance[4])
            {
                index = i;
                distance[4] = distance[i];
            }
        }

        switch (index)
        {
        case 0:
            point0 = start0;
            if (!Geo::foot_point(start1, end1, start0, point1))
            {
                if (Geo::distance_square(start0, start1) <= Geo::distance_square(start0, end1))
                {
                    point1 = start1;
                }
                else
                {
                    point1 = end1;
                }
            }
            break;
        case 1:
            point0 = end0;
            if (!Geo::foot_point(start1, end1, end0, point1))
            {
                if (Geo::distance_square(end0, start1) <= Geo::distance_square(end0, end1))
                {
                    point1 = start1;
                }
                else
                {
                    point1 = end1;
                }
            }
            break;
        case 2:
            point1 = start1;
            if (!Geo::foot_point(start0, end0, start1, point0))
            {
                if (Geo::distance_square(start1, start0) <= Geo::distance_square(start1, end0))
                {
                    point0 = start0;
                }
                else
                {
                    point0 = end0;
                }
            }
            break;
        case 3:
            point1 = end1;
            if (Geo::foot_point(start0, end0, end1, point0))
            {
                if (Geo::distance_square(end1, start0) <= Geo::distance_square(end1, end0))
                {
                    point0 = start0;
                }
                else
                {
                    point0 = end0;
                }
            }
            break;
        }
        return distance[4];
    }
}


double Geo::distance_square(const double x0, const double y0, const double x1, const double y1)
{
    return (x0 - x1) * (x0 - x1) + (y0 - y1) * (y0 - y1);
}

double Geo::distance_square(const Point &point0, const Point &point1)
{
    return (point0.x - point1.x) * (point0.x - point1.x)
        + (point0.y - point1.y) * (point0.y - point1.y);
}

double Geo::distance_square(const Point &point, const Point &start, const Point &end, const bool infinite)
{
    if (start.x == end.x)
    {
        if (infinite)
        {
            return std::pow(point.x - start.x, 2);
        }
        else
        {
            if ((point.y >= start.y && point.y <= end.y) ||
                (point.y <= start.y && point.y >= end.y))
            {
                return std::pow(point.x - start.x, 2);
            }
            else
            {
                return std::min(Geo::distance_square(point, start), Geo::distance_square(point, end));
            }
        }
    }
    else if (start.y == end.y)
    {
        if (infinite)
        {
            return std::pow(point.y - start.y, 2);
        }
        else
        {
            if ((point.x >= start.x && point.x <= end.x) ||
                (point.x <= start.x && point.x >= end.x))
            {
                return std::pow(point.y - start.y, 2);
            }
            else
            {
                return std::min(Geo::distance_square(point, start), Geo::distance_square(point, end));
            }
        }
    }
    
    const double a = end.y - start.y, 
                b = start.x - end.x,
                c = end.x * start.y - start.x * end.y;
    if (infinite)
    {
        return std::pow(a * point.x + b * point.y + c, 2) / (a * a + b * b);
    }
    else
    {
        const double k = ((point.x - start.x) * (end.x - start.x) +
            (point.y - start.y) * (end.y - start.y)) /
            (std::pow(end.x - start.x, 2) + std::pow(end.y - start.y, 2)); 
        const double x = start.x + k * (end.x - start.x);

        if ((x >= start.x && x <= end.x) || (x <= start.x && x >= end.x))
        {
            return std::pow(a * point.x + b * point.y + c, 2) / (a * a + b * b);
        }
        else
        {
            return std::min(Geo::distance_square(point, start), Geo::distance_square(point, end));
        }
    }
}

double Geo::distance_square(const Point &point, const Polyline &polyline)
{
    double dis = Geo::distance_square(point, polyline.front(), polyline[1]);
    for (size_t i = 2, count = polyline.size(); i < count; ++i)
    {
        dis = std::min(dis, Geo::distance_square(point, polyline[i - 1], polyline[i]));
    }
    return dis;
}

double Geo::distance_square(const Point &point, const Polygon &polygon)
{
    double dis = Geo::distance_square(point, polygon.front(), polygon[1]);
    for (size_t i = 2, count = polygon.size(); i < count; ++i)
    {
        dis = std::min(dis, Geo::distance_square(point, polygon[i - 1], polygon[i]));
    }
    return dis;
}


bool Geo::is_inside(const Geo::Point &point, const Geo::Point &start, const Geo::Point &end, const bool infinite)
{
    return Geo::distance(point, start, end, infinite) <= Geo::EPSILON;
}

bool Geo::is_inside(const Point &point, const Polyline &polyline)
{
    for (size_t i = 1, count = polyline.size(); i < count; ++i)
    {
        if (Geo::is_inside(point, polyline[i-1], polyline[i]))
        {
            return true;
        }
    }
    return false;
}

bool Geo::is_inside(const Point &point, const Polygon &polygon, const bool coincide)
{
    if (!polygon.empty() && Geo::is_inside(point, polygon.bounding_rect(), coincide))
    {
        if (coincide)
        {
            for (size_t i = 1, len = polygon.size(); i < len; ++i)
            {
                if (Geo::is_inside(point, polygon[i-1], polygon[i]))
                {
                    return true;
                }
            }
        }
        else
        {
            for (size_t i = 1, len = polygon.size(); i < len; ++i)
            {
                if (Geo::is_inside(point, polygon[i-1], polygon[i]))
                {
                    return false;
                }
            }
        }

        double x = (-DBL_MAX);
        std::vector<Geo::MarkedPoint> points;
        for (const Geo::Point &p : polygon)
        {
            x = std::max(x, p.x);
            points.emplace_back(p.x, p.y);
        }
        // if (polygon.area() < 0)
        // {
        //     std::reverse(points.begin(), points.end());
        // }

        Geo::Point temp, end(x + 80, point.y); // 找到交点并计算其几何数
        for (size_t i = 1, count = points.size(); i < count; ++i)
        {
            if (!Geo::is_parallel(point, end, points[i], points[i - 1]) &&
                Geo::is_intersected(point, end, points[i], points[i - 1], temp))
            {
                points.insert(points.begin() + i++, MarkedPoint(temp.x, temp.y, false));
                ++count;
                if (Geo::cross(temp, end, points[i], points[i - 2]) >= 0) // 为交点计算几何数
                {
                    points[i - 1].value = -1;
                }
                else
                {
                    points[i - 1].value = 1;
                }
            }
        }

        if (points.size() == polygon.size()) // 无交点
        {
            return false;
        }

        // 去除重复交点
        {
            const size_t i = points.size() - 1; // 对最后一个点进行处理
            size_t j0, j1;
            size_t count = points[i].original ? 0 : 1;
            for (j0 = i; j0 > 0; --j0) // 向前查找与points[i]重合的点
            {
                if (std::abs(points[i].x - points[j0 - 1].x) > Geo::EPSILON || 
                    std::abs(points[i].y - points[j0 - 1].y) > Geo::EPSILON)
                {
                    break;
                }
                if (!points[j0 - 1].original)
                {
                    ++count;
                }
            }
            for (j1 = 0; j1 < i; ++j1) // 向后查找与points[i]重合的点
            {
                if (std::abs(points[i].x - points[j1].x) > Geo::EPSILON || 
                    std::abs(points[i].y - points[j1].y) > Geo::EPSILON)
                {
                    break;
                }
                if (!points[j1].original)
                {
                    ++count;
                }
            }
            if (count >= 2)
            {
                int value = 0; // 几何数之和
                for (size_t k = i; k > j0; --k) // 计算前向几何数之和
                {
                    if (!points[k].original)
                    {
                        value += points[k].value;
                    }
                }
                if (!points[j0].original)
                {
                    value += points[j0].value;
                }
                for (size_t k = 0; k <= j1; ++k) // 计算后向几何数之和
                {
                    if (!points[k].original)
                    {
                        value += points[k].value;
                    }
                }
                if (value == 0) // 如果几何数之和为0,移除交点
                {
                    for (size_t k = i; k > j0; --k) // 移除前向交点
                    {
                        if (!points[k].original)
                        {
                            points.erase(points.begin() + k);
                        }
                    }
                    if (!points[j0].original)
                    {
                        points.erase(points.begin() + j0);
                    }
                    for (size_t k = 0; k <= j1; ++k) // 移除后向交点
                    {
                        if (!points[k].original)
                        {
                            points.erase(points.begin() + k);
                        }
                    }
                }
                else
                {
                    bool flag = false; // 标记是否包含原始点
                    for (size_t k = i; k > j0; --k) // 移除前向交点
                    {
                        flag = (flag || points[k].original);
                        points.erase(points.begin() + k);
                    }
                    flag = (flag || points[j0].original);
                    points.erase(points.begin() + j0);
                    for (size_t k = j1; k > 0; --k) // 移除后向交点
                    {
                        flag = (flag || points[k].original);
                        points.erase(points.begin() + k);
                    }
                    points[0].value = value; // Polygon的front与back相同,所以移除back,直接更新front的几何数
                    points[0].original = (flag || points[j0].original);
                }
            }
        }
        for (size_t count, j, i = points.size() - 2; i > 0; --i)
        {
            count = points[i].original ? 0 : 1;
            for (j = i; j > 0; --j) // 向前查找与points[i]重合的点
            {
                if (std::abs(points[i].x - points[j - 1].x) > Geo::EPSILON || 
                    std::abs(points[i].y - points[j - 1].y) > Geo::EPSILON)
                {
                    break;
                }
                if (!points[j - 1].original)
                {
                    ++count;
                }
            }
            if (count < 2)
            {
                continue;
            }

            int value = 0; // 几何数之和
            for (size_t k = i; k > j; --k) // 计算前向几何数之和
            {
                if (!points[k].original)
                {
                    value += points[k].value;
                }
            }
            if (!points[j].original)
            {
                value += points[j].value;
            }
            if (value == 0) // 如果几何数之和为0,移除交点
            {
                for (size_t k = i; k > j; --k) // 移除前向交点
                {
                    if (!points[k].original)
                    {
                        points.erase(points.begin() + k);
                    }
                }
                if (!points[j].original)
                {
                    points.erase(points.begin() + j);
                }
            }
            else
            {
                bool flag = false; // 标记是否包含原始点
                for (size_t k = i; k > j; --k) // 移除前向交点
                {
                    flag = (flag || points[k].original);
                    points.erase(points.begin() + k);
                }
                points[j].value = value; // 保留points[j],更新points[j]的几何数
                points[j].original = (flag || points[j].original);
            }
            i = j > 0 ? j : 1; // 更新i的值，确保循环继续进行
        }

        // 处理重边上的交点
        for (size_t i = 0, j = 1, count = points.size(); j < count; i = j)
        {
            while (i < count && points[i].value == 0) // 找到第一个非零几何数的点
            {
                ++i;
            }
            j = i + 1;
            while (j < count && points[j].value == 0) // 找到下一个非零几何数的点
            {
                ++j;
            }
            if (j >= count) // 如果没有找到下一个非零几何数的点,跳出循环
            {
                break;
            }
            if (polygon.index(points[i]) == SIZE_MAX || polygon.index(points[j]) == SIZE_MAX)
            {
                continue; // 如果points[i]或points[j]不在polygon中,跳过
            }

            if (points[i].value > 0 && points[j].value > 0) // 如果两个点的几何数都为正,移除第二个点
            {
                points.erase(points.begin() + j);
                --count;
            }
            else if (points[i].value < 0 && points[j].value < 0) // 如果两个点的几何数都为负,移除第一个点
            {
                points.erase(points.begin() + i);
                --count;
            }
            else // 如果两个点的几何数符号不同,移除两个点
            {
                points.erase(points.begin() + j--);
                points.erase(points.begin() + i);
                --count;
                --count;
            }
        }

        // 统计非零几何数的点的数量,判断是否为奇数
        return std::count_if(points.begin(), points.end(), [](const Geo::MarkedPoint &p) { return p.value != 0; }) % 2 == 1;
    }
    else
    {
        return false;
    }
}

bool Geo::is_inside(const Point &point, const AABBRect &rect, const bool coincide)
{
    if (rect.empty())
    {
        return false;
    }
    const double x = point.x, y = point.y;
    if (coincide)
    {
        return rect.left() <= x && x <= rect.right() && rect.bottom() <= y && y <= rect.top();
    }
    else
    {
        return rect.left() < x && x < rect.right() && rect.bottom() < y && y < rect.top();
    }
}

bool Geo::is_inside(const Point &point, const Circle &circle, const bool coincide)
{
    if (circle.empty())
    {
        return false;
    }
    if (coincide)
    {
        return Geo::distance_square(point, circle) <= circle.radius * circle.radius;
    }
    else
    {
        return Geo::distance_square(point, circle) < circle.radius * circle.radius;
    }
}

bool Geo::is_inside(const Point &point, const Ellipse &ellipse, const bool coincide)
{
    if (ellipse.empty())
    {
        return false;
    }
    if (ellipse.is_arc())
    {
        double angle0 = Geo::rad_to_2PI(ellipse.arc_angle0());
        double angle1 = Geo::rad_to_2PI(ellipse.arc_angle1());
        if (angle1 < angle0)
        {
            angle1 += Geo::PI * 2;
        }
        const double angle = Geo::rad_to_2PI(Geo::angle(ellipse.center(), point));
        const double angle2 = angle + Geo::PI * 2;
        if ((angle0 <= angle && angle <= angle1) || (angle0 <= angle2 && angle2 <= angle1))
        {
            return Geo::distance(ellipse.c0(), point) + Geo::distance(ellipse.c1(), point)
                == std::max(ellipse.lengtha(), ellipse.lengthb()) * 2;
        }
        else
        {
            return false;
        }
    }
    else
    {
        if (coincide)
        {
            return Geo::distance(ellipse.c0(), point) + Geo::distance(ellipse.c1(), point)
                <= std::max(ellipse.lengtha(), ellipse.lengthb()) * 2;
        }
        else
        {
            return Geo::distance(ellipse.c0(), point) + Geo::distance(ellipse.c1(), point)
                < std::max(ellipse.lengtha(), ellipse.lengthb()) * 2;
        }
    }
}

bool Geo::is_inside(const Point &point, const Point &point0, const Point &point1, const Point &point2, const bool coincide)
{
    if (coincide)
    {
        const bool a = (point2.x - point.x) * (point0.y - point.y) 
            >= (point0.x - point.x) * (point2.y - point.y);
        const bool b = (point0.x - point.x) * (point1.y - point.y)
            >= (point1.x - point.x) * (point0.y - point.y);
        const bool c = (point1.x - point.x) * (point2.y - point.y)
            >= (point2.x - point.x) * (point1.y - point.y);

        return a == b && b == c;
    }
    else
    {
        const bool a = (point2.x - point.x) * (point0.y - point.y) 
            > (point0.x - point.x) * (point2.y - point.y);
        const bool b = (point0.x - point.x) * (point1.y - point.y)
            > (point1.x - point.x) * (point0.y - point.y);
        const bool c = (point1.x - point.x) * (point2.y - point.y)
            > (point2.x - point.x) * (point1.y - point.y);

        return a == b && b == c;
    }
}

bool Geo::is_inside(const Point &point, const Triangle &triangle, const bool coincide)
{
    return Geo::is_inside(point, triangle[0], triangle[1], triangle[2], coincide);
}

bool Geo::is_inside(const Point &start, const Point &end, const Triangle &triangle)
{
    return Geo::is_inside(start, triangle) && Geo::is_inside(end, triangle);
}

bool Geo::is_inside(const Point &point, const Arc &arc)
{
    if (Geo::distance(point.x, point.y, arc.x, arc.y) != arc.radius)
    {
        return false;
    }
    const Geo::Point center(arc.x, arc.y); 
    double angle0 = Geo::angle(arc.control_points[0], center, arc.control_points[2]);
    double angle1 = Geo::angle(arc.control_points[0], center, point);
    if (arc.is_cw())
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
    return std::abs(angle1) <= std::abs(angle0);
}

bool Geo::is_inside(const Triangle &triangle0, const Triangle &triangle1)
{
    return Geo::is_inside(triangle0[0], triangle1) && Geo::is_inside(triangle0[1], triangle1)
        && Geo::is_inside(triangle0[2], triangle1);
}


bool Geo::is_parallel(const Point &point0, const Point &point1, const Point &point2, const Point &point3)
{
    if (point0.x == point1.x && point2.x == point3.x)
    {
        return true;
    }
    else
    {
        return ((point0.y - point1.y) * (point2.x - point3.x)) ==
            ((point2.y - point3.y) * (point0.x - point1.x));
    }
}


bool Geo::is_coincide(const Point &start0, const Point &end0, const Point &start1, const Point &end1)
{
    if ((start0 == start1 && end0 == end1) || (start0 == end1 && end0 == start0))
    {
        return true;
    }

    if (start0.x == end0.x)
    {
        if (start1.x == end1.x && start0.x == start1.x)
        {
            const bool result0 = (start1.y < start0.y && start0.y < end1.y) 
                || (end1.y < start0.y && start0.y < start1.y);
            const bool result1 = (start1.y < end0.y && end0.y < end1.y)
                || (end1.y < end0.y && end0.y < start1.y);
            const bool result2 = (start0.y < start1.y && start1.y < end0.y)
                || (end0.y < start1.y && start1.y < start0.y);
            const bool result3 = (start0.y < end1.y && end1.y < end0.y)
                || (end0.y < end1.y && end1.y < start0.y);
            return result0 || result1 || result2 || result3;
        }
        else
        {
            return false;
        }
    }
    else if (start0.y == end0.y)
    {
        if (start1.y == end1.y && start0.y == start1.y)
        {
            const bool result0 = (start1.x < start0.x && start0.x < end1.x)
                || (end1.x < start0.x && start0.x < start1.x);
            const bool result1 = (start1.x < end0.x && end0.x < end1.x)
                || (end1.x < end0.x && end0.x < start1.x);
            const bool result2 = (start0.x < start1.x && start1.x < end0.x)
                || (end0.x < start1.x && start1.x < start0.x);
            const bool result3 = (start0.x < end1.x && end1.x < end0.x)
                || (end0.x < end1.x && end1.x < start0.x);
            return result0 || result1 || result2 || result3;
        }
        else
        {
            return false;
        }
    }

    const double a0 = end0.y - start0.y, 
                b0 = start0.x - end0.x,
                c0 = end0.x * start0.y - start0.x * end0.y;
    const double a1 = end1.y - start1.y, 
                b1 = start1.x - end1.x,
                c1 = end1.x * start1.y - start1.x * end1.y;
    if (std::abs(a0 * b1 - a1 * b0) < Geo::EPSILON && std::abs(a0 * c1 - a1 * c0) < Geo::EPSILON
        && std::abs(b0 * c1 - b1 * c0) < Geo::EPSILON)
    {
        return Geo::distance((start0 + end0) / 2, (start1 + end1) / 2) * 2
            < Geo::distance(start0, end0) + Geo::distance(start1, end1);
    }
    else
    {
        return false;
    }
}

bool Geo::is_coincide(const Point &start, const Point &end, const Polygon &polygon)
{
    const size_t index0 = polygon.index(start.x, start.y), index1 = polygon.index(end.x, end.y);
    if (std::max(index0, index1) - std::min(index0, index1) == 1)
    {
        return true;
    }

    if (index0 < SIZE_MAX)
    {
        return Geo::is_coincide(start, end, start, polygon.last_point(index0))
            || Geo::is_coincide(start, end, start, polygon.next_point(index0));
    }
    else if (index1 < SIZE_MAX)
    {
        return Geo::is_coincide(start, end, end, polygon.last_point(index1))
            || Geo::is_coincide(start, end, end, polygon.next_point(index1));
    }
    else
    {
        for (size_t i = 1, count = polygon.size(); i < count; ++i)
        {
            if (Geo::is_coincide(start, end, polygon[i - 1], polygon[i]))
            {
                return true;
            }
        }
        return false;
    }
}



bool Geo::is_part(const Point &start0, const Point &end0, const Point &start1, const Point &end1)
{
    if (Geo::is_coincide(start0, end0, start1, end1))
    {
        if (start0.x == end0.x)
        {
            const double top0 = std::max(start0.y, end0.y), bottom0 = std::min(start0.y, end0.y);
            const double top1 = std::max(start1.y, end1.y), bottom1 = std::min(start1.y, end1.y);
            return bottom1 <= bottom0 && bottom0 <= top1 && bottom1 <= top0 && top0 <= top1;
        }
        else
        {
            const double left0 = std::min(start0.x, end0.x), right0 = std::max(start0.x, end0.x);
            const double left1 = std::min(start1.x, end1.x), right1 = std::max(start1.x, end1.x);
            return left1 <= left0 && left0 <= right0 && left1 <= right0 && right0 <= right1;
        }
    }
    else
    {
        return false;
    }
}


bool Geo::is_intersected(const Point &point0, const Point &point1, const Point &point2, const Point &point3, Point &output, const bool infinite)
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

    const double a0 = point1.y - point0.y,
                b0 = point0.x - point1.x,
                c0 = point1.x * point0.y - point0.x * point1.y;
    const double a1 = point3.y - point2.y,
                b1 = point2.x - point3.x,
                c1 = point3.x * point2.y - point2.x * point3.y;
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
                if (Geo::distance(point0, point2) < Geo::EPSILON ||
                    Geo::distance(point0, point3) < Geo::EPSILON)
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

        return left - 5e-14 <= output.x && output.x <= right + 5e-14
            && bottom - 5e-14 <= output.y && output.y <= top + 5e-14;
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
    const double l =  std::sqrt(std::abs(std::pow(circle.radius, 2) - Geo::distance_square(circle, point0, point1, true)));
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

int Geo::is_intersected(const Point &point0, const Point &point1, const Ellipse &ellipse, Point &output0, Point &output1, const bool infinite)
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

    const double value = std::pow(a1, 2) * a0 + std::pow(b1, 2) * b0 - std::pow(c1, 2);
    const double value2 = std::pow(b1, 2);
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

int Geo::is_intersected(const Point &point0, const Point &point1, const Bezier &bezier, std::vector<Point> &intersections,
    const bool infinite, std::vector<std::tuple<size_t, double, double, double>> *tvalues)
{
    const size_t order = bezier.order();
    std::vector<int> nums(order + 1, 1);
    switch (order)
    {
    case 2:
        nums[1] = 2;
        break;
    case 3:
        nums[1] = nums[2] = 3;
        break;
    default:
        {
            std::vector<int> temp(1, 1);
            for (size_t i = 1; i <= order; ++i)
            {
                for (size_t j = 1; j < i; ++j)
                {
                    nums[j] = temp[j - 1] + temp[j];
                }
                temp.assign(nums.begin(), nums.begin() + i + 1);
            }
        }
        break;
    }

    std::vector<Geo::Point> result;
    for (size_t i = 0, end = bezier.size() - order; i < end; i += order)
    {
        Geo::Polyline polyline;
        polyline.append(bezier[i]);
        double t = 0;
        while (t <= 1)
        {
            Geo::Point point;
            for (size_t j = 0; j <= order; ++j)
            {
                point += (bezier[j + i] * (nums[j] * std::pow(1 - t, order - j) * std::pow(t, j))); 
            }
            polyline.append(point);
            t += Geo::Bezier::default_step;
        }
        polyline.append(bezier[i + order]);
        Geo::down_sampling(polyline, Geo::Bezier::default_down_sampling_value);

        if (!Geo::is_intersected(polyline.bounding_rect(), point0, point1))
        {
            continue;
        }

        std::vector<Geo::Point> temp;
        for (size_t j = 1, count = polyline.size(); j < count; ++j)
        {
            if (Geo::Point point; Geo::is_intersected(polyline[j - 1], polyline[j], point0, point1, point, infinite)
                && (!infinite || Geo::is_inside(point, polyline[j - 1], polyline[j], false)))
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
                    for (size_t j = 0; j <= order; ++j)
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
                    for (size_t j = 0; j <= order; ++j)
                    {
                        coord += (bezier[j + i] * (nums[j] * std::pow(1 - x, order - j) * std::pow(x, j)));
                    }
                    if (const double dis = Geo::distance(coord, point0, point1) * 1e9; dis < min_dis)
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
            for (size_t j = 0; j <= order; ++j)
            {
                point += (bezier[j + i] * (nums[j] * std::pow(1 - t, order - j) * std::pow(t, j)));
            }
            if (std::find(result.begin(), result.end(), point) == result.end()
                && Geo::is_inside(point, point0, point1, infinite))
            {
                result.emplace_back(point);
                if (tvalues != nullptr)
                {
                    tvalues->emplace_back(i, t, point.x, point.y);
                }
            }
        }
    }

    if (Geo::distance(bezier.front(), point0, point1, infinite) < Geo::EPSILON
        && std::find(intersections.begin(), intersections.end(), bezier.front()) == intersections.end())
    {
        intersections.emplace_back(bezier.front());
        if (tvalues != nullptr)
        {
            tvalues->emplace_back(0, 0, bezier.front().x, bezier.front().y);
        }
    }
    if (Geo::distance(bezier.back(), point0, point1, infinite) < Geo::EPSILON
        && std::find(intersections.begin(), intersections.end(), bezier.back()) == intersections.end())
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

int Geo::is_intersected(const Point &point0, const Point &point1, const BSpline &bspline, const bool is_cubic, std::vector<Point> &intersections,
    const bool infinite, std::vector<std::tuple<double, double, double>> *tvalues)
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
        if (std::find(result.begin(), result.end(), coord) == result.end()
            && Geo::is_inside(coord, point0, point1, infinite))
        {
            result.emplace_back(coord);
            if (tvalues != nullptr)
            {
                tvalues->emplace_back(t, coord.x, coord.y);
            }
        }
    }

    if (Geo::distance(bspline.front(), point0, point1, infinite) < Geo::EPSILON
        && std::find(intersections.begin(), intersections.end(), bspline.front()) == intersections.end())
    {
        intersections.emplace_back(bspline.front());
        if (tvalues != nullptr)
        {
            tvalues->emplace_back(0, bspline.front().x, bspline.front().y);
        }
    }
    if (Geo::distance(bspline.back(), point0, point1, infinite) < Geo::EPSILON
        && std::find(intersections.begin(), intersections.end(), bspline.back()) == intersections.end())
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
        return !((rect0.top() < rect1.top() && rect0.right() < rect1.right() && rect0.bottom() > rect1.bottom() && rect0.left() > rect1.left())
            || (rect1.top() < rect0.top() && rect1.right() < rect0.right() && rect1.bottom() > rect0.bottom() && rect1.left() > rect0.left()));
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
            if (Geo::is_intersected(polyline0[i-1], polyline0[i], polyline1[j-1], polyline1[j], point))
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
            if (Geo::is_inside(polyline[i-1], polygon))
            {
                return true;
            }
            for (size_t j = 1, count1 = polygon.size(); j < count1; ++j)
            {
                if (Geo::is_intersected(polyline[i-1], polyline[i], polygon[j-1], polygon[j], point))
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
                if (Geo::is_intersected(polyline[i-1], polyline[i], polygon[j-1], polygon[j], point))
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
        if (Geo::distance_square(circle, polyline[i - 1], polyline[i]) < length)
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
        if (Geo::is_intersected(polyline[i - 1], polyline[i], ellipse, output0, output1))
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
        if (Geo::Point point0, point1; Geo::is_intersected(
            polyline[i - 1], polyline[i], arc, point0, point1))
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
            if (Geo::is_intersected(polygon0[i-1], polygon0[i], polygon1[j-1], polygon1[j], point))
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
        if (Geo::distance_square(circle, polygon[i - 1], polygon[i]) < length)
        {
            return true;
        }
    }

    if (!inside)
    {
        return false;
    }

    if (Geo::is_inside(circle, polygon, true) || std::any_of(polygon.begin(), polygon.end(), [&](const Geo::Point &point) { return Geo::is_inside(point, circle, true); }))
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
        if (Geo::is_inside(ellipse.center(), polygon) || Geo::is_inside(ellipse.a0(), polygon)
            || Geo::is_inside(ellipse.a1(), polygon) || Geo::is_inside(ellipse.b0(), polygon)
            || Geo::is_inside(ellipse.b1(), polygon))
        {
            return true;
        }
    }

    Geo::Point output0, output1;
    for (size_t i = 1, count = polygon.size(); i < count; ++i)
    {
        if (Geo::is_intersected(polygon[i - 1], polygon[i], ellipse, output0, output1))
        {
            return true;
        }
    }
    return false;
}

bool Geo::is_intersected(const Circle &circle0, const Circle& circle1, const bool inside)
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
            else
            {
                d = 0;
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
            if (Geo::is_intersected(polyline0[i - 1], polyline0[i], polyline1[j - 1], polyline1[j], point, false)
                && std::find(points.cbegin(), points.cend(), point) == points.cend())
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
        std::sort(distance_table.begin(), distance_table.end(), [](const std::tuple<size_t, size_t, double> &a,
            const std::tuple<size_t, size_t, double> &b) { return std::get<2>(a) > std::get<2>(b); });
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
        std::sort(remove_index.begin(), remove_index.end(), std::greater<size_t>());
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
            if (Geo::is_intersected(polygon[i - 1], polygon[i], polyline[j - 1], polyline[j], point, false)
                && std::find(points.cbegin(), points.cend(), point) == points.cend())
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
        std::sort(distance_table.begin(), distance_table.end(), [](const std::tuple<size_t, size_t, double> &a,
            const std::tuple<size_t, size_t, double> &b) { return std::get<2>(a) > std::get<2>(b); });
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
        std::sort(remove_index.begin(), remove_index.end(), std::greater<size_t>());
        for (const size_t index : remove_index)
        {
            points.erase(points.begin() + index);
        }
    }

    const Geo::Point center0 = circle, center1 = ellipse.center();
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

int Geo::is_intersected(const Circle &circle, const Bezier &bezier, std::vector<Point> &intersections, std::vector<std::tuple<size_t, double, double, double>> *tvalues)
{
    const size_t order = bezier.order();
    std::vector<int> nums(order + 1, 1);
    switch (order)
    {
    case 2:
        nums[1] = 2;
        break;
    case 3:
        nums[1] = nums[2] = 3;
        break;
    default:
        {
            std::vector<int> temp(1, 1);
            for (size_t i = 1; i <= order; ++i)
            {
                for (size_t j = 1; j < i; ++j)
                {
                    nums[j] = temp[j - 1] + temp[j];
                }
                temp.assign(nums.begin(), nums.begin() + i + 1);
            }
        }
        break;
    }

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
            for (size_t j = 0; j <= order; ++j)
            {
                point += (bezier[j + i] * (nums[j] * std::pow(1 - t, order - j) * std::pow(t, j))); 
            }
            polyline.append(point);
            t += Geo::Bezier::default_step;
        }
        polyline.append(bezier[i + order]);
        Geo::down_sampling(polyline, Geo::Bezier::default_down_sampling_value);

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
                    for (size_t j = 0; j <= order; ++j)
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
                    for (size_t j = 0; j <= order; ++j)
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
            for (size_t j = 0; j <= order; ++j)
            {
                point += (bezier[j + i] * (nums[j] * std::pow(1 - t, order - j) * std::pow(t, j)));
            }
            if (std::find(result.begin(), result.end(), point) == result.end()
                && std::abs(Geo::distance(point, circle) - circle.radius) < Geo::EPSILON)
            {
                result.emplace_back(point);
                if (tvalues != nullptr)
                {
                    tvalues->emplace_back(i, t, point.x, point.y);
                }
            }
        }
    }
    if (std::abs(Geo::distance(bezier.front(), circle) - circle.radius) < Geo::EPSILON
        && std::find(result.begin(), result.end(), bezier.front()) == result.end())
    {
        result.emplace_back(bezier.front());
        if (tvalues != nullptr)
        {
            tvalues->emplace_back(0, 0, bezier.front().x, bezier.front().y);
        }
    }
    if (std::abs(Geo::distance(bezier.back(), circle) - circle.radius) < Geo::EPSILON
        && std::find(result.begin(), result.end(), bezier.back()) == result.end())
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

int Geo::is_intersected(const Circle &circle, const BSpline &bspline, const bool is_cubic, std::vector<Point> &intersections, std::vector<std::tuple<double, double, double>> *tvalues) 
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
        if (std::find(result.begin(), result.end(), point) == result.end()
            && std::abs(Geo::distance(point, circle) - circle.radius) < Geo::EPSILON)
        {
            result.emplace_back(point);
            if (tvalues != nullptr)
            {
                tvalues->emplace_back(t, point.x, point.y);
            }
        }
    }
    if (std::abs(Geo::distance(bspline.front(), circle) - circle.radius) < Geo::EPSILON
        && std::find(result.begin(), result.end(), bspline.front()) == result.end())
    {
        result.emplace_back(bspline.front());
        if (tvalues != nullptr)
        {
            tvalues->emplace_back(0, bspline.front().x, bspline.front().y);
        }
    }
    if (std::abs(Geo::distance(bspline.back(), circle) - circle.radius) < Geo::EPSILON
        && std::find(result.begin(), result.end(), bspline.back()) == result.end())
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

int Geo::is_intersected(const Ellipse &ellipse, const Bezier &bezier, std::vector<Point> &intersections, std::vector<std::tuple<size_t, double, double, double>> *tvalues)
{
    const size_t order = bezier.order();
    std::vector<int> nums(order + 1, 1);
    switch (order)
    {
    case 2:
        nums[1] = 2;
        break;
    case 3:
        nums[1] = nums[2] = 3;
        break;
    default:
        {
            std::vector<int> temp(1, 1);
            for (size_t i = 1; i <= order; ++i)
            {
                for (size_t j = 1; j < i; ++j)
                {
                    nums[j] = temp[j - 1] + temp[j];
                }
                temp.assign(nums.begin(), nums.begin() + i + 1);
            }
        }
        break;
    }

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
            for (size_t j = 0; j <= order; ++j)
            {
                point += (bezier[j + i] * (nums[j] * std::pow(1 - t, order - j) * std::pow(t, j))); 
            }
            polyline.append(point);
            t += Geo::Bezier::default_step;
        }
        polyline.append(bezier[i + order]);
        Geo::down_sampling(polyline, Geo::Bezier::default_down_sampling_value);

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
                    for (size_t j = 0; j <= order; ++j)
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
                    for (size_t j = 0; j <= order; ++j)
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
            for (size_t j = 0; j <= order; ++j)
            {
                point += (bezier[j + i] * (nums[j] * std::pow(1 - t, order - j) * std::pow(t, j)));
            }
            if (std::find(result.begin(), result.end(), point) == result.end()
                && Geo::distance(point, ellipse) < eps)
            {
                result.emplace_back(point);
                if (tvalues != nullptr)
                {
                    tvalues->emplace_back(i, t, point.x, point.y);
                }
            }
        }
    }
    if (Geo::distance(bezier.front(), ellipse) < eps
        && std::find(result.begin(), result.end(), bezier.front()) == result.end())
    {
        result.emplace_back(bezier.front());
        if (tvalues != nullptr)
        {
            tvalues->emplace_back(0, 0, bezier.front().x, bezier.front().y);
        }
    }
    if (Geo::distance(bezier.back(), ellipse) < eps
        && std::find(result.begin(), result.end(), bezier.back()) == result.end())
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

int Geo::is_intersected(const Ellipse &ellipse, const BSpline &bspline, const bool is_cubic, std::vector<Point> &intersections, std::vector<std::tuple<double, double, double>> *tvalues)
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
        if (std::find(result.begin(), result.end(), point) == result.end()
            && Geo::distance(point, ellipse) < eps)
        {
            result.emplace_back(point);
            if (tvalues != nullptr)
            {
                tvalues->emplace_back(t, point.x, point.y);
            }
        }
    }
    if (Geo::distance(bspline.front(), ellipse) < eps
        && std::find(result.begin(), result.end(), bspline.front()) == result.end())
    {
        result.emplace_back(bspline.front());
        if (tvalues != nullptr)
        {
            tvalues->emplace_back(0, bspline.front().x, bspline.front().y);
        }
    }
    if (Geo::distance(bspline.back(), ellipse) < eps
        && std::find(result.begin(), result.end(), bspline.back()) == result.end())
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

int Geo::is_intersected(const Arc &arc, const Bezier &bezier, std::vector<Point> &intersections, std::vector<std::tuple<size_t, double, double, double>> *tvalues)
{
    std::vector<Point> temp_intersections;
    std::vector<std::tuple<size_t, double, double, double>> temp_tvalues;
    Geo::is_intersected(Geo::Circle(arc.x, arc.y, arc.radius), bezier, temp_intersections, tvalues == nullptr ? nullptr : &temp_tvalues);
    for (size_t i = 0, count = temp_intersections.size(); i < count; ++i)
    {
        if (Geo::distance(temp_intersections[i], arc) > Geo::EPSILON)
        {
            auto it = std::find_if(temp_tvalues.begin(), temp_tvalues.end(), [&](const std::tuple<size_t, double, double, double> &item)
                { return std::get<2>(item) == temp_intersections[i].x && std::get<3>(item) == temp_intersections[i].y; });
            while (it != tvalues->end())
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

int Geo::is_intersected(const Arc &arc, const BSpline &bspline, const bool is_cubic, std::vector<Point> &intersections, std::vector<std::tuple<double, double, double>> *tvalues)
{
    std::vector<Point> temp_intersections;
    std::vector<std::tuple<double, double, double>> temp_tvalues;
    Geo::is_intersected(Geo::Circle(arc.x, arc.y, arc.radius), bspline, is_cubic, temp_intersections, tvalues == nullptr ? nullptr : &temp_tvalues);
    for (size_t i = 0, count = temp_intersections.size(); i < count; ++i)
    {
        if (Geo::distance(temp_intersections[i], arc) > Geo::EPSILON)
        {
            auto it = std::find_if(temp_tvalues.begin(), temp_tvalues.end(), [&](const std::tuple<double, double, double> &item)
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

int Geo::is_intersected(const Bezier &bezier0, const Bezier &bezier1, std::vector<Point> &intersections,
    std::vector<std::tuple<size_t, double, double, double>> *tvalues0, std::vector<std::tuple<size_t, double, double, double>> *tvalues1)
{
    const size_t order0 = bezier0.order(), order1 = bezier1.order();
    std::vector<int> nums0(order0 + 1, 1), nums1(order1 + 1, 1);
    switch (order0)
    {
    case 2:
        nums0[1] = 2;
        break;
    case 3:
        nums0[1] = nums0[2] = 3;
        break;
    default:
        {
            std::vector<int> temp(1, 1);
            for (size_t i = 1; i <= order0; ++i)
            {
                for (size_t j = 1; j < i; ++j)
                {
                    nums0[j] = temp[j - 1] + temp[j];
                }
                temp.assign(nums0.begin(), nums0.begin() + i + 1);
            }
        }
        break;
    }
    switch (order1)
    {
    case 2:
        nums1[1] = 2;
        break;
    case 3:
        nums1[1] = nums1[2] = 3;
        break;
    default:
        {
            std::vector<int> temp(1, 1);
            for (size_t i = 1; i <= order1; ++i)
            {
                for (size_t j = 1; j < i; ++j)
                {
                    nums1[j] = temp[j - 1] + temp[j];
                }
                temp.assign(nums1.begin(), nums1.begin() + i + 1);
            }
        }
        break;
    }

    const size_t count = intersections.size();
    for (size_t p = 0, end0 = bezier0.size() - order0; p < end0; p += order0)
    {
        Geo::Polyline polyline0;
        for (double t = 0; t <= 1; t += Geo::Bezier::default_step)
        {
            Geo::Point point;
            for (size_t j = 0; j <= order0; ++j)
            {
                point += (bezier0[j + p] * (nums0[j] * std::pow(1 - t, order0 - j) * std::pow(t, j))); 
            }
            polyline0.append(point);
        }
        polyline0.append(bezier0[p + order0]);
        Geo::down_sampling(polyline0, Geo::Bezier::default_down_sampling_value);

        for (size_t q = 0, end1 = bezier1.size() - order1; q < end1; q += order1)
        {
            Geo::Polyline polyline1;
            for (double t = 0; t <= 1; t += Geo::Bezier::default_step)
            {
                Geo::Point point;
                for (size_t j = 0; j <= order1; ++j)
                {
                    point += (bezier1[j + q] * (nums1[j] * std::pow(1 - t, order1 - j) * std::pow(t, j))); 
                }
                polyline1.append(point);
            }
            polyline1.append(bezier1[q + order1]);
            Geo::down_sampling(polyline1, Geo::Bezier::default_down_sampling_value);

            if (!Geo::is_intersected(polyline0.bounding_rect(), polyline1.bounding_rect(), true))
            {
                continue;
            }

            for (size_t a = 1, count0 = polyline0.size(); a < count0; ++a)
            {
                for (size_t b = 1, count1 = polyline1.size(); b < count1; ++b)
                {
                    Geo::Point point;
                    if (!Geo::is_intersected(polyline0[a - 1], polyline0[a],
                        polyline1[b - 1], polyline1[b], point, false))
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
                            for (size_t j = 0; j <= order0; ++j)
                            {
                                coord += (bezier0[j + p] * (nums0[j] * std::pow(1 - x, order0 - j) * std::pow(x, j))); 
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
                            for (size_t j = 0; j <= order1; ++j)
                            {
                                coord += (bezier1[j + q] * (nums1[j] * std::pow(1 - x, order1 - j) * std::pow(x, j))); 
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
                    params[0].order = order0;
                    params[0].values = nums0.data();
                    double *points0 = new double[order0 * 2 + 2];
                    params[0].points = points0;
                    for (size_t k = 0; k <= order0; ++k)
                    {
                        points0[k * 2] = bezier0[k + p].x;
                        points0[k * 2 + 1] = bezier0[k + p].y;
                    }
                    params[1].order = order1;
                    params[1].values = nums1.data();
                    double *points1 = new double[order1 * 2 + 2];
                    params[1].points = points1;
                    for (size_t k = 0; k <= order1; ++k)
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
                    for (size_t j = 0; j <= order0; ++j)
                    {
                        point0 += (bezier0[j + p] * (nums0[j] * std::pow(1 - t0, order0 - j) * std::pow(t0, j))); 
                    }
                    for (size_t j = 0; j <= order1; ++j)
                    {
                        point1 += (bezier1[j + q] * (nums1[j] * std::pow(1 - t1, order1 - j) * std::pow(t1, j))); 
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
                tvalues1->emplace_back(bezier1.size() / (order1 + 1) - 1, 1, bezier1.back().x, bezier1.back().y);
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
                tvalues0->emplace_back(bezier0.size() / (order0 + 1) - 1, 1, bezier0.back().x, bezier0.back().y);
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
                tvalues0->emplace_back(bezier0.size() / (order0 + 1) - 1, 1, bezier0.back().x, bezier0.back().y);
            }
            if (tvalues1 != nullptr)
            {
                tvalues1->emplace_back(bezier1.size() / (order1 + 1) - 1, 1, bezier1.back().x, bezier1.back().y);
            }
        }
    }

    return intersections.size() - count;
}

int Geo::is_intersected(const BSpline &bspline0, const bool is_cubic0, const BSpline &bspline1, const bool is_cubic1, std::vector<Point> &intersections,
    std::vector<std::tuple<double, double, double>> *tvalues0, std::vector<std::tuple<double, double, double>> *tvalues1)
{
    std::vector<Geo::Point> temp_points;
    for (size_t i = 1, count0 = bspline0.shape().size(); i < count0; ++i)
    {
        for (size_t j = 1, count1 = bspline1.shape().size(); j < count1; ++j)
        {
            if (Geo::Point point; Geo::is_intersected(bspline0.shape()[i - 1], bspline0.shape()[i],
                bspline1.shape()[j - 1], bspline1.shape()[j], point, false)
                && std::find(temp_points.begin(), temp_points.end(), point) == temp_points.end())
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
        Geo::BSpline::rbasis( is_cubic1 ? 3 : 2, t1, npts1, knots1, nbasis1);
        Geo::Point point1;
        for (size_t j = 0; j < npts1; ++j)
        {
            point1 += bspline1.control_points[j] * nbasis1[j];
        }
        if (Geo::Point point((point0 + point1) / 2); std::find(intersections.begin(),
            intersections.end(), point) == intersections.end())
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

int Geo::is_intersected(const Bezier &bezier, const BSpline &bspline, const bool is_cubic, std::vector<Point> &intersections,
    std::vector<std::tuple<size_t, double, double, double>> *tvalues0, std::vector<std::tuple<double, double, double>> *tvalues1)
{
    const size_t order = bezier.order();
    std::vector<int> nums(order + 1, 1);
    switch (order)
    {
    case 2:
        nums[1] = 2;
        break;
    case 3:
        nums[1] = nums[2] = 3;
        break;
    default:
        {
            std::vector<int> temp(1, 1);
            for (size_t i = 1; i <= order; ++i)
            {
                for (size_t j = 1; j < i; ++j)
                {
                    nums[j] = temp[j - 1] + temp[j];
                }
                temp.assign(nums.begin(), nums.begin() + i + 1);
            }
        }
        break;
    }

    std::vector<Geo::Point> temp_points;
    for (size_t i = 1, count0 = bezier.shape().size(); i < count0; ++i)
    {
        for (size_t j = 1, count1 = bspline.shape().size(); j < count1; ++j)
        {
            if (Geo::Point point; Geo::is_intersected(bezier.shape()[i - 1], bezier.shape()[i],
                bspline.shape()[j - 1], bspline.shape()[j], point, false)
                && std::find(temp_points.begin(), temp_points.end(), point) == temp_points.end())
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
                for (size_t j = 0; j <= order; ++j)
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
                    for (size_t j = 0; j <= order; ++j)
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
    param.bezier.values = nums.data();
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
        auto [t0, t1] = Math::solve_curve_intersection(&param, Math::CurveIntersectType::BezierBSpline,
            std::get<1>(bezier_values[i]), bspline_values[i]);
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
        if (Geo::Point point((point0 + point1) / 2); std::find(intersections.begin(),
            intersections.end(), point) == intersections.end())
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
            return Geo::is_intersected(rect[0], rect[2], point0, point1, point)
                || Geo::is_intersected(rect[1], rect[3], point0, point1, point);
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
        if (Geo::is_intersected(rect, polyline[i - 1], polyline[i]))
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
        if (Geo::is_intersected(rect, polygon[i - 1], polygon[i]))
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
        if (Geo::is_intersected(rect[i-1], rect[i], circle, point0, point1))
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
        if (Geo::is_inside(ellipse.a0(), rect) || Geo::is_inside(ellipse.a1(), rect)
        || Geo::is_inside(ellipse.b0(), rect) || Geo::is_inside(ellipse.b1(), rect))
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
        if (Geo::Point point0, point1; Geo::is_intersected(
            rect[i - 1], rect[i], arc, point0, point1, false))
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


bool Geo::find_intersections(const Geo::Polyline &polyline0, const Geo::Polyline &polyline1, const Geo::Point &pos, const double distance, std::vector<Geo::Point> &intersections)
{
    std::vector<size_t> index0, index1;
    for (size_t i = 1, count = polyline0.size(); i < count; ++i)
    {
        if (Geo::distance(pos, polyline0[i - 1], polyline0[i], false) <= distance)
        {
            index0.push_back(i);
        }
    }
    for (size_t i = 1, count = polyline1.size(); i < count; ++i)
    {
        if (Geo::distance(pos, polyline1[i - 1], polyline1[i], false) <= distance)
        {
            index1.push_back(i);
        }
    }

    const size_t count = intersections.size();
    for (const size_t i : index0)
    {
        for (const size_t j : index1)
        {
            if (Geo::Point point; Geo::is_intersected(polyline0[i - 1], polyline0[i],
                polyline1[j - 1], polyline1[j], point, false) && Geo::distance(pos, point) <= distance
                && std::find(intersections.begin(), intersections.end(), point) == intersections.end())
            {
                intersections.emplace_back(point);
            }
        }
    }
    return intersections.size() > count;
}

bool Geo::find_intersections(const Geo::Polyline &polyline, const Geo::Bezier &bezier, const Geo::Point &pos, const double distance, std::vector<Geo::Point> &intersections)
{
    const size_t size = intersections.size();
    for (size_t i = 1, count = polyline.size(); i < count; ++i)
    {
        if (Geo::distance(pos, polyline[i - 1], polyline[i], false) > distance)
        {
            continue;
        }

        std::vector<Geo::Point> temp;
        Geo::is_intersected(polyline[i - 1], polyline[i], bezier, temp, false);
        for (const Geo::Point &point : temp)
        {
            if (Geo::distance(point, pos) <= distance && std::find(intersections.begin(),
                intersections.end(), point) == intersections.end())
            {
                intersections.emplace_back(point);
            }
        }
    }
    return intersections.size() > size;
}

bool Geo::find_intersections(const Geo::Polyline &polyline, const Geo::BSpline &bspline, const bool is_cubic, const Geo::Point &pos, const double distance, std::vector<Geo::Point> &intersections)
{
    const size_t size = intersections.size();
    for (size_t i = 1, count = polyline.size(); i < count; ++i)
    {
        if (Geo::distance(pos, polyline[i - 1], polyline[i], false) > distance)
        {
            continue;
        }

        std::vector<Geo::Point> temp;
        Geo::is_intersected(polyline[i - 1], polyline[i], bspline, is_cubic, temp, false);
        for (const Geo::Point &point : temp)
        {
            if (Geo::distance(point, pos) <= distance && std::find(intersections.begin(),
                intersections.end(), point) == intersections.end())
            {
                intersections.emplace_back(point);
            }
        }
    }
    return intersections.size() > size;
}

bool Geo::find_intersections(const Geo::Polyline &polyline, const Geo::Circle &circle, const Geo::Point &pos, const double distance, std::vector<Geo::Point> &intersections)
{
    std::vector<size_t> index;
    for (size_t i = 1, count = polyline.size(); i < count; ++i)
    {
        if (Geo::distance(pos, polyline[i - 1], polyline[i], false) <= distance)
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

bool Geo::find_intersections(const Geo::Polyline &polyline, const Geo::Ellipse &ellipse, const Geo::Point &pos, const double distance, std::vector<Geo::Point> &intersections)
{
    std::vector<size_t> index;
    for (size_t i = 1, count = polyline.size(); i < count; ++i)
    {
        if (Geo::distance(pos, polyline[i - 1], polyline[i], false) <= distance)
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

bool Geo::find_intersections(const Geo::Polyline &polyline, const Geo::Arc &arc, const Geo::Point &pos, const double distance, std::vector<Geo::Point> &intersections)
{
    std::vector<size_t> index;
    for (size_t i = 1, count = polyline.size(); i < count; ++i)
    {
        if (Geo::distance(pos, polyline[i - 1], polyline[i], false) <= distance)
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
            if (Geo::distance(pos, point1) <= distance && std::find(intersections.begin(),
                intersections.end(), point1) == intersections.end())
            {
                intersections.emplace_back(point1);
            }
            [[fallthrough]];
        case 1:
            if (Geo::distance(pos, point0) <= distance && std::find(intersections.begin(),
                intersections.end(), point0) == intersections.end())
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

bool Geo::find_intersections(const Geo::Circle &circle0, const Geo::Circle &circle1, const Geo::Point &pos, const double distance, std::vector<Geo::Point> &intersections)
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

bool Geo::find_intersections(const Geo::Ellipse &ellipse0, const Geo::Ellipse &ellipse1, const Geo::Point &pos, const double distance, std::vector<Geo::Point> &intersections)
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

bool Geo::find_intersections(const Geo::Ellipse &ellipse, const Geo::Circle &circle, const Geo::Point &pos, const double distance, std::vector<Geo::Point> &intersections)
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
        const Geo::Point center0 = circle, center1 = ellipse.center();
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

bool Geo::find_intersections(const Geo::Geometry *object0, const Geo::Geometry *object1, const Geo::Point &pos, const double distance, std::vector<Geo::Point> &intersections)
{
    switch (object0->type())
    {
    case Geo::Type::POLYLINE:
        switch (object1->type())
        {
        case Geo::Type::POLYLINE:
            return Geo::find_intersections(*static_cast<const Geo::Polyline *>(object0),
                *static_cast<const Geo::Polyline *>(object1), pos, distance, intersections);
        case Geo::Type::POLYGON:
            return Geo::find_intersections(*static_cast<const Geo::Polyline *>(object0),
                *static_cast<const Geo::Polygon *>(object1), pos, distance, intersections);
        case Geo::Type::CIRCLE:
            return Geo::find_intersections(*static_cast<const Geo::Polyline *>(object0),
                *static_cast<const Geo::Circle *>(object1), pos, distance, intersections);
        case Geo::Type::ELLIPSE:
            return Geo::find_intersections(*static_cast<const Geo::Polyline *>(object0),
                *static_cast<const Geo::Ellipse *>(object1), pos, distance, intersections);
        case Geo::Type::BEZIER:
            return Geo::find_intersections(*static_cast<const Geo::Polyline *>(object0),
                *static_cast<const Geo::Bezier *>(object1), pos, distance, intersections);
        case Geo::Type::BSPLINE:
            return Geo::find_intersections(*static_cast<const Geo::Polyline *>(object0), *static_cast<const Geo::BSpline *>(object1),
                dynamic_cast<const Geo::CubicBSpline *>(object1), pos, distance, intersections);
        case Geo::Type::ARC:
            return Geo::find_intersections(*static_cast<const Geo::Polyline *>(object0),
                *static_cast<const Geo::Arc *>(object1), pos, distance, intersections);
        default:
            break;
        }
        break;
    case Geo::Type::POLYGON:
        switch (object1->type())
        {
        case Geo::Type::POLYLINE:
            return Geo::find_intersections(*static_cast<const Geo::Polyline *>(object1),
                *static_cast<const Geo::Polygon *>(object0), pos, distance, intersections);
        case Geo::Type::POLYGON:
            return Geo::find_intersections(*static_cast<const Geo::Polygon *>(object0),
                *static_cast<const Geo::Polygon *>(object1), pos, distance, intersections);
        case Geo::Type::CIRCLE:
            return Geo::find_intersections(*static_cast<const Geo::Polygon *>(object0),
                *static_cast<const Geo::Circle *>(object1), pos, distance, intersections);
        case Geo::Type::ELLIPSE:
            return Geo::find_intersections(*static_cast<const Geo::Polygon *>(object0),
                *static_cast<const Geo::Ellipse *>(object1), pos, distance, intersections);
        case Geo::Type::BEZIER:
            return Geo::find_intersections(*static_cast<const Geo::Polygon *>(object0),
                *static_cast<const Geo::Bezier *>(object1), pos, distance, intersections);
        case Geo::Type::BSPLINE:
            return Geo::find_intersections(*static_cast<const Geo::Polygon *>(object0), *static_cast<const Geo::BSpline *>(object1),
                dynamic_cast<const Geo::CubicBSpline *>(object1), pos, distance, intersections);
        case Geo::Type::ARC:
            return Geo::find_intersections(*static_cast<const Geo::Polygon *>(object0),
                *static_cast<const Geo::Arc *>(object1), pos, distance, intersections);
        default:
            break;
        }
        break;
    case Geo::Type::CIRCLE:
        switch (object1->type())
        {
        case Geo::Type::POLYLINE:
            return Geo::find_intersections(*static_cast<const Geo::Polyline *>(object1),
                *static_cast<const Geo::Circle *>(object0), pos, distance, intersections);
        case Geo::Type::POLYGON:
            return Geo::find_intersections(*static_cast<const Geo::Polygon *>(object1),
                *static_cast<const Geo::Circle *>(object0), pos, distance, intersections);
        case Geo::Type::CIRCLE:
            return Geo::find_intersections(*static_cast<const Geo::Circle *>(object0),
                *static_cast<const Geo::Circle *>(object1), pos, distance, intersections);
        case Geo::Type::ELLIPSE:
            return Geo::find_intersections(*static_cast<const Geo::Ellipse *>(object1),
                *static_cast<const Geo::Circle *>(object0), pos, distance, intersections);
        case Geo::Type::BEZIER:
            if (std::vector<Geo::Point> temp; Geo::is_intersected(*static_cast<const Geo::Circle *>(object0),
                *static_cast<const Geo::Bezier *>(object1), temp))
            {
                const size_t count = intersections.size();
                for (const Geo::Point &point : temp)
                {
                    if (std::find(intersections.begin(), intersections.end(), point) == 
                        intersections.end() && Geo::distance(point, pos) < distance)
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
            if (std::vector<Geo::Point> temp; Geo::is_intersected(*static_cast<const Geo::Circle *>(object0),
                *static_cast<const Geo::BSpline *>(object1), dynamic_cast<const Geo::CubicBSpline *>(object1), temp))
            {
                const size_t count = intersections.size();
                for (const Geo::Point &point : temp)
                {
                    if (std::find(intersections.begin(), intersections.end(), point) == 
                        intersections.end() && Geo::distance(point, pos) < distance)
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
                switch (Geo::is_intersected(*static_cast<const Geo::Circle *>(object0),
                    *static_cast<const Geo::Arc *>(object1), points[0], points[1]))
                {
                case 2:
                    if (Geo::distance(pos, points[1]) < distance && std::find(intersections.begin(),
                        intersections.end(), points[1]) == intersections.end())
                    {
                        intersections.emplace_back(points[1]);
                    }
                    [[fallthrough]];
                case 1:
                    if (Geo::distance(pos, points[0]) < distance && std::find(intersections.begin(),
                        intersections.end(), points[0]) == intersections.end())
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
            return Geo::find_intersections(*static_cast<const Geo::Polyline *>(object1),
                *static_cast<const Geo::Ellipse *>(object0), pos, distance, intersections);
        case Geo::Type::POLYGON:
            return Geo::find_intersections(*static_cast<const Geo::Polygon *>(object1),
                *static_cast<const Geo::Ellipse *>(object0), pos, distance, intersections);
        case Geo::Type::CIRCLE:
            return Geo::find_intersections(*static_cast<const Geo::Ellipse *>(object0),
                *static_cast<const Geo::Circle *>(object1), pos, distance, intersections);
        case Geo::Type::ELLIPSE:
            return Geo::find_intersections(*static_cast<const Geo::Ellipse *>(object0),
                *static_cast<const Geo::Ellipse *>(object1), pos, distance, intersections);
        case Geo::Type::BEZIER:
            if (std::vector<Geo::Point> temp; Geo::is_intersected(*static_cast<const Geo::Ellipse *>(object0),
                *static_cast<const Geo::Bezier *>(object1), temp))
            {
                const size_t count = intersections.size();
                for (const Geo::Point &point : temp)
                {
                    if (std::find(intersections.begin(), intersections.end(), point) == 
                        intersections.end() && Geo::distance(point, pos) < distance)
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
            if (std::vector<Geo::Point> temp; Geo::is_intersected(*static_cast<const Geo::Ellipse *>(object0),
                *static_cast<const Geo::BSpline *>(object1), dynamic_cast<const Geo::CubicBSpline *>(object1), temp))
            {
                const size_t count = intersections.size();
                for (const Geo::Point &point : temp)
                {
                    if (std::find(intersections.begin(), intersections.end(), point) == 
                        intersections.end() && Geo::distance(point, pos) < distance)
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
                switch (Geo::is_intersected(*static_cast<const Geo::Ellipse *>(object0),
                    *static_cast<const Geo::Arc *>(object1), points[0], points[1], points[2], points[3]))
                {
                case 4:
                    if (Geo::distance(points[3], pos) < distance && std::find(intersections.begin(),
                        intersections.end(), points[3]) == intersections.end())
                    {
                        intersections.emplace_back(points[3]);
                    }
                    [[fallthrough]];
                case 3:
                    if (Geo::distance(points[2], pos) < distance && std::find(intersections.begin(),
                        intersections.end(), points[2]) == intersections.end())
                    {
                        intersections.emplace_back(points[2]);
                    }
                    [[fallthrough]];
                case 2:
                    if (Geo::distance(points[1], pos) < distance && std::find(intersections.begin(),
                        intersections.end(), points[1]) == intersections.end())
                    {
                        intersections.emplace_back(points[1]);
                    }
                    [[fallthrough]];
                case 1:
                    if (Geo::distance(points[0], pos) < distance && std::find(intersections.begin(),
                        intersections.end(), points[0]) == intersections.end())
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
            return Geo::find_intersections(*static_cast<const Geo::Polyline *>(object1),
                *static_cast<const Geo::Bezier *>(object0), pos, distance, intersections);
        case Geo::Type::POLYGON:
            return Geo::find_intersections(*static_cast<const Geo::Polygon *>(object1),
                *static_cast<const Geo::Bezier *>(object0), pos, distance, intersections);
        case Geo::Type::CIRCLE:
            if (std::vector<Geo::Point> temp; Geo::is_intersected(*static_cast<const Geo::Circle *>(object1),
                *static_cast<const Geo::Bezier *>(object0), temp))
            {
                const size_t count = intersections.size();
                for (const Geo::Point &point : temp)
                {
                    if (std::find(intersections.begin(), intersections.end(), point) == 
                        intersections.end() && Geo::distance(point, pos) < distance)
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
            if (std::vector<Geo::Point> temp; Geo::is_intersected(*static_cast<const Geo::Ellipse *>(object1),
                *static_cast<const Geo::Bezier *>(object0), temp))
            {
                const size_t count = intersections.size();
                for (const Geo::Point &point : temp)
                {
                    if (std::find(intersections.begin(), intersections.end(), point) == 
                        intersections.end() && Geo::distance(point, pos) < distance)
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
            if (std::vector<Geo::Point> temp; Geo::is_intersected(*static_cast<const Geo::Bezier *>(object0),
                *static_cast<const Geo::Bezier *>(object1), temp))
            {
                const size_t count = intersections.size();
                for (const Geo::Point &point : temp)
                {
                    if (std::find(intersections.begin(), intersections.end(), point) == 
                        intersections.end() && Geo::distance(point, pos) < distance)
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
            if (std::vector<Geo::Point> temp; Geo::is_intersected(*static_cast<const Geo::Bezier *>(object0),
                *static_cast<const Geo::BSpline *>(object1), dynamic_cast<const Geo::CubicBSpline *>(object1), temp))
            {
                const size_t count = intersections.size();
                for (const Geo::Point &point : temp)
                {
                    if (std::find(intersections.begin(), intersections.end(), point) == 
                        intersections.end() && Geo::distance(point, pos) < distance)
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
            if (std::vector<Geo::Point> temp; Geo::is_intersected(*static_cast<const Geo::Arc *>(object1),
                *static_cast<const Geo::Bezier *>(object0), temp))
            {
                const size_t count = intersections.size();
                for (const Geo::Point &point : temp)
                {
                    if (std::find(intersections.begin(), intersections.end(), point) == 
                        intersections.end() && Geo::distance(point, pos) < distance)
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
            if (std::vector<Geo::Point> temp; Geo::is_intersected(*static_cast<const Geo::Circle *>(object1),
                *static_cast<const Geo::BSpline *>(object0), dynamic_cast<const Geo::CubicBSpline *>(object0), temp))
            {
                const size_t count = intersections.size();
                for (const Geo::Point &point : temp)
                {
                    if (std::find(intersections.begin(), intersections.end(), point) == 
                        intersections.end() && Geo::distance(point, pos) < distance)
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
            if (std::vector<Geo::Point> temp; Geo::is_intersected(*static_cast<const Geo::Ellipse *>(object1),
                *static_cast<const Geo::BSpline *>(object0), dynamic_cast<const Geo::CubicBSpline *>(object0), temp))
            {
                const size_t count = intersections.size();
                for (const Geo::Point &point : temp)
                {
                    if (std::find(intersections.begin(), intersections.end(), point) == 
                        intersections.end() && Geo::distance(point, pos) < distance)
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
            if (std::vector<Geo::Point> temp; Geo::is_intersected(*static_cast<const Geo::Bezier *>(object1),
                *static_cast<const Geo::BSpline *>(object0), dynamic_cast<const Geo::CubicBSpline *>(object0), temp))
            {
                const size_t count = intersections.size();
                for (const Geo::Point &point : temp)
                {
                    if (std::find(intersections.begin(), intersections.end(), point) == 
                        intersections.end() && Geo::distance(point, pos) < distance)
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
            if (std::vector<Geo::Point> temp; Geo::is_intersected(*static_cast<const Geo::BSpline *>(object0),
                dynamic_cast<const Geo::CubicBSpline *>(object0), *static_cast<const Geo::BSpline *>(object1),
                dynamic_cast<const Geo::CubicBSpline *>(object1), temp))
            {
                const size_t count = intersections.size();
                for (const Geo::Point &point : temp)
                {
                    if (std::find(intersections.begin(), intersections.end(), point) == 
                        intersections.end() && Geo::distance(point, pos) < distance)
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
            if (std::vector<Geo::Point> temp; Geo::is_intersected(*static_cast<const Geo::Arc *>(object1),
                *static_cast<const Geo::BSpline *>(object0), dynamic_cast<const Geo::CubicBSpline *>(object0), temp))
            {
                const size_t count = intersections.size();
                for (const Geo::Point &point : temp)
                {
                    if (std::find(intersections.begin(), intersections.end(), point) == 
                        intersections.end() && Geo::distance(point, pos) < distance)
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
            return Geo::find_intersections(*static_cast<const Geo::Polyline *>(object1),
                *static_cast<const Geo::Arc *>(object0), pos, distance, intersections);
        case Geo::Type::POLYGON:
            return Geo::find_intersections(*static_cast<const Geo::Polygon *>(object1),
                *static_cast<const Geo::Arc *>(object0), pos, distance, intersections);
        case Geo::Type::CIRCLE:
            {
                Geo::Point points[2];
                switch (Geo::is_intersected(*static_cast<const Geo::Circle *>(object1),
                    *static_cast<const Geo::Arc *>(object0), points[0], points[1]))
                {
                case 2:
                    if (Geo::distance(pos, points[1]) < distance && std::find(intersections.begin(),
                        intersections.end(), points[1]) == intersections.end())
                    {
                        intersections.emplace_back(points[1]);
                    }
                    [[fallthrough]];
                case 1:
                    if (Geo::distance(pos, points[0]) < distance && std::find(intersections.begin(),
                        intersections.end(), points[0]) == intersections.end())
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
                switch (Geo::is_intersected(*static_cast<const Geo::Ellipse *>(object1),
                    *static_cast<const Geo::Arc *>(object0), points[0], points[1], points[2], points[3]))
                {
                case 4:
                    if (Geo::distance(points[3], pos) < distance && std::find(intersections.begin(),
                        intersections.end(), points[3]) == intersections.end())
                    {
                        intersections.emplace_back(points[3]);
                    }
                    [[fallthrough]];
                case 3:
                    if (Geo::distance(points[2], pos) < distance && std::find(intersections.begin(),
                        intersections.end(), points[2]) == intersections.end())
                    {
                        intersections.emplace_back(points[2]);
                    }
                    [[fallthrough]];
                case 2:
                    if (Geo::distance(points[1], pos) < distance && std::find(intersections.begin(),
                        intersections.end(), points[1]) == intersections.end())
                    {
                        intersections.emplace_back(points[1]);
                    }
                    [[fallthrough]];
                case 1:
                    if (Geo::distance(points[0], pos) < distance && std::find(intersections.begin(),
                        intersections.end(), points[0]) == intersections.end())
                    {
                        intersections.emplace_back(points[0]);
                    }
                    return true;
                default:
                    return false;
                }
            }
        case Geo::Type::BEZIER:
            if (std::vector<Geo::Point> temp; Geo::is_intersected(*static_cast<const Geo::Arc *>(object0),
                *static_cast<const Geo::Bezier *>(object1), temp))
            {
                const size_t count = intersections.size();
                for (const Geo::Point &point : temp)
                {
                    if (std::find(intersections.begin(), intersections.end(), point) == 
                        intersections.end() && Geo::distance(point, pos) < distance)
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
            if (std::vector<Geo::Point> temp; Geo::is_intersected(*static_cast<const Geo::Arc *>(object0),
                *static_cast<const Geo::BSpline *>(object1), dynamic_cast<const Geo::CubicBSpline *>(object1), temp))
            {
                const size_t count = intersections.size();
                for (const Geo::Point &point : temp)
                {
                    if (std::find(intersections.begin(), intersections.end(), point) == 
                        intersections.end() && Geo::distance(point, pos) < distance)
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
                switch ( Geo::is_intersected(*static_cast<const Geo::Arc *>(object0),
                    *static_cast<const Geo::Arc *>(object1), points[0], points[1]))
                {
                case 2:
                    if (Geo::distance(pos, points[1]) < distance && std::find(intersections.begin(),
                        intersections.end(), points[1]) == intersections.end())
                    {
                        intersections.emplace_back(points[1]);
                    }
                    [[fallthrough]];
                case 1:
                    if (Geo::distance(pos, points[0]) < distance && std::find(intersections.begin(),
                        intersections.end(), points[0]) == intersections.end())
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


bool Geo::NoAABBTest::is_inside(const Geo::Point &point, const Geo::Polygon &polygon, const bool coincide)
{
    if (coincide)
    {
        for (size_t i = 1, len = polygon.size(); i < len; ++i)
        {
            if (Geo::is_inside(point, polygon[i-1], polygon[i]))
            {
                return true;
            }
        }
    }
    else
    {
        for (size_t i = 1, len = polygon.size(); i < len; ++i)
        {
            if (Geo::is_inside(point, polygon[i-1], polygon[i]))
            {
                return false;
            }
        }
    }

    double x = (-DBL_MAX);
    std::vector<Geo::MarkedPoint> points;
    for (const Geo::Point &p : polygon)
    {
        x = std::max(x, p.x);
        points.emplace_back(p.x, p.y);
    }

    Geo::Point temp, end(x + 80, point.y); // 找到交点并计算其几何数
    for (size_t i = 1, count = points.size(); i < count; ++i)
    {
        if (!Geo::is_parallel(point, end, points[i], points[i - 1]) &&
            Geo::is_intersected(point, end, points[i], points[i - 1], temp))
        {
            points.insert(points.begin() + i++, MarkedPoint(temp.x, temp.y, false));
            ++count;
            if (Geo::cross(temp, end, points[i], points[i - 2]) >= 0)
            {
                points[i - 1].value = -1;
            }
            else
            {
                points[i - 1].value = 1;
            }
        }
    }

    if (points.size() == polygon.size()) // 无交点
    {
        return false;
    }

    // 去除重复交点
    {
        const size_t i = points.size() - 1;
        size_t j0, j1;
        size_t count = points[i].original ? 0 : 1;
        for (j0 = i; j0 > 0; --j0)
        {
            if (std::abs(points[i].x - points[j0 - 1].x) > Geo::EPSILON || 
                std::abs(points[i].y - points[j0 - 1].y) > Geo::EPSILON)
            {
                break;
            }
            if (!points[j0 - 1].original)
            {
                ++count;
            }
        }
        for (j1 = 0; j1 < i; ++j1)
        {
            if (std::abs(points[i].x - points[j1].x) > Geo::EPSILON || 
                std::abs(points[i].y - points[j1].y) > Geo::EPSILON)
            {
                break;
            }
            if (!points[j1].original)
            {
                ++count;
            }
        }
        if (count >= 2)
        {
            int value = 0;
            for (size_t k = i; k > j0; --k)
            {
                if (!points[k].original)
                {
                    value += points[k].value;
                }
            }
            if (!points[j0].original)
            {
                value += points[j0].value;
            }
            for (size_t k = 0; k <= j1; ++k)
            {
                if (!points[k].original)
                {
                    value += points[k].value;
                }
            }
            if (value == 0)
            {
                for (size_t k = i; k > j0; --k)
                {
                    if (!points[k].original)
                    {
                        points.erase(points.begin() + k);
                    }
                }
                if (!points[j0].original)
                {
                    points.erase(points.begin() + j0);
                }
                for (size_t k = 0; k <= j1; ++k)
                {
                    if (!points[k].original)
                    {
                        points.erase(points.begin() + k);
                    }
                }
            }
            else
            {
                bool flag = false;
                for (size_t k = i; k > j0; --k)
                {
                    flag = (flag || points[k].original);
                    points.erase(points.begin() + k);
                }
                flag = (flag || points[j0].original);
                points.erase(points.begin() + j0);
                for (size_t k = j1; k > 0; --k)
                {
                    flag = (flag || points[k].original);
                    points.erase(points.begin() + k);
                }
                points[0].value = value;
                points[0].original = (flag || points[j0].original);
            }
        }
    }
    for (size_t count, j, i = points.size() - 2; i > 0; --i)
    {
        count = points[i].original ? 0 : 1;
        for (j = i; j > 0; --j)
        {
            if (std::abs(points[i].x - points[j - 1].x) > Geo::EPSILON || 
                std::abs(points[i].y - points[j - 1].y) > Geo::EPSILON)
            {
                break;
            }
            if (!points[j - 1].original)
            {
                ++count;
            }
        }
        if (count < 2)
        {
            continue;
        }

        int value = 0;
        for (size_t k = i; k > j; --k)
        {
            if (!points[k].original)
            {
                value += points[k].value;
            }
        }
        if (!points[j].original)
        {
            value += points[j].value;
        }
        if (value == 0)
        {
            for (size_t k = i; k > j; --k)
            {
                if (!points[k].original)
                {
                    points.erase(points.begin() + k);
                }
            }
            if (!points[j].original)
            {
                points.erase(points.begin() + j);
            }
        }
        else
        {
            bool flag = false;
            for (size_t k = i; k > j; --k)
            {
                flag = (flag || points[k].original);
                points.erase(points.begin() + k);
            }
            points[j].value = value;
            points[j].original = (flag || points[j].original);
        }
        i = j > 0 ? j : 1;
    }

    // 处理重边上的交点
    for (size_t i = 0, j = 1, count = points.size(); j < count; i = j)
    {
        while (i < count && points[i].value == 0)
        {
            ++i;
        }
        j = i + 1;
        while (j < count && points[j].value == 0)
        {
            ++j;
        }
        if (j >= count)
        {
            break;
        }
        if (polygon.index(points[i]) == SIZE_MAX || polygon.index(points[j]) == SIZE_MAX)
        {
            continue;
        }

        if (points[i].value > 0 && points[j].value > 0)
        {
            points.erase(points.begin() + j);
            --count;
        }
        else if (points[i].value < 0 && points[j].value < 0)
        {
            points.erase(points.begin() + i);
            --count;
        }
        else
        {
            points.erase(points.begin() + j--);
            points.erase(points.begin() + i);
            --count;
            --count;
        }
    }

    return std::count_if(points.begin(), points.end(), [](const Geo::MarkedPoint &p) { return p.value != 0; }) % 2 == 1;
}

bool Geo::NoAABBTest::is_intersected(const Geo::Polyline &polyline0, const Geo::Polyline &polyline1)
{
    Geo::Point point;
    for (size_t i = 1, count0 = polyline0.size(); i < count0; ++i)
    {
        for (size_t j = 1, count1 = polyline1.size(); j < count1; ++j)
        {
            if (Geo::is_intersected(polyline0[i-1], polyline0[i], polyline1[j-1], polyline1[j], point))
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
            if (Geo::is_intersected(polyline[i-1], polyline[i], polygon[j-1], polygon[j], point))
            {
                return true;
            }
            else if (inside && Geo::NoAABBTest::is_inside(polyline[i-1], polygon))
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
            if (Geo::is_intersected(polygon0[i-1], polygon0[i], polygon1[j-1], polygon1[j], point))
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


bool Geo::is_on_left(const Point &point, const Point &start, const Point &end)
{
    return (end.x - start.x) * (point.y - start.y) -
        (end.y - start.y) * (point.x - start.x) > 0;
}


bool Geo::is_point_on(const Point &point, const Triangle &triangle)
{
    return ((Geo::cross(triangle[1] - triangle[0], point - triangle[0]) == 0 &&
        Geo::distance(point, triangle[0]) + Geo::distance(point, triangle[1]) < Geo::distance(triangle[0], triangle[1]) + Geo::EPSILON)
        || (Geo::cross(triangle[2] - triangle[1], point - triangle[1]) == 0 &&
        Geo::distance(point, triangle[1]) + Geo::distance(point, triangle[2]) < Geo::distance(triangle[1], triangle[2]) + Geo::EPSILON)
        || (Geo::cross(triangle[2] - triangle[0], point - triangle[0]) == 0) &&
        Geo::distance(point, triangle[0]) + Geo::distance(point, triangle[2]) < Geo::distance(triangle[0], triangle[2]) + Geo::EPSILON);
}

bool Geo::is_point_on(const Geo::Point &point, std::vector<Geo::Point>::const_iterator begin, std::vector<Geo::Point>::const_iterator end)
{
    for (std::vector<Geo::Point>::const_iterator it0 = begin, it1 = begin + 1; it1 != end; ++it1, ++it0)
    {
        if (Geo::is_inside(point, *it0, *it1))
        {
            return true;
        }
    }
    return false;
}


bool Geo::is_Rectangle(const Polygon &polygon)
{
    Geo::Polygon points(polygon);
    if (polygon.size() > 5)
    {
        for (size_t i = 2, count = polygon.size(); i < count; ++i)
        {
            if (Geo::is_inside(points[i - 1], points[i - 2], points[i]))
            {
                points.remove(--i);
                --count;
            }
        }
    }
    if (polygon.size() != 5)
    {
        return false;
    }

    if (Geo::distance_square(points[0], points[1]) == Geo::distance_square(points[2], points[3]) &&
        Geo::distance_square(points[1], points[2]) == Geo::distance_square(points[0], points[3]))
    {
        const Geo::Point vec0 = points[0] - points[1], vec1 = points[2] - points[1];
        return std::abs(vec0.x * vec1.x + vec0.y * vec1.y) == 0;
    }
    else
    {
        return false;
    }
}


double Geo::cross(const double x0, const double y0, const double x1, const double y1)
{
    return x0 * y1 - x1 * y0;
}

double Geo::cross(const Vector &vec0, const Vector &vec1)
{
    return vec0.x * vec1.y - vec1.x * vec0.y;
}

double Geo::cross(const Point &start0, const Point &end0, const Point &start1, const Point &end1)
{
    return Geo::cross(end0 - start0, end1 - start1);
}


bool Geo::foot_point(const Point &start, const Point &end, const Point &point, Point &foot, const bool infinite)
{
    if (start.x == end.x)
    {
        foot.x = start.x;
        foot.y = point.y;
        if (infinite)
        {
            return true;
        }
        else
        {
            return foot.y >= std::min(start.y, end.y) && foot.y <= std::max(start.y, end.y);
        }
    }
    else
    {
        const double k = (end.y - start.y) / (end.x - start.x);
        const double b = start.y -  k * start.x;
        foot.x = (point.x - k * b + k * point.y) / (1 + k * k);
        foot.y = (k * point.x + k * k * point.y + b) / (1 + k * k);
        if (infinite)
        {
            return true;
        }
        else
        {
            return foot.x >= std::min(start.x, end.x) && foot.x <= std::max(start.x, end.x);
        }
    }
}

bool Geo::foot_point(const Circle &circle, const Point &point, Point &output0, Point &output1)
{
    if (point == circle || Geo::distance(point, circle) == circle.radius)
    {
        return false;
    }
    const double dx = point.x - circle.x;
    const double dy = point.y - circle.y;
    const double dist = std::hypot(dx, dy);
    output0.x = circle.x + circle.radius * dx / dist;
    output0.y = circle.y + circle.radius * dy / dist;
    output1.x = circle.x - circle.radius * dx / dist;
    output1.y = circle.y - circle.radius * dy / dist;
    return true;
}

int Geo::foot_point(const Ellipse &ellipse, const Point &point, std::vector<Point> &output)
{
    if (Geo::distance(ellipse.c0(), point) + Geo::distance(ellipse.c1(), point) == std::max(ellipse.lengtha(), ellipse.lengthb()) * 2)
    {
        return 0;
    }
    const Geo::Point center = ellipse.center();
    const double angle = Geo::angle(ellipse.a0(), ellipse.a1());
    Geo::Point coord = Geo::to_coord(point, center.x, center.y, angle);
    const double a = Geo::distance(ellipse.a0(), ellipse.a1()) / 2;
    const double b = Geo::distance(ellipse.b0(), ellipse.b1()) / 2;
    const double aa = Geo::distance_square(ellipse.a0(), ellipse.a1()) / 4;
    const double bb = Geo::distance_square(ellipse.b0(), ellipse.b1()) / 4;
    Math::EllipseFootParameter parameter;
    parameter.a = b * coord.y;
    parameter.b = -a * coord.x;
    parameter.c = aa - bb;
    std::vector<double> ts;
    for (double t = 0; t < Geo::PI * 2; t += 0.2)
    {
        if (const double t2 = Geo::rad_to_2PI(Math::solve_ellipse_foot(parameter, t)); std::find_if(ts.begin(),
            ts.end(), [=](const double value) { return std::abs(value - t2) < Geo::EPSILON; }) == ts.end())
        {
            coord = Geo::to_coord(Geo::Point(0, 0), center.x, center.y, angle);
            Geo::Point result(a * std::cos(t2), b * std::sin(t2));
            output.emplace_back(Geo::to_coord(result, coord.x, coord.y, -angle));
            ts.push_back(t2);
        }
    }
    return ts.size();
}

int Geo::foot_point(const Point &point, const Bezier &bezier, std::vector<Point> &output,
    std::vector<std::tuple<size_t, double, double, double>> *tvalues)
{
    const size_t order = bezier.order();
    std::vector<int> nums(order, 1);
    switch (order - 1)
    {
    case 1:
        break;
    case 2:
        nums[1] = 2;
        break;
    case 3:
        nums[1] = nums[2] = 3;
        break;
    default:
        {
            std::vector<int> temp(1, 1);
            for (size_t i = 1; i <= order; ++i)
            {
                for (size_t j = 1; j < i; ++j)
                {
                    nums[j] = temp[j - 1] + temp[j];
                }
                temp.assign(nums.begin(), nums.begin() + i + 1);
            }
        }
        break;
    }
    std::vector<int> nums1(order + 1, 1);
    switch (order)
    {
    case 2:
        nums1[1] = 2;
        break;
    case 3:
        nums1[1] = nums1[2] = 3;
        break;
    default:
        {
            std::vector<int> temp(1, 1);
            for (size_t i = 1; i <= order; ++i)
            {
                for (size_t j = 1; j < i; ++j)
                {
                    nums1[j] = temp[j - 1] + temp[j];
                }
                temp.assign(nums1.begin(), nums1.begin() + i + 1);
            }
        }
        break;
    }

    std::vector<Geo::Point> result;
    std::vector<std::tuple<size_t, double, double, double>> temp;
    for (size_t i = 0, end = bezier.size() - order; i < end; i += order)
    {
        double t0 = 0, t1 = Geo::Bezier::default_step;
        double angles[2] = {1, 1};
        angles[0] = Geo::angle(bezier[i], point, bezier[i], bezier[i + 1]);
        std::vector<std::tuple<double, double>> pairs;
        while (t1 <= 1)
        {
            Geo::Point head, tail;
            for (size_t j = 0; j < order; ++j)
            {
                head += (bezier[i + j] * (nums[j] * std::pow(1 - t1, order - j - 1) * std::pow(t1, j)));
                tail += (bezier[i + j + 1] * (nums[j] * std::pow(1 - t1, order - j - 1) * std::pow(t1, j)));
            }
            Geo::Point coord;
            for (size_t j = 0; j <= order; ++j)
            {
                coord += (bezier[j + i] * (nums1[j] * std::pow(1 - t1, order - j) * std::pow(t1, j))); 
            }
            head.rotate(coord.x, coord.y, Geo::PI / 2);
            tail.rotate(coord.x, coord.y, Geo::PI / 2);
            angles[1] = Geo::cross(coord, point, head, tail);
            if (angles[0] * angles[1] <= 0)
            {
                pairs.emplace_back(t0, t1);
            }
            angles[0] = angles[1];
            t0 = t1;
            t1 += Geo::Bezier::default_step;
        }
        for (auto [t0, t1] : pairs)
        {
            double last_t = -1;
            {
                Geo::Point coord;
                for (size_t j = 0; j <= order; ++j)
                {
                    coord += (bezier[i + j] * (nums1[j] * std::pow(1 - t0, order - j) * std::pow(t0, j))); 
                }
                Geo::Point head, tail;
                for (size_t j = 0; j < order; ++j)
                {
                    head += (bezier[i + j] * (nums[j] * std::pow(1 - t0, order - j - 1) * std::pow(t0, j)));
                    tail += (bezier[i + j + 1] * (nums[j] * std::pow(1 - t0, order - j - 1) * std::pow(t0, j)));
                }
                head.rotate(coord.x, coord.y, Geo::PI / 2);
                tail.rotate(coord.x, coord.y, Geo::PI / 2);
                angles[0] = Geo::cross(coord, point, head, tail);
            }
            {
                Geo::Point coord;
                for (size_t j = 0; j <= order; ++j)
                {
                    coord += (bezier[i + j] * (nums1[j] * std::pow(1 - t1, order - j) * std::pow(t1, j))); 
                }
                Geo::Point head, tail;
                for (size_t j = 0; j < order; ++j)
                {
                    head += (bezier[i + j] * (nums[j] * std::pow(1 - t1, order - j - 1) * std::pow(t1, j)));
                    tail += (bezier[i + j + 1] * (nums[j] * std::pow(1 - t1, order - j - 1) * std::pow(t1, j)));
                }
                head.rotate(coord.x, coord.y, Geo::PI / 2);
                tail.rotate(coord.x, coord.y, Geo::PI / 2);
                angles[1] = Geo::cross(coord, point, head, tail);
            }
            double t = (t0 + t1) / 2;
            while (t1 - t0 > 1e-14 && last_t != t && angles[0] * angles[1] != 0)
            {
                Geo::Point coord;
                for (size_t j = 0; j <= order; ++j)
                {
                    coord += (bezier[i + j] * (nums1[j] * std::pow(1 - t, order - j) * std::pow(t, j))); 
                }
                Geo::Point head, tail;
                for (size_t j = 0; j < order; ++j)
                {
                    head += (bezier[i + j] * (nums[j] * std::pow(1 - t, order - j - 1) * std::pow(t, j)));
                    tail += (bezier[i + j + 1] * (nums[j] * std::pow(1 - t, order - j - 1) * std::pow(t, j)));
                }
                head.rotate(coord.x, coord.y, Geo::PI / 2);
                tail.rotate(coord.x, coord.y, Geo::PI / 2);
                const double angle = Geo::angle(coord, point, head, tail);
                if (angle * angles[0] <= 0)
                {
                    t1 = t;
                    angles[1] = angle;
                }
                else if (angle * angles[1] <= 0)
                {
                    t0 = t;
                    angles[0] = angle;
                }
                last_t = t;
                t = (t0 + t1) / 2;
            }

            if (angles[0] == 0)
            {
                t1 = t0;
            }
            else if (angles[1] != 0)
            {
                t1 = (t0 + t1) / 2;
            }
            Geo::Point coord;
            for (size_t j = 0; j <= order; ++j)
            {
                coord += (bezier[i + j] * (nums1[j] * std::pow(1 - t1, order - j) * std::pow(t1, j))); 
            }
            Geo::Point head, tail;
            for (size_t j = 0; j < order; ++j)
            {
                head += (bezier[i + j] * (nums[j] * std::pow(1 - t1, order - j - 1) * std::pow(t1, j)));
                tail += (bezier[i + j + 1] * (nums[j] * std::pow(1 - t1, order - j - 1) * std::pow(t1, j)));
            }
            head.rotate(coord.x, coord.y, Geo::PI / 2);
            tail.rotate(coord.x, coord.y, Geo::PI / 2);
            if (std::abs(Geo::angle(coord, point, head, tail)) < 1e-4 ||
                std::abs(std::abs(Geo::angle(coord, point, head, tail)) - Geo::PI) < 1e-4)
            {
                result.emplace_back(coord);
                temp.emplace_back(i, t1, coord.x, coord.y);
            }
        }
    }

    output.insert(output.end(), result.begin(), result.end());
    if (tvalues != nullptr)
    {
        tvalues->insert(tvalues->end(), temp.begin(), temp.end());
    }
    return result.size();
}

int Geo::foot_point(const Point &point, const BSpline &bspline, std::vector<Point> &output,
    std::vector<std::tuple<double, double, double>> *tvalues)
{
    std::vector<std::tuple<double, double>> pairs;
    {
        const Geo::Point origin(0, 0);
        const std::vector<double> &knots = bspline.knots();
        double t0 = knots[0], t1 = knots[0] + 1e-3;
        double angles[2] = {Geo::angle(bspline.at(t0), point, origin, bspline.vertical(t0)), 1};
        while (t1 <= knots.back())
        {
            angles[1] = Geo::angle(bspline.at(t1), point, origin, bspline.vertical(t1));
            if (angles[0] * angles[1] <= 0)
            {
                pairs.emplace_back(t0, t1);
            }
            angles[0] = angles[1];
            t0 = t1;
            t1 += 1e-3;
        }
    }
    std::vector<Geo::Point> result;
    std::vector<std::tuple<double, double, double>> temp;
    for (auto [t0, t1] : pairs)
    {
        const Geo::Point origin(0, 0);
        double angles[2] = { Geo::angle(bspline.at(t0), point, origin, bspline.vertical(t0)),
            Geo::angle(bspline.at(t1), point, origin, bspline.vertical(t1)) };
        double last_t = -1, t = (t0 + t1) / 2;
        while (t1 - t0 > 1e-14 && last_t != t && angles[0] * angles[1] != 0)
        {
            const double angle = Geo::angle(bspline.at(t), point, origin, bspline.vertical(t));
            if (angle * angles[0] <= 0)
            {
                t1 = t;
                angles[1] = angle;
            }
            else if (angle * angles[1] <= 0)
            {
                t0 = t;
                angles[0] = angle;
            }
            last_t = t;
            t = (t0 + t1) / 2;
        }

        if (angles[0] == 0)
        {
            t1 = t0;
        }
        else if (angles[1] != 0)
        {
            t1 = (t0 + t1) / 2;
        }
        if (const Geo::Point coord = bspline.at(t1); 
            std::abs(Geo::angle(coord, point, origin, bspline.vertical(t1))) < 1e-4 ||
            std::abs(std::abs(Geo::angle(coord, point, origin, bspline.vertical(t1))) - Geo::PI) < 1e-4)
        {
            result.emplace_back(coord);
            temp.emplace_back(t1, coord.x, coord.y);
        }
    }

    output.insert(output.end(), result.begin(), result.end());
    if (tvalues != nullptr)
    {
        tvalues->insert(tvalues->end(), temp.begin(), temp.end());
    }
    return result.size();
}


int Geo::closest_point(const Polyline &polyline, const Point &point, std::vector<Point> &output)
{
    std::vector<size_t> min_indexs({1});
    double min_dis = Geo::distance(point, polyline[0], polyline[1], false);
    for (size_t i = 2, count = polyline.size(); i < count; ++i)
    {
        if (double dis = Geo::distance(point, polyline[i - 1], polyline[i], false); dis == min_dis)
        {
            min_indexs.push_back(i);
        }
        else if (dis < min_dis)
        {
            min_indexs.clear();
            min_dis = dis;
            min_indexs.push_back(i);
        }
    }
    std::vector<Point> temp;
    for (size_t i : min_indexs)
    {
        if (Geo::Point foot; Geo::foot_point(polyline[i - 1], polyline[i], point, foot, false))
        {
            if (std::find(temp.begin(), temp.end(), foot) == temp.end())
            {
                temp.emplace_back(foot);
            }
        }
        else
        {
            foot = Geo::distance(polyline[i - 1], point) <= Geo::distance(polyline[i], point) ? polyline[i - 1] : polyline[i];
            if (std::find(temp.begin(), temp.end(), foot) == temp.end())
            {
                temp.emplace_back(foot);
            }
        }
    }
    output.insert(output.end(), temp.begin(), temp.end());
    return temp.size();
}

int Geo::closest_point(const Polygon &polygon, const Point &point, std::vector<Point> &output)
{
    std::vector<size_t> min_indexs({1});
    double min_dis = Geo::distance(point, polygon[0], polygon[1], false);
    for (size_t i = 2, count = polygon.size(); i < count; ++i)
    {
        if (double dis = Geo::distance(point, polygon[i - 1], polygon[i], false); dis == min_dis)
        {
            min_indexs.push_back(i);
        }
        else if (dis < min_dis)
        {
            min_indexs.clear();
            min_dis = dis;
            min_indexs.push_back(i);
        }
    }
    std::vector<Point> temp;
    for (size_t i : min_indexs)
    {
        if (Geo::Point foot; Geo::foot_point(polygon[i - 1], polygon[i], point, foot, false))
        {
            if (std::find(temp.begin(), temp.end(), foot) == temp.end())
            {
                temp.emplace_back(foot);
            }
        }
        else
        {
            foot = Geo::distance(polygon[i - 1], point) <= Geo::distance(polygon[i], point) ? polygon[i - 1] : polygon[i];
            if (std::find(temp.begin(), temp.end(), foot) == temp.end())
            {
                temp.emplace_back(foot);
            }
        }
    }
    output.insert(output.end(), temp.begin(), temp.end());
    return temp.size();
}

int Geo::closest_point(const Ellipse &ellipse, const Point &point, std::vector<Point> &output)
{   
    if (const Geo::Point center = ellipse.center(); center == point)
    {
        if (ellipse.is_arc())
        {
            const Geo::Point start(ellipse.arc_point0()), end(ellipse.arc_point1());
            double angles[4] = { Geo::angle(start, center, ellipse.a0()),
                Geo::angle(start, center, ellipse.a1()),
                Geo::angle(start, center, ellipse.b0()),
                Geo::angle(start, center, ellipse.b1()) };
            bool mask[4] = { false, false, false, false }; // a0, a1, b0, b1
            for (int i = 0; i < 4; ++i)
            {
                if (angles[i] < 0)
                {
                    angles[i] += Geo::PI * 2;
                }
                if (ellipse.arc_angle0() <= angles[i] && angles[i] <= ellipse.arc_angle1())
                {
                    mask[i] = true; // 判断椭圆弧是否经过轴端点
                }
            }
            if ((mask[0] || mask[1]) && (mask[2] || mask[3])) // 椭圆弧经过a轴和b轴端点
            {
                if (ellipse.lengtha() < ellipse.lengthb())
                {
                    if (mask[0])
                    {
                        output.emplace_back(ellipse.a0());
                    }
                    if (mask[1])
                    {
                        output.emplace_back(ellipse.a1());
                    }
                    return (mask[0] ? 1 : 0) + (mask[1] ? 1 : 0);
                }
                else
                {
                    if (mask[2])
                    {
                        output.emplace_back(ellipse.b0());
                    }
                    if (mask[3])
                    {
                        output.emplace_back(ellipse.b1());
                    }
                    return (mask[2] ? 1 : 0) + (mask[3] ? 1 : 0);
                }
            }
            else if (mask[0] || mask[1]) // 椭圆弧经过a轴端点
            {
                if (ellipse.lengtha() < std::min(Geo::distance(point, ellipse.arc_point0()),
                    Geo::distance(point, ellipse.arc_point1())))
                {
                    output.emplace_back(mask[0] ? ellipse.a0() : ellipse.a1());
                    return 1;
                }
                else
                {
                    if (Geo::distance(point, ellipse.arc_point0()) < Geo::distance(point, ellipse.arc_point1()))
                    {
                        output.emplace_back(ellipse.arc_point0());
                        return 1;
                    }
                    else if (Geo::distance(point, ellipse.arc_point0()) < Geo::distance(point, ellipse.arc_point1()))
                    {
                        output.emplace_back(ellipse.arc_point1());
                        return 1;
                    }
                    else
                    {
                        output.emplace_back(ellipse.arc_point0());
                        output.emplace_back(ellipse.arc_point1());
                        return 2;
                    }
                }
            }
            else if (mask[2] || mask[3]) // 椭圆弧经过b轴端点
            {
                if (ellipse.lengthb(), std::min(Geo::distance(point, ellipse.arc_point0()),
                    Geo::distance(point, ellipse.arc_point1())))
                {
                    output.emplace_back(mask[2] ? ellipse.b0() : ellipse.b1());
                    return 1;
                }
                else
                {
                    if (Geo::distance(point, ellipse.arc_point0()) < Geo::distance(point, ellipse.arc_point1()))
                    {
                        output.emplace_back(ellipse.arc_point0());
                        return 1;
                    }
                    else if (Geo::distance(point, ellipse.arc_point0()) < Geo::distance(point, ellipse.arc_point1()))
                    {
                        output.emplace_back(ellipse.arc_point1());
                        return 1;
                    }
                    else
                    {
                        output.emplace_back(ellipse.arc_point0());
                        output.emplace_back(ellipse.arc_point1());
                        return 2;
                    }
                }
            }
            else // 椭圆弧不经过轴端点
            {
                if (Geo::distance(point, ellipse.arc_point0()) < Geo::distance(point, ellipse.arc_point1()))
                {
                    output.emplace_back(ellipse.arc_point0());
                    return 1;
                }
                else if (Geo::distance(point, ellipse.arc_point0()) > Geo::distance(point, ellipse.arc_point1()))
                {
                    output.emplace_back(ellipse.arc_point1());
                    return 1;
                }
                else
                {
                    output.emplace_back(ellipse.arc_point0());
                    output.emplace_back(ellipse.arc_point1());
                    return 2;
                }
            }
        }
        else
        {
            if (ellipse.lengtha() < ellipse.lengthb())
            {
                output.emplace_back(ellipse.a0());
                output.emplace_back(ellipse.a1());
            }
            else
            {
                output.emplace_back(ellipse.b0());
                output.emplace_back(ellipse.b1());
            }
            return 2;
        }
    }
    else
    {
        const Geo::Point coord = Geo::to_coord(point, center.x, center.y, Geo::angle(ellipse.a0(), ellipse.a1()));
        const double a = ellipse.lengtha(), b = ellipse.lengthb();
        double degree0 = Geo::angle(Geo::Point(0, 0), coord) - Geo::PI / 2,
            degree1 = Geo::angle(Geo::Point(0, 0), coord) + Geo::PI / 2;
        double last_degree0 = degree0 - 1, last_degree1 = degree1 - 1;
        double m0 = (degree1 - degree0) / 3 + degree0, m1 = degree1 - (degree1 - degree0) / 3;
        double x0, y0, x1, y1;
        while (degree1 * 1e16 - degree0 * 1e16 > 1 && (last_degree0 != degree0 || last_degree1 != degree1))
        {
            last_degree0 = degree0, last_degree1 = degree1;
            m0 = (degree1 - degree0) / 3 + degree0, m1 = degree1 - (degree1 - degree0) / 3;
            x0 = a * std::cos(m0), y0 = b * std::sin(m0);
            x1 = a * std::cos(m1), y1 = b * std::sin(m1);
            if (Geo::distance_square(x0, y0, coord.x, coord.y) > Geo::distance_square(x1, y1, coord.x, coord.y))
            {
                degree0 = m0;
            }
            else
            {
                degree1 = m1;
            }
        }
        if (ellipse.is_arc())
        {
            double angle0 = Geo::rad_to_2PI(ellipse.arc_angle0());
            double angle1 = Geo::rad_to_2PI(ellipse.arc_angle1());
            angle0 = angle0 < angle1 ? angle1 - angle0 : Geo::PI * 2 - angle0 + angle1;
            angle1 = Geo::angle(ellipse.arc_point0(), center, point);
            if (angle1 < 0)
            {
                angle1 += Geo::PI * 2;
            }
            if (angle1 <= angle0)
            {
                const double angle = Geo::angle(ellipse.a0(), ellipse.a1());
                const Geo::Point coord2 = Geo::to_coord(Geo::Point(0, 0), center.x, center.y, angle);
                if (Geo::distance(x0, y0, coord.x, coord.y) < Geo::distance(x1, y1, coord.x, coord.y))
                {
                    output.emplace_back(Geo::to_coord(Geo::Point(x0, y0), coord2.x, coord2.y, -angle));
                    return 1;
                }
                else if (Geo::distance(x0, y0, coord.x, coord.y) > Geo::distance(x1, y1, coord.x, coord.y))
                {
                    output.emplace_back(Geo::to_coord(Geo::Point(x1, y1), coord2.x, coord2.y, -angle));
                    return 1;
                }
                else
                {
                    output.emplace_back(Geo::to_coord(Geo::Point(x0, y0), coord2.x, coord2.y, -angle));
                    output.emplace_back(Geo::to_coord(Geo::Point(x1, y1), coord2.x, coord2.y, -angle));
                    return 2;
                }
            }
            else
            {
                if (Geo::distance(point, ellipse.arc_point0()) < Geo::distance(point, ellipse.arc_point1()))
                {
                    output.emplace_back(ellipse.arc_point0());
                    return 1;
                }
                else if (Geo::distance(point, ellipse.arc_point0()) > Geo::distance(point, ellipse.arc_point1()))
                {
                    output.emplace_back(ellipse.arc_point1());
                    return 1;
                }
                else
                {
                    output.emplace_back(ellipse.arc_point0());
                    output.emplace_back(ellipse.arc_point1());
                    return 2;
                }
            }
        }
        else
        {
            const double angle = Geo::angle(ellipse.a0(), ellipse.a1());
            const Geo::Point coord2 = Geo::to_coord(Geo::Point(0, 0), center.x, center.y, angle);
            if (Geo::distance(x0, y0, coord.x, coord.y) < Geo::distance(x1, y1, coord.x, coord.y))
            {
                output.emplace_back(Geo::to_coord(Geo::Point(x0, y0), coord2.x, coord2.y, -angle));
                return 1;
            }
            else if (Geo::distance(x0, y0, coord.x, coord.y) < Geo::distance(x1, y1, coord.x, coord.y))
            {
                output.emplace_back(Geo::to_coord(Geo::Point(x1, y1), coord2.x, coord2.y, -angle));
                return 1;
            }
            else
            {
                output.emplace_back(Geo::to_coord(Geo::Point(x0, y0), coord2.x, coord2.y, -angle));
                output.emplace_back(Geo::to_coord(Geo::Point(x1, y1), coord2.x, coord2.y, -angle));
                return 2;
            }
        }
    }
}

int Geo::closest_point(const Bezier &bezier, const Point &point, std::vector<Point> &output)
{
    const size_t order = bezier.order();
    std::vector<int> nums(order + 1, 1);
    switch (order)
    {
    case 2:
        nums[1] = 2;
        break;
    case 3:
        nums[1] = nums[2] = 3;
        break;
    default:
        {
            std::vector<int> temp(1, 1);
            for (size_t i = 1; i <= order; ++i)
            {
                for (size_t j = 1; j < i; ++j)
                {
                    nums[j] = temp[j - 1] + temp[j];
                }
                temp.assign(nums.begin(), nums.begin() + i + 1);
            }
        }
        break;
    }

    // index, points, distance
    std::vector<std::tuple<size_t, std::vector<Geo::Point>, double>> temp;
    for (size_t i = 0, end = bezier.size() - order; i < end; i += order)
    {
        Geo::Polyline polyline;
        polyline.append(bezier[i]);
        double t = 0;
        while (t <= 1)
        {
            Geo::Point point;
            for (size_t j = 0; j <= order; ++j)
            {
                point += (bezier[j + i] * (nums[j] * std::pow(1 - t, order - j) * std::pow(t, j))); 
            }
            polyline.append(point);
            t += Geo::Bezier::default_step;
        }
        polyline.append(bezier[i + order]);
        Geo::down_sampling(polyline, Geo::Bezier::default_down_sampling_value);

        std::vector<Geo::Point> points;
        Geo::closest_point(polyline, point, points);
        temp.emplace_back(i, points, Geo::distance(point, points.front()));
    }
    std::sort(temp.begin(), temp.end(), [](const auto &a, const auto &b)
        { return std::get<2>(a) < std::get<2>(b); });
    while (temp.size() > 1)
    {
        if (std::get<2>(temp.back()) - std::get<2>(temp.front()) > 1.0)
        {
            temp.pop_back();
        }
        else
        {
            break;
        }
    }

    std::vector<Geo::Point> result;
    for (const auto &[i, points, dis] : temp)
    {
        double t = 0;
        double step = 1e-3, lower = 0, upper = 1;
        double min_dis[2] = {DBL_MAX, DBL_MAX};
        do
        {
            for (double x = lower; x < upper + step; x += step)
            {
                x = x < upper ? x : upper;
                Geo::Point coord;
                for (size_t j = 0; j <= order; ++j)
                {
                    coord += (bezier[j + i] * (nums[j] * std::pow(1 - x, order - j) * std::pow(x, j))); 
                }
                if (double dis = Geo::distance(point, coord); dis < min_dis[1])
                {
                    min_dis[1] = dis;
                    t = x;
                }
            }
            lower = std::max(0.0, t - step);
            upper = std::min(1.0, t + step);
            step = (upper - lower) / 100;
            if (min_dis[0] > min_dis[1])
            {
                min_dis[0] = min_dis[1];
            }
        }
        while (std::abs(min_dis[0] - min_dis[1]) > 1e-4 && step > 1e-12);

        step = 1e-3, lower = std::max(0.0, t - 0.1), upper = std::min(1.0, t + 0.1);
        min_dis[0] = min_dis[1] = DBL_MAX;
        step = (upper - lower) / 100;
        std::vector<double> stored_t;
        while ((upper - lower) * 1e15 > 1)
        {
            int flag = 0;
            for (double x = lower, dis0 = 0; x < upper + step; x += step)
            {
                x = x < upper ? x : upper;
                Geo::Point coord;
                for (size_t j = 0; j <= order; ++j)
                {
                    coord += (bezier[j + i] * (nums[j] * std::pow(1 - x, order - j) * std::pow(x, j)));
                }
                if (const double dis = Geo::distance(coord, point) * 1e9; dis < min_dis[1])
                {
                    min_dis[1] = dis;
                    t = x;
                }
                else if (dis == min_dis[1]) // 需要扩大搜索范围
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
            if (min_dis[1] < 2e-5)
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
                step = (upper - lower) / 100;
            }
            else
            {
                lower = std::max(0.0, t - step * 2);
                upper = std::min(1.0, t + step * 2);
                step = (upper - lower) / 100;
            }
        }

        Geo::Point coord;
        for (size_t j = 0; j <= order; ++j)
        {
            coord += (bezier[j + i] * (nums[j] * std::pow(1 - t, order - j) * std::pow(t, j))); 
        }
        result.emplace_back(coord);
    }

    std::sort(result.begin(), result.end(), [&](const Geo::Point &a, const Geo::Point &b)
        { return Geo::distance(a, point) < Geo::distance(b, point); });
    while (result.size() > 1)
    {
        if (Geo::distance(point, result.back()) > Geo::distance(point, result.front()))
        {
            result.pop_back();
        }
        else
        {
            break;
        }
    }
    output.insert(output.end(), result.begin(), result.end());
    return result.size();
}

int Geo::closest_point(const BSpline &bspline, const bool is_cubic, const Point &point, std::vector<Point> &output)
{
    const std::vector<double> &knots = bspline.knots();
    const size_t npts = bspline.control_points.size();
    const size_t nplusc = npts + (is_cubic ? 4 : 3);
    const size_t p1 = std::max(npts * 8.0, bspline.shape().length() / Geo::BSpline::default_step);

    double t = knots[0];
    double step = (knots[nplusc - 1] - t) / (p1 - 1);
    std::vector<double> temp;
    double min_dis[2] = {DBL_MAX, DBL_MAX};
    while (t <= knots[nplusc - 1])
    {
        std::vector<double> nbasis;
        Geo::BSpline::rbasis(is_cubic ? 3 : 2, t, npts, knots, nbasis);
        Geo::Point coord;
        for (size_t i = 0; i < npts; ++i)
        {
            coord += bspline.control_points[i] * nbasis[i];
        }
        if (double dis = Geo::distance(coord, point); dis < min_dis[0])
        {
            temp.clear();
            min_dis[0] = dis;
            temp.push_back(t);
        }
        else if (dis == min_dis[0])
        {
            temp.push_back(t);
        }
        t += step;
    }

    std::vector<std::tuple<double, Geo::Point>> result;
    for (double v : temp)
    {
        step = 1e-3;
        double lower = knots[0], upper = knots[nplusc - 1];
        min_dis[0] = min_dis[1] = DBL_MAX;
        do
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
                if (double dis = Geo::distance(point, coord); dis < min_dis[1])
                {
                    min_dis[1] = dis;
                    v = x;
                }
            }
            lower = std::max(0.0, v - step);
            upper = std::min(1.0, v + step);
            step = (upper - lower) / 100;
            if (min_dis[0] > min_dis[1])
            {
                min_dis[0] = min_dis[1];
            }
        }
        while (std::abs(min_dis[0] - min_dis[1]) > 1e-4 && step > 1e-12);

        step = 1e-3, lower = std::max(knots[0], t - 0.1), upper = std::min(knots[nplusc - 1], t + 0.1);
        min_dis[0] = min_dis[1] = DBL_MAX;
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
                if (const double dis = Geo::distance(coord, point) * 1e9; dis < min_dis[1])
                {
                    min_dis[1] = dis;
                    t = x;
                }
                else if (dis == min_dis[1]) // 需要扩大搜索范围
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
            if (min_dis[1] < 2e-5)
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

        std::vector<double> nbasis;
        Geo::BSpline::rbasis(is_cubic ? 3 : 2, v, npts, knots, nbasis);
        Geo::Point coord;
        for (size_t i = 0; i < npts; ++i)
        {
            coord += bspline.control_points[i] * nbasis[i];
        }
        result.emplace_back(std::min(min_dis[0], min_dis[1]), coord);
    }

    std::sort(result.begin(), result.end(), [](const auto &a, const auto &b)
        { return std::get<0>(a) < std::get<0>(b); });
    while (result.size() > 1)
    {
        if (std::get<0>(result.back()) > std::get<0>(result.front()))
        {
            result.pop_back();
        }
        else
        {
            break;
        }
    }
    for (const auto &[dis, coord] : result)
    {
        output.emplace_back(coord);
    }
    return result.size();
}


bool Geo::tangency_point(const Point &point, const Circle &circle, Point &output0, Point &output1)
{
    if (Geo::distance_square(point, circle) <= std::pow(circle.radius, 2))
    {
        return false;
    }
    const Geo::Point point1(-100, (std::pow(circle.radius, 2) - (point.x - circle.x) * (-100 - circle.x))  / (point.y - circle.y) + circle.y);
    const Geo::Point point2(100, (std::pow(circle.radius, 2) - (point.x - circle.x) * (100 - circle.x))  / (point.y - circle.y) + circle.y);
    return Geo::is_intersected(point1, point2, circle, output0, output1, true);
}

bool Geo::tangency_point(const Point &point, const Ellipse &ellipse, Point &output0, Point &output1)
{
    if (Geo::is_inside(point, ellipse, true))
    {
        return false;
    }
    const Geo::Point center = ellipse.center();
    const double angle = Geo::angle(ellipse.a0(), ellipse.a1());
    Geo::Point coord = Geo::to_coord(point, center.x, center.y, angle);
    const double aa = Geo::distance_square(ellipse.a0(), ellipse.a1()) / 4;
    const double bb = Geo::distance_square(ellipse.b0(), ellipse.b1()) / 4;
    const double a1 = coord.x / aa, b1 = coord.y / bb;
    const double a = std::pow(a1, 2) * aa  + std::pow(b1, 2) * bb;
    if (b1 != 0)
    {
        const double b = -2 * a1 * aa;
        const double c = (1 - std::pow(b1, 2) * bb) * aa;
        output0.x = (-b - std::sqrt(std::pow(b, 2) - 4 * a * c)) / (2 * a);
        output1.x = (-b + std::sqrt(std::pow(b, 2) - 4 * a * c)) / (2 * a);
        output0.y = (1 - a1 * output0.x) / b1;
        output1.y = (1 - a1 * output1.x) / b1;
    }
    else
    {
        const double b = -2 * b1 * bb;
        const double c = (1 - std::pow(a1, 2) * aa) * bb;
        output0.y = (-b - std::sqrt(std::pow(b, 2) - 4 * a * c)) / (2 * a);
        output1.y = (-b + std::sqrt(std::pow(b, 2) - 4 * a * c)) / (2 * a);
        output0.x = (1 - b1 * output0.y) / a1;
        output1.x = (1 - b1 * output1.y) / a1;
    }
    coord = Geo::to_coord(Geo::Point(0, 0), center.x, center.y, angle);
    output0 = Geo::to_coord(output0, coord.x, coord.y, -angle);
    output1 = Geo::to_coord(output1, coord.x, coord.y, -angle);
    return true;
}

int Geo::tangency_point(const Point &point, const Bezier &bezier, std::vector<Point> &output,
    std::vector<std::tuple<size_t, double, double, double>> *tvalues)
{
    const size_t order = bezier.order();
    std::vector<int> nums(order, 1);
    switch (order - 1)
    {
    case 1:
        break;
    case 2:
        nums[1] = 2;
        break;
    case 3:
        nums[1] = nums[2] = 3;
        break;
    default:
        {
            std::vector<int> temp(1, 1);
            for (size_t i = 1; i <= order; ++i)
            {
                for (size_t j = 1; j < i; ++j)
                {
                    nums[j] = temp[j - 1] + temp[j];
                }
                temp.assign(nums.begin(), nums.begin() + i + 1);
            }
        }
        break;
    }
    std::vector<int> nums1(order + 1, 1);
    switch (order)
    {
    case 2:
        nums1[1] = 2;
        break;
    case 3:
        nums1[1] = nums1[2] = 3;
        break;
    default:
        {
            std::vector<int> temp(1, 1);
            for (size_t i = 1; i <= order; ++i)
            {
                for (size_t j = 1; j < i; ++j)
                {
                    nums1[j] = temp[j - 1] + temp[j];
                }
                temp.assign(nums1.begin(), nums1.begin() + i + 1);
            }
        }
        break;
    }

    std::vector<Geo::Point> result;
    std::vector<std::tuple<size_t, double, double, double>> temp;
    for (size_t i = 0, end = bezier.size() - order; i < end; i += order)
    {
        double t0 = 0, t1 = Geo::Bezier::default_step;
        double angles[2] = {1, 1};
        angles[0] = Geo::angle(bezier[i], point, bezier[i], bezier[i + 1]);
        std::vector<std::tuple<double, double>> pairs;
        while (t1 <= 1)
        {
            Geo::Point head, tail;
            for (size_t j = 0; j < order; ++j)
            {
                head += (bezier[i + j] * (nums[j] * std::pow(1 - t1, order - j - 1) * std::pow(t1, j)));
                tail += (bezier[i + j + 1] * (nums[j] * std::pow(1 - t1, order - j - 1) * std::pow(t1, j)));
            }
            Geo::Point coord;
            for (size_t j = 0; j <= order; ++j)
            {
                coord += (bezier[j + i] * (nums1[j] * std::pow(1 - t1, order - j) * std::pow(t1, j))); 
            }
            angles[1] = Geo::cross(coord, point, head, tail);
            if (angles[0] * angles[1] <= 0)
            {
                pairs.emplace_back(t0, t1);
            }
            angles[0] = angles[1];
            t0 = t1;
            t1 += Geo::Bezier::default_step;
        }
        for (auto [t0, t1] : pairs)
        {
            double last_t = -1;
            {
                Geo::Point coord;
                for (size_t j = 0; j <= order; ++j)
                {
                    coord += (bezier[i + j] * (nums1[j] * std::pow(1 - t0, order - j) * std::pow(t0, j))); 
                }
                Geo::Point head, tail;
                for (size_t j = 0; j < order; ++j)
                {
                    head += (bezier[i + j] * (nums[j] * std::pow(1 - t0, order - j - 1) * std::pow(t0, j)));
                    tail += (bezier[i + j + 1] * (nums[j] * std::pow(1 - t0, order - j - 1) * std::pow(t0, j)));
                }
                angles[0] = Geo::cross(coord, point, head, tail);
            }
            {
                Geo::Point coord;
                for (size_t j = 0; j <= order; ++j)
                {
                    coord += (bezier[i + j] * (nums1[j] * std::pow(1 - t1, order - j) * std::pow(t1, j))); 
                }
                Geo::Point head, tail;
                for (size_t j = 0; j < order; ++j)
                {
                    head += (bezier[i + j] * (nums[j] * std::pow(1 - t1, order - j - 1) * std::pow(t1, j)));
                    tail += (bezier[i + j + 1] * (nums[j] * std::pow(1 - t1, order - j - 1) * std::pow(t1, j)));
                }
                angles[1] = Geo::cross(coord, point, head, tail);
            }
            double t = (t0 + t1) / 2;
            while (t1 - t0 > 1e-14 && last_t != t && angles[0] * angles[1] != 0)
            {
                Geo::Point coord;
                for (size_t j = 0; j <= order; ++j)
                {
                    coord += (bezier[i + j] * (nums1[j] * std::pow(1 - t, order - j) * std::pow(t, j))); 
                }
                Geo::Point head, tail;
                for (size_t j = 0; j < order; ++j)
                {
                    head += (bezier[i + j] * (nums[j] * std::pow(1 - t, order - j - 1) * std::pow(t, j)));
                    tail += (bezier[i + j + 1] * (nums[j] * std::pow(1 - t, order - j - 1) * std::pow(t, j)));
                }
                const double angle = Geo::angle(coord, point, head, tail);
                if (angle * angles[0] <= 0)
                {
                    t1 = t;
                    angles[1] = angle;
                }
                else if (angle * angles[1] <= 0)
                {
                    t0 = t;
                    angles[0] = angle;
                }
                last_t = t;
                t = (t0 + t1) / 2;
            }

            if (angles[0] == 0)
            {
                t1 = t0;
            }
            else if (angles[1] != 0)
            {
                t1 = (t0 + t1) / 2;
            }
            Geo::Point coord;
            for (size_t j = 0; j <= order; ++j)
            {
                coord += (bezier[i + j] * (nums1[j] * std::pow(1 - t1, order - j) * std::pow(t1, j))); 
            }
            Geo::Point head, tail;
            for (size_t j = 0; j < order; ++j)
            {
                head += (bezier[i + j] * (nums[j] * std::pow(1 - t1, order - j - 1) * std::pow(t1, j)));
                tail += (bezier[i + j + 1] * (nums[j] * std::pow(1 - t1, order - j - 1) * std::pow(t1, j)));
            }
            if (std::abs(Geo::angle(coord, point, head, tail)) < 1e-4 ||
                std::abs(std::abs(Geo::angle(coord, point, head, tail)) - Geo::PI) < 1e-4)
            {
                result.emplace_back(coord);
                temp.emplace_back(i, t1, coord.x, coord.y);
            }
        }
    }

    output.insert(output.end(), result.begin(), result.end());
    if (tvalues != nullptr)
    {
        tvalues->insert(tvalues->end(), temp.begin(), temp.end());
    }
    return result.size();
}

int Geo::tangency_point(const Point &point, const BSpline &bspline,
    std::vector<Point> &output, std::vector<std::tuple<double, double, double>> *tvalues)
{
    std::vector<std::tuple<double, double>> pairs;
    {
        const Geo::Point origin(0, 0);
        const std::vector<double> &knots = bspline.knots();
        double t0 = knots[0], t1 = knots[0] + 1e-3;
        double angles[2] = {Geo::angle(bspline.at(t0), point, origin, bspline.tangent(t0)), 1};
        while (t1 <= knots.back())
        {
            angles[1] = Geo::angle(bspline.at(t1), point, origin, bspline.tangent(t1));
            if (angles[0] * angles[1] <= 0)
            {
                pairs.emplace_back(t0, t1);
            }
            angles[0] = angles[1];
            t0 = t1;
            t1 += 1e-3;
        }
    }
    std::vector<Geo::Point> result;
    std::vector<std::tuple<double, double, double>> temp;
    for (auto [t0, t1] : pairs)
    {
        const Geo::Point origin(0, 0);
        double angles[2] = { Geo::angle(bspline.at(t0), point, origin, bspline.tangent(t0)),
            Geo::angle(bspline.at(t1), point, origin, bspline.tangent(t1)) };
        double last_t = -1, t = (t0 + t1) / 2;
        while (t1 - t0 > 1e-14 && last_t != t && angles[0] * angles[1] != 0)
        {
            const double angle = Geo::angle(bspline.at(t), point, origin, bspline.tangent(t));
            if (angle * angles[0] <= 0)
            {
                t1 = t;
                angles[1] = angle;
            }
            else if (angle * angles[1] <= 0)
            {
                t0 = t;
                angles[0] = angle;
            }
            last_t = t;
            t = (t0 + t1) / 2;
        }

        if (angles[0] == 0)
        {
            t1 = t0;
        }
        else if (angles[1] != 0)
        {
            t1 = (t0 + t1) / 2;
        }
        if (const Geo::Point coord = bspline.at(t1); 
            std::abs(Geo::angle(coord, point, origin, bspline.tangent(t1))) < 1e-4 ||
            std::abs(std::abs(Geo::angle(coord, point, origin, bspline.tangent(t1))) - Geo::PI) < 1e-4)
        {
            result.emplace_back(coord);
            temp.emplace_back(t1, coord.x, coord.y);
        }
    }

    output.insert(output.end(), result.begin(), result.end());
    if (tvalues != nullptr)
    {
        tvalues->insert(tvalues->end(), temp.begin(), temp.end());
    }
    return result.size();
}


bool Geo::split(const Polyline &polyline, const Point &pos, Polyline &output0, Polyline &output1)
{
    if (pos == polyline.front() || pos == polyline.back())
    {
        return false;
    }

    output0.clear(), output1.clear();
    for (size_t i = 1, count = polyline.size(); i < count; ++i)
    {
        if (Geo::is_inside(pos, polyline[i - 1], polyline[i], false))
        {
            output0.append(polyline[i - 1]);
            output0.append(pos);
            output1.append(pos);
            output1.append(polyline.begin() + i, polyline.end());
            break;
        }
        else
        {
            output0.append(polyline[i - 1]);
        }
    }
    return output0.size() > 1 && output1.size() > 1;
}

bool Geo::split(const Bezier &bezier, const Point &pos, Bezier &output0, Bezier &output1)
{
    const size_t order = bezier.order();
    if (output0.order() != order || output1.order() != order || pos == bezier.front() || pos == bezier.back())
    {
        return false;
    }

    std::vector<int> nums(order + 1, 1);
    switch (order)
    {
    case 2:
        nums[1] = 2;
        break;
    case 3:
        nums[1] = nums[2] = 3;
        break;
    default:
        {
            std::vector<int> temp(1, 1);
            for (size_t i = 1; i <= order; ++i)
            {
                for (size_t j = 1; j < i; ++j)
                {
                    nums[j] = temp[j - 1] + temp[j];
                }
                temp.assign(nums.begin(), nums.begin() + i + 1);
            }
        }
        break;
    }

    // index, points, distance
    std::vector<std::tuple<size_t, std::vector<Geo::Point>, double>> temp;
    for (size_t i = 0, end = bezier.size() - order; i < end; i += order)
    {
        Geo::Polyline polyline;
        polyline.append(bezier[i]);
        double t = 0;
        while (t <= 1)
        {
            Geo::Point point;
            for (size_t j = 0; j <= order; ++j)
            {
                point += (bezier[j + i] * (nums[j] * std::pow(1 - t, order - j) * std::pow(t, j))); 
            }
            polyline.append(point);
            t += Geo::Bezier::default_step;
        }
        polyline.append(bezier[i + order]);
        Geo::down_sampling(polyline, Geo::Bezier::default_down_sampling_value);

        std::vector<Geo::Point> points;
        Geo::closest_point(polyline, pos, points);
        temp.emplace_back(i, points, Geo::distance(pos, points.front()));
    }
    std::sort(temp.begin(), temp.end(), [](const auto &a, const auto &b)
        { return std::get<2>(a) < std::get<2>(b); });
    while (temp.size() > 1)
    {
        if (std::get<2>(temp.back()) - std::get<2>(temp.front()) > 1.0)
        {
            temp.pop_back();
        }
        else
        {
            break;
        }
    }

    Geo::Point result_pos;
    size_t result_i = 0;
    double result_dis = DBL_MAX, result_t = 0;
    for (const auto &[i, points, dis] : temp)
    {
        double t = 0;
        double step = 1e-3, lower = 0, upper = 1;
        double min_dis[2] = {DBL_MAX, DBL_MAX};
        do
        {
            for (double x = lower; x < upper + step; x += step)
            {
                x = x < upper ? x : upper;
                Geo::Point coord;
                for (size_t j = 0; j <= order; ++j)
                {
                    coord += (bezier[j + i] * (nums[j] * std::pow(1 - x, order - j) * std::pow(x, j))); 
                }
                if (double dis = Geo::distance(pos, coord); dis < min_dis[1])
                {
                    min_dis[1] = dis;
                    t = x;
                }
            }
            lower = std::max(0.0, t - step);
            upper = std::min(1.0, t + step);
            step = (upper - lower) / 100;
            if (min_dis[0] > min_dis[1])
            {
                min_dis[0] = min_dis[1];
            }
        }
        while (std::abs(min_dis[0] - min_dis[1]) > 1e-4 && step > 1e-12);

        lower = std::max(0.0, t - 0.1), upper = std::min(1.0, t + 0.1);
        step = (upper - lower) / 100; 
        min_dis[0] = min_dis[1] = DBL_MAX;
        std::vector<double> stored_t;
        while ((upper - lower) * 1e15 > 1)
        {
            int flag = 0;
            for (double x = lower, dis0 = 0; x < upper + step; x += step)
            {
                x = x < upper ? x : upper;
                Geo::Point coord;
                for (size_t j = 0; j <= order; ++j)
                {
                    coord += (bezier[j + i] * (nums[j] * std::pow(1 - x, order - j) * std::pow(x, j)));
                }
                if (const double dis = Geo::distance(coord, pos) * 1e9; dis < min_dis[1])
                {
                    min_dis[1] = dis;
                    t = x;
                }
                else if (dis == min_dis[1]) // 需要扩大搜索范围
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
            if (min_dis[1] < 2e-5)
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

        if (const double d = std::min(min_dis[0], min_dis[1]); d < result_dis)
        {
            result_dis = d;
            result_t = t;
            result_i = i;
            result_pos.clear();
            for (size_t j = 0; j <= order; ++j)
            {
                result_pos += (bezier[j + i] * (nums[j] * std::pow(1 - t, order - j) * std::pow(t, j))); 
            }
        }
    }

    if (result_dis > Geo::EPSILON * 1e9)
    {
        return false;
    }

    output0.clear(), output1.clear();
    std::vector<Geo::Point> control_points;
    for (size_t i = 0; i <= order; ++i)
    {
        control_points.emplace_back(bezier[result_i + i]);
    }
    std::vector<Geo::Point> temp_points, result_points0, result_points1;
    for (size_t k = 0; k < order; ++k)
    {
        for (size_t i = 1, count = control_points.size(); i < count; ++i)
        {
            temp_points.emplace_back(control_points[i - 1] + (control_points[i] - control_points[i - 1]) * result_t);
        }
        result_points0.emplace_back(temp_points.front());
        result_points1.emplace_back(temp_points.back());
        control_points.assign(temp_points.begin(), temp_points.end());
        temp_points.clear();
    }
    result_points0.back() = result_points1.back() = result_pos;
    std::reverse(result_points1.begin(), result_points1.end());

    output0.append(bezier.begin(), bezier.begin() + result_i + 1);
    output0.append(result_points0.begin(), result_points0.end());
    output1.append(result_points1.begin(), result_points1.end());
    output1.append(bezier.begin() + result_i + order, bezier.end());
    output0.update_shape(Geo::Bezier::default_step, Geo::Bezier::default_down_sampling_value);
    output1.update_shape(Geo::Bezier::default_step, Geo::Bezier::default_down_sampling_value);
    return output0.size() > 1 && output1.size() > 1;
}

bool Geo::split(const Bezier &bezier, const size_t i, const double t, Bezier &output0, Bezier &output1)
{
    const size_t order = bezier.order();
    if (output0.order() != order || output1.order() != order || (i == 0 && t == 0)
        || (i == (bezier.size() / (order + 1) -1) && t == 1))
    {
        return false;
    }

    std::vector<int> nums(order + 1, 1);
    switch (order)
    {
    case 2:
        nums[1] = 2;
        break;
    case 3:
        nums[1] = nums[2] = 3;
        break;
    default:
        {
            std::vector<int> temp(1, 1);
            for (size_t i = 1; i <= order; ++i)
            {
                for (size_t j = 1; j < i; ++j)
                {
                    nums[j] = temp[j - 1] + temp[j];
                }
                temp.assign(nums.begin(), nums.begin() + i + 1);
            }
        }
        break;
    }

    output0.clear(), output1.clear();
    std::vector<Geo::Point> control_points;
    Geo::Point pos;
    for (size_t j = 0; j <= order; ++j)
    {
        control_points.emplace_back(bezier[j + i]);
        pos += (control_points.back() * (nums[j] * std::pow(1 - t, order - j) * std::pow(t, j))); 
    }
    std::vector<Geo::Point> temp_points, result_points0, result_points1;
    for (size_t k = 0; k < order; ++k)
    {
        for (size_t j = 1, count = control_points.size(); j < count; ++j)
        {
            temp_points.emplace_back(control_points[j - 1] + (control_points[j] - control_points[j - 1]) * t);
        }
        result_points0.emplace_back(temp_points.front());
        result_points1.emplace_back(temp_points.back());
        control_points.assign(temp_points.begin(), temp_points.end());
        temp_points.clear();
    }
    result_points0.back() = result_points1.back() = pos;
    std::reverse(result_points1.begin(), result_points1.end());

    output0.append(bezier.begin(), bezier.begin() + i + 1);
    output0.append(result_points0.begin(), result_points0.end());
    output1.append(result_points1.begin(), result_points1.end());
    output1.append(bezier.begin() + i + order, bezier.end());
    output0.update_shape(Geo::Bezier::default_step, Geo::Bezier::default_down_sampling_value);
    output1.update_shape(Geo::Bezier::default_step, Geo::Bezier::default_down_sampling_value);
    return output0.size() > 1 && output1.size() > 1;
}

bool Geo::split(const BSpline &bspline, const bool is_cubic, const Point &pos, BSpline &output0, BSpline &output1)
{
    if (is_cubic && (dynamic_cast<const Geo::CubicBSpline *>(&output0) == nullptr
        || dynamic_cast<const Geo::CubicBSpline *>(&output1) == nullptr))
    {
        return false;
    }
    else if (!is_cubic && (dynamic_cast<const Geo::QuadBSpline *>(&output0) == nullptr
        || dynamic_cast<const Geo::QuadBSpline *>(&output1) == nullptr))
    {
        return false;
    }

    std::vector<double> knots = bspline.knots();
    const size_t p = is_cubic ? 3 : 2;
    Geo::Point anchor;
    double t = knots[0];
    {
        const size_t npts = bspline.control_points.size();
        const size_t nplusc = npts + (is_cubic ? 4 : 3);
        const size_t p1 = std::max(npts * 8.0, bspline.shape().length() / Geo::BSpline::default_step);
        double step = (knots[nplusc - 1] - t) / (p1 - 1);
        std::vector<double> temp;
        double min_dis[2] = {DBL_MAX, DBL_MAX};
        while (t <= knots[nplusc - 1])
        {
            std::vector<double> nbasis;
            Geo::BSpline::rbasis(is_cubic ? 3 : 2, t, npts, knots, nbasis);
            Geo::Point coord;
            for (size_t i = 0; i < npts; ++i)
            {
                coord += bspline.control_points[i] * nbasis[i];
            }
            if (double dis = Geo::distance(coord, pos); dis < min_dis[0])
            {
                temp.clear();
                min_dis[0] = dis;
                temp.push_back(t);
            }
            else if (dis == min_dis[0])
            {
                temp.push_back(t);
            }
            t += step;
        }

        std::vector<std::tuple<double, double, Geo::Point>> result; // dis, t, point
        for (double v : temp)
        {
            step = 1e-3;
            double lower = knots[0], upper = knots[nplusc - 1];
            min_dis[0] = min_dis[1] = DBL_MAX;
            do
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
                    if (double dis = Geo::distance(pos, coord); dis < min_dis[1])
                    {
                        min_dis[1] = dis;
                        v = x;
                    }
                }
                lower = std::max(0.0, v - step);
                upper = std::min(1.0, v + step);
                step = (upper - lower) / 100;
                if (min_dis[0] > min_dis[1])
                {
                    min_dis[0] = min_dis[1];
                }
            }
            while (std::abs(min_dis[0] - min_dis[1]) > 1e-4 && step > 1e-12);

            step = 1e-3, lower = std::max(knots[0], t - 0.1), upper = std::min(knots[nplusc - 1], t + 0.1);
            min_dis[0] = min_dis[1] = DBL_MAX;
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
                    if (const double dis = Geo::distance(coord, pos) * 1e9; dis < min_dis[1])
                    {
                        min_dis[1] = dis;
                        t = x;
                    }
                    else if (dis == min_dis[1]) // 需要扩大搜索范围
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
                if (min_dis[1] < 2e-5)
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

            std::vector<double> nbasis;
            Geo::BSpline::rbasis(is_cubic ? 3 : 2, v, npts, knots, nbasis);
            Geo::Point coord;
            for (size_t i = 0; i < npts; ++i)
            {
                coord += bspline.control_points[i] * nbasis[i];
            }
            result.emplace_back(std::min(min_dis[0], min_dis[1]), v, coord);
        }

        std::sort(result.begin(), result.end(), [](const auto &a, const auto &b)
            { return std::get<0>(a) < std::get<0>(b); });
        t = std::get<1>(result.front());
        anchor = std::get<2>(result.front());
    }

    size_t k = 0, anchor_index = 1;
    for (size_t i = 1, count = knots.size(); i < count; ++i)
    {
        if (knots[i - 1] <= t && t <= knots[i])
        {
            k = --i;
            break;
        }
    }

    std::vector<Geo::Point> path_points(bspline.path_points);
    {
        const Geo::Polyline &shape = bspline.shape();
        std::vector<double> lenghts({0});
        for (size_t i = 1, count = shape.size(); i < count; ++i)
        {
            lenghts.push_back(lenghts.back() + Geo::distance(shape[i - 1], shape[i]));
        }
        std::vector<double> distances;
        for (const Geo::Point &point : path_points)
        {
            size_t index = 0;
            double min_dis = DBL_MAX;
            for (size_t i = 0, count = shape.size(); i < count; ++i)
            {
                if (const double dis = Geo::distance(shape[i], point); dis < min_dis)
                {
                    min_dis = dis;
                    index = i;
                }
            }
            distances.push_back(lenghts[index]);
        }
        double anchor_dis = 0, min_dis = DBL_MAX;
        size_t index = 0;
        for (size_t i = 0, count = shape.size(); i < count; ++i)
        {
            if (const double dis = Geo::distance(shape[i], anchor); dis < min_dis)
            {
                min_dis = dis;
                index = i;
            }
        }
        anchor_dis = lenghts[index];
        for (size_t i = 1, count = path_points.size(); i < count; ++i)
        {
            if (distances[i - 1] <= anchor_dis && anchor_dis <= distances[i])
            {
                anchor_index = i;
                path_points.insert(path_points.begin() + i, p, anchor);
                break;
            }
        }
    }

    std::vector<Geo::Point> control_points(bspline.control_points);
    for (size_t n = 0; n < p; ++n)
    {
        std::vector<Geo::Point> array(p);
        for (size_t i = k, j = p - 1; i > k - p; --i, --j)
        {
            double alpha = (t - knots[i]);
            double dev =  (knots[i + p] - knots[i]);
            alpha = (dev == 0) ? 0 : alpha / dev;
            array[j] = control_points[i - 1] * (1 - alpha) + control_points[i] * alpha;
        }
        for (size_t i = k - p + 1, j = 0; i < k; ++i, ++j)
        {
            control_points[i] = array[j];
        }
        control_points.insert(control_points.begin() + k, array[p - 1]);
        knots.insert(knots.begin() + k + 1, t);
    }
    knots.insert(knots.begin() + k + 1, t);

    if (is_cubic)
    {
        {
            std::vector<double> temp_knots(knots.begin(), knots.begin() + k + 5);
            const double value = temp_knots.back();
            for (size_t i = 0, npts = temp_knots.size(); i < npts; ++i)
            {
                temp_knots[i] /= value;
            }
            output0 = Geo::CubicBSpline(control_points.begin(), control_points.begin() + k + 1, temp_knots, false);
            output0.path_points.assign(path_points.begin(), path_points.begin() + anchor_index + 1);
        }
        {
            std::vector<double> temp_knots(knots.begin() + k + 1, knots.end());
            const double left = temp_knots.front(), value = temp_knots.back() - temp_knots.front();
            for (size_t i = 0, npts = temp_knots.size(); i < npts; ++i)
            {
                temp_knots[i] = (temp_knots[i] - left) / value;
            }
            output1 = Geo::CubicBSpline(control_points.begin() + k, control_points.end(), temp_knots, false);
            output1.path_points.assign(path_points.begin() + anchor_index, path_points.end());
        }
    }
    else
    {
        {
            std::vector<double> temp_knots(knots.begin(), knots.begin() + k + 4);
            const double value = temp_knots.back();
            for (size_t i = 0, npts = temp_knots.size(); i < npts; ++i)
            {
                temp_knots[i] /= value;
            }
            output0 = Geo::QuadBSpline(control_points.begin(), control_points.begin() + k + 1, temp_knots, false);
            output0.path_points.assign(path_points.begin(), path_points.begin() + anchor_index + 1);
        }
        {
            std::vector<double> temp_knots(knots.begin() + k + 1, knots.end());
            const double left = temp_knots.front(), value = temp_knots.back() - temp_knots.front();
            for (size_t i = 0, npts = temp_knots.size(); i < npts; ++i)
            {
                temp_knots[i] = (temp_knots[i] - left) / value;
            }
            output1 = Geo::QuadBSpline(control_points.begin() + k, control_points.end(), temp_knots, false);
            output1.path_points.assign(path_points.begin() + anchor_index, path_points.end());
        }
    }
    return output0.shape().size() > 1 || output1.shape().size() > 1;
}

bool Geo::split(const BSpline &bspline, const bool is_cubic, const double t, BSpline &output0, BSpline &output1)
{
    if (t == 0 || t == 1)
    {
        return false;
    }
    else if (is_cubic && (dynamic_cast<const Geo::CubicBSpline *>(&output0) == nullptr
        || dynamic_cast<const Geo::CubicBSpline *>(&output1) == nullptr))
    {
        return false;
    }
    else if (!is_cubic && (dynamic_cast<const Geo::QuadBSpline *>(&output0) == nullptr
        || dynamic_cast<const Geo::QuadBSpline *>(&output1) == nullptr))
    {
        return false;
    }

    std::vector<double> knots = bspline.knots();
    const size_t p = is_cubic ? 3 : 2;
    std::vector<double> nbasis;
    Geo::Point anchor;
    {
        const size_t npts = bspline.control_points.size();
        Geo::BSpline::rbasis(is_cubic ? 3 : 2, t, npts, knots, nbasis);
        for (size_t i = 0; i < npts; ++i)
        {
            anchor += bspline.control_points[i] * nbasis[i];
        }
    }
    size_t k = 0, anchor_index = 1;
    for (size_t i = 1, count = knots.size(); i < count; ++i)
    {
        if (knots[i - 1] <= t && t <= knots[i])
        {
            k = --i;
            break;
        }
    }

    std::vector<Geo::Point> path_points(bspline.path_points);
    {
        const Geo::Polyline &shape = bspline.shape();
        std::vector<double> lenghts({0});
        for (size_t i = 1, count = shape.size(); i < count; ++i)
        {
            lenghts.push_back(lenghts.back() + Geo::distance(shape[i - 1], shape[i]));
        }
        std::vector<double> distances;
        for (const Geo::Point &point : path_points)
        {
            size_t index = 0;
            double min_dis = DBL_MAX;
            for (size_t i = 0, count = shape.size(); i < count; ++i)
            {
                if (const double dis = Geo::distance(shape[i], point); dis < min_dis)
                {
                    min_dis = dis;
                    index = i;
                }
            }
            distances.push_back(lenghts[index]);
        }
        double anchor_dis = 0, min_dis = DBL_MAX;
        size_t index = 0;
        for (size_t i = 0, count = shape.size(); i < count; ++i)
        {
            if (const double dis = Geo::distance(shape[i], anchor); dis < min_dis)
            {
                min_dis = dis;
                index = i;
            }
        }
        anchor_dis = lenghts[index];
        for (size_t i = 1, count = path_points.size(); i < count; ++i)
        {
            if (distances[i - 1] <= anchor_dis && anchor_dis <= distances[i])
            {
                anchor_index = i;
                path_points.insert(path_points.begin() + i, p, anchor);
                break;
            }
        }
    }

    std::vector<Geo::Point> control_points(bspline.control_points);
    for (size_t n = 0; n < p; ++n)
    {
        std::vector<Geo::Point> array(p);
        for (size_t i = k, j = p - 1; i > k - p; --i, --j)
        {
            double alpha = (t - knots[i]);
            double dev =  (knots[i + p] - knots[i]);
            alpha = (dev == 0) ? 0 : alpha / dev;
            array[j] = control_points[i - 1] * (1 - alpha) + control_points[i] * alpha;
        }
        for (size_t i = k - p + 1, j = 0; i < k; ++i, ++j)
        {
            control_points[i] = array[j];
        }
        control_points.insert(control_points.begin() + k, array[p - 1]);
        knots.insert(knots.begin() + k + 1, t);
    }
    knots.insert(knots.begin() + k + 1, t);

    if (is_cubic)
    {
        {
            std::vector<double> temp_knots(knots.begin(), knots.begin() + k + 5);
            const double value = temp_knots.back();
            for (size_t i = 0, npts = temp_knots.size(); i < npts; ++i)
            {
                temp_knots[i] /= value;
            }
            output0 = Geo::CubicBSpline(control_points.begin(), control_points.begin() + k + 1, temp_knots, false);
            output0.path_points.assign(path_points.begin(), path_points.begin() + anchor_index + 1);
        }
        {
            std::vector<double> temp_knots(knots.begin() + k + 1, knots.end());
            const double left = temp_knots.front(), value = temp_knots.back() - temp_knots.front();
            for (size_t i = 0, npts = temp_knots.size(); i < npts; ++i)
            {
                temp_knots[i] = (temp_knots[i] - left) / value;
            }
            output1 = Geo::CubicBSpline(control_points.begin() + k, control_points.end(), temp_knots, false);
            output1.path_points.assign(path_points.begin() + anchor_index, path_points.end());
        }
    }
    else
    {
        {
            std::vector<double> temp_knots(knots.begin(), knots.begin() + k + 4);
            const double value = temp_knots.back();
            for (size_t i = 0, npts = temp_knots.size(); i < npts; ++i)
            {
                temp_knots[i] /= value;
            }
            output0 = Geo::QuadBSpline(control_points.begin(), control_points.begin() + k + 1, temp_knots, false);
            output0.path_points.assign(path_points.begin(), path_points.begin() + anchor_index + 1);
        }
        {
            std::vector<double> temp_knots(knots.begin() + k + 1, knots.end());
            const double left = temp_knots.front(), value = temp_knots.back() - temp_knots.front();
            for (size_t i = 0, npts = temp_knots.size(); i < npts; ++i)
            {
                temp_knots[i] = (temp_knots[i] - left) / value;
            }
            output1 = Geo::QuadBSpline(control_points.begin() + k, control_points.end(), temp_knots, false);
            output1.path_points.assign(path_points.begin() + anchor_index, path_points.end());
        }
    }
    return output0.shape().size() > 1 || output1.shape().size() > 1;
}

bool Geo::split(const Arc &arc, const Point &pos, Arc &output0, Arc &output1)
{
    if (Geo::distance(pos, arc) < Geo::EPSILON && Geo::distance(pos, arc.control_points[0]) > Geo::EPSILON
        && Geo::distance(pos, arc.control_points[2]) > Geo::EPSILON)
    {
        const Geo::Point center(arc.x, arc.y);
        const double angle0 = Geo::angle(center, arc.control_points[0]);
        const double angle1 = Geo::angle(center, pos);
        const double angle2 = Geo::angle(center, arc.control_points[2]);
        output0 = Arc(arc.x, arc.y, arc.radius, angle0, angle1, !arc.is_cw());
        output1 = Arc(arc.x, arc.y, arc.radius, angle1, angle2, !arc.is_cw());
        return true;
    }
    else
    {
        return false;
    }
}


double Geo::angle(const Point &start, const Point &end)
{
    const Geo::Point vec = end - start;
    return std::atan2(vec.y, vec.x);
}

double Geo::angle(const Point &point0, const Point &point1, const Point &point2)
{
    const Geo::Point vec0 = point0 - point1, vec1 = point2 - point1;
    double value = vec0 * vec1 / (vec0.length() * vec1.length());
    if (value > 1)
    {
        value = 1;
    }
    else if (value < -1)
    {
        value = -1;
    }
    return vec0.cross(vec1) > 0 ? std::acos(value) : -std::acos(value);
}

double Geo::angle(const Point &start0, const Point &end0, const Point &start1, const Point &end1)
{
    const Geo::Point vec0 = end0 - start0, vec1 = end1 - start1;
    double value = vec0 * vec1 / (vec0.length() * vec1.length());
    if (value > 1)
    {
        value = 1;
    }
    else if (value < -1)
    {
        value = -1;
    }
    return vec0.cross(vec1) > 0 ? std::acos(value) : -std::acos(value);
}

double Geo::rad_to_PI(double value)
{
    if (std::abs(value) > 2 * Geo::PI)
    {
        value -= (std::round(value / (Geo::PI * 2)) * 2 * Geo::PI);
    }
    if (value < -Geo::PI)
    {
        return value + 2 * Geo::PI;
    }
    else if (value > Geo::PI)
    {
        return value - 2 * Geo::PI;
    }
    else
    {
        return value;
    }
}

double Geo::rad_to_2PI(double value)
{
    if (std::abs(value) > 2 * Geo::PI)
    {
        value -= (std::round(value / (Geo::PI * 2)) * 2 * Geo::PI);
    }
    if (value < 0)
    {
        value += (2 * Geo::PI);
    }
    return value;
}

double Geo::rad_to_degree(double value)
{
    return value * 180 / Geo::PI;
}

double Geo::degree_to_180(double value)
{
    if (std::abs(value) > 360)
    {
        value -= (std::round(value / 360) * 360);
    }
    if (value < -180)
    {
        return value + 360;
    }
    else if (value > 180)
    {
        return value - 360;
    }
    else
    {
        return value;
    }
}

double Geo::degree_to_360(double value)
{
    if (std::abs(value) > 360)
    {
        value -= (std::round(value / 360) * 360);
    }
    if (value < 0)
    {
        value += 360;
    }
    return value;
}

double Geo::degree_to_rad(double value)
{
    return value * Geo::PI / 180;
}


Geo::Point Geo::to_coord(const Geo::Point &point, const double x, const double y, const double rad)
{
    const double x0 = point.x - x, y0 = point.y - y;
    return Geo::Point(x0 * std::cos(rad) + y0 * std::sin(rad), y0 * std::cos(rad) - x0 * std::sin(rad));
}


bool Geo::angle_to_arc(const Point &point0, const Point &point1, const Point &point2, const double radius, Polyline &arc, const double step, const double down_sampling_value)
{
    if (radius <= 0)
    {
        return false;
    }

    const double rad1 = std::abs(Geo::angle(point0, point1, point2));
    const double len = radius / std::tan(rad1/2.0);
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
    const double len = radius / std::tan(rad1/2.0);
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


Geo::Polyline Geo::arc_to_polyline(const Geo::Point &center, const double radius, double start_angle, double end_angle, const bool is_cw, const double down_sampling_value)
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

Geo::Bezier Geo::arc_to_bezier(const Geo::Arc &arc)
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
    return Geo::Bezier(points.begin(), points.end(), 3, false);
}

Geo::Polygon Geo::circle_to_polygon(const double x, const double y, const double r, const double down_sampling_value)
{
    const double v = std::asin(1 / r);
    const double step = std::isnan(v) ? Geo::PI / 32 : std::min(v, Geo::PI / 64);
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

Geo::Bezier Geo::circle_to_bezier(const Geo::Circle &circle)
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
    return Geo::Bezier(points.begin(), points.end(), 3, false);
}

Geo::Polygon Geo::ellipse_to_polygon(const double x, const double y, const double a, const double b, const double rad, const double down_sampling_value)
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
    return Geo::ellipse_to_polygon(ellipse.center().x, ellipse.center().y, ellipse.lengtha(), ellipse.lengthb(), ellipse.angle(), down_sampling_value);
}

Geo::Polyline Geo::ellipse_to_polyline(const double x, const double y, const double a, const double b,
    const double rad, const double start_angle, double end_angle, const double down_sampling_value)
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
    return Geo::ellipse_to_polyline(ellipse.center().x, ellipse.center().y, ellipse.lengtha(),
        ellipse.lengthb(), ellipse.angle(), ellipse.arc_param0(), ellipse.arc_param1(), down_sampling_value);
}

Geo::Bezier Geo::ellipse_to_bezier(const Geo::Ellipse &ellipse)
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
        Geo::Bezier bezier(points.begin(), points.end(), 3, false);
        if (std::vector<Geo::Point> output; ellipse.arc_angle0() < ellipse.arc_angle1())
        {
            if (Geo::Bezier b0(3), b1(3); Geo::closest_point(bezier, ellipse.arc_point0(), output)
                && Geo::split(bezier, output.front(), b0, b1))
            {
                output.clear();
                if (Geo::Bezier b2(3), b3(3); Geo::closest_point(b1, ellipse.arc_point1(), output)
                    && Geo::split(b1, output.front(), b2, b3))
                {
                    points.assign(b2.begin(), b2.end());
                }
            }
        }
        else
        {
            if (Geo::Bezier b0(3), b1(3); Geo::closest_point(bezier, ellipse.arc_point0(), output)
                && Geo::split(bezier, output.front(), b0, b1))
            {
                output.clear();
                if (Geo::Bezier b2(3), b3(3); Geo::closest_point(b0, ellipse.arc_point1(), output)
                    && Geo::split(b0, output.front(), b2, b3))
                {
                    points.assign(b1.begin(), b1.end());
                    points.insert(points.end(), b2.begin(), b2.end());
                }
            }
        }
    }
    return Geo::Bezier(points.begin(), points.end(), 3, false);
}


std::vector<unsigned int> Geo::ear_cut_to_indexs(const Geo::Polygon &polygon)
{
    std::vector<std::vector<std::array<double, 2>>> points;
    points.emplace_back();
    for (const Point &point : polygon)
    {
        points.front().emplace_back(std::array<double, 2>({point.x, point.y}));
    }
    return mapbox::earcut<unsigned int>(points);
}

std::vector<Geo::MarkedPoint> Geo::ear_cut_to_coords(const Geo::Polygon &polygon)
{
    std::vector<MarkedPoint> result;
    for (const unsigned int i : Geo::ear_cut_to_indexs(polygon))
    {
        result.emplace_back(polygon[i].x, polygon[i].y);
    }
    return result;
}

std::vector<Geo::Point> Geo::ear_cut_to_points(const Geo::Polygon &polygon)
{
    std::vector<Point> result;
    for (const unsigned int i : Geo::ear_cut_to_indexs(polygon))
    {
        result.emplace_back(polygon[i]);
    }
    return result;
}

std::vector<Geo::Triangle> Geo::ear_cut_to_triangles(const Geo::Polygon &polygon)
{
    std::vector<unsigned int> indexs;
    if (polygon.is_cw())
    {
        for (size_t i = 0, count = polygon.size() - 1; i < count; ++i)
        {
            indexs.push_back(count - i);
        }
    }
    else
    {
        for (size_t i = 0, count = polygon.size() - 1; i < count; ++i)
        {
            indexs.push_back(i);
        }
    }

    std::vector<Geo::Triangle> triangles;
    bool is_ear, is_cut;
    while (indexs.size() > 3)
    {
        is_cut = false;
        for (size_t pre, cur, nxt, i = 0, count = indexs.size(); i < count; ++i)
        {
            pre = i > 0 ? indexs[i - 1] : indexs[count - 1];
            cur = indexs[i];
            nxt = i < count - 1 ? indexs[i + 1] : indexs[0];
            if ((polygon[cur] - polygon[pre]).cross(polygon[nxt] - polygon[cur]) > 0)
            {
                is_ear = true;
                for (size_t index : indexs)
                {
                    if (index == pre || index == cur || index == nxt)
                    {
                        continue;
                    }
                    if (is_inside(polygon[index], polygon[pre], polygon[cur], polygon[nxt]))
                    {
                        is_ear = false;
                        break;
                    }
                }
                if (is_ear)
                {
                    triangles.emplace_back(polygon[pre], polygon[cur], polygon[nxt]);
                    indexs.erase(indexs.begin() + i--);
                    --count;
                    is_cut = true;
                }
            }
        }

        if (!is_cut)
        {
            triangles.clear();
            break;
        }
    }

    if (indexs.size() == 3)
    {
        triangles.emplace_back(polygon[indexs[0]], polygon[indexs[1]], polygon[indexs[2]]);
    }
    
    return triangles;
}


bool Geo::offset(const Geo::Polyline &input, Geo::Polyline &result, const double distance)
{
    if (distance != 0)
    {
        Polyline temp(input);
        result.clear();
        double area = 0;
        for (size_t i = 1, count = temp.size(); i < count; ++i)
        {
            area += (temp[i].x * (temp[i+1 != count ? i+1 : 0].y - temp[i-1].y));
        }
        area += (temp.front().x * (temp[1].y - temp.back().y));
        if (area > 0)
        {
            temp.flip();
        }
        Point a, b;
        result.append(temp.front() + (temp[1] - temp[0]).vertical().normalize() * distance);
        for (size_t i = 1, end = temp.size() - 1; i < end; ++i)
        {
            a = (temp[i] - temp[i > 0 ? i - 1 : end]).vertical().normalize();
            b = (temp[i < end ? i + 1 : 0] - temp[i]).vertical().normalize();
            result.append(temp[i] + (a + b).normalize() * (distance / std::sqrt((1 + a * b) / 2)));
        }
        result.append(temp.back() + (temp.back() - temp[temp.size() - 2]).vertical().normalize() * distance);
        return true;
    }
    else
    {
        result = input;
        return true;
    }
}

bool Geo::offset(const Geo::Polygon &input, Geo::Polygon &result, const double distance)
{
    if (distance == 0)
    {
        result = input;
        return true;
    }

    Polygon temp(input);
    temp.reorder_points();
    result.clear();
    std::vector<Point> points;
    Point a, b;
    std::vector<bool> error_edges;
    if (distance > 0)
    {
        for (size_t i = 0, count = temp.size(); i < count; ++i)
        {
            a = (temp[i] - temp[i > 0 ? i - 1 : count - 2]).vertical().normalize();
            b = (temp[i < count - 1 ? i + 1 : 1] - temp[i]).vertical().normalize();
            points.emplace_back(temp[i] + (a + b).normalize() * (distance / std::sqrt((1 + a * b) / 2)));
        }
        result.append(points.cbegin(), points.cend());

        for (size_t i = 1, count = result.size(); i < count; ++i)
        {
            error_edges.push_back((temp[i] - temp[i - 1]) * (result[i] - result[i - 1]) < 0);
        }
        for (size_t i = 0, edge_count = error_edges.size(), point_count = temp.size(),
                count = result.size(), of = 0, j = 0; i < edge_count; ++i)
        {
            if (error_edges[i])
            {
                j = i - of++;
                if (error_edges[(i + 1) % edge_count])
                {
                    b = (temp.next_point(i) - temp[i]).vertical().normalize() * distance;
                    a.x = a.y = std::numeric_limits<double>::infinity();
                    Geo::is_intersected(result.last_point(j), result[j],
                        temp[i] + b, temp.next_point(i) + b, a, true);
                    if (!std::isinf(a.x) && !std::isinf(a.y))
                    {
                        result[j] = a;
                    }
                    a.x = a.y = std::numeric_limits<double>::infinity();
                    Geo::is_intersected(result.next_point(result.next_point_index(j)),
                        result.next_point(result.next_point_index(result.next_point_index(j))),
                        temp[i] + b, temp.next_point(i) + b, a, true);
                    if (!std::isinf(a.x) && !std::isinf(a.y))
                    {
                        result.next_point(result.next_point_index(j)) = a;
                    }
                    result.remove(result.next_point_index(j));
                    --count;
                    ++i;
                }
                else
                {
                    a.x = a.y = std::numeric_limits<double>::infinity();
                    if (Geo::is_intersected(result.last_point(j), result[j],
                        result.next_point(j), result.next_point(result.next_point_index(j)), a, true))
                    {
                        if (!std::isinf(a.x) && !std::isinf(a.y))
                        {
                            result[j] = a;
                        }
                        result.remove(result.next_point_index(j));
                        --count;
                    }
                    else
                    {
                        b = (temp[i] - temp.last_point(i)).vertical().normalize() * distance;
                        Geo::is_intersected(result.next_point(result.next_point_index(j)),
                            result.next_point(result.next_point_index(result.next_point_index(j))),
                            temp[i] + b, temp.last_point(i) + b, a, true);
                        b = (temp.next_point(temp.next_point_index(i)) - temp.next_point(i)).vertical().normalize() * distance;
                        Geo::is_intersected(result.last_point(j), result.last_point(result.last_point_index(j)),
                            temp.next_point(i) + b, temp.next_point(temp.next_point_index(i)) + b, b, true);

                        if ((temp.next_point(temp.next_point_index(i)) - temp.last_point(i)) * (a - b) < 0)
                        {
                            result.next_point(result.next_point_index(j)) = a;
                        }
                        else
                        {
                            result.last_point(j) = b;
                        }

                        size_t temp_index = result.next_point_index(j);
                        result.remove(temp_index);
                        --count;
                        if (temp_index > j)
                        {
                            result.remove(j % count--);
                        }
                        else
                        {
                            result.remove((j - 1) % count--);
                        }
                        ++of;
                    }
                }
            }
        }
    }
    else
    {
        for (size_t i = 0, count = temp.size(); i < count; ++i)
        {
            a = (temp[i] - temp[i > 0 ? i - 1 : count - 2]).vertical().normalize();
            b = (temp[i < count - 1 ? i + 1 : 1] - temp[i]).vertical().normalize();
            points.emplace_back(temp[i] + (a + b).normalize() * (distance / std::sqrt((1 + a * b) / 2)));
        }
        result.append(points.cbegin(), points.cend());

        for (size_t i = 1, count = result.size(); i < count; ++i)
        {
            error_edges.push_back((temp[i] - temp[i - 1]) * (result[i] - result[i - 1]) < 0);
        }
        for (size_t i = 0, edge_count = error_edges.size(), point_count = temp.size(),
                count = result.size(), of = 0, j = 0; i < edge_count; ++i)
        {
            if (error_edges[i])
            {
                j = i - of++;
                if (error_edges[(i + 1) % edge_count])
                {
                    b = (temp.next_point(i) - temp[i]).vertical().normalize() * distance;
                    a.x = a.y = std::numeric_limits<double>::infinity();
                    Geo::is_intersected(result.last_point(j), result[j],
                        temp[i] + b, temp.next_point(i) + b, a, true);
                    if (!std::isinf(a.x) && !std::isinf(a.y))
                    {
                        result[j] = a;
                    }
                    a.x = a.y = std::numeric_limits<double>::infinity();
                    Geo::is_intersected(result.next_point(result.next_point_index(j)),
                        result.next_point(result.next_point_index(result.next_point_index(j))),
                        temp[i] + b, temp.next_point(i) + b, a, true);
                    if (!std::isinf(a.x) && !std::isinf(a.y))
                    {
                        result.next_point(result.next_point_index(j)) = a;
                    }
                    result.remove(result.next_point_index(j));
                    --count;
                    ++i;
                }
                else
                {
                    a.x = a.y = std::numeric_limits<double>::infinity();
                    if (Geo::is_intersected(result.last_point(j), result[j],
                        result.next_point(j), result.next_point(result.next_point_index(j)), a, true))
                    {
                        if (!std::isinf(a.x) && !std::isinf(a.y))
                        {
                            result[j] = a;
                        }
                        result.remove(result.next_point_index(j));
                        --count;
                    }
                    else
                    {
                        b = (temp[i] - temp.last_point(i)).vertical().normalize() * distance;
                        Geo::is_intersected(result.next_point(result.next_point_index(j)),
                            result.next_point(result.next_point_index(result.next_point_index(j))),
                            temp[i] + b, temp.last_point(i) + b, a, true);
                        b = (temp.next_point(temp.next_point_index(i)) - temp.next_point(i)).vertical().normalize() * distance;
                        Geo::is_intersected(result.last_point(j), result.last_point(result.last_point_index(j)),
                            temp.next_point(i) + b, temp.next_point(temp.next_point_index(i)) + b, b, true);

                        if ((temp.next_point(temp.next_point_index(i)) - temp.last_point(i)) * (a - b) < 0)
                        {
                            result.next_point(result.next_point_index(j)) = a;
                        }
                        else
                        {
                            result.last_point(j) = b;
                        }

                        size_t temp_index = result.next_point_index(j);
                        result.remove(temp_index);
                        --count;
                        if (temp_index > j)
                        {
                            result.remove(j % count--);
                        }
                        else
                        {
                            result.remove((j - 1) % count--);
                        }
                        ++of;
                    }
                }
            }
        }
    }

    for (size_t i = 0, count = result.size(); i < count; ++i)
    {
        if (std::isnan(result[i].x) || std::isnan(result[i].y)
            || std::isinf(result[i].x) || std::isinf(result[i].y))
        {
            result.remove(i--);
            --count;
        }
    }
    result.back() = result.front();

    for (size_t i = 0, count = result.size() - 1; i < count; ++i)
    {
        if (Geo::is_inside(result[i], result[i + 1], result.next_point(i + 1))
            && Geo::is_inside(result[i + 1], result[i], result.last_point(i)))
        {
            result.remove(i + 1);
            result.remove(i--);
            --count;
            --count;
        }
    }
    for (size_t i = 0, count = result.size(); i < count; ++i)
    {
        if (Geo::is_inside(result[i], result.last_point(i), result.next_point(i)))
        {
            result.remove(i--);
            --count;
        }
    }

    return true;
}

bool Geo::offset(const Geo::Polygon &input, std::vector<Geo::Polygon> &result, const double distance,
    const Offset::JoinType join_type, const Offset::EndType end_type, const double epsilon)
{
    if (distance == 0)
    {
        result.emplace_back(input);
        return true;
    }

    std::vector<int64_t> values;
    for (const Geo::Point &point : input)
    {
        values.push_back(point.x * 100'000'000);
        values.push_back(point.y * 100'000'000);
    }

    Clipper2Lib::Paths64 subject;
    subject.push_back(Clipper2Lib::MakePath(values));
    Clipper2Lib::Paths64 solution;
    Clipper2Lib::ClipperOffset offsetter;
    offsetter.AddPaths(subject, static_cast<Clipper2Lib::JoinType>(join_type), static_cast<Clipper2Lib::EndType>(end_type));
    offsetter.Execute(distance * 1e8, solution);
    solution = Clipper2Lib::SimplifyPaths(solution, epsilon);

    const size_t count = result.size();
    for (const Clipper2Lib::Path64 &path : solution)
    {
        std::vector<Geo::Point> points;
        for (const Clipper2Lib::Point64 &point : path)
        {
            points.emplace_back(point.x / 1e8, point.y / 1e8);
        }
        result.emplace_back(points.begin(), points.end());
    }
    return result.size() > count;
}

bool Geo::offset(const Geo::Circle &input, Geo::Circle &result, const double distance)
{
    if (distance >= 0 || -distance < input.radius)
    {
        result.x = input.x;
        result.y = input.y;
        result.radius = input.radius + distance;
        return true;
    }
    else
    {
        return false;
    }
}

bool Geo::offset(const Geo::AABBRect &input, Geo::AABBRect &result, const double distance)
{
    if (distance >= 0 || -distance * 2 < std::min(input.width(), input.height()))
    {
        result.set_top(input.top() + distance);
        result.set_right(input.right() + distance);
        result.set_bottom(input.bottom() + distance);
        result.set_left(input.left() + distance);
        return true;
    }
    else
    {
        return false;
    }
}


bool Geo::polygon_union(const Geo::Polygon &polygon0, const Geo::Polygon &polygon1, std::vector<Geo::Polygon> &output)
{
    Geo::Polygon polygon2(polygon0), polygon3(polygon1);
    polygon2.reorder_points(false);
    polygon3.reorder_points(false);
    std::vector<MarkedPoint> points0, points1;
    for (const Point &point : polygon2)
    {
        points0.emplace_back(point.x, point.y);
    }
    for (const Point &point : polygon3)
    {
        points1.emplace_back(point.x, point.y);
    }

    Point point, pre_point; // 找到交点并计算其几何数
    const AABBRect rect(polygon1.bounding_rect());
    for (size_t i = 1, count0 = points0.size(); i < count0; ++i)
    {
        if (!Geo::is_intersected(rect, points0[i - 1], points0[i])) // 粗筛
        {
            continue;
        }

        pre_point = points0[i - 1];
        for (size_t k = 1, j = 1, count1 = points1.size(); j < count1; ++j)
        {
            k = j - 1;
            while (!points1[k].active)
            {
                --k; // 跳过非活动交点
            }
            while (j < count1 && (points1[k] == points1[j] || !points1[j].active))
            {
                k = j;
                ++j;
                while (!points1[k].active)
                {
                    --k; // 跳过非活动交点
                }
            }
            if (j >= count1)
            {
                continue;
            }
            k = j - 1;
            while (!points1[k].active)
            {
                --k; // 跳过非活动交点
            }

            if (!Geo::is_parallel(pre_point, points0[i], points1[k], points1[j]) &&
                Geo::is_intersected(pre_point, points0[i], points1[k], points1[j], point))
            {
                points0.insert(points0.begin() + i++, MarkedPoint(point.x, point.y, false));
                points1.insert(points1.begin() + j++, MarkedPoint(point.x, point.y, false));
                ++count0;
                ++count1;
                if (Geo::cross(pre_point, points0[i], points1[k], points1[j]) >= 0)
                {
                    points0[i - 1].value = 1;
                    points1[j - 1].value = -1;
                }
                else
                {
                    points0[i - 1].value = -1;
                    points1[j - 1].value = 1;
                }
            }
        }

        // 将本次循环添加的交点休眠,令下次循环polygon1处于无活动交点状态以排除干扰
        for (MarkedPoint &p : points1)
        {
            if (!p.original)
            {
                p.active = false;
            }
        }
    }

    if (points0.size() == polygon0.size()) // 无交点
    {
        if (std::all_of(polygon0.begin(), polygon0.end(), [&](const Point &point) { return Geo::is_inside(point, polygon1, true); }))
        {
            output.emplace_back(polygon1); // 无交点,且polygon0的点都在polygon1内,则并集必然为polygon1
            return true;
        }
        else if (std::all_of(polygon1.begin(), polygon1.end(), [&](const Point &point) { return Geo::is_inside(point, polygon0, true); }))
        {
            output.emplace_back(polygon0); // 无交点,且polygon1的点都在polygon0内,则并集必然为polygon0
            return true;
        }
        else
        {
            return false;
        }
    }

    // 调整交点顺序,同一条边上的交点按照顺序排列
    std::vector<MarkedPoint> points;
    for (size_t i = 1, j = 0, count = points0.size() - 1; i < count; ++i)
    {
        if (points0[i].original)
        {
            continue;
        }
        else
        {
            j = i;
        }
        while (j < count && !points0[j].original)
        {
            ++j;
        }
        if (j == i + 1)
        {
            ++i;
            continue;
        }

        points.assign(points0.begin() + i, j < count ? points0.begin() + j : points0.end());
        std::sort(points.begin(), points.end(), [&](const MarkedPoint &p0, const MarkedPoint &p1)
            { return Geo::distance_square(p0, points0[i - 1]) < Geo::distance_square(p1, points0[i - 1]); });
        for (size_t k = i, n = 0; k < j; ++k)
        {
            points0[k] = points[n++];
        }
        i = j;
    }
    for (size_t i = points0.size() - 1; i > 1;)
    {
        if (polygon0.front() == points0[i])
        {
            if (!points0[i].original)
            {
                points0.insert(points0.begin(), points0[i]);
                points0.erase(points0.begin() + i + 1);
            }
            else
            {
                --i;
            }
        }
        else
        {
            break;
        }
    }
    for (size_t i = 1, j = 0, count = points1.size() - 1; i < count; ++i)
    {
        if (points1[i].original)
        {
            continue;
        }
        else
        {
            j = i;
        }
        while (j < count && !points1[j].original)
        {
            ++j;
        }
        if (j == i + 1)
        {
            ++i;
            continue;
        }

        points.assign(points1.begin() + i, j < count ? points1.begin() + j : points1.end());
        std::sort(points.begin(), points.end(), [&](const MarkedPoint &p0, const MarkedPoint &p1)
            { return Geo::distance_square(p0, points1[i - 1]) < Geo::distance_square(p1, points1[i - 1]); });
        for (size_t k = i, n = 0; k < j; ++k)
        {
            points1[k] = points[n++];
        }
        i = j;
    }
    for (size_t i = points1.size() - 1; i > 1;)
    {
        if (polygon1.front() == points1[i])
        {
            if (!points1[i].original)
            {
                points1.insert(points1.begin(), points1[i]);
                points1.erase(points1.begin() + i + 1);
            }
            else
            {
                --i;
            }
        }
        else
        {
            break;
        }
    }

    // 去除重复交点
    int value;
    Geo::Point point_a, point_b, point_c, point_d;
    bool flags[5];
    for (size_t count, j, i = points0.size() - 1; i > 0; --i)
    {
        count = points0[i].original ? 0 : 1;
        for (j = i; j > 0; --j)
        {
            if (std::abs(points0[i].x - points0[j - 1].x) > Geo::EPSILON ||
                std::abs(points0[i].y - points0[j - 1].y) > Geo::EPSILON)
            {
                break;
            }
            if (!points0[j - 1].original)
            {
                ++count;
            }
        }
        if (count < 2)
        {
            continue;
        }

        value = 0;
        for (size_t k = i; k > j; --k)
        {
            if (!points0[k].original)
            {
                value += points0[k].value;
            }
        }
        if (!points0[j].original)
        {
            value += points0[j].value;
        }
        if (count < 4)
        {
            if (value == 0)
            {
                for (size_t k = i; k > j; --k)
                {
                    if (!points0[k].original)
                    {
                        points0.erase(points0.begin() + k);
                    }
                }
                if (!points0[j].original)
                {
                    points0.erase(points0.begin() + j);
                }
            }
            else
            {
                flags[0] = false;
                for (size_t k = i; k > j; --k)
                {
                    flags[0] = (flags[0] || points0[k].original);
                    points0.erase(points0.begin() + k);
                }
                points0[j].value = value;
                points0[j].original = (flags[0] || points0[j].original);
            }
        }
        else
        {
            point = points0[i];
            point_a = polygon0.last_point(polygon0.index(point.x, point.y));
            point_b = polygon0.next_point(polygon0.index(point.x, point.y));
            point_c = polygon1.last_point(polygon1.index(point.x, point.y));
            point_d = polygon1.next_point(polygon1.index(point.x, point.y));

            flags[2] = Geo::cross(point_a - point, point_b - point) > 0;
            flags[3] = Geo::cross(point_a - point, point_c - point) > 0;
            flags[4] = Geo::cross(point_c - point, point_b - point) > 0;
            flags[0] = !(flags[2] == flags[3] && flags[3] == flags[4]);
            flags[2] = Geo::cross(point_a - point, point_b - point) > 0;
            flags[3] = Geo::cross(point_a - point, point_d - point) > 0;
            flags[4] = Geo::cross(point_d - point, point_b - point) > 0;
            flags[1] = !(flags[2] == flags[3] && flags[3] == flags[4]);

            if (flags[0] && flags[1])
            {
                for (size_t k = i; k > j; --k)
                {
                    if (!points0[k].original)
                    {
                        points0.erase(points0.begin() + k);
                    }
                }
                if (!points0[j].original)
                {
                    points0.erase(points0.begin() + j);
                }
            }
            else
            {
                flags[0] = false;
                for (size_t k = i; k > j; --k)
                {
                    flags[0] = (flags[0] || points0[k].original);
                    points0.erase(points0.begin() + k);
                }
                points0[j].value = value;
                points0[j].original = (flags[0] || points0[j].original);
            }
        }
        i = j > 0 ? j : 1;
    }
    for (size_t count, j, i = points1.size() - 1; i > 0; --i)
    {
        count = points1[i].original ? 0 : 1;
        for (j = i; j > 0; --j)
        {
            if (std::abs(points1[i].x - points1[j - 1].x) > Geo::EPSILON || 
                std::abs(points1[i].y - points1[j - 1].y) > Geo::EPSILON)
            {
                break;
            }
            if (!points1[j - 1].original)
            {
                ++count;
            }
        }
        if (count < 2)
        {
            continue;
        }

        value = 0;
        for (size_t k = i; k > j; --k)
        {
            if (!points1[k].original)
            {
                value += points1[k].value;
            }
        }
        if (!points1[j].original)
        {
            value += points1[j].value;
        }
        if (count < 4)
        {
            if (value == 0)
            {
                for (size_t k = i; k > j; --k)
                {
                    if (!points1[k].original)
                    {
                        points1.erase(points1.begin() + k);
                    }
                }
                if (!points1[j].original)
                {
                    points1.erase(points1.begin() + j);
                }
            }
            else
            {
                flags[0] = false;
                for (size_t k = i; k > j; --k)
                {
                    flags[0] = (flags[0] || points1[k].original);
                    points1.erase(points1.begin() + k);
                }
                points1[j].value = value;
                points1[j].original = (flags[0] || points1[j].original);
            }
        }
        else
        {
            point = points1[i];
            point_a = polygon1.last_point(polygon1.index(point.x, point.y));
            point_b = polygon1.next_point(polygon1.index(point.x, point.y));
            point_c = polygon0.last_point(polygon0.index(point.x, point.y));
            point_d = polygon0.next_point(polygon0.index(point.x, point.y));

            flags[2] = Geo::cross(point_a - point, point_b - point) > 0;
            flags[3] = Geo::cross(point_a - point, point_c - point) > 0;
            flags[4] = Geo::cross(point_c - point, point_b - point) > 0;
            flags[0] = !(flags[2] == flags[3] && flags[3] == flags[4]);
            flags[2] = Geo::cross(point_a - point, point_b - point) > 0;
            flags[3] = Geo::cross(point_a - point, point_d - point) > 0;
            flags[4] = Geo::cross(point_d - point, point_b - point) > 0;
            flags[1] = !(flags[2] == flags[3] && flags[3] == flags[4]);

            if (flags[0] && flags[1])
            {
                for (size_t k = i; k > j; --k)
                {
                    if (!points1[k].original)
                    {
                        points1.erase(points1.begin() + k);
                    }
                }
                if (!points1[j].original)
                {
                    points1.erase(points1.begin() + j);
                }
            }
            else
            {
                flags[0] = false;
                for (size_t k = i; k > j; --k)
                {
                    flags[0] = (flags[0] || points1[k].original);
                    points1.erase(points1.begin() + k);
                }
                points1[j].value = value;
                points1[j].original = (flags[0] || points1[j].original);
            }
        }
        i = j > 0 ? j : 1;
    }

    if (std::count_if(points0.begin(), points0.end(), [](const MarkedPoint &p) { return p.value < 0; }) == 0 ||
        std::count_if(points1.begin(), points1.end(), [](const MarkedPoint &p) { return p.value < 0; }) == 0)
    {
        return false; // 交点都是出点,即两多边形只有一个点相交
    }

    // 处理重边上的交点
    std::vector<Geo::MarkedPoint>::iterator it0, it1;
    for (size_t i = 0, j = 1, count0 = points0.size(), count1 = polygon3.size(); j < count0; i = j)
    {
        while (i < count0 && points0[i].value == 0)
        {
            ++i;
        }
        j = i + 1;
        while (j < count0 && points0[j].value == 0)
        {
            ++j;
        }
        if (j >= count0)
        {
            break;
        }
        if (!Geo::is_coincide(points0[i], points0[j], polygon2))
        {
            continue;
        }

        it0 = std::find_if(points1.begin(), points1.end(), [&](const MarkedPoint &p)
                { return !p.original && Geo::distance(p, points0[i]) < Geo::EPSILON; });
        it1 = std::find_if(points1.begin(), points1.end(), [&](const MarkedPoint &p)
            { return !p.original && Geo::distance(p, points0[j]) < Geo::EPSILON; });
        if (it0 == points1.end() || it1 == points1.end() || it0->value * it1->value <= 0)
        {
            continue;
        }

        for (size_t k = 1; k < count1; ++k)
        {
            if (!Geo::is_part(*it0, *it1, polygon3[k - 1], polygon3[k]))
            {
                continue;
            }

            if (it0->value < 0 && it1->value < 0)
            {
                points1.erase(it0);
                if (points0[i].value != 0)
                {
                    points0.erase(points0.begin() + i);
                    --count0;
                }
            }
            else if (it0->value > 0 && it1->value > 0)
            {
                points1.erase(it1);
                if (points0[j].value != 0)
                {
                    points0.erase(points0.begin() + j);
                    --count0;
                }
            }
            break;
        }
    }
    for (size_t i = points0.size() - 1, j = 0;;)
    {
        while (i > 0 && points0[i].value == 0)
        {
            --i;
        }
        if (i == 0)
        {
            break;
        }
        while (j < i && points0[j].value == 0)
        {
            ++j;
        }
        if (j >= i || !Geo::is_coincide(points0[i], points0[j], polygon2))
        {
            break;
        }

        it0 = std::find_if(points1.begin(), points1.end(), [&](const MarkedPoint &p)
                { return !p.original && Geo::distance(p, points0[i]) < Geo::EPSILON; });
        it1 = std::find_if(points1.begin(), points1.end(), [&](const MarkedPoint &p)
            { return !p.original && Geo::distance(p, points0[j]) < Geo::EPSILON; });
        if (it0 == points1.end() || it1 == points1.end() || it0->value * it1->value <= 0)
        {
            break;
        }

        for (size_t k = 1, count1 = polygon3.size(); k < count1; ++k)
        {
            if (!Geo::is_part(*it0, *it1, polygon3[k - 1], polygon3[k]))
            {
                continue;
            }

            if (it0->value < 0 && it1->value < 0)
            {
                points1.erase(it0);
                if (points0[i].value != 0)
                {
                    points0.erase(points0.begin() + i);
                }
            }
            else if (it0->value > 0 && it1->value > 0)
            {
                points1.erase(it1);
                if (points0[j].value != 0)
                {
                    points0.erase(points0.begin() + j);
                }
            }
            break;
        }
        break;
    }
    for (size_t i = 0, j = 1, count0 = polygon2.size(), count1 = points1.size(); j < count1; i = j)
    {
        while (i < count1 && points1[i].value == 0)
        {
            ++i;
        }
        j = i + 1;
        while (j < count1 && points1[j].value == 0)
        {
            ++j;
        }
        if (j >= count1)
        {
            break;
        }
        if (!Geo::is_coincide(points1[i], points1[j], polygon3))
        {
            continue;
        }

        it0 = std::find_if(points0.begin(), points0.end(), [&](const MarkedPoint &p)
                { return !p.original && Geo::distance(p, points1[i]) < Geo::EPSILON; });
        it1 = std::find_if(points0.begin(), points0.end(), [&](const MarkedPoint &p)
            { return !p.original && Geo::distance(p, points1[j]) < Geo::EPSILON; });
        if (it0 == points0.end() || it1 == points0.end() || it0->value * it1->value <= 0)
        {
            continue;
        }

        for (size_t k = 1; k < count0; ++k)
        {
            if (!Geo::is_part(*it0, *it1, polygon2[k - 1], polygon2[k]))
            {
                continue;
            }

            if (it0->value < 0 && it1->value < 0)
            {
                points0.erase(it0);
                if (points1[i].value != 0)
                {
                    points1.erase(points1.begin() + i);
                    --count1;
                }
            }
            else if (it0->value > 0 && it1->value > 0)
            {
                points0.erase(it1);
                if (points1[j].value != 0)
                {
                    points1.erase(points1.begin() + j);
                    --count1;
                }
            }
            break;
        }
    }
    for (size_t i = points1.size() - 1, j = 0;;)
    {
        while (i > 0 && points1[i].value == 0)
        {
            --i;
        }
        if (i == 0)
        {
            break;
        }
        while (j < i && points1[j].value == 0)
        {
            ++j;
        }
        if (j >= i || !Geo::is_coincide(points1[i], points1[j], polygon3))
        {
            break;
        }

        it0 = std::find_if(points0.begin(), points0.end(), [&](const MarkedPoint &p)
            { return !p.original && Geo::distance(p, points1[i]) < Geo::EPSILON; });
        it1 = std::find_if(points0.begin(), points0.end(), [&](const MarkedPoint &p)
            { return !p.original && Geo::distance(p, points1[j]) < Geo::EPSILON; });
        if (it0 == points0.end() || it1 == points0.end() || it0->value * it1->value <= 0)
        {
            break;
        }

        for (size_t k = 1, count0 = polygon2.size(); k < count0; ++k)
        {
            if (!Geo::is_part(*it0, *it1, polygon2[k - 1], polygon2[k]))
            {
                continue;
            }

            if (it0->value < 0 && it1->value < 0)
            {
                points0.erase(it0);
                if (points1[i].value != 0)
                {
                    points1.erase(points1.begin() + i);
                }
            }
            else if (it0->value > 0 && it1->value > 0)
            {
                points0.erase(it1);
                if (points1[j].value != 0)
                {
                    points1.erase(points1.begin() + j);
                }
            }
            break;
        }
        break;
    }

    std::vector<Point> result;
    size_t index0 = 0, index1 = 0;
    size_t count0 = points0.size(), count1 = points1.size();
    size_t count2 = count0 + count1;
    for (MarkedPoint &p : points0)
    {
        p.active = true;
    }
    for (MarkedPoint &p : points1)
    {
        p.active = true;
    }
    output.clear();

    while (count0 > 0 && count1 > 0)
    {
        output.emplace_back();

        index0 = index1 = 0;
        while (index0 < count0 && points0[index0].value < 1)
        {
            ++index0;
        }

        if (index0 >= count0)
        {
            output.pop_back();
            break;
        }

        while (result.size() < count2 && (result.size() < 4 || result.front() != result.back()))
        {
            while (result.size() < count2 && (result.size() < 4 || result.front() != result.back()))
            {
                if (points0[index0].value > -1)
                {
                    if (points0[index0].original)
                    {
                        points0[index0].active = false;
                    }
                    result.emplace_back(points0[index0++]);
                }
                else
                {
                    index1 = 0;
                    while (index1 < count1 && points1[index1] != points0[index0])
                    {
                        ++index1;
                    }
                    index1 %= count1;
                    result.emplace_back(points1[index1++]);
                    ++index0;
                    index0 %= count0;
                    index1 %= count1;
                    break;
                }
                index0 %= count0;
            }

            while (result.size() < count2 && (result.size() < 4 || result.front() != result.back()))
            {
                if (points1[index1].value > -1)
                {
                    if (points1[index1].original)
                    {
                        points1[index1].active = false;
                    }
                    result.emplace_back(points1[index1++]);
                }
                else
                {
                    index0 = 0;
                    while (index0 < count0 && points0[index0] != points1[index1])
                    {
                        ++index0;
                    }
                    index0 %= count0;
                    result.emplace_back(points0[index0++]);
                    ++index1;
                    index0 %= count0;
                    index1 %= count1;
                    break;
                }
                index1 %= count1;
            }
        }

        for (size_t i = result.size() - 1; i > 1; --i)
        {
            if (Geo::is_inside(result[i - 1], result[i - 2], result[i]))
            {
                result.erase(result.begin() + i - 1);
            }
            else if (Geo::distance(result[i - 2], result[i]) < Geo::EPSILON)
            {
                result.erase(result.begin() + i--);
                result.erase(result.begin() + i);
            }
        }
        if (result.size() > 4)
        {
            if (Geo::is_inside(result.front(), result[1], result[result.size() - 2]))
            {
                result.pop_back();
                std::swap(result.front(), result.back());
                result.pop_back();
            }
            else if (Geo::distance(result[1], result[result.size() - 2]) < Geo::EPSILON)
            {
                result.pop_back();
                result.pop_back();
                result.erase(result.begin());
            }
        }
        output.back().append(result.begin(), result.end());

        it0 = points0.begin();
        while (it0 != points0.end())
        {
            if (!it0->active || std::find(result.begin(), result.end(), *it0) != result.end())
            {
                it0 = points0.erase(it0);
            }
            else
            {
                ++it0;
            }
        }
        it1 = points1.begin();
        while (it1 != points1.end())
        {
            if (!it1->active || std::find(result.begin(), result.end(), *it1) != result.end())
            {
                it1 = points1.erase(it1);
            }
            else
            {
                ++it1;
            }
        }

        if (output.back().area() == 0)
        {
            output.pop_back();
        }

        if (std::count_if(points0.cbegin(), points0.cend(), [](const MarkedPoint &p){ return p.value < 0; }) == 0 ||
            std::count_if(points1.cbegin(), points1.cend(), [](const MarkedPoint &p){ return p.value < 0; }) == 0)
        {
            break;
        }

        count0 = points0.size();
        count1 = points1.size();
        count2 = count0 + count1;
        result.clear();
    }

    return !output.empty();
}

bool Geo::polygon_intersection(const Geo::Polygon &polygon0, const Geo::Polygon &polygon1, std::vector<Geo::Polygon> &output)
{
    Geo::Polygon polygon2(polygon0), polygon3(polygon1);
    polygon2.reorder_points(false);
    polygon3.reorder_points(false);
    std::vector<MarkedPoint> points0, points1;
    for (const Point &point : polygon2)
    {
        points0.emplace_back(point.x, point.y);
    }
    for (const Point &point : polygon3)
    {
        points1.emplace_back(point.x, point.y);
    }

    Point point, pre_point; // 找到交点并计算其几何数
    const AABBRect rect(polygon1.bounding_rect());
    for (size_t i = 1, count0 = points0.size(); i < count0; ++i)
    {
        if (!Geo::is_intersected(rect, points0[i - 1], points0[i])) // 粗筛
        {
            continue;
        }

        pre_point = points0[i - 1];
        for (size_t k = 1, j = 1, count1 = points1.size(); j < count1; ++j)
        {
            k = j - 1;
            while (!points1[k].active)
            {
                --k; // 跳过非活动交点
            }
            while (j < count1 && (points1[k] == points1[j] || !points1[j].active))
            {
                k = j;
                ++j;
                while (!points1[k].active)
                {
                    --k; // 跳过非活动交点
                }
            }
            if (j >= count1)
            {
                continue;
            }
            k = j - 1;
            while (!points1[k].active)
            {
                --k; // 跳过非活动交点
            }
            if (!Geo::is_parallel(pre_point, points0[i], points1[k], points1[j]) &&
                Geo::is_intersected(pre_point, points0[i], points1[k], points1[j], point))
            {
                points0.insert(points0.begin() + i++, MarkedPoint(point.x, point.y, false));
                points1.insert(points1.begin() + j++, MarkedPoint(point.x, point.y, false));
                ++count0;
                ++count1;
                if (Geo::cross(pre_point, points0[i], points1[k], points1[j]) >= 0)
                {
                    points0[i - 1].value = 1;
                    points1[j - 1].value = -1;
                }
                else
                {
                    points0[i - 1].value = -1;
                    points1[j - 1].value = 1;
                }
            }
        }

        // 将本次循环添加的交点休眠,令下次循环polygon1处于无活动交点状态以排除干扰
        for (MarkedPoint &p : points1)
        {
            if (!p.original)
            {
                p.active = false;
            }
        }
    }

    if (points0.size() == polygon0.size()) // 无交点
    {
        if (std::all_of(polygon0.begin(), polygon0.end(), [&](const Point &point) { return Geo::is_inside(point, polygon1, true); }))
        {
            output.emplace_back(polygon0); // 无交点,且polygon0的点都在polygon1内,则交集必然为polygon0
            return true;
        }
        else if (std::all_of(polygon1.begin(), polygon1.end(), [&](const Point &point) { return Geo::is_inside(point, polygon0, true); }))
        {
            output.emplace_back(polygon1); // 无交点,且polygon1的点都在polygon0内,则交集必然为polygon1
            return true;
        }
        else
        {
            return false;
        }
    }

    // 调整交点顺序,同一条边上的交点按照顺序排列
    std::vector<MarkedPoint> points;
    for (size_t i = 1, j = 0, count = points0.size() - 1; i < count; ++i)
    {
        if (points0[i].original)
        {
            continue;
        }
        else
        {
            j = i;
        }
        while (j < count && !points0[j].original)
        {
            ++j;
        }
        if (j == i + 1)
        {
            ++i;
            continue;
        }

        points.assign(points0.begin() + i, j < count ? points0.begin() + j : points0.end());
        std::sort(points.begin(), points.end(), [&](const MarkedPoint &p0, const MarkedPoint &p1)
            { return Geo::distance_square(p0, points0[i - 1]) < Geo::distance_square(p1, points0[i - 1]); });
        for (size_t k = i, n = 0; k < j; ++k)
        {
            points0[k] = points[n++];
        }
        i = j;
    }
    for (size_t i = 1, j = 0, count = points1.size() - 1; i < count; ++i)
    {
        if (points1[i].original)
        {
            continue;
        }
        else
        {
            j = i;
        }
        while (j < count && !points1[j].original)
        {
            ++j;
        }
        if (j == i + 1)
        {
            ++i;
            continue;
        }

        points.assign(points1.begin() + i, j < count ? points1.begin() + j : points1.end());
        std::sort(points.begin(), points.end(), [&](const MarkedPoint &p0, const MarkedPoint &p1)
            { return Geo::distance_square(p0, points1[i - 1]) < Geo::distance_square(p1, points1[i - 1]); });
        for (size_t k = i, n = 0; k < j; ++k)
        {
            points1[k] = points[n++];
        }
        i = j;
    }

    // 去除重复交点
    int value;
    Geo::Point point_a, point_b, point_c, point_d;
    bool flags[5];
    for (size_t count, j, i = points0.size() - 1; i > 0; --i)
    {
        count = points0[i].original ? 0 : 1;
        for (j = i; j > 0; --j)
        {
            if (std::abs(points0[i].x - points0[j - 1].x) > Geo::EPSILON ||
                std::abs(points0[i].y - points0[j - 1].y) > Geo::EPSILON)
            {
                break;
            }
            if (!points0[j - 1].original)
            {
                ++count;
            }
        }
        if (count < 2)
        {
            continue;
        }

        value = 0;
        for (size_t k = i; k > j; --k)
        {
            if (!points0[k].original)
            {
                value += points0[k].value;
            }
        }
        if (!points0[j].original)
        {
            value += points0[j].value;
        }
        if (count < 4)
        {
            if (value == 0)
            {
                for (size_t k = i; k > j; --k)
                {
                    if (!points0[k].original)
                    {
                        points0.erase(points0.begin() + k);
                    }
                }
                if (!points0[j].original)
                {
                    points0.erase(points0.begin() + j);
                }
            }
            else
            {
                flags[0] = false;
                for (size_t k = i; k > j; --k)
                {
                    flags[0] = (flags[0] || points0[k].original);
                    points0.erase(points0.begin() + k);
                }
                points0[j].value = value;
                points0[j].original = (flags[0] || points0[j].original);
            }
        }
        else
        {
            point = points0[i];
            point_a = polygon0.last_point(polygon0.index(point.x, point.y));
            point_b = polygon0.next_point(polygon0.index(point.x, point.y));
            point_c = polygon1.last_point(polygon1.index(point.x, point.y));
            point_d = polygon1.next_point(polygon1.index(point.x, point.y));

            flags[2] = Geo::cross(point_a - point, point_b - point) > 0;
            flags[3] = Geo::cross(point_a - point, point_c - point) > 0;
            flags[4] = Geo::cross(point_c - point, point_b - point) > 0;
            flags[0] = !(flags[2] == flags[3] && flags[3] == flags[4]);
            flags[2] = Geo::cross(point_a - point, point_b - point) > 0;
            flags[3] = Geo::cross(point_a - point, point_d - point) > 0;
            flags[4] = Geo::cross(point_d - point, point_b - point) > 0;
            flags[1] = !(flags[2] == flags[3] && flags[3] == flags[4]);

            if (flags[0] && flags[1])
            {
                for (size_t k = i; k > j; --k)
                {
                    if (!points0[k].original)
                    {
                        points0.erase(points0.begin() + k);
                    }
                }
                if (!points0[j].original)
                {
                    points0.erase(points0.begin() + j);
                }
            }
            else
            {
                flags[0] = false;
                for (size_t k = i; k > j; --k)
                {
                    flags[0] = (flags[0] || points0[k].original);
                    points0.erase(points0.begin() + k);
                }
                points0[j].value = value;
                points0[j].original = (flags[0] || points0[j].original);
            }
        }
        i = j > 0 ? j : 1;
    }
    for (size_t count, j, i = points1.size() - 1; i > 0; --i)
    {
        count = points1[i].original ? 0 : 1;
        for (j = i; j > 0; --j)
        {
            if (std::abs(points1[i].x - points1[j - 1].x) > Geo::EPSILON || 
                std::abs(points1[i].y - points1[j - 1].y) > Geo::EPSILON)
            {
                break;
            }
            if (!points1[j - 1].original)
            {
                ++count;
            }
        }
        if (count < 2)
        {
            continue;
        }

        value = 0;
        for (size_t k = i; k > j; --k)
        {
            if (!points1[k].original)
            {
                value += points1[k].value;
            }
        }
        if (!points1[j].original)
        {
            value += points1[j].value;
        }
        if (count < 4)
        {
            if (value == 0)
            {
                for (size_t k = i; k > j; --k)
                {
                    if (!points1[k].original)
                    {
                        points1.erase(points1.begin() + k);
                    }
                }
                if (!points1[j].original)
                {
                    points1.erase(points1.begin() + j);
                }
            }
            else
            {
                flags[0] = false;
                for (size_t k = i; k > j; --k)
                {
                    flags[0] = (flags[0] || points1[k].original);
                    points1.erase(points1.begin() + k);
                }
                points1[j].value = value;
                points1[j].original = (flags[0] || points1[j].original);
            }
        }
        else
        {
            point = points1[i];
            point_a = polygon1.last_point(polygon1.index(point.x, point.y));
            point_b = polygon1.next_point(polygon1.index(point.x, point.y));
            point_c = polygon0.last_point(polygon0.index(point.x, point.y));
            point_d = polygon0.next_point(polygon0.index(point.x, point.y));

            flags[2] = Geo::cross(point_a - point, point_b - point) > 0;
            flags[3] = Geo::cross(point_a - point, point_c - point) > 0;
            flags[4] = Geo::cross(point_c - point, point_b - point) > 0;
            flags[0] = !(flags[2] == flags[3] && flags[3] == flags[4]);
            flags[2] = Geo::cross(point_a - point, point_b - point) > 0;
            flags[3] = Geo::cross(point_a - point, point_d - point) > 0;
            flags[4] = Geo::cross(point_d - point, point_b - point) > 0;
            flags[1] = !(flags[2] == flags[3] && flags[3] == flags[4]);

            if (flags[0] && flags[1])
            {
                for (size_t k = i; k > j; --k)
                {
                    if (!points1[k].original)
                    {
                        points1.erase(points1.begin() + k);
                    }
                }
                if (!points1[j].original)
                {
                    points1.erase(points1.begin() + j);
                }
            }
            else
            {
                flags[0] = false;
                for (size_t k = i; k > j; --k)
                {
                    flags[0] = (flags[0] || points1[k].original);
                    points1.erase(points1.begin() + k);
                }
                points1[j].value = value;
                points1[j].original = (flags[0] || points1[j].original);
            }
        }
        i = j > 0 ? j : 1;
    }

    if (std::count_if(points0.begin(), points0.end(), [](const MarkedPoint &p) { return p.value < 0; }) == 0 ||
        std::count_if(points1.begin(), points1.end(), [](const MarkedPoint &p) { return p.value < 0; }) == 0)
    {
        return false; // 交点都是出点,即两多边形只有一个点相交
    }

    // 处理重边上的交点
    std::vector<Geo::MarkedPoint>::iterator it0, it1;
    for (size_t i = 0, j = 1, count0 = points0.size(), count1 = polygon3.size(); j < count0; i = j)
    {
        while (i < count0 && points0[i].value == 0)
        {
            ++i;
        }
        j = i + 1;
        while (j < count0 && points0[j].value == 0)
        {
            ++j;
        }
        if (j >= count0)
        {
            break;
        }
        if (!Geo::is_coincide(points0[i], points0[j], polygon2))
        {
            continue;
        }

        it0 = std::find_if(points1.begin(), points1.end(), [&](const MarkedPoint &p)
                { return !p.original && Geo::distance(p, points0[i]) < Geo::EPSILON; });
        it1 = std::find_if(points1.begin(), points1.end(), [&](const MarkedPoint &p)
            { return !p.original && Geo::distance(p, points0[j]) < Geo::EPSILON; });
        if (it0 == points1.end() || it1 == points1.end() || it0->value * it1->value <= 0)
        {
            continue;
        }

        for (size_t k = 1; k < count1; ++k)
        {
            if (!Geo::is_part(*it0, *it1, polygon3[k - 1], polygon3[k]))
            {
                continue;
            }

            if (it0->value > 0 && it1->value > 0)
            {
                points1.erase(it0);
                if (points0[i].value != 0)
                {
                    points0.erase(points0.begin() + i);
                    --count0;
                }
            }
            else if (it0->value < 0 && it1->value < 0)
            {
                points1.erase(it1);
                if (points0[j].value != 0)
                {
                    points0.erase(points0.begin() + j);
                    --count0;
                }
            }
            break;
        }
    }
    for (size_t i = points0.size() - 1, j = 0;;)
    {
        while (i > 0 && points0[i].value == 0)
        {
            --i;
        }
        if (i == 0)
        {
            break;
        }
        while (j < i && points0[j].value == 0)
        {
            ++j;
        }
        if (j >= i || !Geo::is_coincide(points0[i], points0[j], polygon2))
        {
            break;
        }

        it0 = std::find_if(points1.begin(), points1.end(), [&](const MarkedPoint &p)
                { return !p.original && Geo::distance(p, points0[i]) < Geo::EPSILON; });
        it1 = std::find_if(points1.begin(), points1.end(), [&](const MarkedPoint &p)
            { return !p.original && Geo::distance(p, points0[j]) < Geo::EPSILON; });
        if (it0 == points1.end() || it1 == points1.end() || it0->value * it1->value <= 0)
        {
            break;
        }

        for (size_t k = 1, count1 = polygon3.size(); k < count1; ++k)
        {
            if (!Geo::is_part(*it0, *it1, polygon3[k - 1], polygon3[k]))
            {
                continue;
            }

            if (it0->value > 0 && it1->value > 0)
            {
                points1.erase(it0);
                if (points0[i].value != 0)
                {
                    points0.erase(points0.begin() + i);
                }
            }
            else if (it0->value < 0 && it1->value < 0)
            {
                points1.erase(it1);
                if (points0[j].value != 0)
                {
                    points0.erase(points0.begin() + j);
                }
            }
            break;
        }
        break;
    }
    for (size_t i = 0, j = 1, count0 = polygon2.size(), count1 = points1.size(); j < count1; i = j)
    {
        while (i < count1 && points1[i].value == 0)
        {
            ++i;
        }
        j = i + 1;
        while (j < count1 && points1[j].value == 0)
        {
            ++j;
        }
        if (j >= count1)
        {
            break;
        }
        if (!Geo::is_coincide(points1[i], points1[j], polygon3))
        {
            continue;
        }

        it0 = std::find_if(points0.begin(), points0.end(), [&](const MarkedPoint &p)
                { return !p.original && Geo::distance(p, points1[i]) < Geo::EPSILON; });
        it1 = std::find_if(points0.begin(), points0.end(), [&](const MarkedPoint &p)
            { return !p.original && Geo::distance(p, points1[j]) < Geo::EPSILON; });
        if (it0 == points0.end() || it1 == points0.end() || it0->value * it1->value <= 0)
        {
            continue;
        }

        for (size_t k = 1; k < count0; ++k)
        {
            if (!Geo::is_part(*it0, *it1, polygon2[k - 1], polygon2[k]))
            {
                continue;
            }

            if (it0->value > 0 && it1->value > 0)
            {
                points0.erase(it0);
                if (points1[i].value != 0)
                {
                    points1.erase(points1.begin() + i);
                    --count1;
                }
            }
            else if (it0->value < 0 && it1->value < 0)
            {
                points0.erase(it1);
                if (points1[j].value != 0)
                {
                    points1.erase(points1.begin() + j);
                    --count1;
                }
            }
            break;
        }
    }
    for (size_t i = points1.size() - 1, j = 0;;)
    {
        while (i > 0 && points1[i].value == 0)
        {
            --i;
        }
        if (i == 0)
        {
            break;
        }
        while (j < i && points1[j].value == 0)
        {
            ++j;
        }
        if (j >= i || !Geo::is_coincide(points1[i], points1[j], polygon3))
        {
            break;
        }

        it0 = std::find_if(points0.begin(), points0.end(), [&](const MarkedPoint &p)
            { return !p.original && Geo::distance(p, points1[i]) < Geo::EPSILON; });
        it1 = std::find_if(points0.begin(), points0.end(), [&](const MarkedPoint &p)
            { return !p.original && Geo::distance(p, points1[j]) < Geo::EPSILON; });
        if (it0 == points0.end() || it1 == points0.end() || it0->value * it1->value <= 0)
        {
            break;
        }

        for (size_t k = 1, count0 = polygon2.size(); k < count0; ++k)
        {
            if (!Geo::is_part(*it0, *it1, polygon2[k - 1], polygon2[k]))
            {
                continue;
            }

            if (it0->value > 0 && it1->value > 0)
            {
                points0.erase(it0);
                if (points1[i].value != 0)
                {
                    points1.erase(points1.begin() + i);
                }
            }
            else if (it0->value < 0 && it1->value < 0)
            {
                points0.erase(it1);
                if (points1[j].value != 0)
                {
                    points1.erase(points1.begin() + j);
                }
            }
            break;
        }
        break;
    }

    std::vector<Point> result;
    size_t index0 = 0, index1 = 0;
    size_t count0 = points0.size(), count1 = points1.size();
    size_t count2 = count0 + count1;
    for (MarkedPoint &p : points0)
    {
        p.active = true;
    }
    for (MarkedPoint &p : points1)
    {
        p.active = true;
    }
    output.clear();

    while (count0 > 0 && count1 > 0)
    {
        output.emplace_back();

        index0 = index1 = 0;
        while (index0 < count0 && points0[index0].value < 1)
        {
            ++index0;
        }

        if (index0 >= count0)
        {
            output.pop_back();
            break;
        }

        while (result.size() < count2 && (result.size() < 4 || result.front() != result.back()))
        {
            while (result.size() < count2 && (result.size() < 4 || result.front() != result.back()))
            {
                if (points0[index0].value < 1)
                {
                    if (points0[index0].original)
                    {
                        points0[index0].active = false;
                    }
                    result.emplace_back(points0[index0++]);
                }
                else
                {
                    index1 = 0;
                    while (index1 < count1 && points1[index1] != points0[index0])
                    {
                        ++index1;
                    }
                    index1 %= count1;
                    result.emplace_back(points1[index1++]);
                    ++index0;
                    index0 %= count0;
                    index1 %= count1;
                    break;
                }
                index0 %= count0;
            }

            while (result.size() < count2 && (result.size() < 4 || result.front() != result.back()))
            {
                if (points1[index1].value < 1)
                {
                    if (points1[index1].original)
                    {
                        points1[index1].active = false;
                    }
                    result.emplace_back(points1[index1++]);
                }
                else
                {
                    index0 = 0;
                    while (index0 < count0 && points0[index0] != points1[index1])
                    {
                        ++index0;
                    }
                    index0 %= count0;
                    result.emplace_back(points0[index0++]);
                    ++index1;
                    index0 %= count0;
                    index1 %= count1;
                    break;
                }
                index1 %= count1;
            }
        }

        for (size_t i = result.size() - 1; i > 1; --i)
        {
            if (Geo::is_inside(result[i - 1], result[i - 2], result[i]))
            {
                result.erase(result.begin() + i - 1);
            }
            else if (Geo::distance(result[i - 2], result[i]) < Geo::EPSILON)
            {
                result.erase(result.begin() + i--);
                result.erase(result.begin() + i);
            }
        }
        if (result.size() > 4)
        {
            if (Geo::is_inside(result.front(), result[1], result[result.size() - 2]))
            {
                result.pop_back();
                std::swap(result.front(), result.back());
                result.pop_back();
            }
            else if (Geo::distance(result[1], result[result.size() - 2]) < Geo::EPSILON)
            {
                result.pop_back();
                result.pop_back();
                result.erase(result.begin());
            }
        }
        output.back().append(result.begin(), result.end());

        it0 = points0.begin();
        while (it0 != points0.end())
        {
            if (!it0->active || std::find(result.begin(), result.end(), *it0) != result.end())
            {
                it0 = points0.erase(it0);
            }
            else
            {
                ++it0;
            }
        }
        it1 = points1.begin();
        while (it1 != points1.end())
        {
            if (!it1->active || std::find(result.begin(), result.end(), *it1) != result.end())
            {
                it1 = points1.erase(it1);
            }
            else
            {
                ++it1;
            }
        }

        if (output.back().area() == 0)
        {
            output.pop_back();
        }

        if (std::count_if(points0.cbegin(), points0.cend(), [](const MarkedPoint &p){ return p.value < 0; }) == 0 ||
            std::count_if(points1.cbegin(), points1.cend(), [](const MarkedPoint &p){ return p.value < 0; }) == 0)
        {
            break;
        }

        count0 = points0.size();
        count1 = points1.size();
        count2 = count0 + count1;
        result.clear();
    }

    return !output.empty();
}

bool Geo::polygon_difference(const Geo::Polygon &polygon0, const Geo::Polygon &polygon1, std::vector<Geo::Polygon> &output)
{
    Geo::Polygon polygon2(polygon0), polygon3(polygon1);
    polygon2.reorder_points(true);
    polygon3.reorder_points(false);
    std::vector<Geo::MarkedPoint> points0, points1;
    for (const Geo::Point &point : polygon2)
    {
        points0.emplace_back(point.x, point.y);
    }
    for (const Geo::Point &point : polygon3)
    {
        points1.emplace_back(point.x, point.y);
    }

    Geo::Point point, pre_point; // 找到交点并计算其几何数
    const Geo::AABBRect rect(polygon1.bounding_rect());
    for (size_t i = 1, count0 = points0.size(); i < count0; ++i)
    {
        if (!Geo::is_intersected(rect, points0[i - 1], points0[i])) // 粗筛
        {
            continue;
        }

        pre_point = points0[i - 1];
        for (size_t k = 1, j = 1, count1 = points1.size(); j < count1; ++j)
        {
            k = j - 1;
            while (!points1[k].active)
            {
                --k; // 跳过非活动交点
            }
            while (j < count1 && (points1[k] == points1[j] || !points1[j].active))
            {
                k = j;
                ++j;
                while (!points1[k].active)
                {
                    --k; // 跳过非活动交点
                }
            }
            if (j >= count1)
            {
                continue;
            }
            k = j - 1;
            while (!points1[k].active)
            {
                --k; // 跳过非活动交点
            }
            if (!Geo::is_parallel(pre_point, points0[i], points1[k], points1[j]) &&
                Geo::is_intersected(pre_point, points0[i], points1[k], points1[j], point))
            {
                points0.insert(points0.begin() + i++, Geo::MarkedPoint(point.x, point.y, false));
                points1.insert(points1.begin() + j++, Geo::MarkedPoint(point.x, point.y, false));
                ++count0;
                ++count1;
                if (Geo::cross(pre_point, points0[i], points1[k], points1[j]) >= 0)
                {
                    points0[i - 1].value = 1;
                    points1[j - 1].value = -1;
                }
                else
                {
                    points0[i - 1].value = -1;
                    points1[j - 1].value = 1;
                }
            }
        }

        // 将本次循环添加的交点休眠,令下次循环polygon1处于无活动交点状态以排除干扰
        for (Geo::MarkedPoint &p : points1)
        {
            if (!p.original)
            {
                p.active = false;
            }
        }
    }

    if (points0.size() == polygon0.size()) // 无交点
    {
        return false;
    }

    // 调整交点顺序,同一条边上的交点按照顺序排列
    std::vector<Geo::MarkedPoint> points;
    for (size_t i = 1, j = 0, count = points0.size() - 1; i < count; ++i)
    {
        if (points0[i].original)
        {
            continue;
        }
        else
        {
            j = i;
        }
        while (j < count && !points0[j].original)
        {
            ++j;
        }
        if (j == i + 1)
        {
            ++i;
            continue;
        }

        points.assign(points0.begin() + i, j < count ? points0.begin() + j : points0.end());
        std::sort(points.begin(), points.end(), [&](const Geo::MarkedPoint &p0, const Geo::MarkedPoint &p1)
            { return Geo::distance(p0, points0[i - 1]) < Geo::distance(p1, points0[i - 1]); });
        for (size_t k = i, n = 0; k < j; ++k)
        {
            points0[k] = points[n++];
        }
        i = j;
    }
    for (size_t i = points0.size() - 1; i > 1;)
    {
        if (polygon0.front() == points0[i])
        {
            if (!points0[i].original)
            {
                points0.insert(points0.begin(), points0[i]);
                points0.erase(points0.begin() + i + 1);
            }
            else
            {
                --i;
            }
        }
        else
        {
            break;
        }
    }
    for (size_t i = 1, j = 0, count = points1.size() - 1; i < count; ++i)
    {
        if (points1[i].original)
        {
            continue;
        }
        else
        {
            j = i;
        }
        while (j < count && !points1[j].original)
        {
            ++j;
        }
        if (j == i + 1)
        {
            ++i;
            continue;
        }

        points.assign(points1.begin() + i, j < count ? points1.begin() + j : points1.end());
        std::sort(points.begin(), points.end(), [&](const MarkedPoint &p0, const MarkedPoint &p1)
            { return Geo::distance(p0, points1[i - 1]) < Geo::distance(p1, points1[i - 1]); });
        for (size_t k = i, n = 0; k < j; ++k)
        {
            points1[k] = points[n++];
        }
        i = j;
    }
    for (size_t i = points1.size() - 1; i > 1;)
    {
        if (polygon1.front() == points1[i])
        {
            if (!points1[i].original)
            {
                points1.insert(points1.begin(), points1[i]);
                points1.erase(points1.begin() + i + 1);
            }
            else
            {
                --i;
            }
        }
        else
        {
            break;
        }
    }

    // 去除重复交点
    int value;
    Geo::Point point_a, point_b, point_c, point_d;
    bool flags[5];
    for (size_t count, j, i = points0.size() - 1; i > 0; --i)
    {
        count = points0[i].original ? 0 : 1;
        for (j = i; j > 0; --j)
        {
            if (std::abs(points0[i].x - points0[j - 1].x) > Geo::EPSILON ||
                std::abs(points0[i].y - points0[j - 1].y) > Geo::EPSILON)
            {
                break;
            }
            if (!points0[j - 1].original)
            {
                ++count;
            }
        }
        if (count < 2)
        {
            continue;
        }

        value = 0;
        for (size_t k = i; k > j; --k)
        {
            if (!points0[k].original)
            {
                value += points0[k].value;
            }
        }
        if (!points0[j].original)
        {
            value += points0[j].value;
        }
        if (count < 4)
        {
            if (value == 0)
            {
                for (size_t k = i; k > j; --k)
                {
                    if (!points0[k].original)
                    {
                        points0.erase(points0.begin() + k);
                    }
                }
                if (!points0[j].original)
                {
                    points0.erase(points0.begin() + j);
                }
            }
            else
            {
                flags[0] = false;
                for (size_t k = i; k > j; --k)
                {
                    flags[0] = (flags[0] || points0[k].original);
                    points0.erase(points0.begin() + k);
                }
                points0[j].value = value;
                points0[j].original = (flags[0] || points0[j].original);
            }
        }
        else
        {
            point = points0[i];
            point_a = polygon0.last_point(polygon0.index(point.x, point.y));
            point_b = polygon0.next_point(polygon0.index(point.x, point.y));
            point_c = polygon1.last_point(polygon1.index(point.x, point.y));
            point_d = polygon1.next_point(polygon1.index(point.x, point.y));

            flags[2] = Geo::cross(point_a - point, point_b - point) > 0;
            flags[3] = Geo::cross(point_a - point, point_c - point) > 0;
            flags[4] = Geo::cross(point_c - point, point_b - point) > 0;
            flags[0] = !(flags[2] == flags[3] && flags[3] == flags[4]);
            flags[2] = Geo::cross(point_a - point, point_b - point) > 0;
            flags[3] = Geo::cross(point_a - point, point_d - point) > 0;
            flags[4] = Geo::cross(point_d - point, point_b - point) > 0;
            flags[1] = !(flags[2] == flags[3] && flags[3] == flags[4]);

            if (flags[0] && flags[1])
            {
                for (size_t k = i; k > j; --k)
                {
                    if (!points0[k].original)
                    {
                        points0.erase(points0.begin() + k);
                    }
                }
                if (!points0[j].original)
                {
                    points0.erase(points0.begin() + j);
                }
            }
            else
            {
                flags[0] = false;
                for (size_t k = i; k > j; --k)
                {
                    flags[0] = (flags[0] || points0[k].original);
                    points0.erase(points0.begin() + k);
                }
                points0[j].value = value;
                points0[j].original = (flags[0] || points0[j].original);
            }
        }
        i = j > 0 ? j : 1;
    }
    for (size_t count, j, i = points1.size() - 1; i > 0; --i)
    {
        count = points1[i].original ? 0 : 1;
        for (j = i; j > 0; --j)
        {
            if (std::abs(points1[i].x - points1[j - 1].x) > Geo::EPSILON || 
                std::abs(points1[i].y - points1[j - 1].y) > Geo::EPSILON)
            {
                break;
            }
            if (!points1[j - 1].original)
            {
                ++count;
            }
        }
        if (count < 2)
        {
            continue;
        }

        value = 0;
        for (size_t k = i; k > j; --k)
        {
            if (!points1[k].original)
            {
                value += points1[k].value;
            }
        }
        if (!points1[j].original)
        {
            value += points1[j].value;
        }
        if (count < 4)
        {
            if (value == 0)
            {
                for (size_t k = i; k > j; --k)
                {
                    if (!points1[k].original)
                    {
                        points1.erase(points1.begin() + k);
                    }
                }
                if (!points1[j].original)
                {
                    points1.erase(points1.begin() + j);
                }
            }
            else
            {
                flags[0] = false;
                for (size_t k = i; k > j; --k)
                {
                    flags[0] = (flags[0] || points1[k].original);
                    points1.erase(points1.begin() + k);
                }
                points1[j].value = value;
                points1[j].original = (flags[0] || points1[j].original);
            }
        }
        else
        {
            point = points1[i];
            point_a = polygon1.last_point(polygon1.index(point.x, point.y));
            point_b = polygon1.next_point(polygon1.index(point.x, point.y));
            point_c = polygon0.last_point(polygon0.index(point.x, point.y));
            point_d = polygon0.next_point(polygon0.index(point.x, point.y));

            flags[2] = Geo::cross(point_a - point, point_b - point) > 0;
            flags[3] = Geo::cross(point_a - point, point_c - point) > 0;
            flags[4] = Geo::cross(point_c - point, point_b - point) > 0;
            flags[0] = !(flags[2] == flags[3] && flags[3] == flags[4]);
            flags[2] = Geo::cross(point_a - point, point_b - point) > 0;
            flags[3] = Geo::cross(point_a - point, point_d - point) > 0;
            flags[4] = Geo::cross(point_d - point, point_b - point) > 0;
            flags[1] = !(flags[2] == flags[3] && flags[3] == flags[4]);

            if (flags[0] && flags[1])
            {
                for (size_t k = i; k > j; --k)
                {
                    if (!points1[k].original)
                    {
                        points1.erase(points1.begin() + k);
                    }
                }
                if (!points1[j].original)
                {
                    points1.erase(points1.begin() + j);
                }
            }
            else
            {
                flags[0] = false;
                for (size_t k = i; k > j; --k)
                {
                    flags[0] = (flags[0] || points1[k].original);
                    points1.erase(points1.begin() + k);
                }
                points1[j].value = value;
                points1[j].original = (flags[0] || points1[j].original);
            }
        }
        i = j > 0 ? j : 1;
    }

    if (std::count_if(points0.begin(), points0.end(), [](const Geo::MarkedPoint &p) { return p.value < 0; }) == 0 || 
        std::count_if(points1.begin(), points1.end(), [](const Geo::MarkedPoint &p) { return p.value < 0; }) == 0)
    {
        return false; // 交点都是出点,即两多边形只有一个点相交
    }

    // 处理重边上的交点
    std::vector<Geo::MarkedPoint>::iterator it0, it1;
    for (size_t i = 0, j = 1, count0 = points0.size(), count1 = polygon3.size(); j < count0; i = j)
    {
        while (i < count0 && points0[i].value == 0)
        {
            ++i;
        }
        j = i + 1;
        while (j < count0 && points0[j].value == 0)
        {
            ++j;
        }
        if (j >= count0)
        {
            break;
        }
        if (!Geo::is_coincide(points0[i], points0[j], polygon2))
        {
            continue;
        }

        it0 = std::find_if(points1.begin(), points1.end(), [&](const MarkedPoint &p)
                { return !p.original && Geo::distance(p, points0[i]) < Geo::EPSILON; });
        it1 = std::find_if(points1.begin(), points1.end(), [&](const MarkedPoint &p)
            { return !p.original && Geo::distance(p, points0[j]) < Geo::EPSILON; });
        if (it0 == points1.end() || it1 == points1.end() || it0->value * it1->value <= 0)
        {
            continue;
        }

        for (size_t k = 1; k < count1; ++k)
        {
            if (!Geo::is_part(*it0, *it1, polygon3[k - 1], polygon3[k]))
            {
                continue;
            }

            if (it0->value < 0 && it1->value < 0)
            {
                points1.erase(it0);
                if (points0[i].value != 0)
                {
                    points0.erase(points0.begin() + i);
                    --count0;
                }
            }
            else if (it0->value > 0 && it1->value > 0)
            {
                points1.erase(it1);
                if (points0[j].value != 0)
                {
                    points0.erase(points0.begin() + j);
                    --count0;
                }
            }
            break;
        }
    }
    for (size_t i = points0.size() - 1, j = 0;;)
    {
        while (i > 0 && points0[i].value == 0)
        {
            --i;
        }
        if (i == 0)
        {
            break;
        }
        while (j < i && points0[j].value == 0)
        {
            ++j;
        }
        if (j >= i || !Geo::is_coincide(points0[i], points0[j], polygon2))
        {
            break;
        }

        it0 = std::find_if(points1.begin(), points1.end(), [&](const MarkedPoint &p)
                { return !p.original && Geo::distance(p, points0[i]) < Geo::EPSILON; });
        it1 = std::find_if(points1.begin(), points1.end(), [&](const MarkedPoint &p)
            { return !p.original && Geo::distance(p, points0[j]) < Geo::EPSILON; });
        if (it0 == points1.end() || it1 == points1.end() || it0->value * it1->value <= 0)
        {
            break;
        }

        for (size_t k = 1, count1 = polygon3.size(); k < count1; ++k)
        {
            if (!Geo::is_part(*it0, *it1, polygon3[k - 1], polygon3[k]))
            {
                continue;
            }

            if (it0->value < 0 && it1->value < 0)
            {
                points1.erase(it0);
                if (points0[i].value != 0)
                {
                    points0.erase(points0.begin() + i);
                }
            }
            else if (it0->value > 0 && it1->value > 0)
            {
                points1.erase(it1);
                if (points0[j].value != 0)
                {
                    points0.erase(points0.begin() + j);
                }
            }
            break;
        }
        break;
    }
    for (size_t i = 0, j = 1, count0 = polygon2.size(), count1 = points1.size(); j < count1; i = j)
    {
        while (i < count1 && points1[i].value == 0)
        {
            ++i;
        }
        j = i + 1;
        while (j < count1 && points1[j].value == 0)
        {
            ++j;
        }
        if (j >= count1)
        {
            break;
        }
        if (!Geo::is_coincide(points1[i], points1[j], polygon3))
        {
            continue;
        }

        it0 = std::find_if(points0.begin(), points0.end(), [&](const MarkedPoint &p)
                { return !p.original && Geo::distance(p, points1[i]) < Geo::EPSILON; });
        it1 = std::find_if(points0.begin(), points0.end(), [&](const MarkedPoint &p)
            { return !p.original && Geo::distance(p, points1[j]) < Geo::EPSILON; });
        if (it0 == points0.end() || it1 == points0.end() || it0->value * it1->value <= 0)
        {
            continue;
        }

        for (size_t k = 1; k < count0; ++k)
        {
            if (!Geo::is_part(*it0, *it1, polygon2[k - 1], polygon2[k]))
            {
                continue;
            }

            if (it0->value < 0 && it1->value < 0)
            {
                points0.erase(it0);
                if (points1[i].value != 0)
                {
                    points1.erase(points1.begin() + i);
                    --count1;
                }
            }
            else if (it0->value > 0 && it1->value > 0)
            {
                points0.erase(it1);
                if (points1[j].value != 0)
                {
                    points1.erase(points1.begin() + j);
                    --count1;
                }
            }
            break;
        }
    }
    for (size_t i = points1.size() - 1, j = 0;;)
    {
        while (i > 0 && points1[i].value == 0)
        {
            --i;
        }
        if (i == 0)
        {
            break;
        }
        while (j < i && points1[j].value == 0)
        {
            ++j;
        }
        if (j >= i || !Geo::is_coincide(points1[i], points1[j], polygon3))
        {
            break;
        }

        it0 = std::find_if(points0.begin(), points0.end(), [&](const MarkedPoint &p)
            { return !p.original && Geo::distance(p, points1[i]) < Geo::EPSILON; });
        it1 = std::find_if(points0.begin(), points0.end(), [&](const MarkedPoint &p)
            { return !p.original && Geo::distance(p, points1[j]) < Geo::EPSILON; });
        if (it0 == points0.end() || it1 == points0.end() || it0->value * it1->value <= 0)
        {
            break;
        }

        for (size_t k = 1, count0 = polygon2.size(); k < count0; ++k)
        {
            if (!Geo::is_part(*it0, *it1, polygon2[k - 1], polygon2[k]))
            {
                continue;
            }

            if (it0->value < 0 && it1->value < 0)
            {
                points0.erase(it0);
                if (points1[i].value != 0)
                {
                    points1.erase(points1.begin() + i);
                }
            }
            else if (it0->value > 0 && it1->value > 0)
            {
                points0.erase(it1);
                if (points1[j].value != 0)
                {
                    points1.erase(points1.begin() + j);
                }
            }
            break;
        }
        break;
    }

    std::vector<Geo::Point> result;
    size_t index0 = 0, index1 = 0;
    size_t count0 = points0.size(), count1 = points1.size();
    size_t count2 = count0 + count1;
    for (Geo::MarkedPoint &p : points0)
    {
        p.active = true;
    }
    for (Geo::MarkedPoint &p : points1)
    {
        p.active = true;
    }
    output.clear();

    while (count0 > 0 && count1 > 0)
    {
        output.emplace_back();

        index0 = index1 = 0;
        while (index0 < count0 && points0[index0].value < 1)
        {
            ++index0;
        }

        if (index0 >= count0)
        {
            output.pop_back();
            break;
        }

        while (result.size() < count2 && (result.size() < 4 || result.front() != result.back()))
        {
            while (result.size() < count2 && (result.size() < 4 || result.front() != result.back()))
            {
                if (points0[index0].value > -1)
                {
                    if (points0[index0].original)
                    {
                        points0[index0].active = false;
                    }
                    result.emplace_back(points0[index0++]);
                }
                else
                {
                    index1 = 0;
                    while (index1 < count1 && points1[index1] != points0[index0])
                    {
                        ++index1;
                    }
                    index1 %= count1;
                    result.emplace_back(points1[index1++]);
                    ++index0;
                    index0 %= count0;
                    index1 %= count1;
                    break;
                }
                index0 %= count0;
            }

            while (result.size() < count2 && (result.size() < 4 || result.front() != result.back()))
            {
                if (points1[index1].value > -1)
                {
                    if (points1[index1].original)
                    {
                        points1[index1].active = false;
                    }
                    result.emplace_back(points1[index1++]);
                }
                else
                {
                    index0 = 0;
                    while (index0 < count0 && points0[index0] != points1[index1])
                    {
                        ++index0;
                    }
                    index0 %= count0;
                    result.emplace_back(points0[index0++]);
                    ++index1;
                    index0 %= count0;
                    index1 %= count1;
                    break;
                }
                index1 %= count1;
            }
        }

        for (size_t i = result.size() - 1; i > 1; --i)
        {
            if (Geo::is_inside(result[i - 1], result[i - 2], result[i]))
            {
                result.erase(result.begin() + i - 1);
            }
            else if (Geo::distance(result[i - 2], result[i]) < Geo::EPSILON)
            {
                result.erase(result.begin() + i--);
                result.erase(result.begin() + i);
            }
        }
        if (result.size() > 4)
        {
            if (Geo::is_inside(result.front(), result[1], result[result.size() - 2]))
            {
                result.pop_back();
                std::swap(result.front(), result.back());
                result.pop_back();
            }
            else if (Geo::distance(result[1], result[result.size() - 2]) < Geo::EPSILON)
            {
                result.pop_back();
                result.pop_back();
                result.erase(result.begin());
            }
        }
        output.back().append(result.begin(), result.end());

        it0 = points0.begin();
        while (it0 != points0.end())
        {
            if (!it0->active || std::find(result.begin(), result.end(), *it0) != result.end())
            {
                it0 = points0.erase(it0);
            }
            else
            {
                ++it0;
            }
        }
        it1 = points1.begin();
        while (it1 != points1.end())
        {
            if (!it1->active || std::find(result.begin(), result.end(), *it1) != result.end())
            {
                it1 = points1.erase(it1);
            }
            else
            {
                ++it1;
            }
        }

        if (output.back().area() == 0)
        {
            output.pop_back();
        }

        if (std::count_if(points0.cbegin(), points0.cend(), [](const MarkedPoint &p){ return p.value < 0; }) == 0 ||
            std::count_if(points1.cbegin(), points1.cend(), [](const MarkedPoint &p){ return p.value < 0; }) == 0)
        {
            break;
        }

        count0 = points0.size();
        count1 = points1.size();
        count2 = count0 + count1;
        result.clear();
    }

    return !output.empty();
}

bool Geo::polygon_xor(const Polygon &polygon0, const Polygon &polygon1, std::vector<Polygon> &output)
{
    Geo::Polygon polygon2(polygon0), polygon3(polygon1);
    polygon2.reorder_points(true);
    polygon3.reorder_points(false);
    std::vector<Geo::MarkedPoint> points0, points1;
    for (const Geo::Point &point : polygon2)
    {
        points0.emplace_back(point.x, point.y);
    }
    for (const Geo::Point &point : polygon3)
    {
        points1.emplace_back(point.x, point.y);
    }

    Geo::Point point, pre_point; // 找到交点并计算其几何数
    const Geo::AABBRect rect(polygon1.bounding_rect());
    for (size_t i = 1, count0 = points0.size(); i < count0; ++i)
    {
        if (!Geo::is_intersected(rect, points0[i - 1], points0[i])) // 粗筛
        {
            continue;
        }

        pre_point = points0[i - 1];
        for (size_t k = 1, j = 1, count1 = points1.size(); j < count1; ++j)
        {
            k = j - 1;
            while (!points1[k].active)
            {
                --k; // 跳过非活动交点
            }
            while (j < count1 && (points1[k] == points1[j] || !points1[j].active))
            {
                k = j;
                ++j;
                while (!points1[k].active)
                {
                    --k; // 跳过非活动交点
                }
            }
            if (j >= count1)
            {
                continue;
            }
            k = j - 1;
            while (!points1[k].active)
            {
                --k; // 跳过非活动交点
            }
            if (!Geo::is_parallel(pre_point, points0[i], points1[k], points1[j]) &&
                Geo::is_intersected(pre_point, points0[i], points1[k], points1[j], point))
            {
                points0.insert(points0.begin() + i++, Geo::MarkedPoint(point.x, point.y, false));
                points1.insert(points1.begin() + j++, Geo::MarkedPoint(point.x, point.y, false));
                ++count0;
                ++count1;
                if (Geo::cross(pre_point, points0[i], points1[k], points1[j]) >= 0)
                {
                    points0[i - 1].value = 1;
                    points1[j - 1].value = -1;
                }
                else
                {
                    points0[i - 1].value = -1;
                    points1[j - 1].value = 1;
                }
            }
        }

        // 将本次循环添加的交点休眠,令下次循环polygon1处于无活动交点状态以排除干扰
        for (Geo::MarkedPoint &p : points1)
        {
            if (!p.original)
            {
                p.active = false;
            }
        }
    }

    if (points0.size() == polygon0.size()) // 无交点
    {
        return false;
    }

    // 调整交点顺序,同一条边上的交点按照顺序排列
    std::vector<Geo::MarkedPoint> points;
    for (size_t i = 1, j = 0, count = points0.size() - 1; i < count; ++i)
    {
        if (points0[i].original)
        {
            continue;
        }
        else
        {
            j = i;
        }
        while (j < count && !points0[j].original)
        {
            ++j;
        }
        if (j == i + 1)
        {
            ++i;
            continue;
        }

        points.assign(points0.begin() + i, j < count ? points0.begin() + j : points0.end());
        std::sort(points.begin(), points.end(), [&](const Geo::MarkedPoint &p0, const Geo::MarkedPoint &p1)
            { return Geo::distance(p0, points0[i - 1]) < Geo::distance(p1, points0[i - 1]); });
        for (size_t k = i, n = 0; k < j; ++k)
        {
            points0[k] = points[n++];
        }
        i = j;
    }
    for (size_t i = points0.size() - 1; i > 1;)
    {
        if (polygon0.front() == points0[i])
        {
            if (!points0[i].original)
            {
                points0.insert(points0.begin(), points0[i]);
                points0.erase(points0.begin() + i + 1);
            }
            else
            {
                --i;
            }
        }
        else
        {
            break;
        }
    }
    for (size_t i = 1, j = 0, count = points1.size() - 1; i < count; ++i)
    {
        if (points1[i].original)
        {
            continue;
        }
        else
        {
            j = i;
        }
        while (j < count && !points1[j].original)
        {
            ++j;
        }
        if (j == i + 1)
        {
            ++i;
            continue;
        }

        points.assign(points1.begin() + i, j < count ? points1.begin() + j : points1.end());
        std::sort(points.begin(), points.end(), [&](const MarkedPoint &p0, const MarkedPoint &p1)
            { return Geo::distance(p0, points1[i - 1]) < Geo::distance(p1, points1[i - 1]); });
        for (size_t k = i, n = 0; k < j; ++k)
        {
            points1[k] = points[n++];
        }
        i = j;
    }
    for (size_t i = points1.size() - 1; i > 1;)
    {
        if (polygon1.front() == points1[i])
        {
            if (!points1[i].original)
            {
                points1.insert(points1.begin(), points1[i]);
                points1.erase(points1.begin() + i + 1);
            }
            else
            {
                --i;
            }
        }
        else
        {
            break;
        }
    }

    // 去除重复交点
    int value;
    Geo::Point point_a, point_b, point_c, point_d;
    bool flags[5];
    for (size_t count, j, i = points0.size() - 1; i > 0; --i)
    {
        count = points0[i].original ? 0 : 1;
        for (j = i; j > 0; --j)
        {
            if (std::abs(points0[i].x - points0[j - 1].x) > Geo::EPSILON ||
                std::abs(points0[i].y - points0[j - 1].y) > Geo::EPSILON)
            {
                break;
            }
            if (!points0[j - 1].original)
            {
                ++count;
            }
        }
        if (count < 2)
        {
            continue;
        }

        value = 0;
        for (size_t k = i; k > j; --k)
        {
            if (!points0[k].original)
            {
                value += points0[k].value;
            }
        }
        if (!points0[j].original)
        {
            value += points0[j].value;
        }
        if (count < 4)
        {
            if (value == 0)
            {
                for (size_t k = i; k > j; --k)
                {
                    if (!points0[k].original)
                    {
                        points0.erase(points0.begin() + k);
                    }
                }
                if (!points0[j].original)
                {
                    points0.erase(points0.begin() + j);
                }
            }
            else
            {
                flags[0] = false;
                for (size_t k = i; k > j; --k)
                {
                    flags[0] = (flags[0] || points0[k].original);
                    points0.erase(points0.begin() + k);
                }
                points0[j].value = value;
                points0[j].original = (flags[0] || points0[j].original);
            }
        }
        else
        {
            point = points0[i];
            point_a = polygon0.last_point(polygon0.index(point.x, point.y));
            point_b = polygon0.next_point(polygon0.index(point.x, point.y));
            point_c = polygon1.last_point(polygon1.index(point.x, point.y));
            point_d = polygon1.next_point(polygon1.index(point.x, point.y));

            flags[2] = Geo::cross(point_a - point, point_b - point) > 0;
            flags[3] = Geo::cross(point_a - point, point_c - point) > 0;
            flags[4] = Geo::cross(point_c - point, point_b - point) > 0;
            flags[0] = !(flags[2] == flags[3] && flags[3] == flags[4]);
            flags[2] = Geo::cross(point_a - point, point_b - point) > 0;
            flags[3] = Geo::cross(point_a - point, point_d - point) > 0;
            flags[4] = Geo::cross(point_d - point, point_b - point) > 0;
            flags[1] = !(flags[2] == flags[3] && flags[3] == flags[4]);

            if (flags[0] && flags[1])
            {
                for (size_t k = i; k > j; --k)
                {
                    if (!points0[k].original)
                    {
                        points0.erase(points0.begin() + k);
                    }
                }
                if (!points0[j].original)
                {
                    points0.erase(points0.begin() + j);
                }
            }
            else
            {
                flags[0] = false;
                for (size_t k = i; k > j; --k)
                {
                    flags[0] = (flags[0] || points0[k].original);
                    points0.erase(points0.begin() + k);
                }
                points0[j].value = value;
                points0[j].original = (flags[0] || points0[j].original);
            }
        }
        i = j > 0 ? j : 1;
    }
    for (size_t count, j, i = points1.size() - 1; i > 0; --i)
    {
        count = points1[i].original ? 0 : 1;
        for (j = i; j > 0; --j)
        {
            if (std::abs(points1[i].x - points1[j - 1].x) > Geo::EPSILON || 
                std::abs(points1[i].y - points1[j - 1].y) > Geo::EPSILON)
            {
                break;
            }
            if (!points1[j - 1].original)
            {
                ++count;
            }
        }
        if (count < 2)
        {
            continue;
        }

        value = 0;
        for (size_t k = i; k > j; --k)
        {
            if (!points1[k].original)
            {
                value += points1[k].value;
            }
        }
        if (!points1[j].original)
        {
            value += points1[j].value;
        }
        if (count < 4)
        {
            if (value == 0)
            {
                for (size_t k = i; k > j; --k)
                {
                    if (!points1[k].original)
                    {
                        points1.erase(points1.begin() + k);
                    }
                }
                if (!points1[j].original)
                {
                    points1.erase(points1.begin() + j);
                }
            }
            else
            {
                flags[0] = false;
                for (size_t k = i; k > j; --k)
                {
                    flags[0] = (flags[0] || points1[k].original);
                    points1.erase(points1.begin() + k);
                }
                points1[j].value = value;
                points1[j].original = (flags[0] || points1[j].original);
            }
        }
        else
        {
            point = points1[i];
            point_a = polygon1.last_point(polygon1.index(point.x, point.y));
            point_b = polygon1.next_point(polygon1.index(point.x, point.y));
            point_c = polygon0.last_point(polygon0.index(point.x, point.y));
            point_d = polygon0.next_point(polygon0.index(point.x, point.y));

            flags[2] = Geo::cross(point_a - point, point_b - point) > 0;
            flags[3] = Geo::cross(point_a - point, point_c - point) > 0;
            flags[4] = Geo::cross(point_c - point, point_b - point) > 0;
            flags[0] = !(flags[2] == flags[3] && flags[3] == flags[4]);
            flags[2] = Geo::cross(point_a - point, point_b - point) > 0;
            flags[3] = Geo::cross(point_a - point, point_d - point) > 0;
            flags[4] = Geo::cross(point_d - point, point_b - point) > 0;
            flags[1] = !(flags[2] == flags[3] && flags[3] == flags[4]);

            if (flags[0] && flags[1])
            {
                for (size_t k = i; k > j; --k)
                {
                    if (!points1[k].original)
                    {
                        points1.erase(points1.begin() + k);
                    }
                }
                if (!points1[j].original)
                {
                    points1.erase(points1.begin() + j);
                }
            }
            else
            {
                flags[0] = false;
                for (size_t k = i; k > j; --k)
                {
                    flags[0] = (flags[0] || points1[k].original);
                    points1.erase(points1.begin() + k);
                }
                points1[j].value = value;
                points1[j].original = (flags[0] || points1[j].original);
            }
        }
        i = j > 0 ? j : 1;
    }

    if (std::count_if(points0.begin(), points0.end(), [](const Geo::MarkedPoint &p) { return p.value < 0; }) == 0 || 
        std::count_if(points1.begin(), points1.end(), [](const Geo::MarkedPoint &p) { return p.value < 0; }) == 0)
    {
        return false; // 交点都是出点,即两多边形只有一个点相交
    }

    // 处理重边上的交点
    std::vector<Geo::MarkedPoint>::iterator it0, it1;
    for (size_t i = 0, j = 1, count0 = points0.size(), count1 = polygon3.size(); j < count0; i = j)
    {
        while (i < count0 && points0[i].value == 0)
        {
            ++i;
        }
        j = i + 1;
        while (j < count0 && points0[j].value == 0)
        {
            ++j;
        }
        if (j >= count0)
        {
            break;
        }
        if (!Geo::is_coincide(points0[i], points0[j], polygon2))
        {
            continue;
        }

        it0 = std::find_if(points1.begin(), points1.end(), [&](const MarkedPoint &p)
                { return !p.original && Geo::distance(p, points0[i]) < Geo::EPSILON; });
        it1 = std::find_if(points1.begin(), points1.end(), [&](const MarkedPoint &p)
            { return !p.original && Geo::distance(p, points0[j]) < Geo::EPSILON; });
        if (it0 == points1.end() || it1 == points1.end() || it0->value * it1->value <= 0)
        {
            continue;
        }

        for (size_t k = 1; k < count1; ++k)
        {
            if (!Geo::is_part(*it0, *it1, polygon3[k - 1], polygon3[k]))
            {
                continue;
            }

            if (it0->value < 0 && it1->value < 0)
            {
                points1.erase(it0);
                if (points0[i].value != 0)
                {
                    points0.erase(points0.begin() + i);
                    --count0;
                }
            }
            else if (it0->value > 0 && it1->value > 0)
            {
                points1.erase(it1);
                if (points0[j].value != 0)
                {
                    points0.erase(points0.begin() + j);
                    --count0;
                }
            }
            break;
        }
    }
    for (size_t i = points0.size() - 1, j = 0;;)
    {
        while (i > 0 && points0[i].value == 0)
        {
            --i;
        }
        if (i == 0)
        {
            break;
        }
        while (j < i && points0[j].value == 0)
        {
            ++j;
        }
        if (j >= i || !Geo::is_coincide(points0[i], points0[j], polygon2))
        {
            break;
        }

        it0 = std::find_if(points1.begin(), points1.end(), [&](const MarkedPoint &p)
                { return !p.original && Geo::distance(p, points0[i]) < Geo::EPSILON; });
        it1 = std::find_if(points1.begin(), points1.end(), [&](const MarkedPoint &p)
            { return !p.original && Geo::distance(p, points0[j]) < Geo::EPSILON; });
        if (it0 == points1.end() || it1 == points1.end() || it0->value * it1->value <= 0)
        {
            break;
        }

        for (size_t k = 1, count1 = polygon3.size(); k < count1; ++k)
        {
            if (!Geo::is_part(*it0, *it1, polygon3[k - 1], polygon3[k]))
            {
                continue;
            }

            if (it0->value < 0 && it1->value < 0)
            {
                points1.erase(it0);
                if (points0[i].value != 0)
                {
                    points0.erase(points0.begin() + i);
                }
            }
            else if (it0->value > 0 && it1->value > 0)
            {
                points1.erase(it1);
                if (points0[j].value != 0)
                {
                    points0.erase(points0.begin() + j);
                }
            }
            break;
        }
        break;
    }
    for (size_t i = 0, j = 1, count0 = polygon2.size(), count1 = points1.size(); j < count1; i = j)
    {
        while (i < count1 && points1[i].value == 0)
        {
            ++i;
        }
        j = i + 1;
        while (j < count1 && points1[j].value == 0)
        {
            ++j;
        }
        if (j >= count1)
        {
            break;
        }
        if (!Geo::is_coincide(points1[i], points1[j], polygon3))
        {
            continue;
        }

        it0 = std::find_if(points0.begin(), points0.end(), [&](const MarkedPoint &p)
                { return !p.original && Geo::distance(p, points1[i]) < Geo::EPSILON; });
        it1 = std::find_if(points0.begin(), points0.end(), [&](const MarkedPoint &p)
            { return !p.original && Geo::distance(p, points1[j]) < Geo::EPSILON; });
        if (it0 == points0.end() || it1 == points0.end() || it0->value * it1->value <= 0)
        {
            continue;
        }

        for (size_t k = 1; k < count0; ++k)
        {
            if (!Geo::is_part(*it0, *it1, polygon2[k - 1], polygon2[k]))
            {
                continue;
            }

            if (it0->value < 0 && it1->value < 0)
            {
                points0.erase(it0);
                if (points1[i].value != 0)
                {
                    points1.erase(points1.begin() + i);
                    --count1;
                }
            }
            else if (it0->value > 0 && it1->value > 0)
            {
                points0.erase(it1);
                if (points1[j].value != 0)
                {
                    points1.erase(points1.begin() + j);
                    --count1;
                }
            }
            break;
        }
    }
    for (size_t i = points1.size() - 1, j = 0;;)
    {
        while (i > 0 && points1[i].value == 0)
        {
            --i;
        }
        if (i == 0)
        {
            break;
        }
        while (j < i && points1[j].value == 0)
        {
            ++j;
        }
        if (j >= i || !Geo::is_coincide(points1[i], points1[j], polygon3))
        {
            break;
        }

        it0 = std::find_if(points0.begin(), points0.end(), [&](const MarkedPoint &p)
            { return !p.original && Geo::distance(p, points1[i]) < Geo::EPSILON; });
        it1 = std::find_if(points0.begin(), points0.end(), [&](const MarkedPoint &p)
            { return !p.original && Geo::distance(p, points1[j]) < Geo::EPSILON; });
        if (it0 == points0.end() || it1 == points0.end() || it0->value * it1->value <= 0)
        {
            break;
        }

        for (size_t k = 1, count0 = polygon2.size(); k < count0; ++k)
        {
            if (!Geo::is_part(*it0, *it1, polygon2[k - 1], polygon2[k]))
            {
                continue;
            }

            if (it0->value < 0 && it1->value < 0)
            {
                points0.erase(it0);
                if (points1[i].value != 0)
                {
                    points1.erase(points1.begin() + i);
                }
            }
            else if (it0->value > 0 && it1->value > 0)
            {
                points0.erase(it1);
                if (points1[j].value != 0)
                {
                    points1.erase(points1.begin() + j);
                }
            }
            break;
        }
        break;
    }

    std::vector<Geo::Point> result;
    size_t index0 = 0, index1 = 0;
    size_t count0 = points0.size(), count1 = points1.size();
    size_t count2 = count0 + count1;
    for (Geo::MarkedPoint &p : points0)
    {
        p.active = true;
    }
    for (Geo::MarkedPoint &p : points1)
    {
        p.active = true;
    }
    
    std::vector<Geo::MarkedPoint> points2(points0.begin(), points0.end());
    std::vector<Geo::MarkedPoint> points3(points1.begin(), points1.end());
    output.clear();

    while (count0 > 0 && count1 > 0)
    {
        output.emplace_back();

        index0 = index1 = 0;
        while (index0 < count0 && points0[index0].value < 1)
        {
            ++index0;
        }

        if (index0 >= count0)
        {
            output.pop_back();
            break;
        }

        while (result.size() < count2 && (result.size() < 4 || result.front() != result.back()))
        {
            while (result.size() < count2 && (result.size() < 4 || result.front() != result.back()))
            {
                if (points0[index0].value > -1)
                {
                    if (points0[index0].original)
                    {
                        points0[index0].active = false;
                    }
                    result.emplace_back(points0[index0++]);
                }
                else
                {
                    index1 = 0;
                    while (index1 < count1 && points1[index1] != points0[index0])
                    {
                        ++index1;
                    }
                    index1 %= count1;
                    result.emplace_back(points1[index1++]);
                    ++index0;
                    index0 %= count0;
                    index1 %= count1;
                    break;
                }
                index0 %= count0;
            }

            while (result.size() < count2 && (result.size() < 4 || result.front() != result.back()))
            {
                if (points1[index1].value > -1)
                {
                    if (points1[index1].original)
                    {
                        points1[index1].active = false;
                    }
                    result.emplace_back(points1[index1++]);
                }
                else
                {
                    index0 = 0;
                    while (index0 < count0 && points0[index0] != points1[index1])
                    {
                        ++index0;
                    }
                    index0 %= count0;
                    result.emplace_back(points0[index0++]);
                    ++index1;
                    index0 %= count0;
                    index1 %= count1;
                    break;
                }
                index1 %= count1;
            }
        }

        for (size_t i = result.size() - 1; i > 1; --i)
        {
            if (Geo::is_inside(result[i - 1], result[i - 2], result[i]))
            {
                result.erase(result.begin() + i - 1);
            }
            else if (Geo::distance(result[i - 2], result[i]) < Geo::EPSILON)
            {
                result.erase(result.begin() + i--);
                result.erase(result.begin() + i);
            }
        }
        if (result.size() > 4)
        {
            if (Geo::is_inside(result.front(), result[1], result[result.size() - 2]))
            {
                result.pop_back();
                std::swap(result.front(), result.back());
                result.pop_back();
            }
            else if (Geo::distance(result[1], result[result.size() - 2]) < Geo::EPSILON)
            {
                result.pop_back();
                result.pop_back();
                result.erase(result.begin());
            }
        }
        output.back().append(result.begin(), result.end());

        it0 = points0.begin();
        while (it0 != points0.end())
        {
            if (!it0->active || std::find(result.begin(), result.end(), *it0) != result.end())
            {
                it0 = points0.erase(it0);
            }
            else
            {
                ++it0;
            }
        }
        it1 = points1.begin();
        while (it1 != points1.end())
        {
            if (!it1->active || std::find(result.begin(), result.end(), *it1) != result.end())
            {
                it1 = points1.erase(it1);
            }
            else
            {
                ++it1;
            }
        }

        if (output.back().area() == 0)
        {
            output.pop_back();
        }

        if (std::count_if(points0.cbegin(), points0.cend(), [](const MarkedPoint &p){ return p.value < 0; }) == 0 ||
            std::count_if(points1.cbegin(), points1.cend(), [](const MarkedPoint &p){ return p.value < 0; }) == 0)
        {
            break;
        }

        count0 = points0.size();
        count1 = points1.size();
        count2 = count0 + count1;
        result.clear();
    }

    for (Geo::MarkedPoint &point : points2)
    {
        if (point.value != 0)
        {
            point.value = -point.value;
        }
    }
    for (Geo::MarkedPoint &point : points3)
    {
        if (point.value != 0)
        {
            point.value = -point.value;
        }
    }
    index0 = index1 = 0;
    count0 = points2.size(), count1 = points3.size();
    count2 = count0 + count1;
    result.clear();
    while (count0 > 0 && count1 > 0)
    {
        output.emplace_back();

        index0 = index1 = 0;
        while (index0 < count0 && points2[index0].value < 1)
        {
            ++index0;
        }

        if (index0 >= count0)
        {
            output.pop_back();
            break;
        }

        while (result.size() < count2 && (result.size() < 4 || result.front() != result.back()))
        {
            while (result.size() < count2 && (result.size() < 4 || result.front() != result.back()))
            {
                if (points2[index0].value > -1)
                {
                    if (points2[index0].original)
                    {
                        points2[index0].active = false;
                    }
                    result.emplace_back(points2[index0++]);
                }
                else
                {
                    index1 = 0;
                    while (index1 < count1 && points3[index1] != points2[index0])
                    {
                        ++index1;
                    }
                    index1 %= count1;
                    result.emplace_back(points3[index1++]);
                    ++index0;
                    index0 %= count0;
                    index1 %= count1;
                    break;
                }
                index0 %= count0;
            }

            while (result.size() < count2 && (result.size() < 4 || result.front() != result.back()))
            {
                if (points3[index1].value > -1)
                {
                    if (points3[index1].original)
                    {
                        points3[index1].active = false;
                    }
                    result.emplace_back(points3[index1++]);
                }
                else
                {
                    index0 = 0;
                    while (index0 < count0 && points2[index0] != points3[index1])
                    {
                        ++index0;
                    }
                    index0 %= count0;
                    result.emplace_back(points2[index0++]);
                    ++index1;
                    index0 %= count0;
                    index1 %= count1;
                    break;
                }
                index1 %= count1;
            }
        }

        for (size_t i = result.size() - 1; i > 1; --i)
        {
            if (Geo::is_inside(result[i - 1], result[i - 2], result[i]))
            {
                result.erase(result.begin() + i - 1);
            }
            else if (Geo::distance(result[i - 2], result[i]) < Geo::EPSILON)
            {
                result.erase(result.begin() + i--);
                result.erase(result.begin() + i);
            }
        }
        if (result.size() > 4)
        {
            if (Geo::is_inside(result.front(), result[1], result[result.size() - 2]))
            {
                result.pop_back();
                std::swap(result.front(), result.back());
                result.pop_back();
            }
            else if (Geo::distance(result[1], result[result.size() - 2]) < Geo::EPSILON)
            {
                result.pop_back();
                result.pop_back();
                result.erase(result.begin());
            }
        }
        output.back().append(result.begin(), result.end());

        it0 = points2.begin();
        while (it0 != points2.end())
        {
            if (!it0->active || std::find(result.begin(), result.end(), *it0) != result.end())
            {
                it0 = points2.erase(it0);
            }
            else
            {
                ++it0;
            }
        }
        it1 = points3.begin();
        while (it1 != points3.end())
        {
            if (!it1->active || std::find(result.begin(), result.end(), *it1) != result.end())
            {
                it1 = points3.erase(it1);
            }
            else
            {
                ++it1;
            }
        }

        if (output.back().area() == 0)
        {
            output.pop_back();
        }

        if (std::count_if(points2.cbegin(), points2.cend(), [](const MarkedPoint &p){ return p.value < 0; }) == 0 ||
            std::count_if(points3.cbegin(), points3.cend(), [](const MarkedPoint &p){ return p.value < 0; }) == 0)
        {
            break;
        }

        count0 = points2.size();
        count1 = points3.size();
        count2 = count0 + count1;
        result.clear();
    }

    return !output.empty();
}

bool Geo::merge_ear_cut_triangles(const std::vector<Geo::Triangle> &triangles, std::vector<Geo::Polygon> &polygons)
{
    if (triangles.empty())
    {
        return false;
    }
    
    polygons.clear();
    const size_t triangles_count = triangles.size();
    size_t merged_count = 1, index = 0;
    int index0, index1, index2;
    std::vector<bool> merged(triangles_count, false), current_triangles(triangles_count, false);
    Geo::Polygon points;
    bool flag;

    while (merged_count < triangles_count)
    {
        while (index < triangles_count && merged[index])
        {
            ++index;
        }
        if (index == triangles_count)
        {
            break;
        }

        points.append(triangles[index][0]);
        points.append(triangles[index][1]);
        points.append(triangles[index][2]);
        merged[index] = true;
        current_triangles.assign(triangles_count, false);
        current_triangles[index++] = true;

        flag = true;
        for (size_t i = index; i < triangles_count; ++i)
        {
            if (merged[i])
            {
                continue;
            }

            for (size_t j = 1, count = points.size(); j < count; ++j)
            {
                index0 = index1 = index2 = -1;
                if (points[j - 1] == triangles[i][0])
                {
                    index0 = 0;
                }
                else if (points[j - 1] == triangles[i][1])
                {
                    index0 = 1;
                }
                else if (points[j - 1] == triangles[i][2])
                {
                    index0 = 2;
                }

                if (points[j] == triangles[i][0])
                {
                    index1 = 0;
                }
                else if (points[j] == triangles[i][1])
                {
                    index1 = 1;
                }
                else if (points[j] == triangles[i][2])
                {
                    index1 = 2;
                }

                if (index0 == -1 || index1 == -1)
                {
                    continue;
                }
                index2 = 3 - index0 - index1;

                flag = true;
                for (size_t k = 0; k < triangles_count; ++k)
                {
                    if (current_triangles[k] && Geo::is_inside(triangles[i][index2], triangles[k]))
                    {
                        flag = false;
                        break;
                    }
                }

                if (flag)
                {
                    points.insert(j, triangles[i][index2]);
                    ++merged_count;
                    merged[i] = true;
                    current_triangles[i] = true;
                    break;
                }
            }
        }
        
        index = 0;
        polygons.emplace_back(points);
        points.clear();
    }

    return !polygons.empty();
}


void Geo::down_sampling(Geo::Polyline &points, const double distance)
{
    if (points.size() <= 2 || distance <= 0)
    {
        return;
    }
	std::vector<bool> mask(points.size(), true);
	std::vector<std::tuple<size_t, size_t>> stack;
	mask.front() = mask.back() = false;
	stack.emplace_back(0, mask.size() - 1);
	while (!stack.empty())
	{
		const auto [index0, index1] = stack.back();
		stack.pop_back();
		double maxDistance = -1;
		size_t index = index0;
		for (size_t i = index0 + 1; i < index1; ++i)
		{
            if (const double currentDistance = Geo::distance(points[i], points[index0], points[index1], false);
                currentDistance > distance && maxDistance < currentDistance)
            {
                maxDistance = currentDistance;
                index = i;
            }
		}
		if (index > index0)
		{
			mask[index] = false;
			stack.emplace_back(index0, index);
			stack.emplace_back(index, index1);
		}
	}

	for (size_t i = points.size() - 2; i > 0; --i)
	{
		if (mask[i])
		{
			points.remove(i);
		}
	}
}