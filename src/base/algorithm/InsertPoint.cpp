#include <algorithm>
#include "base/Algorithm.hpp"
#include "base/Math.hpp"


bool Geo::insert_point(Geo::Polyline &polyline, const size_t i, const double t)
{
    if (i + 1 >= polyline.size())
    {
        return false;
    }
    polyline.insert(i + 1, polyline.shape_point(i, t));
    return true;
}

bool Geo::insert_point(Geo::Polygon &polygon, const size_t i, const double t)
{
    if (i + 1 >= polygon.size())
    {
        return false;
    }
    polygon.insert(i + 1, polygon.shape_point(i, t));
    return true;
}

bool Geo::insert_point(Geo::CubicBezier &bezier, const size_t i, const double t)
{
    const int order = 3;
    if (i >= bezier.control_points.size() / order || t <= 0 || t >= 1)
    {
        return false;
    }

    const int nums[4] = {1, 3, 3, 1};
    std::vector<Geo::Point> control_points, result_points0, result_points1;
    for (int j = 0; j <= order; ++j)
    {
        control_points.emplace_back(bezier.control_points[j + i * order]);
    }

    for (int k = 0; k < order; ++k)
    {
        std::vector<Geo::Point> temp_points;
        for (size_t j = 1, count = control_points.size(); j < count; ++j)
        {
            temp_points.emplace_back(control_points[j - 1] + (control_points[j] - control_points[j - 1]) * t);
        }
        result_points0.emplace_back(temp_points.front());
        result_points1.emplace_back(temp_points.back());
        control_points.assign(temp_points.begin(), temp_points.end());
    }
    result_points0.insert(result_points0.end(), result_points1.rbegin() + 1, result_points1.rend());

    bezier.control_points.erase(bezier.control_points.begin() + i * order + 1, bezier.control_points.begin() + i * order + 3);
    bezier.control_points.insert(bezier.control_points.begin() + i * order + 1, result_points0.begin(), result_points0.end());
    bezier.update_shape(Geo::CubicBezier::default_step, Geo::CubicBezier::default_down_sampling_value);
    return true;
}

bool Geo::insert_point(Geo::Polyline &polyline, const Geo::Point &point)
{
    size_t min_index = 1;
    double min_dis = Geo::distance(point, polyline[0], polyline[1], false);
    for (size_t i = 2, count = polyline.size(); i < count; ++i)
    {
        if (polyline[i - 1] == polyline[i])
        {
            continue;
        }
        if (double dis = Geo::distance(point, polyline[i - 1], polyline[i], false); dis < min_dis)
        {
            min_index = i;
            min_dis = dis;
        }
    }
    if (Geo::Point foot; Geo::foot_point(polyline[min_index - 1], polyline[min_index], point, foot, false))
    {
        polyline.insert(min_index, foot);
    }
    else
    {
        foot = Geo::distance(polyline[min_index - 1], point) <= Geo::distance(polyline[min_index], point) ? polyline[min_index - 1] : polyline[min_index];
        polyline.insert(min_index, foot);
    }
    return true;
}

bool Geo::insert_point(Geo::Polygon &polygon, const Geo::Point &point)
{
    size_t min_index = 1;
    double min_dis = Geo::distance(point, polygon[0], polygon[1], false);
    for (size_t i = 2, count = polygon.size(); i < count; ++i)
    {
        if (polygon[i - 1] == polygon[i])
        {
            continue;
        }
        if (double dis = Geo::distance(point, polygon[i - 1], polygon[i], false); dis < min_dis)
        {
            min_index = i;
            min_dis = dis;
        }
    }
    if (Geo::Point foot; Geo::foot_point(polygon[min_index - 1], polygon[min_index], point, foot, false))
    {
        polygon.insert(min_index, foot);
    }
    else
    {
        foot = Geo::distance(polygon[min_index - 1], point) <= Geo::distance(polygon[min_index], point) ? polygon[min_index - 1] : polygon[min_index];
        polygon.insert(min_index, foot);
    }
    return true;
}