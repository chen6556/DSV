#include <algorithm>
#include "base/Algorithm.hpp"
#include "base/Math.hpp"


int Geo::closest_point(const Polyline &polyline, const Point &point, std::vector<Point> &output)
{
    std::vector<size_t> min_indexs({1});
    double min_dis = Geo::distance(point, polyline[0], polyline[1], false);
    for (size_t i = 2, count = polyline.size(); i < count; ++i)
    {
        if (polyline[i - 1] == polyline[i])
        {
            continue;
        }
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
        if (polygon[i - 1] == polygon[i])
        {
            continue;
        }
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
            double angles[4] = {Geo::angle(start, center, ellipse.a0()), Geo::angle(start, center, ellipse.a1()),
                                Geo::angle(start, center, ellipse.b0()), Geo::angle(start, center, ellipse.b1())};
            bool mask[4] = {false, false, false, false}; // a0, a1, b0, b1
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
                if (ellipse.lengtha() < std::min(Geo::distance(point, ellipse.arc_point0()), Geo::distance(point, ellipse.arc_point1())))
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
                if (ellipse.lengthb(), std::min(Geo::distance(point, ellipse.arc_point0()), Geo::distance(point, ellipse.arc_point1())))
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
        double degree0 = Geo::angle(Geo::Point(0, 0), coord) - Geo::PI / 2, degree1 = Geo::angle(Geo::Point(0, 0), coord) + Geo::PI / 2;
        double last_degree0 = degree0 - 1, last_degree1 = degree1 - 1;
        double m0, m1, x0 = 0, y0 = 0, x1 = 0, y1 = 0;
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
    }
}

int Geo::closest_point(const CubicBezier &bezier, const Point &point, std::vector<Point> &output,
                       std::vector<std::tuple<size_t, double, double, double>> *tvalues)
{
    const int order = 3;
    const int nums[4] = {1, 3, 3, 1};
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
            for (int j = 0; j <= order; ++j)
            {
                point += (bezier[j + i] * (nums[j] * std::pow(1 - t, order - j) * std::pow(t, j)));
            }
            polyline.append(point);
            t += Geo::CubicBezier::default_step;
        }
        polyline.append(bezier[i + order]);
        Geo::down_sampling(polyline, Geo::CubicBezier::default_down_sampling_value);

        std::vector<Geo::Point> points;
        Geo::closest_point(polyline, point, points);
        temp.emplace_back(i, points, Geo::distance(point, points.front()));
    }
    std::sort(temp.begin(), temp.end(), [](const auto &a, const auto &b) { return std::get<2>(a) < std::get<2>(b); });
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

    std::vector<std::tuple<size_t, double, Geo::Point>> result;
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
                for (int j = 0; j <= order; ++j)
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
        } while (std::abs(min_dis[0] - min_dis[1]) > 1e-4 && step > 1e-12);

        lower = std::max(0.0, t - 0.1), upper = std::min(1.0, t + 0.1);
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
                for (int j = 0; j <= order; ++j)
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
        for (int j = 0; j <= order; ++j)
        {
            coord += (bezier[j + i] * (nums[j] * std::pow(1 - t, order - j) * std::pow(t, j)));
        }
        result.emplace_back(i, t, coord);
    }

    std::sort(result.begin(), result.end(),
              [&](const auto &a, const auto &b) { return Geo::distance(std::get<2>(a), point) < Geo::distance(std::get<2>(b), point); });
    while (result.size() > 1)
    {
        if (Geo::distance(point, std::get<2>(result.back())) > Geo::distance(point, std::get<2>(result.front())))
        {
            result.pop_back();
        }
        else
        {
            break;
        }
    }

    if (tvalues == nullptr)
    {
        for (const auto &[i, t, point] : result)
        {
            output.emplace_back(point);
        }
    }
    else
    {
        for (const auto &[i, t, point] : result)
        {
            output.emplace_back(point);
            tvalues->emplace_back(i, t, point.x, point.y);
        }
    }
    return result.size();
}

int Geo::closest_point(const BSpline &bspline, const bool is_cubic, const Point &point, std::vector<Point> &output,
                       std::vector<std::tuple<double, double, double>> *tvalues)
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

    std::vector<std::tuple<double, double, Geo::Point>> result; // distance, t, point
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
        } while (std::abs(min_dis[0] - min_dis[1]) > 1e-4 && step > 1e-12);

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
        result.emplace_back(std::min(min_dis[0], min_dis[1]), v, coord);
    }

    std::sort(result.begin(), result.end(), [](const auto &a, const auto &b) { return std::get<0>(a) < std::get<0>(b); });
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
    if (tvalues == nullptr)
    {
        for (const auto &[dis, t, coord] : result)
        {
            output.emplace_back(coord);
        }
    }
    else
    {
        for (const auto &[dis, t, coord] : result)
        {
            output.emplace_back(coord);
            tvalues->emplace_back(t, coord.x, coord.y);
        }
    }
    return result.size();
}
