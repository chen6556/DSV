#include <cassert>
#include <algorithm>
#include <array>
#include <functional>

#include "base/EarCut/EarCut.hpp"
#include "base/Algorithm.hpp"
#include "base/Collision.hpp"


double Geo::distance(const double x0, const double y0, const double x1, const double y1)
{
    return std::sqrt((x0 - x1) * (x0 - x1) + (y0 - y1) * (y0 - y1));
}

double Geo::distance(const Point &point0, const Point &point1)
{
    return std::sqrt((point0.x - point1.x) * (point0.x - point1.x)
                        + (point0.y - point1.y) * (point0.y - point1.y));
}

double Geo::distance(const Point &point, const Line &line, const bool infinite)
{
    if (line.front().x == line.back().x)
    {
        if (infinite)
        {
            return std::abs(point.x - line.front().x);
        }
        else
        {
            if ((point.y >= line.front().y && point.y <= line.back().y) ||
                (point.y <= line.front().y && point.y >= line.back().y))
            {
                return std::abs(point.x - line.front().x);
            }
            else
            {
                return std::min(Geo::distance(point, line.front()), Geo::distance(point, line.back()));
            }
        }
    }
    else if (line.front().y == line.back().y)
    {
        if (infinite)
        {
            return std::abs(point.y - line.front().y);
        }
        else
        {
            if ((point.x >= line.front().x && point.x <= line.back().x) ||
                (point.x <= line.front().x && point.x >= line.back().x))
            {
                return std::abs(point.y - line.front().y);
            }
            else
            {
                return std::min(Geo::distance(point, line.front()), Geo::distance(point, line.back()));
            }
        }
    }
    
    const double a = line.back().y - line.front().y, 
                b = line.front().x - line.back().x,
                c = line.back().x * line.front().y - line.front().x * line.back().y;
    if (infinite)
    {
        return std::abs(a * point.x + b * point.y + c) / std::sqrt(a * a + b * b);
    }
    else
    {
        const double k = ((point.x - line.front().x) * (line.back().x - line.front().x) +
            (point.y - line.front().y) * (line.back().y - line.front().y)) /
            (std::pow(line.back().x - line.front().x, 2) + std::pow(line.back().y - line.front().y, 2)); 
        const double x = line.front().x + k * (line.back().x - line.front().x);

        if ((x >= line.front().x && x <= line.back().x) || (x <= line.front().x && x >= line.back().x))
        {
            return std::abs(a * point.x + b * point.y + c) / std::sqrt(a * a + b * b);
        }
        else
        {
            return std::min(Geo::distance(point, line.front()), Geo::distance(point, line.back()));
        }
    }
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
        return std::abs(a * point.x + b * point.y + c) / std::sqrt(a * a + b * b);
    }
    else
    {
        const double k = ((point.x - start.x) * (end.x - start.x) +
            (point.y - start.y) * (end.y - start.y)) /
            (std::pow(end.x - start.x, 2) + std::pow(end.y - start.y, 2)); 
        const double x = start.x + k * (end.x - start.x);

        if ((x >= start.x && x <= end.x) || (x <= start.x && x >= end.x))
        {
            return std::abs(a * point.x + b * point.y + c) / std::sqrt(a * a + b * b);
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
    const Geo::Point center = ellipse.center();
    const Geo::Point coord = Geo::to_coord(point, center.x, center.y, Geo::angle(ellipse.a0(), ellipse.a1()));
    const double a = ellipse.lengtha(), b = ellipse.lengthb();
    double degree0 = Geo::rad_to_2PI(Geo::angle(Geo::Point(0, 0), coord)) - Geo::PI / 6,
        degree1 = Geo::rad_to_2PI(Geo::angle(Geo::Point(0, 0), coord)) + Geo::PI / 6;
    if (degree0 > degree1)
    {
        std::swap(degree0, degree1);
    }
    if (coord.x >= 0)
    {
        if (coord.y >= 0)
        {
            if (degree0 < 0)
            {
                degree0 = 0;
            }
            if (degree1 > Geo::PI / 2)
            {
                degree1 = Geo::PI / 2;
            }
        }
        else
        {
            if (degree0 < Geo::PI * 1.5)
            {
                degree0 = Geo::PI * 1.5;
            }
            if (degree1 > Geo::PI * 2)
            {
                degree1 = Geo::PI * 2;
            }
        }
    }
    else
    {
        if (coord.y >= 0)
        {
            if (degree0 < Geo::PI / 2)
            {
                degree0 = Geo::PI / 2;
            }
            if (degree1 > Geo::PI)
            {
                degree1 = Geo::PI;
            }
        }
        else
        {
            if (degree0 < Geo::PI)
            {
                degree0 = Geo::PI;
            }
            if (degree1 > Geo::PI * 1.5)
            {
                degree1 = Geo::PI * 1.5;
            }
        }
    }
    double x0 = a * std::cos(degree0), y0 = b * std::sin(degree0);
    double x1 = a * std::cos(degree1), y1 = b * std::sin(degree1);
    while (std::abs(Geo::distance_square(x0, y0, coord.x, coord.y) - Geo::distance_square(x1, y1, coord.x, coord.y)) > Geo::EPSILON)
    {
        if (Geo::distance_square(x0, y0, coord.x, coord.y) > Geo::distance_square(x1, y1, coord.x, coord.y))
        {
            degree0 = (degree0 + degree1) / 2;
            x0 = a * std::cos(degree0);
            y0 = b * std::sin(degree0);
        }
        else
        {
            degree1 = (degree0 + degree1) / 2;
            x1 = a * std::cos(degree1);
            y1 = b * std::sin(degree1);
        }
    }
    return std::min(Geo::distance(x0, y0, coord.x, coord.y), Geo::distance(x1, y1, coord.x, coord.y));
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

double Geo::distance_square(const Point &point, const Line &line, const bool infinite)
{
    if (line.front().x == line.back().x)
    {
        if (infinite)
        {
            return std::pow(point.x - line.front().x, 2);
        }
        else
        {
            if ((point.y >= line.front().y && point.y <= line.back().y) ||
                (point.y <= line.front().y && point.y >= line.back().y))
            {
                return std::pow(point.x - line.front().x, 2);
            }
            else
            {
                return std::min(Geo::distance_square(point, line.front()), Geo::distance_square(point, line.back()));
            }
        }
    }
    else if (line.front().y == line.back().y)
    {
        if (infinite)
        {
            return std::pow(point.y - line.front().y, 2);
        }
        else
        {
            if ((point.x >= line.front().x && point.x <= line.back().x) ||
                (point.x <= line.front().x && point.x >= line.back().x))
            {
                return std::pow(point.y - line.front().y, 2);
            }
            else
            {
                return std::min(Geo::distance_square(point, line.front()), Geo::distance_square(point, line.back()));
            }
        }
    }
    
    const double a = line.back().y - line.front().y, 
                b = line.front().x - line.back().x,
                c = line.back().x * line.front().y - line.front().x * line.back().y;
    if (infinite)
    {
        return std::pow(a * point.x + b * point.y + c, 2) / (a * a + b * b);
    }
    else
    {
        const double k = ((point.x - line.front().x) * (line.back().x - line.front().x) +
            (point.y - line.front().y) * (line.back().y - line.front().y)) /
            (std::pow(line.back().x - line.front().x, 2) + std::pow(line.back().y - line.front().y, 2)); 
        const double x = line.front().x + k * (line.back().x - line.front().x);

        if ((x >= line.front().x && x <= line.back().x) || (x <= line.front().x && x >= line.back().x))
        {
            return std::pow(a * point.x + b * point.y + c, 2) / (a * a + b * b);
        }
        else
        {
            return std::min(Geo::distance_square(point, line.front()), Geo::distance_square(point, line.back()));
        }
    }
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


bool Geo::is_inside(const Geo::Point &point, const Geo::Line &line, const bool infinite)
{
    if (std::abs(Geo::cross(line.back() - line.front(), point - line.front())) < Geo::EPSILON)
    {
        return infinite || Geo::distance(point, line.front()) + Geo::distance(point, line.back()) < line.length() + Geo::EPSILON;
    }
    else
    {
        return false;
    }
}

bool Geo::is_inside(const Geo::Point &point, const Geo::Point &start, const Geo::Point &end, const bool infinite)
{
    if (std::abs(Geo::cross(end - start, point - start)) < Geo::EPSILON)
    {
        return infinite || Geo::distance(point, start) + Geo::distance(point, end) < Geo::distance(start, end) + Geo::EPSILON;
    }
    else
    {
        return false;
    }
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
        for (size_t count, j, i = points.size() - 1; i > 0; --i)
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

bool Geo::is_parallel(const Line &line0, const Line &line1)
{
    return Geo::is_parallel(line0.front(), line0.back(), line1.front(), line1.back());
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

bool Geo::is_part(const Line &line0, const Line &line1)
{
    return Geo::is_part(line0.front(), line0.back(), line1.front(), line1.back());
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

bool Geo::is_intersected(const Line &line0, const Line &line1, Point &output, const bool infinite)
{
    return Geo::is_intersected(line0.front(), line0.back(), line1.front(), line1.back(), output, infinite);
}

int Geo::is_intersected(const Point &point0, const Point &point1, const Circle &circle, Point &output0, Point &output1, const bool infinite)
{
    if (Geo::distance_square(circle, point0, point1, infinite) > std::pow(circle.radius, 2))
    {
        return 0;
    }
    Geo::Point foot;
    Geo::foot_point(point0, point1, circle, foot, true);
    const double l =  std::sqrt(std::pow(circle.radius, 2) - Geo::distance_square(circle, point0, point1, true));
    output0 = foot + (point0 - point1).normalize() * l;
    output1 = foot + (point1 - point0).normalize() * l;
    if (infinite)
    {
        return Geo::distance_square(circle, point0, point1, true) < std::pow(circle.radius, 2) ? 2 : 1;
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
        const double a = std::pow(a1, 2) * a0  + std::pow(b1, 2) * b0;
        if (b1 != 0)
        {
            const double b = 2 * a1 * a0 * c1;
            const double c = (std::pow(c1, 2) - std::pow(b1, 2) * b0) * a0;
            output0.x = (-b - std::sqrt(std::pow(b, 2) - 4 * a *c)) / (2 * a);
            output1.x = (-b + std::sqrt(std::pow(b, 2) - 4 * a *c)) / (2 * a);
            output0.y = (-a1 * output0.x - c1) / b1;
            output1.y = (-a1 * output1.x - c1) / b1;
        }
        else
        {
            const double b = 2 * b1 * b0 * c1;
            const double c = (std::pow(c1, 2) - std::pow(a1, 2) * a0) * b0;
            output0.y = (-b - std::sqrt(std::pow(b, 2) - 4 * a *c)) / (2 * a);
            output1.y = (-b + std::sqrt(std::pow(b, 2) - 4 * a *c)) / (2 * a);
            output0.x = (-b1 * output0.y - c1) / a1;
            output1.x = (-b1 * output1.y - c1) / a1;
        }
        if (infinite)
        {
            const Geo::Point coord = Geo::to_coord(Geo::Point(0, 0), center.x, center.y, angle);
            output0 = Geo::to_coord(point2, coord.x, coord.y, -angle);
            output1 = Geo::to_coord(point3, coord.x, coord.y, -angle);
            return 2;
        }
        else
        {
            if (Geo::is_inside(output0, point2, point3))
            {
                if (Geo::is_inside(output1, point2, point3))
                {
                    const Geo::Point coord = Geo::to_coord(Geo::Point(0, 0), center.x, center.y, angle);
                    output0 = Geo::to_coord(point2, coord.x, coord.y, -angle);
                    output1 = Geo::to_coord(point3, coord.x, coord.y, -angle);
                    return 2;
                }
                else
                {
                    const Geo::Point coord = Geo::to_coord(Geo::Point(0, 0), center.x, center.y, angle);
                    output0 = Geo::to_coord(point2, coord.x, coord.y, -angle);
                    return 1;
                }
            }
            else
            {
                if (Geo::is_inside(output1, point2, point3))
                {
                    output0 = output1;
                    const Geo::Point coord = Geo::to_coord(Geo::Point(0, 0), center.x, center.y, angle);
                    output0 = Geo::to_coord(point2, coord.x, coord.y, -angle);
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
        const double a = std::pow(a1, 2) * a0 + std::pow(b1, 2) * b0;
        if (b1 != 0)
        {
            const double b = 2 * a1 * a0 * c1;
            const double c = (std::pow(c1, 2) - std::pow(b1, 2) * b0) * a0;
            output0.x = -b / (2 * a);
            output0.y = (-a1 * output0.x - c1) / b1;
        }
        else
        {
            const double b = 2 * b1 * b0 * c1;
            const double c = (std::pow(c1, 2) - std::pow(a1, 2) * a0) * b0;
            output0.y = -b / (2 * a);
            output0.x = (-b1 * output0.y - c1) / a1;
        }
        if (infinite)
        {
            const Geo::Point coord = Geo::to_coord(Geo::Point(0, 0), center.x, center.y, angle);
            output0 = Geo::to_coord(point2, coord.x, coord.y, -angle);
            return 1;
        }
        else
        {
            if (Geo::is_inside(output0, point2, point3))
            {
                const Geo::Point coord = Geo::to_coord(Geo::Point(0, 0), center.x, center.y, angle);
                output0 = Geo::to_coord(point2, coord.x, coord.y, -angle);
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
            const double dx = point1.x - point0.x;
            const double dy = point1.y - point0.y;
            const bool b0 = (rect[0].x - point0.x) * dy >= (rect[0].y - point0.y) * dx;
            const bool b1 = (rect[1].x - point0.x) * dy >= (rect[1].y - point0.y) * dx;
            const bool b2 = (rect[2].x - point0.x) * dy >= (rect[2].y - point0.y) * dx;
            const bool b3 = (rect[3].x - point0.x) * dy >= (rect[3].y - point0.y) * dx;
            return !(b0 == b1 && b1 == b2 && b2 == b3);
        }
    }
}

bool Geo::is_intersected(const AABBRect &rect, const Line &line)
{
    return Geo::is_intersected(rect, line.front(), line.back());
}

bool Geo::is_intersected(const AABBRect &rect, const Polyline &polyline)
{
    if (polyline.empty() || !Geo::is_intersected(rect, polyline.bounding_rect()))
    {
        return false;
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
    
    for (size_t i = 1, count = polygon.size(); i < count; ++i)
    {
        if (Geo::is_intersected(rect, polygon[i - 1], polygon[i]))
        {
            return true;
        }
    }
    for (size_t i = 0; i < 4; ++i)
    {
        if (Geo::is_inside(rect[i], polygon))
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

    if (Geo::is_inside(circle, rect, true))
    {
        return true;
    }
    for (const Geo::Point &point : rect)
    {
        if (Geo::is_inside(point, circle, true))
        {
            return true;
        }
    }
    const double length = circle.radius * circle.radius;
    for (size_t i = 1; i < 5; ++i)
    {
        if (Geo::distance_square(circle, rect[i-1], rect[i]) <= length)
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

    if (Geo::is_inside(ellipse.center(), rect)
        || Geo::is_inside(ellipse.a0(), rect) || Geo::is_inside(ellipse.a1(), rect)
        || Geo::is_inside(ellipse.b0(), rect) || Geo::is_inside(ellipse.b1(), rect))
    {
        return true;
    }
    for (const Geo::Point &point : rect)
    {
        if (Geo::is_inside(point, ellipse, true))
        {
            return true;
        }
    }
    Geo::Point point0, point1;
    for (int i = 0; i < 4; ++i)
    {
        if (Geo::is_intersected(rect[i + 1], rect[i], ellipse, point0, point1))
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

bool Geo::is_intersected(const Line &line, const Triangle &triangle, Point &output0, Point &output1)
{
    return Geo::is_intersected(line.front(), line.back(), triangle, output0, output1);
}

bool Geo::is_intersected(const Geometry *object0, const Geometry *object1)
{
    return Geo::is_intersected(object0->bounding_rect(), object1->bounding_rect())
        && Geo::NoAABBTest::is_intersected(object0, object1);
}

bool Geo::is_intersected(const AABBRect &rect, const Geometry *object)
{
    return Geo::is_intersected(rect, object->bounding_rect()) && Geo::NoAABBTest::is_intersected(rect, object);
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
            else if (inside && Geo::is_inside(polyline[i-1], polygon))
            {
                return true;
            }
        }
    }
    if (inside)
    {
        return Geo::is_inside(polyline.back(), polygon);
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
            if (Geo::is_inside(point, polygon1, true))
            {
                return true;
            }
        }
        for (const Geo::Point &point : polygon1)
        {
            if (Geo::is_inside(point, polygon0, true))
            {
                return true;
            }
        }
    }
    return false;
}

bool Geo::NoAABBTest::is_intersected(const Geo::AABBRect &rect, const Geo::Polyline &polyline)
{
    for (size_t i = 1, count = polyline.size(); i < count; ++i)
    {
        if (Geo::is_intersected(rect, polyline[i - 1], polyline[i]))
        {
            return true;
        }
    }
    return false;
}

bool Geo::NoAABBTest::is_intersected(const Geo::AABBRect &rect, const Geo::Polygon &polygon)
{
    for (size_t i = 1, count = polygon.size(); i < count; ++i)
    {
        if (Geo::is_intersected(rect, polygon[i - 1], polygon[i]))
        {
            return true;
        }
    }
    for (size_t i = 0; i < 4; ++i)
    {
        if (Geo::is_inside(rect[i], polygon))
        {
            return true;
        }
    }
    return false;
}

bool Geo::NoAABBTest::is_intersected(const Geo::AABBRect &rect, const Geo::Ellipse &ellipse)
{
    if (Geo::is_inside(ellipse.center(), rect)
        || Geo::is_inside(ellipse.a0(), rect) || Geo::is_inside(ellipse.a1(), rect)
        || Geo::is_inside(ellipse.b0(), rect) || Geo::is_inside(ellipse.b1(), rect))
    {
        return true;
    }
    for (const Geo::Point &point : rect)
    {
        if (Geo::is_inside(point, ellipse, true))
        {
            return true;
        }
    }
    Geo::Point point0, point1;
    for (int i = 0; i < 4; ++i)
    {
        if (Geo::is_intersected(rect[i + 1], rect[i], ellipse, point0, point1))
        {
            return true;
        }
    }
    return false;
}

bool Geo::NoAABBTest::is_intersected(const Geo::Geometry *object0, const Geo::Geometry *object1)
{
    switch (object0->type())
    {
    case Geo::Type::POLYGON:
        switch (object1->type())
        {
        case Geo::Type::POLYGON:
            return Geo::Collision::gjk(*static_cast<const Geo::Polygon *>(object0),
                *static_cast<const Geo::Polygon *>(object1));
        case Geo::Type::AABBRECT:
            return Geo::Collision::gjk(*static_cast<const Geo::AABBRect *>(object1),
                *static_cast<const Geo::Polygon *>(object0));
        case Geo::Type::POLYLINE:
            return Geo::NoAABBTest::is_intersected(*static_cast<const Geo::Polyline *>(object1),
                *static_cast<const Geo::Polygon *>(object0));
        case Geo::Type::BEZIER:
            return Geo::NoAABBTest::is_intersected(static_cast<const Geo::Bezier *>(object1)->shape(),
                *static_cast<const Geo::Polygon *>(object0));
        case Geo::Type::CIRCLE:
            return Geo::is_intersected(*static_cast<const Geo::Polygon *>(object0),
                *static_cast<const Geo::Circle *>(object1));
        default:
            return false;
        }
    case Geo::Type::AABBRECT:
        switch (object1->type())
        {
        case Geo::Type::POLYGON:
            return Geo::Collision::gjk(*static_cast<const Geo::AABBRect *>(object0),
                *static_cast<const Geo::Polygon *>(object1));
        case Geo::Type::AABBRECT:
            return Geo::NoAABBTest::is_intersected(*static_cast<const Geo::AABBRect *>(object0),
                *static_cast<const Geo::AABBRect *>(object1));
        case Geo::Type::POLYLINE:
            return Geo::NoAABBTest::is_intersected(*static_cast<const Geo::AABBRect *>(object0),
                *static_cast<const Geo::Polyline *>(object1));
        case Geo::Type::BEZIER:
            return Geo::NoAABBTest::is_intersected(*static_cast<const Geo::AABBRect *>(object0),
                static_cast<const Geo::Bezier *>(object1)->shape());
        case Geo::Type::CIRCLE:
            return Geo::is_intersected(*static_cast<const Geo::AABBRect *>(object0),
                *static_cast<const Geo::Circle *>(object1));
        default:
            return false;
        }
    case Geo::Type::POLYLINE:
        switch (object1->type())
        {
        case Geo::Type::POLYGON:
            return Geo::NoAABBTest::is_intersected(*static_cast<const Geo::Polyline *>(object0),
                *static_cast<const Geo::Polygon *>(object1));
        case Geo::Type::AABBRECT:
            return Geo::NoAABBTest::is_intersected(*static_cast<const Geo::AABBRect *>(object1),
                *static_cast<const Geo::Polyline *>(object0));
        case Geo::Type::POLYLINE:
            return Geo::NoAABBTest::is_intersected(*static_cast<const Geo::Polyline *>(object0),
                *static_cast<const Geo::Polyline *>(object1));
        case Geo::Type::BEZIER:
            return Geo::NoAABBTest::is_intersected(*static_cast<const Geo::Polyline *>(object0),
                static_cast<const Geo::Bezier *>(object1)->shape());
        case Geo::Type::CIRCLE:
            return Geo::is_intersected(*static_cast<const Geo::Polyline *>(object0),
                *static_cast<const Geo::Circle *>(object1));
        default:
            return false;
        }
    case Geo::Type::CIRCLE:
        switch (object1->type())
        {
        case Geo::Type::POLYGON:
            return Geo::is_intersected(*static_cast<const Geo::Polygon *>(object1),
                *static_cast<const Geo::Circle *>(object0));
        case Geo::Type::AABBRECT:
            return Geo::is_intersected(*static_cast<const Geo::AABBRect *>(object1),
                *static_cast<const Geo::Circle *>(object0));
        case Geo::Type::POLYLINE:
            return Geo::is_intersected(*static_cast<const Geo::Polyline *>(object1),
                *static_cast<const Geo::Circle *>(object0));
        case Geo::Type::BEZIER:
            return Geo::is_intersected(static_cast<const Geo::Bezier *>(object1)->shape(),
                *static_cast<const Geo::Circle *>(object0));
        case Geo::Type::CIRCLE:
            return Geo::is_intersected(*static_cast<const Geo::Circle *>(object0),
                *static_cast<const Geo::Circle *>(object1));
        default:
            return false;
        }
    default:
        return false;
    }
}

bool Geo::NoAABBTest::is_intersected(const Geo::AABBRect &rect, const Geo::Geometry *object)
{
    switch (object->type())
    {
    case Geo::Type::POLYGON:
        return Geo::Collision::gjk(rect, *static_cast<const Geo::Polygon *>(object));
    case Geo::Type::AABBRECT:
        return Geo::is_intersected(rect, *static_cast<const Geo::AABBRect *>(object));
    case Geo::Type::POLYLINE:
        return Geo::NoAABBTest::is_intersected(rect, *static_cast<const Geo::Polyline *>(object));
    case Geo::Type::BEZIER:
        return Geo::NoAABBTest::is_intersected(rect, static_cast<const Geo::Bezier *>(object)->shape());
    case Geo::Type::CIRCLE:
        return Geo::is_intersected(rect, *static_cast<const Geo::Circle *>(object));
    case Geo::Type::ELLIPSE:
        return Geo::NoAABBTest::is_intersected(rect, *static_cast<const Geo::Ellipse *>(object));
    default:
        return false;
    }
}


bool Geo::is_on_left(const Point &point, const Point &start, const Point &end)
{
    return (end.x - start.x) * (point.y - start.y) -
        (end.y - start.y) * (point.x - end.x) > 0;
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

bool Geo::foot_point(const Line &line, const Point &point, Point &foot, const bool infinite)
{
    return Geo::foot_point(line.front(), line.back(), point, foot, infinite);
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
    Geo::Point coord =  Geo::to_coord(Geo::Point(0, 0), center.x, center.y, angle);
    const double aa = Geo::distance_square(ellipse.a0(), ellipse.a1()) / 4;
    const double bb = Geo::distance_square(ellipse.b0(), ellipse.b1()) / 4;
    const double a1 = coord.x / aa, b1 = coord.y / bb, c1 = -1;
    const double a = std::pow(a1, 2) * aa  + std::pow(b1, 2) * bb;
    if (b1 != 0)
    {
        const double b = 2 * a1 * aa * c1;
        const double c = (std::pow(c1, 2) - std::pow(b1, 2) * bb) * aa;
        output0.x = (-b - std::sqrt(std::pow(b, 2) - 4 * a *c)) / (2 * a);
        output1.x = (-b + std::sqrt(std::pow(b, 2) - 4 * a *c)) / (2 * a);
        output0.y = (-a1 * output0.x - c1) / b1;
        output1.y = (-a1 * output1.x - c1) / b1;
    }
    else
    {
        const double b = 2 * b1 * bb * c1;
        const double c = (std::pow(c1, 2) - std::pow(a1, 2) * aa) * bb;
        output0.y = (-b - std::sqrt(std::pow(b, 2) - 4 * a *c)) / (2 * a);
        output1.y = (-b + std::sqrt(std::pow(b, 2) - 4 * a *c)) / (2 * a);
        output0.x = (-b1 * output0.y - c1) / a1;
        output1.x = (-b1 * output1.y - c1) / a1;
    }
    coord = Geo::to_coord(Geo::Point(0, 0), center.x, center.y, angle);
    output0 = Geo::to_coord(output0, coord.x, coord.y, -angle);
    output1 = Geo::to_coord(output1, coord.x, coord.y, -angle);
    return true;
}


double Geo::angle(const Point &start, const Point &end)
{
    const Geo::Point vec = end - start;
    double value = vec.x / vec.length();
    if (value > 1)
    {
        value = 1;
    }
    else if (value < -1)
    {
        value = -1;
    }
    return vec.y > 0 ? std::acos(value) : -std::acos(value);
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

double Geo::angle(const Line &line0, const Line &line1)
{
    const Geo::Point vec0 = line0.back() - line0.front(), vec1 = line1.back() - line1.front();
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


bool Geo::angle_to_arc(const Point &point0, const Point &point1, const Point &point2, const double radius, Polyline &arc, const double step)
{
    if (radius <= 0)
    {
        return false;
    }

    arc.clear();
    const double len = radius / std::tan(std::abs(Geo::angle(point0, point1, point2)) / 2);
    if (Geo::distance_square(point1, point0) >= len * len && Geo::distance_square(point2, point1) >= len * len)
    {
        double c = std::atan(len / radius) * radius;
        if (c < step * 2)
        {
            return false;
        }
        const Geo::Vector vec0 = (point0 - point1).normalize() * len;
        const Geo::Vector vec1 = (point2 - point1).normalize() * len;
        const Geo::Point center = point1 + (vec0 + vec1);
        Geo::Point foot0, foot1;
        Geo::foot_point(point0, point1, center, foot0, true);
        Geo::foot_point(point2, point1, center, foot1, true);
        Geo::Vector vec = (foot0 - center).normalize() * radius;
        double degree = std::asin(step / radius) * 2;
        if (Geo::angle(foot0, center, foot1) < 0)
        {
            degree = -degree;
        }
        while (c > 0)
        {
            arc.append(center + vec);
            vec.rotate(0, 0, degree);
            c -= step;
        }
        return true;
    }
    else
    {
        return false;
    }
}


Geo::Polygon Geo::circle_to_polygon(const double x, const double y, const double r)
{
    double c = r * Geo::PI;
    const double step = std::asin(1 / r) * 2;
    double degree = 0;
    std::vector<Geo::Point> points;
    while (c-- > 0)
    {
        points.emplace_back(r * std::cos(degree) + x, r * std::sin(degree) + y);
        degree += step;
    }
    if (points.size() >= 3)
    {
        return Geo::Polygon(points.cbegin(), points.cend());
    }
    else
    {
        return Geo::Polygon();
    }
}

Geo::Polygon Geo::circle_to_polygon(const Circle &circle)
{
    return Geo::circle_to_polygon(circle.x, circle.y, circle.radius);
}


Geo::Polygon Geo::ellipse_to_polygon(const double x, const double y, const double a, const double b, const double rad)
{
    double c = std::max(a, b) * Geo::PI;
    const double step = std::asin(1 / std::max(a, b)) * 2;
    double degree = 0;
    std::vector<Geo::Point> points;
    while (c-- > 0)
    {
        points.emplace_back(x + a * std::cos(rad) * std::cos(degree) - b * std::sin(rad) * std::sin(degree),
            y + a * std::sin(rad) * std::cos(degree) + b * std::cos(rad) * std::sin(degree));
        degree += step;
    }
    if (points.size() >= 3)
    {
        return Geo::Polygon(points.cbegin(), points.cend());
    }
    else
    {
        return Geo::Polygon();
    }
}

Geo::Polygon Geo::ellipse_to_polygon(const Ellipse &ellipse)
{
    return Geo::ellipse_to_polygon(ellipse.center().x, ellipse.center().y, ellipse.lengtha(), ellipse.lengthb(), ellipse.angle());
}


std::vector<size_t> Geo::ear_cut_to_indexs(const Geo::Polygon &polygon)
{
    std::vector<std::vector<std::array<double, 2>>> points;
    points.emplace_back(std::vector<std::array<double, 2>>());
    for (const Point &point : polygon)
    {
        points.front().emplace_back(std::array<double, 2>({point.x, point.y}));
    }
    return mapbox::earcut<size_t>(points);
}

std::vector<size_t> Geo::ear_cut_to_indexs_test(const Geo::Polygon &polygon)
{
    std::vector<size_t> indexs, ear_indexs;
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

    bool is_ear;
    while (indexs.size() > 3)
    {
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
                    ear_indexs.push_back(pre);
                    ear_indexs.push_back(cur);
                    ear_indexs.push_back(nxt);
                    indexs.erase(indexs.begin() + i--);
                    --count;
                }
            }
        }
    }

    ear_indexs.insert(ear_indexs.end(), indexs.begin(), indexs.end());
    return ear_indexs;
}

std::vector<Geo::MarkedPoint> Geo::ear_cut_to_coords(const Geo::Polygon &polygon)
{
    std::vector<MarkedPoint> result;
    for (size_t i : Geo::ear_cut_to_indexs(polygon))
    {
        result.emplace_back(polygon[i].x, polygon[i].y);
    }
    return result;
}

std::vector<Geo::Point> Geo::ear_cut_to_points(const Geo::Polygon &polygon)
{
    std::vector<Point> result;
    for (size_t i : Geo::ear_cut_to_indexs(polygon))
    {
        result.emplace_back(polygon[i]);
    }
    return result;
}

std::vector<Geo::Triangle> Geo::ear_cut_to_triangles(const Geo::Polygon &polygon)
{
    std::vector<size_t> indexs;
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

bool Geo::offset_test(const Geo::Polygon &input, Geo::Polygon &result, const double distance)
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

    std::vector<Polygon> polygons;
    if (result.is_self_intersected() && Geo::merge_ear_cut_triangles(Geo::ear_cut_to_triangles(result), polygons))
    {
        std::sort(polygons.begin(), polygons.end(),
            [](const Polygon &a, const Polygon &b) { return a.area() < b.area(); });
        temp = polygons.back();
        polygons.pop_back();
        bool flag;
        std::vector<Polygon> polygons2;
        while (!polygons.empty())
        {
            flag = true;
            for (size_t i = 0, count = polygons.size(); i < count; ++i)
            {
                if (Geo::polygon_union(temp, polygons[i], polygons2))
                {
                    if (polygons2.size() > 1)
                    {
                        temp = *std::max_element(polygons2.begin(), polygons2.end(), 
                            [](const Polygon &a, const Polygon &b) { return a.area() < b.area(); });
                    }
                    else
                    {
                        temp = polygons2.front();
                    }
                    polygons.erase(polygons.begin() + i);
                    polygons2.clear();
                    flag = false;
                    break;
                }
            }
            if (flag)
            {
                break;
            }
        }

        if (!temp.is_self_intersected())
        {
            result = temp;
        }
    }

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
