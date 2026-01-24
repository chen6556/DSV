#include <algorithm>
#include "base/Algorithm.hpp"


bool Geo::split(const Polyline &polyline, const Point &pos, Polyline &output0, Polyline &output1)
{
    if (pos == polyline.front() || pos == polyline.back())
    {
        return false;
    }

    output0.clear(), output1.clear();
    for (size_t i = 1, count = polyline.size(); i < count; ++i)
    {
        if (polyline[i - 1] != polyline[i] && Geo::is_inside(pos, polyline[i - 1], polyline[i], false))
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

bool Geo::split(const CubicBezier &bezier, const Point &pos, CubicBezier &output0, CubicBezier &output1)
{
    if (pos == bezier.front() || pos == bezier.back())
    {
        return false;
    }

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
        Geo::closest_point(polyline, pos, points);
        temp.emplace_back(i, points, Geo::distance(pos, points.front()));
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
                for (int j = 0; j <= order; ++j)
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
        } while (std::abs(min_dis[0] - min_dis[1]) > 1e-4 && step > 1e-12);

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
                for (int j = 0; j <= order; ++j)
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
            for (int j = 0; j <= order; ++j)
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
    for (int i = 0; i <= order; ++i)
    {
        control_points.emplace_back(bezier[result_i + i]);
    }
    std::vector<Geo::Point> temp_points, result_points0, result_points1;
    for (int k = 0; k < order; ++k)
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
    output0.update_shape(Geo::CubicBezier::default_step, Geo::CubicBezier::default_down_sampling_value);
    output1.update_shape(Geo::CubicBezier::default_step, Geo::CubicBezier::default_down_sampling_value);
    return output0.size() > 1 && output1.size() > 1;
}

bool Geo::split(const CubicBezier &bezier, const size_t i, const double t, CubicBezier &output0, CubicBezier &output1)
{
    const int order = 3;
    if ((i == 0 && t == 0) || (i == (bezier.size() / (order + 1) - 1) && t == 1))
    {
        return false;
    }

    const int nums[4] = {1, 3, 3, 1};
    output0.clear(), output1.clear();
    std::vector<Geo::Point> control_points;
    Geo::Point pos;
    for (int j = 0; j <= order; ++j)
    {
        control_points.emplace_back(bezier[j + i]);
        pos += (control_points.back() * (nums[j] * std::pow(1 - t, order - j) * std::pow(t, j)));
    }
    std::vector<Geo::Point> temp_points, result_points0, result_points1;
    for (int k = 0; k < order; ++k)
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
    output0.update_shape(Geo::CubicBezier::default_step, Geo::CubicBezier::default_down_sampling_value);
    output1.update_shape(Geo::CubicBezier::default_step, Geo::CubicBezier::default_down_sampling_value);
    return output0.size() > 1 && output1.size() > 1;
}

bool Geo::split(const BSpline &bspline, const bool is_cubic, const Point &pos, BSpline &output0, BSpline &output1)
{
    if (is_cubic &&
        (dynamic_cast<const Geo::CubicBSpline *>(&output0) == nullptr || dynamic_cast<const Geo::CubicBSpline *>(&output1) == nullptr))
    {
        return false;
    }
    else if (!is_cubic &&
             (dynamic_cast<const Geo::QuadBSpline *>(&output0) == nullptr || dynamic_cast<const Geo::QuadBSpline *>(&output1) == nullptr))
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

        std::sort(result.begin(), result.end(), [](const auto &a, const auto &b) { return std::get<0>(a) < std::get<0>(b); });
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
            double dev = (knots[i + p] - knots[i]);
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
    else if (is_cubic &&
             (dynamic_cast<const Geo::CubicBSpline *>(&output0) == nullptr || dynamic_cast<const Geo::CubicBSpline *>(&output1) == nullptr))
    {
        return false;
    }
    else if (!is_cubic &&
             (dynamic_cast<const Geo::QuadBSpline *>(&output0) == nullptr || dynamic_cast<const Geo::QuadBSpline *>(&output1) == nullptr))
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
            double dev = (knots[i + p] - knots[i]);
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
    if (Geo::distance(pos, arc) < Geo::EPSILON && Geo::distance(pos, arc.control_points[0]) > Geo::EPSILON &&
        Geo::distance(pos, arc.control_points[2]) > Geo::EPSILON)
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

bool Geo::split(const Ellipse &ellipse, const Point &pos, Ellipse &output0, Ellipse &output1)
{
    if (ellipse.is_arc() && Geo::distance(pos, ellipse) < Geo::EPSILON && Geo::distance(pos, ellipse.arc_point0()) > Geo::EPSILON &&
        Geo::distance(pos, ellipse.arc_point1()) > Geo::EPSILON)
    {
        const Geo::Point center(ellipse.center());
        const double angle0 = ellipse.arc_angle0(), angle1 = ellipse.arc_angle1();
        const double angle2 = Geo::rad_to_2PI(Geo::angle(center, pos)) - ellipse.angle();
        const double a = ellipse.lengtha(), b = ellipse.lengthb();
        output0 = Geo::Ellipse(center, a, b, angle0, angle2, false);
        output1 = Geo::Ellipse(center, a, b, angle2, angle1, false);
        return true;
    }
    else
    {
        return false;
    }
}
