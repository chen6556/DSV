#include <algorithm>
#include "base/Algorithm.hpp"


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
    if (std::abs(Geo::distance(point.x, point.y, arc.x, arc.y) - arc.radius) >= Geo::EPSILON)
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