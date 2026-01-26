#include <future>
#include <algorithm>
#include <clipper2/clipper.h>
#include "base/Algorithm.hpp"


bool Geo::offset(const Geo::Polyline &input, Geo::Polyline &result, const double distance)
{
    if (distance != 0)
    {
        Polyline temp(input);
        temp.remove_repeated_points();
        result.clear();
        double area = 0;
        for (size_t i = 1, count = temp.size(); i < count; ++i)
        {
            area += (temp[i].x * (temp[i + 1 != count ? i + 1 : 0].y - temp[i - 1].y));
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
    temp.remove_repeated_points();
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
        for (size_t i = 0, edge_count = error_edges.size(), count = result.size(), of = 0, j = 0; i < edge_count; ++i)
        {
            if (error_edges[i])
            {
                j = i - of++;
                if (error_edges[(i + 1) % edge_count])
                {
                    b = (temp.next_point(i) - temp[i]).vertical().normalize() * distance;
                    a.x = a.y = std::numeric_limits<double>::infinity();
                    Geo::is_intersected(result.last_point(j), result[j], temp[i] + b, temp.next_point(i) + b, a, true);
                    if (!std::isinf(a.x) && !std::isinf(a.y))
                    {
                        result[j] = a;
                    }
                    a.x = a.y = std::numeric_limits<double>::infinity();
                    Geo::is_intersected(result.next_point(result.next_point_index(j)),
                                        result.next_point(result.next_point_index(result.next_point_index(j))), temp[i] + b,
                                        temp.next_point(i) + b, a, true);
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
                    if (Geo::is_intersected(result.last_point(j), result[j], result.next_point(j),
                                            result.next_point(result.next_point_index(j)), a, true))
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
                                            result.next_point(result.next_point_index(result.next_point_index(j))), temp[i] + b,
                                            temp.last_point(i) + b, a, true);
                        b = (temp.next_point(temp.next_point_index(i)) - temp.next_point(i)).vertical().normalize() * distance;
                        Geo::is_intersected(result.last_point(j), result.last_point(result.last_point_index(j)), temp.next_point(i) + b,
                                            temp.next_point(temp.next_point_index(i)) + b, b, true);

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
        for (size_t i = 0, edge_count = error_edges.size(), count = result.size(), of = 0, j = 0; i < edge_count; ++i)
        {
            if (error_edges[i])
            {
                j = i - of++;
                if (error_edges[(i + 1) % edge_count])
                {
                    b = (temp.next_point(i) - temp[i]).vertical().normalize() * distance;
                    a.x = a.y = std::numeric_limits<double>::infinity();
                    Geo::is_intersected(result.last_point(j), result[j], temp[i] + b, temp.next_point(i) + b, a, true);
                    if (!std::isinf(a.x) && !std::isinf(a.y))
                    {
                        result[j] = a;
                    }
                    a.x = a.y = std::numeric_limits<double>::infinity();
                    Geo::is_intersected(result.next_point(result.next_point_index(j)),
                                        result.next_point(result.next_point_index(result.next_point_index(j))), temp[i] + b,
                                        temp.next_point(i) + b, a, true);
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
                    if (Geo::is_intersected(result.last_point(j), result[j], result.next_point(j),
                                            result.next_point(result.next_point_index(j)), a, true))
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
                                            result.next_point(result.next_point_index(result.next_point_index(j))), temp[i] + b,
                                            temp.last_point(i) + b, a, true);
                        b = (temp.next_point(temp.next_point_index(i)) - temp.next_point(i)).vertical().normalize() * distance;
                        Geo::is_intersected(result.last_point(j), result.last_point(result.last_point_index(j)), temp.next_point(i) + b,
                                            temp.next_point(temp.next_point_index(i)) + b, b, true);

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
        if (std::isnan(result[i].x) || std::isnan(result[i].y) || std::isinf(result[i].x) || std::isinf(result[i].y))
        {
            result.remove(i--);
            --count;
        }
    }
    result.back() = result.front();

    for (size_t i = 0, count = result.size() - 1; i < count; ++i)
    {
        if (Geo::is_inside(result[i], result[i + 1], result.next_point(i + 1)) &&
            Geo::is_inside(result[i + 1], result[i], result.last_point(i)))
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

bool Geo::offset(const Geo::Polygon &input, std::vector<Geo::Polygon> &result, const double distance, const Offset::JoinType join_type,
                 const Offset::EndType end_type, const double epsilon)
{
    if (distance == 0)
    {
        result.emplace_back(input);
        return true;
    }

    std::vector<int64_t> values;
    Geo::Polygon polygon0(input);
    polygon0.remove_repeated_points();
    for (const Geo::Point &point : polygon0)
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


namespace
{
    double bezier_offset_error(const double distance, const Geo::Point &point0, const Geo::Point &point1, const Geo::CubicBezier &bezier)
    {
        if (std::vector<Geo::Point> intersections; Geo::is_intersected(point0, point1, bezier, intersections, false) > 0)
        {
            double min_err = DBL_MAX;
            for (const Geo::Point &point : intersections)
            {
                if (const double err = std::abs(Geo::distance(point, point0) - std::abs(distance)) / std::abs(distance);
                    err < min_err)
                {
                    min_err = err;
                }
            }
            return min_err;
        }
        else
        {
            return -1;
        }
    }
}


bool Geo::offset(const Geo::CubicBezier &bezier, std::vector<Geo::CubicBezier> &result, const double distance, const double tolerance,
                 const int sample_count)
{
    result.clear();
    std::vector<size_t> split_indexs;
    std::vector<Geo::Point> points(bezier.rbegin(), bezier.rend()), cache, output;
    while (points.size() > 3)
    {
        cache.clear();
        for (int i = 0; i < 3; ++i)
        {
            cache.emplace_back(points.back());
            points.pop_back();
        }
        cache.emplace_back(points.back());
        const Geo::CubicBezier shape0(cache.begin(), cache.end(), false);

        Geo::Point anchor(shape0.shape_point(0, 0.5));
        Geo::Point anchor_offset = anchor + shape0.vertical(0, 0.5).normalize() * distance;

        const Geo::Point vec0(Geo::distance(cache[0], cache[1]) > Geo::EPSILON ? (cache[1] - cache[0]).normalize()
                                                                                : (cache[2] - cache[0]).normalize());
        const Geo::Point vec1(Geo::distance(cache[2], cache[3]) > Geo::EPSILON ? (cache[3] - cache[2]).normalize()
                                                                                : (cache[3] - cache[1]).normalize());
        Geo::Point temp[4];
        temp[0] = vec0.vertical() * distance + cache[0];
        temp[1] = vec0.vertical() * distance + cache[1];
        temp[2] = vec1.vertical() * distance + cache[2];
        temp[3] = vec1.vertical() * distance + cache[3];
        if (const double delta = vec0.x * vec1.y - vec0.y * vec1.x; delta == 0)
        {
            if (output.empty() || (!split_indexs.empty() && output.size() == split_indexs.back() + 1))
            {
                output.emplace_back(temp[0]);
            }
            for (int i = 1; i <= 3; ++i)
            {
                output.emplace_back(temp[i]);
            }
        }
        else
        {
            const double a = std::abs(((8 * anchor_offset.x - 4 * cache[0].x - 4 * cache[3].x) * vec1.y -
                                        (8 * anchor_offset.y - 4 * cache[0].y - 4 * cache[3].y) * vec1.x) /
                                        delta);
            const double b = std::abs(((8 * anchor_offset.x - 4 * cache[0].x - 4 * cache[3].x) * vec0.y -
                                        (8 * anchor_offset.y - 4 * cache[0].y - 4 * cache[3].y) * vec0.x) /
                                        delta);
            if (Geo::Point mid; Geo::is_intersected(temp[0], temp[1], temp[2], temp[3], mid, false))
            {
                temp[1] = temp[0] + vec0 * std::min({a / 3, Geo::distance(temp[0], mid), Geo::distance(cache[0], cache[1])});
                temp[2] = temp[3] - vec1 * std::min({b / 3, Geo::distance(temp[3], mid), Geo::distance(cache[2], cache[3])});
            }
            else
            {
                temp[1] = temp[0] + vec0 * std::min(a / 3, Geo::distance(cache[0], cache[1]));
                temp[2] = temp[3] - vec1 * std::min(b / 3, Geo::distance(cache[2], cache[3]));
            }
            const Geo::CubicBezier shape1({temp[0], temp[1], temp[2], temp[3]}, false);

            double max_err = 0;
            std::vector<std::future<double>> futures;
            for (int i = 1; i < sample_count; ++i)
            {
                const double t = static_cast<double>(i) / static_cast<double>(sample_count);
                anchor = shape0.shape_point(0, t);
                anchor_offset = anchor + shape0.vertical(0, t).normalize() * distance * 1.5;
                futures.emplace_back(std::async(std::launch::async, bezier_offset_error, distance, anchor, anchor_offset, std::cref(shape1)));
            }
            for (std::future<double> &f : futures)
            {
                f.wait();
                if (const double err = f.get(); err > max_err)
                {
                    max_err = err;
                }
            }
            if (Geo::CubicBezier shape2, shape3; max_err >= tolerance && Geo::split(shape0, 0, 0.5, shape2, shape3))
            {
                if (points.back() == shape3.back())
                {
                    for (int i = 2; i >= 0; --i)
                    {
                        points.emplace_back(shape3[i]);
                    }
                    for (int i = 2; i >= 0; --i)
                    {
                        points.emplace_back(shape2[i]);
                    }
                }
                else
                {
                    for (int i = 2; i >= 0; --i)
                    {
                        points.emplace_back(shape2[i]);
                    }
                    for (int i = 2; i >= 0; --i)
                    {
                        points.emplace_back(shape3[i]);
                    }
                }
            }
            else
            {
                if ((temp[3] - temp[0]) * (cache[3] - cache[0]) > 0)
                {
                    if (output.empty() || (!split_indexs.empty() && output.size() == split_indexs.back() + 1))
                    {
                        output.emplace_back(temp[0]);
                    }
                    for (int i = 1; i <= 3; ++i)
                    {
                        output.emplace_back(temp[i]);
                    }
                }
                else if (!output.empty())
                {
                    if (split_indexs.empty() || split_indexs.back() < output.size() - 1)
                    {
                        split_indexs.push_back(output.size() - 1);
                    }
                }
            }
        }
    }

    std::reverse(split_indexs.begin(), split_indexs.end());
    for (size_t i = 0, count = output.size(); i < count; ++i)
    {
        if (split_indexs.empty())
        {
            result.emplace_back(output.begin() + i, output.end(), false);
            if (result.size() > 1)
            {
                if (std::vector<std::tuple<size_t, double, double, double>> tvalue0, tvalue1;
                    Geo::is_intersected(result.back(), result[result.size() - 2], cache, &tvalue0, &tvalue1))
                {
                    if (output[i] == result.back().front())
                    {
                        if (Geo::CubicBezier shape0, shape1;
                            Geo::split(result.back(), std::get<0>(tvalue0.front()), std::get<1>(tvalue0.front()), shape0, shape1))
                        {
                            result[result.size() - 1] = shape1;
                        }
                        if (Geo::CubicBezier shape0, shape1; Geo::split(result[result.size() - 2], std::get<0>(tvalue1.front()),
                                                                        std::get<1>(tvalue1.front()), shape0, shape1))
                        {
                            result[result.size() - 2] = shape0;
                        }
                    }
                    else
                    {
                        if (Geo::CubicBezier shape0, shape1;
                            Geo::split(result.back(), std::get<0>(tvalue0.back()), std::get<1>(tvalue0.back()), shape0, shape1))
                        {
                            result[result.size() - 1] = shape1;
                        }
                        if (Geo::CubicBezier shape0, shape1; Geo::split(result[result.size() - 2], std::get<0>(tvalue1.back()),
                                                                        std::get<1>(tvalue1.back()), shape0, shape1))
                        {
                            result[result.size() - 2] = shape0;
                        }
                    }
                }
            }
            break;
        }
        else
        {
            result.emplace_back(output.begin() + i, output.begin() + split_indexs.back() + 1, false);
            i = split_indexs.back();
            split_indexs.pop_back();

            if (result.size() > 1)
            {
                if (std::vector<std::tuple<size_t, double, double, double>> tvalue0, tvalue1;
                    Geo::is_intersected(result.back(), result[result.size() - 2], cache, &tvalue0, &tvalue1))
                {
                    if (output[i] == result.back().front())
                    {
                        if (Geo::CubicBezier shape0, shape1;
                            Geo::split(result.back(), std::get<0>(tvalue0.front()), std::get<1>(tvalue0.front()), shape0, shape1))
                        {
                            result[result.size() - 1] = shape1;
                        }
                        if (Geo::CubicBezier shape0, shape1; Geo::split(result[result.size() - 2], std::get<0>(tvalue1.front()),
                                                                        std::get<1>(tvalue1.front()), shape0, shape1))
                        {
                            result[result.size() - 2] = shape0;
                        }
                    }
                    else
                    {
                        if (Geo::CubicBezier shape0, shape1;
                            Geo::split(result.back(), std::get<0>(tvalue0.back()), std::get<1>(tvalue0.back()), shape0, shape1))
                        {
                            result[result.size() - 1] = shape1;
                        }
                        if (Geo::CubicBezier shape0, shape1; Geo::split(result[result.size() - 2], std::get<0>(tvalue1.back()),
                                                                        std::get<1>(tvalue1.back()), shape0, shape1))
                        {
                            result[result.size() - 2] = shape0;
                        }
                    }
                }
            }
        }
    }

    return !result.empty();
}
