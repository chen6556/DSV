#include <algorithm>
#include <clipper2/clipper.h>
#include "base/Algorithm.hpp"


bool Geo::offset(const Geo::Polyline &input, Geo::Polyline &result, const double distance)
{
    if (distance != 0)
    {
        Polyline temp(input);
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
