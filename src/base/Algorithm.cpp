#include <algorithm>
#include "base/Algorithm.hpp"


bool Geo::is_parallel(const Point &point0, const Point &point1, const Point &point2, const Point &point3)
{
    return ((point0.y - point1.y) * (point2.x - point3.x)) == ((point2.y - point3.y) * (point0.x - point1.x));
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
            const bool result0 = (start1.y < start0.y && start0.y < end1.y) || (end1.y < start0.y && start0.y < start1.y);
            const bool result1 = (start1.y < end0.y && end0.y < end1.y) || (end1.y < end0.y && end0.y < start1.y);
            const bool result2 = (start0.y < start1.y && start1.y < end0.y) || (end0.y < start1.y && start1.y < start0.y);
            const bool result3 = (start0.y < end1.y && end1.y < end0.y) || (end0.y < end1.y && end1.y < start0.y);
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
            const bool result0 = (start1.x < start0.x && start0.x < end1.x) || (end1.x < start0.x && start0.x < start1.x);
            const bool result1 = (start1.x < end0.x && end0.x < end1.x) || (end1.x < end0.x && end0.x < start1.x);
            const bool result2 = (start0.x < start1.x && start1.x < end0.x) || (end0.x < start1.x && start1.x < start0.x);
            const bool result3 = (start0.x < end1.x && end1.x < end0.x) || (end0.x < end1.x && end1.x < start0.x);
            return result0 || result1 || result2 || result3;
        }
        else
        {
            return false;
        }
    }

    const double a0 = end0.y - start0.y, b0 = start0.x - end0.x, c0 = end0.x * start0.y - start0.x * end0.y;
    const double a1 = end1.y - start1.y, b1 = start1.x - end1.x, c1 = end1.x * start1.y - start1.x * end1.y;
    if (std::abs(a0 * b1 - a1 * b0) < Geo::EPSILON && std::abs(a0 * c1 - a1 * c0) < Geo::EPSILON &&
        std::abs(b0 * c1 - b1 * c0) < Geo::EPSILON)
    {
        return Geo::distance((start0 + end0) / 2, (start1 + end1) / 2) * 2 < Geo::distance(start0, end0) + Geo::distance(start1, end1);
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
        return Geo::is_coincide(start, end, start, polygon.last_point(index0)) ||
               Geo::is_coincide(start, end, start, polygon.next_point(index0));
    }
    else if (index1 < SIZE_MAX)
    {
        return Geo::is_coincide(start, end, end, polygon.last_point(index1)) ||
               Geo::is_coincide(start, end, end, polygon.next_point(index1));
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

bool Geo::is_on_left(const Point &point, const Point &start, const Point &end)
{
    return (end.x - start.x) * (point.y - start.y) - (end.y - start.y) * (point.x - start.x) > 0;
}


bool Geo::is_point_on(const Point &point, const Triangle &triangle)
{
    return (
        (Geo::cross(triangle[1] - triangle[0], point - triangle[0]) == 0 &&
         Geo::distance(point, triangle[0]) + Geo::distance(point, triangle[1]) < Geo::distance(triangle[0], triangle[1]) + Geo::EPSILON) ||
        (Geo::cross(triangle[2] - triangle[1], point - triangle[1]) == 0 &&
         Geo::distance(point, triangle[1]) + Geo::distance(point, triangle[2]) < Geo::distance(triangle[1], triangle[2]) + Geo::EPSILON) ||
        (Geo::cross(triangle[2] - triangle[0], point - triangle[0]) == 0) &&
            Geo::distance(point, triangle[0]) + Geo::distance(point, triangle[2]) < Geo::distance(triangle[0], triangle[2]) + Geo::EPSILON);
}

bool Geo::is_point_on(const Geo::Point &point, const std::vector<Geo::Point>::const_iterator &begin,
                      const std::vector<Geo::Point>::const_iterator &end)
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


void Geo::down_sampling(Geo::Polyline &points, const double distance)
{
    points.remove_repeated_points();
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


void Geo::remove_repeated_point(std::vector<Geo::Point> &points)
{
    for (size_t i = points.size() - 1; i > 0; --i)
    {
        if (points[i - 1] == points[i])
        {
            points.erase(points.begin() + i);
        }
    }
}