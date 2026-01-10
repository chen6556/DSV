#include <algorithm>
#include "base/Algorithm.hpp"


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
    if (order == 2)
    {
        nums[1] = 2;
    }
    else
    {
        nums[1] = nums[2] = 3;
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
