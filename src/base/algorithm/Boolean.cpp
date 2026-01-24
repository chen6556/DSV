#include <algorithm>
#include "base/Algorithm.hpp"


namespace
{
void calc_polygon_points(std::vector<Geo::MarkedPoint> &points0, std::vector<Geo::MarkedPoint> &points1, const Geo::AABBRect &rect)
{
    for (size_t i = 1, count0 = points0.size(); i < count0; ++i)
    {
        if (!Geo::is_intersected(rect, points0[i - 1], points0[i])) // 粗筛
        {
            continue;
        }

        const Geo::Point pre_point = points0[i - 1];
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

            if (Geo::Point point; !Geo::is_parallel(pre_point, points0[i], points1[k], points1[j]) &&
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
}

void sort_polygon_points(std::vector<Geo::MarkedPoint> &points, const Geo::Point &front)
{
    for (size_t i = 1, j = 0, count = points.size() - 1; i < count; ++i)
    {
        if (points[i].original)
        {
            continue;
        }
        else
        {
            j = i;
        }
        while (j < count && !points[j].original)
        {
            ++j;
        }
        if (j == i + 1)
        {
            ++i;
            continue;
        }

        std::vector<Geo::MarkedPoint> temp(points.begin() + i, j < count ? points.begin() + j : points.end());
        std::sort(temp.begin(), temp.end(), [&](const Geo::MarkedPoint &p0, const Geo::MarkedPoint &p1)
                  { return Geo::distance_square(p0, points[i - 1]) < Geo::distance_square(p1, points[i - 1]); });
        for (size_t k = i, n = 0; k < j; ++k)
        {
            points[k] = temp[n++];
        }
        i = j;
    }
    for (size_t i = points.size() - 1; i > 1;)
    {
        if (front == points[i])
        {
            if (!points[i].original)
            {
                points.insert(points.begin(), points[i]);
                points.erase(points.begin() + i + 1);
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
}

void remove_polygon_repeated_intersections(std::vector<Geo::MarkedPoint> &points, const Geo::Polygon &polygon0,
                                           const Geo::Polygon &polygon1)
{
    for (size_t count = 0, j = 0, i = points.size() - 1; i > 0; --i)
    {
        count = points[i].original ? 0 : 1;
        for (j = i; j > 0; --j)
        {
            if (std::abs(points[i].x - points[j - 1].x) > Geo::EPSILON || std::abs(points[i].y - points[j - 1].y) > Geo::EPSILON)
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
        if (count < 4)
        {
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
        }
        else
        {
            const Geo::MarkedPoint point = points[i];
            const Geo::Point &point_a = polygon0.last_point(polygon0.index(point.x, point.y));
            const Geo::Point &point_b = polygon0.next_point(polygon0.index(point.x, point.y));
            const Geo::Point &point_c = polygon1.last_point(polygon1.index(point.x, point.y));
            const Geo::Point &point_d = polygon1.next_point(polygon1.index(point.x, point.y));

            bool flags[5];
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
                flags[0] = false;
                for (size_t k = i; k > j; --k)
                {
                    flags[0] = (flags[0] || points[k].original);
                    points.erase(points.begin() + k);
                }
                points[j].value = value;
                points[j].original = (flags[0] || points[j].original);
            }
        }
        i = j > 0 ? j : 1;
    }
}
} // namespace


bool Geo::polygon_union(const Geo::Polygon &polygon0, const Geo::Polygon &polygon1, std::vector<Geo::Polygon> &output)
{
    Geo::Polygon polygon2(polygon0), polygon3(polygon1);
    polygon2.remove_repeated_points();
    polygon3.remove_repeated_points();
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

    // 找到交点并计算其几何数
    calc_polygon_points(points0, points1, polygon1.bounding_rect());

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
    sort_polygon_points(points0, polygon0.front());
    sort_polygon_points(points1, polygon1.front());
    // 去除重复交点
    remove_polygon_repeated_intersections(points0, polygon0, polygon1);
    remove_polygon_repeated_intersections(points1, polygon1, polygon0);

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

        it0 = std::find_if(points1.begin(), points1.end(),
                           [&](const MarkedPoint &p) { return !p.original && Geo::distance(p, points0[i]) < Geo::EPSILON; });
        it1 = std::find_if(points1.begin(), points1.end(),
                           [&](const MarkedPoint &p) { return !p.original && Geo::distance(p, points0[j]) < Geo::EPSILON; });
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

        it0 = std::find_if(points1.begin(), points1.end(),
                           [&](const MarkedPoint &p) { return !p.original && Geo::distance(p, points0[i]) < Geo::EPSILON; });
        it1 = std::find_if(points1.begin(), points1.end(),
                           [&](const MarkedPoint &p) { return !p.original && Geo::distance(p, points0[j]) < Geo::EPSILON; });
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

        it0 = std::find_if(points0.begin(), points0.end(),
                           [&](const MarkedPoint &p) { return !p.original && Geo::distance(p, points1[i]) < Geo::EPSILON; });
        it1 = std::find_if(points0.begin(), points0.end(),
                           [&](const MarkedPoint &p) { return !p.original && Geo::distance(p, points1[j]) < Geo::EPSILON; });
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

        it0 = std::find_if(points0.begin(), points0.end(),
                           [&](const MarkedPoint &p) { return !p.original && Geo::distance(p, points1[i]) < Geo::EPSILON; });
        it1 = std::find_if(points0.begin(), points0.end(),
                           [&](const MarkedPoint &p) { return !p.original && Geo::distance(p, points1[j]) < Geo::EPSILON; });
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

        if (std::count_if(points0.cbegin(), points0.cend(), [](const MarkedPoint &p) { return p.value < 0; }) == 0 ||
            std::count_if(points1.cbegin(), points1.cend(), [](const MarkedPoint &p) { return p.value < 0; }) == 0)
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
    polygon2.remove_repeated_points();
    polygon3.remove_repeated_points();
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

    // 找到交点并计算其几何数
    calc_polygon_points(points0, points1, polygon1.bounding_rect());

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
    sort_polygon_points(points0, polygon0.front());
    sort_polygon_points(points1, polygon1.front());
    // 去除重复交点
    remove_polygon_repeated_intersections(points0, polygon0, polygon1);
    remove_polygon_repeated_intersections(points1, polygon1, polygon0);

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

        it0 = std::find_if(points1.begin(), points1.end(),
                           [&](const MarkedPoint &p) { return !p.original && Geo::distance(p, points0[i]) < Geo::EPSILON; });
        it1 = std::find_if(points1.begin(), points1.end(),
                           [&](const MarkedPoint &p) { return !p.original && Geo::distance(p, points0[j]) < Geo::EPSILON; });
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

        it0 = std::find_if(points1.begin(), points1.end(),
                           [&](const MarkedPoint &p) { return !p.original && Geo::distance(p, points0[i]) < Geo::EPSILON; });
        it1 = std::find_if(points1.begin(), points1.end(),
                           [&](const MarkedPoint &p) { return !p.original && Geo::distance(p, points0[j]) < Geo::EPSILON; });
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

        it0 = std::find_if(points0.begin(), points0.end(),
                           [&](const MarkedPoint &p) { return !p.original && Geo::distance(p, points1[i]) < Geo::EPSILON; });
        it1 = std::find_if(points0.begin(), points0.end(),
                           [&](const MarkedPoint &p) { return !p.original && Geo::distance(p, points1[j]) < Geo::EPSILON; });
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

        it0 = std::find_if(points0.begin(), points0.end(),
                           [&](const MarkedPoint &p) { return !p.original && Geo::distance(p, points1[i]) < Geo::EPSILON; });
        it1 = std::find_if(points0.begin(), points0.end(),
                           [&](const MarkedPoint &p) { return !p.original && Geo::distance(p, points1[j]) < Geo::EPSILON; });
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

        if (std::count_if(points0.cbegin(), points0.cend(), [](const MarkedPoint &p) { return p.value < 0; }) == 0 ||
            std::count_if(points1.cbegin(), points1.cend(), [](const MarkedPoint &p) { return p.value < 0; }) == 0)
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
    polygon2.remove_repeated_points();
    polygon3.remove_repeated_points();
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

    // 找到交点并计算其几何数
    calc_polygon_points(points0, points1, polygon1.bounding_rect());

    if (points0.size() == polygon0.size()) // 无交点
    {
        return false;
    }

    // 调整交点顺序,同一条边上的交点按照顺序排列
    sort_polygon_points(points0, polygon0.front());
    sort_polygon_points(points1, polygon1.front());
    // 去除重复交点
    remove_polygon_repeated_intersections(points0, polygon0, polygon1);
    remove_polygon_repeated_intersections(points1, polygon1, polygon0);

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

        it0 = std::find_if(points1.begin(), points1.end(),
                           [&](const MarkedPoint &p) { return !p.original && Geo::distance(p, points0[i]) < Geo::EPSILON; });
        it1 = std::find_if(points1.begin(), points1.end(),
                           [&](const MarkedPoint &p) { return !p.original && Geo::distance(p, points0[j]) < Geo::EPSILON; });
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

        it0 = std::find_if(points1.begin(), points1.end(),
                           [&](const MarkedPoint &p) { return !p.original && Geo::distance(p, points0[i]) < Geo::EPSILON; });
        it1 = std::find_if(points1.begin(), points1.end(),
                           [&](const MarkedPoint &p) { return !p.original && Geo::distance(p, points0[j]) < Geo::EPSILON; });
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

        it0 = std::find_if(points0.begin(), points0.end(),
                           [&](const MarkedPoint &p) { return !p.original && Geo::distance(p, points1[i]) < Geo::EPSILON; });
        it1 = std::find_if(points0.begin(), points0.end(),
                           [&](const MarkedPoint &p) { return !p.original && Geo::distance(p, points1[j]) < Geo::EPSILON; });
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

        it0 = std::find_if(points0.begin(), points0.end(),
                           [&](const MarkedPoint &p) { return !p.original && Geo::distance(p, points1[i]) < Geo::EPSILON; });
        it1 = std::find_if(points0.begin(), points0.end(),
                           [&](const MarkedPoint &p) { return !p.original && Geo::distance(p, points1[j]) < Geo::EPSILON; });
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

        if (std::count_if(points0.cbegin(), points0.cend(), [](const MarkedPoint &p) { return p.value < 0; }) == 0 ||
            std::count_if(points1.cbegin(), points1.cend(), [](const MarkedPoint &p) { return p.value < 0; }) == 0)
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
    polygon2.remove_repeated_points();
    polygon3.remove_repeated_points();
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

    // 找到交点并计算其几何数
    calc_polygon_points(points0, points1, polygon1.bounding_rect());

    if (points0.size() == polygon0.size()) // 无交点
    {
        return false;
    }

    // 调整交点顺序,同一条边上的交点按照顺序排列
    sort_polygon_points(points0, polygon0.front());
    sort_polygon_points(points1, polygon1.front());
    // 去除重复交点
    remove_polygon_repeated_intersections(points0, polygon0, polygon1);
    remove_polygon_repeated_intersections(points1, polygon1, polygon0);

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

        it0 = std::find_if(points1.begin(), points1.end(),
                           [&](const MarkedPoint &p) { return !p.original && Geo::distance(p, points0[i]) < Geo::EPSILON; });
        it1 = std::find_if(points1.begin(), points1.end(),
                           [&](const MarkedPoint &p) { return !p.original && Geo::distance(p, points0[j]) < Geo::EPSILON; });
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

        it0 = std::find_if(points1.begin(), points1.end(),
                           [&](const MarkedPoint &p) { return !p.original && Geo::distance(p, points0[i]) < Geo::EPSILON; });
        it1 = std::find_if(points1.begin(), points1.end(),
                           [&](const MarkedPoint &p) { return !p.original && Geo::distance(p, points0[j]) < Geo::EPSILON; });
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

        it0 = std::find_if(points0.begin(), points0.end(),
                           [&](const MarkedPoint &p) { return !p.original && Geo::distance(p, points1[i]) < Geo::EPSILON; });
        it1 = std::find_if(points0.begin(), points0.end(),
                           [&](const MarkedPoint &p) { return !p.original && Geo::distance(p, points1[j]) < Geo::EPSILON; });
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

        it0 = std::find_if(points0.begin(), points0.end(),
                           [&](const MarkedPoint &p) { return !p.original && Geo::distance(p, points1[i]) < Geo::EPSILON; });
        it1 = std::find_if(points0.begin(), points0.end(),
                           [&](const MarkedPoint &p) { return !p.original && Geo::distance(p, points1[j]) < Geo::EPSILON; });
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

        if (std::count_if(points0.cbegin(), points0.cend(), [](const MarkedPoint &p) { return p.value < 0; }) == 0 ||
            std::count_if(points1.cbegin(), points1.cend(), [](const MarkedPoint &p) { return p.value < 0; }) == 0)
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

        if (std::count_if(points2.cbegin(), points2.cend(), [](const MarkedPoint &p) { return p.value < 0; }) == 0 ||
            std::count_if(points3.cbegin(), points3.cend(), [](const MarkedPoint &p) { return p.value < 0; }) == 0)
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


bool Geo::circle_union(const Circle &circle0, const Circle &circle1, std::vector<Arc> &output)
{
    if (Geo::Point point0, point1; Geo::is_intersected(circle0, circle1, point0, point1) == 2)
    {
        output.emplace_back(point0, circle0 + (circle0 - circle1).normalized() * circle0.radius, point1);
        output.emplace_back(point1, circle1 + (circle1 - circle0).normalized() * circle1.radius, point0);
        return true;
    }
    else
    {
        return false;
    }
}

bool Geo::circle_intersection(const Circle &circle0, const Circle &circle1, std::vector<Arc> &output)
{
    if (Geo::Point point0, point1; Geo::is_intersected(circle0, circle1, point0, point1) == 2)
    {
        output.emplace_back(point0, circle0 + (circle1 - circle0).normalized() * circle0.radius, point1);
        output.emplace_back(point1, circle1 + (circle0 - circle1).normalized() * circle1.radius, point0);
        return true;
    }
    else
    {
        return false;
    }
}

bool Geo::circle_difference(const Circle &circle0, const Circle &circle1, std::vector<Arc> &output)
{
    if (Geo::Point point0, point1; Geo::is_intersected(circle0, circle1, point0, point1) == 2)
    {
        output.emplace_back(point0, circle0 + (circle0 - circle1).normalized() * circle0.radius, point1);
        output.emplace_back(point0, circle1 + (circle0 - circle1).normalized() * circle1.radius, point1);
        return true;
    }
    else
    {
        return false;
    }
}

bool Geo::circle_xor(const Circle &circle0, const Circle &circle1, std::vector<Arc> &output)
{
    if (Geo::Point point0, point1; Geo::is_intersected(circle0, circle1, point0, point1) == 2)
    {
        output.emplace_back(point0, circle0 + (circle1 - circle0).normalized() * circle0.radius, point1);
        output.emplace_back(point0, circle1 + (circle1 - circle0).normalized() * circle1.radius, point1);
        output.emplace_back(point1, circle0 + (circle0 - circle1).normalized() * circle0.radius, point0);
        output.emplace_back(point1, circle1 + (circle0 - circle1).normalized() * circle1.radius, point0);
        return true;
    }
    else
    {
        return false;
    }
}


bool Geo::ellipse_union(const Ellipse &ellipse0, const Ellipse &ellipse1, std::vector<Ellipse> &output)
{
    if (ellipse0.is_arc() || ellipse1.is_arc())
    {
        return false;
    }

    const Geo::Point center[2] = {ellipse0.center(), ellipse1.center()};
    std::vector<Geo::Point> points;
    {
        Geo::Point temp[4];
        switch (Geo::is_intersected(ellipse0, ellipse1, temp[0], temp[1], temp[2], temp[3]))
        {
        case 4:
            points.emplace_back(temp[3]);
        case 3:
            points.emplace_back(temp[2]);
        case 2:
            points.emplace_back(temp[1]);
            points.emplace_back(temp[0]);
            break;
        default:
            return false;
        }
    }

    std::sort(points.begin(), points.end(),
              [&](const Geo::Point &p0, const Geo::Point &p1) { return Geo::angle(center[0], p0) < Geo::angle(center[0], p1); });
    const double angle[2] = {ellipse0.angle(), ellipse1.angle()};
    std::vector<Geo::Point> vecs0, vecs1; // 按圆心角大小对交点排序并计算出交点的切向量
    for (const Geo::Point &point : points)
    {
        vecs0.emplace_back(ellipse0.angle_tangency(Geo::angle(center[0], point) - angle[0]));
        vecs1.emplace_back(ellipse1.angle_tangency(Geo::angle(center[1], point) - angle[1]));
    }

    const double a[2] = {ellipse0.lengtha(), ellipse1.lengtha()};
    const double b[2] = {ellipse0.lengthb(), ellipse1.lengthb()};
    for (int i = 0, count = points.size(); i < count; ++i)
    {
        if (Geo::cross(vecs0[i], vecs1[i]) > 0)
        {
            double angle0 = Geo::angle(center[0], points[i]) - angle[0];
            double angle1 = Geo::angle(center[0], points[(i + 1) % count]) - angle[0];
            output.emplace_back(center[0], a[0], b[0], angle0, angle1, false);
            output.back().rotate(center[0].x, center[0].y, angle[0]);
        }
        else if (Geo::cross(vecs0[i], vecs1[i]) < 0)
        {
            double angle0 = Geo::angle(center[1], points[i]) - angle[1];
            double angle1 = Geo::angle(center[1], points[(i + 1) % count]) - angle[1];
            output.emplace_back(center[1], a[1], b[1], angle0, angle1, false);
            output.back().rotate(center[1].x, center[1].y, angle[1]);
        }
    }
    return true;
}

bool Geo::ellipse_intersection(const Ellipse &ellipse0, const Ellipse &ellipse1, std::vector<Ellipse> &output)
{
    if (ellipse0.is_arc() || ellipse1.is_arc())
    {
        return false;
    }

    const Geo::Point center[2] = {ellipse0.center(), ellipse1.center()};
    std::vector<Geo::Point> points;
    {
        Geo::Point temp[4];
        switch (Geo::is_intersected(ellipse0, ellipse1, temp[0], temp[1], temp[2], temp[3]))
        {
        case 4:
            points.emplace_back(temp[3]);
        case 3:
            points.emplace_back(temp[2]);
        case 2:
            points.emplace_back(temp[1]);
            points.emplace_back(temp[0]);
            break;
        default:
            return false;
        }
    }

    std::sort(points.begin(), points.end(),
              [&](const Geo::Point &p0, const Geo::Point &p1) { return Geo::angle(center[0], p0) < Geo::angle(center[0], p1); });
    const double angle[2] = {ellipse0.angle(), ellipse1.angle()};
    std::vector<Geo::Point> vecs0, vecs1; // 按圆心角大小对交点排序并计算出交点的切向量
    for (const Geo::Point &point : points)
    {
        vecs0.emplace_back(ellipse0.angle_tangency(Geo::angle(center[0], point) - angle[0]));
        vecs1.emplace_back(ellipse1.angle_tangency(Geo::angle(center[1], point) - angle[1]));
    }

    const double a[2] = {ellipse0.lengtha(), ellipse1.lengtha()};
    const double b[2] = {ellipse0.lengthb(), ellipse1.lengthb()};
    for (int i = 0, count = points.size(); i < count; ++i)
    {
        if (Geo::cross(vecs0[i], vecs1[i]) < 0)
        {
            double angle0 = Geo::angle(center[0], points[i]) - angle[0];
            double angle1 = Geo::angle(center[0], points[(i + 1) % count]) - angle[0];
            output.emplace_back(center[0], a[0], b[0], angle0, angle1, false);
            output.back().rotate(center[0].x, center[0].y, angle[0]);
        }
        else if (Geo::cross(vecs0[i], vecs1[i]) > 0)
        {
            double angle0 = Geo::angle(center[1], points[i]) - angle[1];
            double angle1 = Geo::angle(center[1], points[(i + 1) % count]) - angle[1];
            output.emplace_back(center[1], a[1], b[1], angle0, angle1, false);
            output.back().rotate(center[1].x, center[1].y, angle[1]);
        }
    }
    return true;
}

bool Geo::ellipse_difference(const Ellipse &ellipse0, const Ellipse &ellipse1, std::vector<Ellipse> &output)
{
    if (ellipse0.is_arc() || ellipse1.is_arc())
    {
        return false;
    }

    const Geo::Point center[2] = {ellipse0.center(), ellipse1.center()};
    std::vector<Geo::Point> points;
    {
        Geo::Point temp[4];
        switch (Geo::is_intersected(ellipse0, ellipse1, temp[0], temp[1], temp[2], temp[3]))
        {
        case 4:
            points.emplace_back(temp[3]);
        case 3:
            points.emplace_back(temp[2]);
        case 2:
            points.emplace_back(temp[1]);
            points.emplace_back(temp[0]);
            break;
        default:
            return false;
        }
    }

    std::sort(points.begin(), points.end(),
              [&](const Geo::Point &p0, const Geo::Point &p1) { return Geo::angle(center[0], p0) < Geo::angle(center[0], p1); });
    const double angle[2] = {ellipse0.angle(), ellipse1.angle()};
    std::vector<Geo::Point> vecs0, vecs1; // 按圆心角大小对交点排序并计算出交点的切向量
    for (const Geo::Point &point : points)
    {
        vecs0.emplace_back(ellipse0.angle_tangency(Geo::angle(center[0], point) - angle[0]));
        vecs1.emplace_back(ellipse1.angle_tangency(Geo::angle(center[1], point) - angle[1]));
    }

    const double a[2] = {ellipse0.lengtha(), ellipse1.lengtha()};
    const double b[2] = {ellipse0.lengthb(), ellipse1.lengthb()};
    for (int i = 0, count = points.size(); i < count; ++i)
    {
        if (Geo::cross(vecs0[i], vecs1[i]) > 0)
        {
            double angle0 = Geo::angle(center[0], points[i]) - angle[0];
            double angle1 = Geo::angle(center[0], points[(i + 1) % count]) - angle[0];
            output.emplace_back(center[0], a[0], b[0], angle0, angle1, false);
            output.back().rotate(center[0].x, center[0].y, angle[0]);
            angle0 = Geo::angle(center[1], points[i]) - angle[1];
            angle1 = Geo::angle(center[1], points[(i + 1) % count]) - angle[1];
            output.emplace_back(center[1], a[1], b[1], angle0, angle1, false);
            output.back().rotate(center[1].x, center[1].y, angle[1]);
        }
    }
    return true;
}

bool Geo::ellipse_xor(const Ellipse &ellipse0, const Ellipse &ellipse1, std::vector<Ellipse> &output)
{
    if (ellipse0.is_arc() || ellipse1.is_arc())
    {
        return false;
    }

    const Geo::Point center[2] = {ellipse0.center(), ellipse1.center()};
    std::vector<Geo::Point> points;
    {
        Geo::Point temp[4];
        switch (Geo::is_intersected(ellipse0, ellipse1, temp[0], temp[1], temp[2], temp[3]))
        {
        case 4:
            points.emplace_back(temp[3]);
        case 3:
            points.emplace_back(temp[2]);
        case 2:
            points.emplace_back(temp[1]);
            points.emplace_back(temp[0]);
            break;
        default:
            return false;
        }
    }

    std::sort(points.begin(), points.end(),
              [&](const Geo::Point &p0, const Geo::Point &p1) { return Geo::angle(center[0], p0) < Geo::angle(center[0], p1); });
    const double angle[2] = {ellipse0.angle(), ellipse1.angle()};
    std::vector<Geo::Point> vecs0, vecs1; // 按圆心角大小对交点排序并计算出交点的切向量
    for (const Geo::Point &point : points)
    {
        vecs0.emplace_back(ellipse0.angle_tangency(Geo::angle(center[0], point) - angle[0]));
        vecs1.emplace_back(ellipse1.angle_tangency(Geo::angle(center[1], point) - angle[1]));
    }

    const double a[2] = {ellipse0.lengtha(), ellipse1.lengtha()};
    const double b[2] = {ellipse0.lengthb(), ellipse1.lengthb()};
    for (int i = 0, count = points.size(); i < count; ++i)
    {
        double angle0 = Geo::angle(center[0], points[i]) - angle[0];
        double angle1 = Geo::angle(center[0], points[(i + 1) % count]) - angle[0];
        output.emplace_back(center[0], a[0], b[0], angle0, angle1, false);
        output.back().rotate(center[0].x, center[0].y, angle[0]);
        angle0 = Geo::angle(center[1], points[i]) - angle[1];
        angle1 = Geo::angle(center[1], points[(i + 1) % count]) - angle[1];
        output.emplace_back(center[1], a[1], b[1], angle0, angle1, false);
        output.back().rotate(center[1].x, center[1].y, angle[1]);
    }
    return true;
}


bool Geo::circle_ellipse_union(const Circle &circle, const Ellipse &ellipse, std::vector<Geo::Arc> &output0,
                               std::vector<Geo::Ellipse> &output1)
{
    if (ellipse.is_arc())
    {
        return false;
    }

    std::vector<Geo::Point> points;
    {
        Geo::Point temp[4];
        switch (Geo::is_intersected(circle, ellipse, temp[0], temp[1], temp[2], temp[3]))
        {
        case 4:
            points.emplace_back(temp[3]);
        case 3:
            points.emplace_back(temp[2]);
        case 2:
            points.emplace_back(temp[1]);
            points.emplace_back(temp[0]);
            break;
        default:
            return false;
        }
    }

    std::sort(points.begin(), points.end(),
              [&](const Geo::Point &p0, const Geo::Point &p1) { return Geo::angle(circle, p0) < Geo::angle(circle, p1); });
    const double angle = ellipse.angle();
    const Geo::Point center = ellipse.center();
    std::vector<Geo::Point> vecs0, vecs1; // 按圆心角大小对交点排序并计算出交点的切向量
    for (const Geo::Point &point : points)
    {
        vecs0.emplace_back((point - circle).vertical());
        vecs1.emplace_back(ellipse.angle_tangency(Geo::angle(center, point) - angle));
    }

    const double a = ellipse.lengtha(), b = ellipse.lengthb();
    for (int i = 0, count = points.size(); i < count; ++i)
    {
        if (Geo::cross(vecs0[i], vecs1[i]) > 0)
        {
            double angle0 = Geo::rad_to_2PI(Geo::angle(circle, points[i]));
            double angle1 = Geo::rad_to_2PI(Geo::angle(circle, points[(i + 1) % count]));
            output0.emplace_back(circle.x, circle.y, circle.radius, angle0, angle1, true);
        }
        else if (Geo::cross(vecs0[i], vecs1[i]) < 0)
        {
            double angle0 = Geo::angle(center, points[i]) - angle;
            double angle1 = Geo::angle(center, points[(i + 1) % count]) - angle;
            output1.emplace_back(center, a, b, angle0, angle1, false);
            output1.back().rotate(center.x, center.y, angle);
        }
    }
    return true;
}

bool Geo::circle_ellipse_intersection(const Circle &circle, const Ellipse &ellipse, std::vector<Geo::Arc> &output0,
                                      std::vector<Geo::Ellipse> &output1)
{
    if (ellipse.is_arc())
    {
        return false;
    }

    std::vector<Geo::Point> points;
    {
        Geo::Point temp[4];
        switch (Geo::is_intersected(circle, ellipse, temp[0], temp[1], temp[2], temp[3]))
        {
        case 4:
            points.emplace_back(temp[3]);
        case 3:
            points.emplace_back(temp[2]);
        case 2:
            points.emplace_back(temp[1]);
            points.emplace_back(temp[0]);
            break;
        default:
            return false;
        }
    }

    std::sort(points.begin(), points.end(),
              [&](const Geo::Point &p0, const Geo::Point &p1) { return Geo::angle(circle, p0) < Geo::angle(circle, p1); });
    const double angle = ellipse.angle();
    const Geo::Point center = ellipse.center();
    std::vector<Geo::Point> vecs0, vecs1; // 按圆心角大小对交点排序并计算出交点的切向量
    for (const Geo::Point &point : points)
    {
        vecs0.emplace_back((point - circle).vertical());
        vecs1.emplace_back(ellipse.angle_tangency(Geo::angle(center, point) - angle));
    }

    const double a = ellipse.lengtha(), b = ellipse.lengthb();
    for (int i = 0, count = points.size(); i < count; ++i)
    {
        if (Geo::cross(vecs0[i], vecs1[i]) < 0)
        {
            double angle0 = Geo::angle(circle, points[i]);
            double angle1 = Geo::angle(circle, points[(i + 1) % count]);
            output0.emplace_back(circle.x, circle.y, circle.radius, angle0, angle1, true);
        }
        else if (Geo::cross(vecs0[i], vecs1[i]) > 0)
        {
            double angle0 = Geo::angle(center, points[i]) - angle;
            double angle1 = Geo::angle(center, points[(i + 1) % count]) - angle;
            output1.emplace_back(center, a, b, angle0, angle1, false);
            output1.back().rotate(center.x, center.y, angle);
        }
    }
    return true;
}

bool Geo::circle_ellipse_difference(const Circle &circle, const Ellipse &ellipse, std::vector<Geo::Arc> &output0,
                                    std::vector<Geo::Ellipse> &output1)
{
    if (ellipse.is_arc())
    {
        return false;
    }

    std::vector<Geo::Point> points;
    {
        Geo::Point temp[4];
        switch (Geo::is_intersected(circle, ellipse, temp[0], temp[1], temp[2], temp[3]))
        {
        case 4:
            points.emplace_back(temp[3]);
        case 3:
            points.emplace_back(temp[2]);
        case 2:
            points.emplace_back(temp[1]);
            points.emplace_back(temp[0]);
            break;
        default:
            return false;
        }
    }

    std::sort(points.begin(), points.end(),
              [&](const Geo::Point &p0, const Geo::Point &p1) { return Geo::angle(circle, p0) < Geo::angle(circle, p1); });
    const double angle = ellipse.angle();
    const Geo::Point center = ellipse.center();
    std::vector<Geo::Point> vecs0, vecs1; // 按圆心角大小对交点排序并计算出交点的切向量
    for (const Geo::Point &point : points)
    {
        vecs0.emplace_back((point - circle).vertical());
        vecs1.emplace_back(ellipse.angle_tangency(Geo::angle(center, point) - angle));
    }

    const double a = ellipse.lengtha(), b = ellipse.lengthb();
    for (int i = 0, count = points.size(); i < count; ++i)
    {
        if (Geo::cross(vecs0[i], vecs1[i]) > 0)
        {
            double angle0 = Geo::angle(circle, points[i]);
            double angle1 = Geo::angle(circle, points[(i + 1) % count]);
            output0.emplace_back(circle.x, circle.y, circle.radius, angle0, angle1, true);
            angle0 = Geo::angle(center, points[i]) - angle;
            angle1 = Geo::angle(center, points[(i + 1) % count]) - angle;
            output1.emplace_back(center, a, b, angle0, angle1, false);
            output1.back().rotate(center.x, center.y, angle);
        }
    }
    return true;
}

bool Geo::ellipse_circle_difference(const Ellipse &ellipse, const Circle &circle, std::vector<Geo::Ellipse> &output0,
                                    std::vector<Geo::Arc> &output1)
{
    if (ellipse.is_arc())
    {
        return false;
    }

    std::vector<Geo::Point> points;
    {
        Geo::Point temp[4];
        switch (Geo::is_intersected(circle, ellipse, temp[0], temp[1], temp[2], temp[3]))
        {
        case 4:
            points.emplace_back(temp[3]);
        case 3:
            points.emplace_back(temp[2]);
        case 2:
            points.emplace_back(temp[1]);
            points.emplace_back(temp[0]);
            break;
        default:
            return false;
        }
    }

    const Geo::Point center = ellipse.center();
    std::sort(points.begin(), points.end(),
              [&](const Geo::Point &p0, const Geo::Point &p1) { return Geo::angle(center, p0) < Geo::angle(center, p1); });
    const double angle = ellipse.angle();
    std::vector<Geo::Point> vecs0, vecs1; // 按圆心角大小对交点排序并计算出交点的切向量
    for (const Geo::Point &point : points)
    {
        vecs0.emplace_back(ellipse.angle_tangency(Geo::angle(center, point) - angle));
        vecs1.emplace_back((point - circle).vertical());
    }

    const double a = ellipse.lengtha(), b = ellipse.lengthb();
    for (int i = 0, count = points.size(); i < count; ++i)
    {
        if (Geo::cross(vecs0[i], vecs1[i]) > 0)
        {
            double angle0 = Geo::angle(center, points[i]) - angle;
            double angle1 = Geo::angle(center, points[(i + 1) % count]) - angle;
            output0.emplace_back(center, a, b, angle0, angle1, false);
            output0.back().rotate(center.x, center.y, angle);
            angle0 = Geo::angle(circle, points[i]);
            angle1 = Geo::angle(circle, points[(i + 1) % count]);
            output1.emplace_back(circle.x, circle.y, circle.radius, angle0, angle1, true);
        }
    }
    return true;
}

bool Geo::circle_ellipse_xor(const Circle &circle, const Ellipse &ellipse, std::vector<Geo::Arc> &output0,
                             std::vector<Geo::Ellipse> &output1)
{
    if (ellipse.is_arc())
    {
        return false;
    }

    std::vector<Geo::Point> points;
    {
        Geo::Point temp[4];
        switch (Geo::is_intersected(circle, ellipse, temp[0], temp[1], temp[2], temp[3]))
        {
        case 4:
            points.emplace_back(temp[3]);
        case 3:
            points.emplace_back(temp[2]);
        case 2:
            points.emplace_back(temp[1]);
            points.emplace_back(temp[0]);
            break;
        default:
            return false;
        }
    }

    std::sort(points.begin(), points.end(),
              [&](const Geo::Point &p0, const Geo::Point &p1) { return Geo::angle(circle, p0) < Geo::angle(circle, p1); });
    const double angle = ellipse.angle();
    const Geo::Point center = ellipse.center();
    std::vector<Geo::Point> vecs0, vecs1; // 按圆心角大小对交点排序并计算出交点的切向量
    for (const Geo::Point &point : points)
    {
        vecs0.emplace_back((point - circle).vertical());
        vecs1.emplace_back(ellipse.angle_tangency(Geo::angle(center, point) - angle));
    }

    const double a = ellipse.lengtha(), b = ellipse.lengthb();
    for (int i = 0, count = points.size(); i < count; ++i)
    {
        double angle0 = Geo::angle(circle, points[i]);
        double angle1 = Geo::angle(circle, points[(i + 1) % count]);
        output0.emplace_back(circle.x, circle.y, circle.radius, angle0, angle1, true);
        angle0 = Geo::angle(center, points[i]) - angle;
        angle1 = Geo::angle(center, points[(i + 1) % count]) - angle;
        output1.emplace_back(center, a, b, angle0, angle1, false);
        output1.back().rotate(center.x, center.y, angle);
    }
    return true;
}


namespace
{
void remove_circle_ellipse_repeated_intersections(std::vector<Geo::MarkedPoint> &points)
{
    for (size_t count = 0, j = 0, i = points.size() - 1; i > 0; --i)
    {
        count = points[i].original ? 0 : 1;
        for (j = i; j > 0; --j)
        {
            if (std::abs(points[i].x - points[j - 1].x) > Geo::EPSILON || std::abs(points[i].y - points[j - 1].y) > Geo::EPSILON)
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
}
} // namespace


bool Geo::polygon_circle_union(const Polygon &polygon, const Circle &circle, std::vector<Geo::Polyline> &output0,
                               std::vector<Geo::Arc> &output1)
{
    Geo::Polygon polygon0(polygon);
    polygon0.remove_repeated_points();
    polygon0.reorder_points(false);
    std::vector<MarkedPoint> points0, points1;
    for (const Point &point : polygon0)
    {
        points0.emplace_back(point.x, point.y);
    }

    // 找到交点并计算其几何数
    for (size_t i = 1, count0 = points0.size(); i < count0; ++i)
    {
        Geo::Point point0, point1, pre_point = points0[i - 1];
        switch (Geo::is_intersected(pre_point, points0[i], circle, point0, point1, false))
        {
        case 2:
            if (Geo::distance(point0, pre_point) > Geo::distance(point1, pre_point))
            {
                std::swap(point0, point1);
            }
            points0.insert(points0.begin() + i++, MarkedPoint(point1.x, point1.y, false));
            points1.emplace_back(point1.x, point1.y, false);
            ++count0;
            if (Geo::cross(Geo::Point(points0[i]) - pre_point, (point1 - circle).vertical()) >= 0)
            {
                points0[i - 1].value = 1;
                points1.back().value = -1;
            }
            else
            {
                points0[i - 1].value = -1;
                points1.back().value = 1;
            }
        case 1:
            points0.insert(points0.begin() + i++, MarkedPoint(point0.x, point0.y, false));
            points1.emplace_back(point0.x, point0.y, false);
            ++count0;
            if (Geo::cross(Geo::Point(points0[i]) - pre_point, (point0 - circle).vertical()) >= 0)
            {
                points0[i - 1].value = 1;
                points1.back().value = -1;
            }
            else
            {
                points0[i - 1].value = -1;
                points1.back().value = 1;
            }
            break;
        default:
            break;
        }
    }

    if (points1.empty()) // 无交点
    {
        return false;
    }

    // 调整交点顺序,同一条边上的交点按照顺序排列
    sort_polygon_points(points0, polygon.front());
    std::sort(points1.begin(), points1.end(),
              [&](const MarkedPoint &a, const MarkedPoint &b) { return Geo::angle(circle, a) < Geo::angle(circle, b); });

    // 去除重复交点
    remove_circle_ellipse_repeated_intersections(points0);
    remove_circle_ellipse_repeated_intersections(points1);

    if (std::count_if(points0.begin(), points0.end(), [](const MarkedPoint &p) { return p.value < 0; }) == 0 ||
        std::count_if(points1.begin(), points1.end(), [](const MarkedPoint &p) { return p.value < 0; }) == 0)
    {
        return false; // 交点都是出点,即两多边形只有一个点相交
    }

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
    output0.clear(), output1.clear();

    while (count0 > 0 && count1 > 0)
    {
        size_t index0 = 0, index1 = 0;
        while (index0 < count0 && points0[index0].value < 1)
        {
            ++index0;
        }
        if (index0 >= count0)
        {
            break;
        }

        std::vector<Point> result, line_points, arc_points;
        while (result.size() < count2 && (result.size() < 3 || result.front() != result.back()))
        {
            while (result.size() < count2 && (result.size() < 3 || result.front() != result.back()))
            {
                if (points0[index0].value > -1)
                {
                    if (points0[index0].original)
                    {
                        points0[index0].active = false;
                    }
                    result.emplace_back(points0[index0++]);
                    line_points.emplace_back(result.back());
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
                    line_points.emplace_back(result.back());
                    ++index0;
                    index0 %= count0;
                    index1 %= count1;
                    break;
                }
                index0 %= count0;
            }

            if (line_points.size() > 1)
            {
                output0.emplace_back(line_points.begin(), line_points.end());
            }
            arc_points.emplace_back(line_points.back());
            line_points.clear();

            while (result.size() < count2 && (result.size() < 3 || result.front() != result.back()))
            {
                if (points1[index1].value > -1)
                {
                    if (points1[index1].original)
                    {
                        points1[index1].active = false;
                    }
                    result.emplace_back(points1[index1++]);
                    arc_points.emplace_back(result.back());
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
                    arc_points.emplace_back(result.back());
                    ++index1;
                    index0 %= count0;
                    index1 %= count1;
                    break;
                }
                index1 %= count1;
            }

            if (arc_points.size() > 1)
            {
                output1.emplace_back(circle.x, circle.y, circle.radius, Geo::angle(circle, arc_points.front()),
                                     Geo::angle(circle, arc_points.back()), true);
            }
            line_points.emplace_back(arc_points.back());
            arc_points.clear();
        }

        std::vector<Geo::MarkedPoint>::iterator it = points0.begin();
        while (it != points0.end())
        {
            if (!it->active || std::find(result.begin(), result.end(), *it) != result.end())
            {
                it = points0.erase(it);
            }
            else
            {
                ++it;
            }
        }
        it = points1.begin();
        while (it != points1.end())
        {
            if (!it->active || std::find(result.begin(), result.end(), *it) != result.end())
            {
                it = points1.erase(it);
            }
            else
            {
                ++it;
            }
        }

        if (std::count_if(points0.cbegin(), points0.cend(), [](const MarkedPoint &p) { return p.value < 0; }) == 0 ||
            std::count_if(points1.cbegin(), points1.cend(), [](const MarkedPoint &p) { return p.value < 0; }) == 0)
        {
            break;
        }

        count0 = points0.size();
        count1 = points1.size();
        count2 = count0 + count1;
    }
    return true;
}

bool Geo::polygon_circle_intersection(const Polygon &polygon, const Circle &circle, std::vector<Geo::Polyline> &output0,
                                      std::vector<Geo::Arc> &output1)
{
    Geo::Polygon polygon0(polygon);
    polygon0.remove_repeated_points();
    polygon0.reorder_points(false);
    std::vector<MarkedPoint> points0, points1;
    for (const Point &point : polygon0)
    {
        points0.emplace_back(point.x, point.y);
    }

    // 找到交点并计算其几何数
    for (size_t i = 1, count0 = points0.size(); i < count0; ++i)
    {
        Geo::Point point0, point1, pre_point = points0[i - 1];
        switch (Geo::is_intersected(pre_point, points0[i], circle, point0, point1, false))
        {
        case 2:
            if (Geo::distance(point0, pre_point) > Geo::distance(point1, pre_point))
            {
                std::swap(point0, point1);
            }
            points0.insert(points0.begin() + i++, MarkedPoint(point1.x, point1.y, false));
            points1.emplace_back(point1.x, point1.y, false);
            ++count0;
            if (Geo::cross(Geo::Point(points0[i]) - pre_point, (point1 - circle).vertical()) >= 0)
            {
                points0[i - 1].value = 1;
                points1.back().value = -1;
            }
            else
            {
                points0[i - 1].value = -1;
                points1.back().value = 1;
            }
        case 1:
            points0.insert(points0.begin() + i++, MarkedPoint(point0.x, point0.y, false));
            points1.emplace_back(point0.x, point0.y, false);
            ++count0;
            if (Geo::cross(Geo::Point(points0[i]) - pre_point, (point0 - circle).vertical()) >= 0)
            {
                points0[i - 1].value = 1;
                points1.back().value = -1;
            }
            else
            {
                points0[i - 1].value = -1;
                points1.back().value = 1;
            }
            break;
        default:
            break;
        }
    }

    if (points1.empty()) // 无交点
    {
        return false;
    }

    // 调整交点顺序,同一条边上的交点按照顺序排列
    sort_polygon_points(points0, polygon.front());
    std::sort(points1.begin(), points1.end(),
              [&](const MarkedPoint &a, const MarkedPoint &b) { return Geo::angle(circle, a) < Geo::angle(circle, b); });

    // 去除重复交点
    remove_circle_ellipse_repeated_intersections(points0);
    remove_circle_ellipse_repeated_intersections(points1);

    if (std::count_if(points0.begin(), points0.end(), [](const MarkedPoint &p) { return p.value < 0; }) == 0 ||
        std::count_if(points1.begin(), points1.end(), [](const MarkedPoint &p) { return p.value < 0; }) == 0)
    {
        return false; // 交点都是出点,即两多边形只有一个点相交
    }

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
    output0.clear(), output1.clear();

    while (count0 > 0 && count1 > 0)
    {
        size_t index0 = 0, index1 = 0;
        while (index0 < count0 && points0[index0].value < 1)
        {
            ++index0;
        }
        if (index0 >= count0)
        {
            break;
        }

        std::vector<Point> result, line_points, arc_points;
        while (result.size() < count2 && (result.size() < 3 || result.front() != result.back()))
        {
            while (result.size() < count2 && (result.size() < 3 || result.front() != result.back()))
            {
                if (points0[index0].value < 1)
                {
                    if (points0[index0].original)
                    {
                        points0[index0].active = false;
                    }
                    result.emplace_back(points0[index0++]);
                    line_points.emplace_back(result.back());
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
                    line_points.emplace_back(result.back());
                    ++index0;
                    index0 %= count0;
                    index1 %= count1;
                    break;
                }
                index0 %= count0;
            }

            if (line_points.size() > 1)
            {
                output0.emplace_back(line_points.begin(), line_points.end());
            }
            arc_points.emplace_back(line_points.back());
            line_points.clear();

            while (result.size() < count2 && (result.size() < 3 || result.front() != result.back()))
            {
                if (points1[index1].value < 1)
                {
                    if (points1[index1].original)
                    {
                        points1[index1].active = false;
                    }
                    result.emplace_back(points1[index1++]);
                    arc_points.emplace_back(result.back());
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
                    arc_points.emplace_back(result.back());
                    ++index1;
                    index0 %= count0;
                    index1 %= count1;
                    break;
                }
                index1 %= count1;
            }

            if (arc_points.size() > 1)
            {
                output1.emplace_back(circle.x, circle.y, circle.radius, Geo::angle(circle, arc_points.front()),
                                     Geo::angle(circle, arc_points.back()), true);
            }
            line_points.emplace_back(arc_points.back());
            arc_points.clear();
        }

        std::vector<Geo::MarkedPoint>::iterator it = points0.begin();
        while (it != points0.end())
        {
            if (!it->active || std::find(result.begin(), result.end(), *it) != result.end())
            {
                it = points0.erase(it);
            }
            else
            {
                ++it;
            }
        }
        it = points1.begin();
        while (it != points1.end())
        {
            if (!it->active || std::find(result.begin(), result.end(), *it) != result.end())
            {
                it = points1.erase(it);
            }
            else
            {
                ++it;
            }
        }

        if (std::count_if(points0.cbegin(), points0.cend(), [](const MarkedPoint &p) { return p.value < 0; }) == 0 ||
            std::count_if(points1.cbegin(), points1.cend(), [](const MarkedPoint &p) { return p.value < 0; }) == 0)
        {
            break;
        }

        count0 = points0.size();
        count1 = points1.size();
        count2 = count0 + count1;
    }
    return true;
}

bool Geo::polygon_circle_difference(const Polygon &polygon, const Circle &circle, std::vector<Geo::Polyline> &output0,
                                    std::vector<Geo::Arc> &output1)
{
    Geo::Polygon polygon0(polygon);
    polygon0.remove_repeated_points();
    polygon0.reorder_points(false);
    std::vector<MarkedPoint> points0, points1;
    for (const Point &point : polygon0)
    {
        points0.emplace_back(point.x, point.y);
    }

    // 找到交点并计算其几何数
    for (size_t i = 1, count0 = points0.size(); i < count0; ++i)
    {
        Geo::Point point0, point1, pre_point = points0[i - 1];
        switch (Geo::is_intersected(pre_point, points0[i], circle, point0, point1, false))
        {
        case 2:
            if (Geo::distance(point0, pre_point) > Geo::distance(point1, pre_point))
            {
                std::swap(point0, point1);
            }
            points0.insert(points0.begin() + i++, MarkedPoint(point1.x, point1.y, false));
            points1.emplace_back(point1.x, point1.y, false);
            ++count0;
            if (Geo::cross(Geo::Point(points0[i]) - pre_point, (point1 - circle).vertical()) >= 0)
            {
                points0[i - 1].value = 1;
                points1.back().value = -1;
            }
            else
            {
                points0[i - 1].value = -1;
                points1.back().value = 1;
            }
        case 1:
            points0.insert(points0.begin() + i++, MarkedPoint(point0.x, point0.y, false));
            points1.emplace_back(point0.x, point0.y, false);
            ++count0;
            if (Geo::cross(Geo::Point(points0[i]) - pre_point, (point0 - circle).vertical()) >= 0)
            {
                points0[i - 1].value = 1;
                points1.back().value = -1;
            }
            else
            {
                points0[i - 1].value = -1;
                points1.back().value = 1;
            }
            break;
        default:
            break;
        }
    }

    if (points1.empty()) // 无交点
    {
        return false;
    }

    // 调整交点顺序,同一条边上的交点按照顺序排列
    sort_polygon_points(points0, polygon.front());
    std::sort(points1.begin(), points1.end(),
              [&](const MarkedPoint &a, const MarkedPoint &b) { return Geo::angle(circle, a) > Geo::angle(circle, b); }); // 顺时针排序

    // 去除重复交点
    remove_circle_ellipse_repeated_intersections(points0);
    remove_circle_ellipse_repeated_intersections(points1);

    if (std::count_if(points0.begin(), points0.end(), [](const MarkedPoint &p) { return p.value < 0; }) == 0 ||
        std::count_if(points1.begin(), points1.end(), [](const MarkedPoint &p) { return p.value < 0; }) == 0)
    {
        return false; // 交点都是出点,即两多边形只有一个点相交
    }

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
    output0.clear(), output1.clear();

    while (count0 > 0 && count1 > 0)
    {
        size_t index0 = 0, index1 = 0;
        while (index0 < count0 && points0[index0].value < 1)
        {
            ++index0;
        }
        if (index0 >= count0)
        {
            break;
        }

        std::vector<Point> result, line_points, arc_points;
        while (result.size() < count2 && (result.size() < 3 || result.front() != result.back()))
        {
            while (result.size() < count2 && (result.size() < 3 || result.front() != result.back()))
            {
                if (points0[index0].value > -1)
                {
                    if (points0[index0].original)
                    {
                        points0[index0].active = false;
                    }
                    result.emplace_back(points0[index0++]);
                    line_points.emplace_back(result.back());
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
                    line_points.emplace_back(result.back());
                    ++index0;
                    index0 %= count0;
                    index1 %= count1;
                    break;
                }
                index0 %= count0;
            }

            if (line_points.size() > 1)
            {
                output0.emplace_back(line_points.begin(), line_points.end());
            }
            arc_points.emplace_back(line_points.back());
            line_points.clear();

            while (result.size() < count2 && (result.size() < 3 || result.front() != result.back()))
            {
                if (points1[index1].value > -1)
                {
                    if (points1[index1].original)
                    {
                        points1[index1].active = false;
                    }
                    result.emplace_back(points1[index1++]);
                    arc_points.emplace_back(result.back());
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
                    arc_points.emplace_back(result.back());
                    ++index1;
                    index0 %= count0;
                    index1 %= count1;
                    break;
                }
                index1 %= count1;
            }

            if (arc_points.size() > 1)
            {
                output1.emplace_back(circle.x, circle.y, circle.radius, Geo::angle(circle, arc_points.front()),
                                     Geo::angle(circle, arc_points.back()), false);
            }
            line_points.emplace_back(arc_points.back());
            arc_points.clear();
        }

        std::vector<Geo::MarkedPoint>::iterator it = points0.begin();
        while (it != points0.end())
        {
            if (!it->active || std::find(result.begin(), result.end(), *it) != result.end())
            {
                it = points0.erase(it);
            }
            else
            {
                ++it;
            }
        }
        it = points1.begin();
        while (it != points1.end())
        {
            if (!it->active || std::find(result.begin(), result.end(), *it) != result.end())
            {
                it = points1.erase(it);
            }
            else
            {
                ++it;
            }
        }

        if (std::count_if(points0.cbegin(), points0.cend(), [](const MarkedPoint &p) { return p.value < 0; }) == 0 ||
            std::count_if(points1.cbegin(), points1.cend(), [](const MarkedPoint &p) { return p.value < 0; }) == 0)
        {
            break;
        }

        count0 = points0.size();
        count1 = points1.size();
        count2 = count0 + count1;
    }
    return true;
}

bool Geo::circle_polygon_difference(const Circle &circle, const Polygon &polygon, std::vector<Geo::Arc> &output0,
                                    std::vector<Geo::Polyline> &output1)
{
    Geo::Polygon polygon0(polygon);
    polygon0.remove_repeated_points();
    polygon0.reorder_points(true);
    std::vector<MarkedPoint> points0, points1;
    for (const Point &point : polygon0)
    {
        points1.emplace_back(point.x, point.y);
    }

    // 找到交点并计算其几何数
    for (size_t i = 1, count1 = points1.size(); i < count1; ++i)
    {
        Geo::Point point0, point1, pre_point = points1[i - 1];
        switch (Geo::is_intersected(pre_point, points1[i], circle, point0, point1, false))
        {
        case 2:
            if (Geo::distance(point0, pre_point) > Geo::distance(point1, pre_point))
            {
                std::swap(point0, point1);
            }
            points0.emplace_back(point1.x, point1.y, false);
            points1.insert(points1.begin() + i++, MarkedPoint(point1.x, point1.y, false));
            ++count1;
            if (Geo::cross((point1 - circle).vertical(), Geo::Point(points1[i]) - pre_point) >= 0)
            {
                points0.back().value = -1;
                points1[i - 1].value = 1;
            }
            else
            {
                points0.back().value = 1;
                points1[i - 1].value = -1;
            }
        case 1:
            points0.emplace_back(point0.x, point0.y, false);
            points1.insert(points1.begin() + i++, MarkedPoint(point0.x, point0.y, false));
            ++count1;
            if (Geo::cross((point0 - circle).vertical(), Geo::Point(points1[i]) - pre_point) >= 0)
            {
                points0.back().value = -1;
                points1[i - 1].value = 1;
            }
            else
            {
                points0.back().value = 1;
                points1[i - 1].value = -1;
            }
            break;
        default:
            break;
        }
    }

    if (points0.empty()) // 无交点
    {
        return false;
    }

    // 调整交点顺序,同一条边上的交点按照顺序排列
    sort_polygon_points(points1, polygon.front());
    std::sort(points0.begin(), points0.end(),
              [&](const MarkedPoint &a, const MarkedPoint &b) { return Geo::angle(circle, a) < Geo::angle(circle, b); });

    // 去除重复交点
    remove_circle_ellipse_repeated_intersections(points0);
    remove_circle_ellipse_repeated_intersections(points1);

    if (std::count_if(points0.begin(), points0.end(), [](const MarkedPoint &p) { return p.value < 0; }) == 0 ||
        std::count_if(points1.begin(), points1.end(), [](const MarkedPoint &p) { return p.value < 0; }) == 0)
    {
        return false; // 交点都是出点,即两多边形只有一个点相交
    }

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
    output0.clear(), output1.clear();

    while (count0 > 0 && count1 > 0)
    {
        size_t index0 = 0, index1 = 0;
        while (index0 < count0 && points0[index0].value < 1)
        {
            ++index0;
        }
        if (index0 >= count0)
        {
            break;
        }

        std::vector<Point> result, arc_points, line_points;
        while (result.size() < count2 && (result.size() < 3 || result.front() != result.back()))
        {
            while (result.size() < count2 && (result.size() < 3 || result.front() != result.back()))
            {
                if (points0[index0].value > -1)
                {
                    if (points0[index0].original)
                    {
                        points0[index0].active = false;
                    }
                    result.emplace_back(points0[index0++]);
                    arc_points.emplace_back(result.back());
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
                    arc_points.emplace_back(result.back());
                    ++index0;
                    index0 %= count0;
                    index1 %= count1;
                    break;
                }
                index0 %= count0;
            }

            if (arc_points.size() > 1)
            {
                output0.emplace_back(circle.x, circle.y, circle.radius, Geo::angle(circle, arc_points.front()),
                                     Geo::angle(circle, arc_points.back()), true);
            }
            line_points.emplace_back(arc_points.back());
            arc_points.clear();

            while (result.size() < count2 && (result.size() < 3 || result.front() != result.back()))
            {
                if (points1[index1].value > -1)
                {
                    if (points1[index1].original)
                    {
                        points1[index1].active = false;
                    }
                    result.emplace_back(points1[index1++]);
                    line_points.emplace_back(result.back());
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
                    line_points.emplace_back(result.back());
                    ++index1;
                    index0 %= count0;
                    index1 %= count1;
                    break;
                }
                index1 %= count1;
            }

            if (line_points.size() > 1)
            {
                output1.emplace_back(line_points.begin(), line_points.end());
            }
            arc_points.emplace_back(line_points.back());
            line_points.clear();
        }

        std::vector<Geo::MarkedPoint>::iterator it = points0.begin();
        while (it != points0.end())
        {
            if (!it->active || std::find(result.begin(), result.end(), *it) != result.end())
            {
                it = points0.erase(it);
            }
            else
            {
                ++it;
            }
        }
        it = points1.begin();
        while (it != points1.end())
        {
            if (!it->active || std::find(result.begin(), result.end(), *it) != result.end())
            {
                it = points1.erase(it);
            }
            else
            {
                ++it;
            }
        }

        if (std::count_if(points0.cbegin(), points0.cend(), [](const MarkedPoint &p) { return p.value < 0; }) == 0 ||
            std::count_if(points1.cbegin(), points1.cend(), [](const MarkedPoint &p) { return p.value < 0; }) == 0)
        {
            break;
        }

        count0 = points0.size();
        count1 = points1.size();
        count2 = count0 + count1;
    }
    return true;
}

bool Geo::polygon_circle_xor(const Polygon &polygon, const Circle &circle, std::vector<Geo::Polyline> &output0,
                             std::vector<Geo::Arc> &output1)
{
    Geo::Polygon polygon0(polygon);
    polygon0.remove_repeated_points();
    polygon0.reorder_points(true);
    std::vector<MarkedPoint> points0, points1;
    for (const Point &point : polygon0)
    {
        points0.emplace_back(point.x, point.y);
    }

    // 找到交点并计算其几何数
    for (size_t i = 1, count0 = points0.size(); i < count0; ++i)
    {
        Geo::Point point0, point1, pre_point = points0[i - 1];
        switch (Geo::is_intersected(pre_point, points0[i], circle, point0, point1, false))
        {
        case 2:
            if (Geo::distance(point0, pre_point) > Geo::distance(point1, pre_point))
            {
                std::swap(point0, point1);
            }
            points0.insert(points0.begin() + i++, MarkedPoint(point1.x, point1.y, false));
            points1.emplace_back(point1.x, point1.y, false);
            ++count0;
            if (Geo::cross(Geo::Point(points0[i]) - pre_point, (point1 - circle).vertical()) >= 0)
            {
                points0[i - 1].value = 1;
                points1.back().value = -1;
            }
            else
            {
                points0[i - 1].value = -1;
                points1.back().value = 1;
            }
        case 1:
            points0.insert(points0.begin() + i++, MarkedPoint(point0.x, point0.y, false));
            points1.emplace_back(point0.x, point0.y, false);
            ++count0;
            if (Geo::cross(Geo::Point(points0[i]) - pre_point, (point0 - circle).vertical()) >= 0)
            {
                points0[i - 1].value = 1;
                points1.back().value = -1;
            }
            else
            {
                points0[i - 1].value = -1;
                points1.back().value = 1;
            }
            break;
        default:
            break;
        }
    }

    if (points1.empty()) // 无交点
    {
        return false;
    }

    // 调整交点顺序,同一条边上的交点按照顺序排列
    sort_polygon_points(points0, polygon.front());
    std::sort(points1.begin(), points1.end(),
              [&](const MarkedPoint &a, const MarkedPoint &b) { return Geo::angle(circle, a) < Geo::angle(circle, b); });

    // 去除重复交点
    remove_circle_ellipse_repeated_intersections(points0);
    remove_circle_ellipse_repeated_intersections(points1);

    if (std::count_if(points0.begin(), points0.end(), [](const MarkedPoint &p) { return p.value < 0; }) == 0 ||
        std::count_if(points1.begin(), points1.end(), [](const MarkedPoint &p) { return p.value < 0; }) == 0)
    {
        return false; // 交点都是出点,即两多边形只有一个点相交
    }

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
    output0.clear(), output1.clear();
    std::vector<Geo::MarkedPoint> points2(points0.begin(), points0.end());
    std::vector<Geo::MarkedPoint> points3(points1.begin(), points1.end());
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

    while (count0 > 0 && count1 > 0)
    {
        size_t index0 = 0, index1 = 0;
        while (index0 < count0 && points0[index0].value < 1)
        {
            ++index0;
        }
        if (index0 >= count0)
        {
            break;
        }

        std::vector<Point> result, line_points, arc_points;
        while (result.size() < count2 && (result.size() < 3 || result.front() != result.back()))
        {
            while (result.size() < count2 && (result.size() < 3 || result.front() != result.back()))
            {
                if (points0[index0].value > -1)
                {
                    if (points0[index0].original)
                    {
                        points0[index0].active = false;
                    }
                    result.emplace_back(points0[index0++]);
                    line_points.emplace_back(result.back());
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
                    line_points.emplace_back(result.back());
                    ++index0;
                    index0 %= count0;
                    index1 %= count1;
                    break;
                }
                index0 %= count0;
            }

            if (line_points.size() > 1)
            {
                output0.emplace_back(line_points.begin(), line_points.end());
            }
            arc_points.emplace_back(line_points.back());
            line_points.clear();

            while (result.size() < count2 && (result.size() < 3 || result.front() != result.back()))
            {
                if (points1[index1].value > -1)
                {
                    if (points1[index1].original)
                    {
                        points1[index1].active = false;
                    }
                    result.emplace_back(points1[index1++]);
                    arc_points.emplace_back(result.back());
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
                    arc_points.emplace_back(result.back());
                    ++index1;
                    index0 %= count0;
                    index1 %= count1;
                    break;
                }
                index1 %= count1;
            }

            if (arc_points.size() > 1)
            {
                output1.emplace_back(circle.x, circle.y, circle.radius, Geo::angle(circle, arc_points.front()),
                                     Geo::angle(circle, arc_points.back()), true);
            }
            line_points.emplace_back(arc_points.back());
            arc_points.clear();
        }

        std::vector<Geo::MarkedPoint>::iterator it = points0.begin();
        while (it != points0.end())
        {
            if (!it->active || std::find(result.begin(), result.end(), *it) != result.end())
            {
                it = points0.erase(it);
            }
            else
            {
                ++it;
            }
        }
        it = points1.begin();
        while (it != points1.end())
        {
            if (!it->active || std::find(result.begin(), result.end(), *it) != result.end())
            {
                it = points1.erase(it);
            }
            else
            {
                ++it;
            }
        }

        if (std::count_if(points0.cbegin(), points0.cend(), [](const MarkedPoint &p) { return p.value < 0; }) == 0 ||
            std::count_if(points1.cbegin(), points1.cend(), [](const MarkedPoint &p) { return p.value < 0; }) == 0)
        {
            break;
        }

        count0 = points0.size();
        count1 = points1.size();
        count2 = count0 + count1;
    }

    count0 = points2.size(), count1 = points3.size();
    count2 = count0 + count1;
    while (count0 > 0 && count1 > 0)
    {
        size_t index0 = 0, index1 = 0;
        while (index0 < count0 && points2[index0].value < 1)
        {
            ++index0;
        }
        if (index0 >= count0)
        {
            break;
        }

        std::vector<Point> result, line_points, arc_points;
        while (result.size() < count2 && (result.size() < 3 || result.front() != result.back()))
        {
            while (result.size() < count2 && (result.size() < 3 || result.front() != result.back()))
            {
                if (points2[index0].value > -1)
                {
                    if (points2[index0].original)
                    {
                        points2[index0].active = false;
                    }
                    result.emplace_back(points2[index0++]);
                    line_points.emplace_back(result.back());
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
                    line_points.emplace_back(result.back());
                    ++index0;
                    index0 %= count0;
                    index1 %= count1;
                    break;
                }
                index0 %= count0;
            }

            if (line_points.size() > 1)
            {
                output0.emplace_back(line_points.begin(), line_points.end());
            }
            arc_points.emplace_back(line_points.back());
            line_points.clear();

            while (result.size() < count2 && (result.size() < 3 || result.front() != result.back()))
            {
                if (points3[index1].value > -1)
                {
                    if (points3[index1].original)
                    {
                        points3[index1].active = false;
                    }
                    result.emplace_back(points3[index1++]);
                    arc_points.emplace_back(result.back());
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
                    arc_points.emplace_back(result.back());
                    ++index1;
                    index0 %= count0;
                    index1 %= count1;
                    break;
                }
                index1 %= count1;
            }

            if (arc_points.size() > 1)
            {
                output1.emplace_back(circle.x, circle.y, circle.radius, Geo::angle(circle, arc_points.front()),
                                     Geo::angle(circle, arc_points.back()), true);
            }
            line_points.emplace_back(arc_points.back());
            arc_points.clear();
        }

        std::vector<Geo::MarkedPoint>::iterator it = points2.begin();
        while (it != points2.end())
        {
            if (!it->active || std::find(result.begin(), result.end(), *it) != result.end())
            {
                it = points2.erase(it);
            }
            else
            {
                ++it;
            }
        }
        it = points3.begin();
        while (it != points3.end())
        {
            if (!it->active || std::find(result.begin(), result.end(), *it) != result.end())
            {
                it = points3.erase(it);
            }
            else
            {
                ++it;
            }
        }

        if (std::count_if(points2.cbegin(), points2.cend(), [](const MarkedPoint &p) { return p.value < 0; }) == 0 ||
            std::count_if(points3.cbegin(), points3.cend(), [](const MarkedPoint &p) { return p.value < 0; }) == 0)
        {
            break;
        }

        count0 = points2.size();
        count1 = points3.size();
        count2 = count0 + count1;
    }

    return true;
}


bool Geo::polygon_ellipse_union(const Polygon &polygon, const Ellipse &ellipse, std::vector<Geo::Polyline> &output0,
                                std::vector<Geo::Ellipse> &output1)
{
    if (ellipse.is_arc())
    {
        return false;
    }

    Geo::Polygon polygon0(polygon);
    polygon0.remove_repeated_points();
    polygon0.reorder_points(false);
    std::vector<MarkedPoint> points0, points1;
    for (const Point &point : polygon0)
    {
        points0.emplace_back(point.x, point.y);
    }
    const Geo::Point center(ellipse.center());
    const double a = ellipse.lengtha(), b = ellipse.lengthb(), rad = ellipse.angle();

    // 找到交点并计算其几何数
    for (size_t i = 1, count0 = points0.size(); i < count0; ++i)
    {
        Geo::Point point0, point1, pre_point = points0[i - 1];
        switch (Geo::is_intersected(pre_point, points0[i], ellipse, point0, point1, false))
        {
        case 2:
            if (Geo::distance(point0, pre_point) > Geo::distance(point1, pre_point))
            {
                std::swap(point0, point1);
            }
            points0.insert(points0.begin() + i++, MarkedPoint(point1.x, point1.y, false));
            points1.emplace_back(point1.x, point1.y, false);
            ++count0;
            if (Geo::cross(Geo::Point(points0[i]) - pre_point, ellipse.angle_tangency(Geo::angle(center, point1) - rad)) >= 0)
            {
                points0[i - 1].value = 1;
                points1.back().value = -1;
            }
            else
            {
                points0[i - 1].value = -1;
                points1.back().value = 1;
            }
        case 1:
            points0.insert(points0.begin() + i++, MarkedPoint(point0.x, point0.y, false));
            points1.emplace_back(point0.x, point0.y, false);
            ++count0;
            if (Geo::cross(Geo::Point(points0[i]) - pre_point, ellipse.angle_tangency(Geo::angle(center, point0) - rad)) >= 0)
            {
                points0[i - 1].value = 1;
                points1.back().value = -1;
            }
            else
            {
                points0[i - 1].value = -1;
                points1.back().value = 1;
            }
            break;
        default:
            break;
        }
    }

    if (points1.empty()) // 无交点
    {
        return false;
    }

    // 调整交点顺序,同一条边上的交点按照顺序排列
    sort_polygon_points(points0, polygon.front());
    std::sort(points1.begin(), points1.end(),
              [&](const MarkedPoint &p0, const MarkedPoint &p1) { return Geo::angle(center, p0) < Geo::angle(center, p1); });

    // 去除重复交点
    remove_circle_ellipse_repeated_intersections(points0);
    remove_circle_ellipse_repeated_intersections(points1);

    if (std::count_if(points0.begin(), points0.end(), [](const MarkedPoint &p) { return p.value < 0; }) == 0 ||
        std::count_if(points1.begin(), points1.end(), [](const MarkedPoint &p) { return p.value < 0; }) == 0)
    {
        return false; // 交点都是出点,即两多边形只有一个点相交
    }

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
    output0.clear(), output1.clear();

    while (count0 > 0 && count1 > 0)
    {
        size_t index0 = 0, index1 = 0;
        while (index0 < count0 && points0[index0].value < 1)
        {
            ++index0;
        }
        if (index0 >= count0)
        {
            break;
        }

        std::vector<Point> result, line_points, ellipse_points;
        while (result.size() < count2 && (result.size() < 3 || result.front() != result.back()))
        {
            while (result.size() < count2 && (result.size() < 3 || result.front() != result.back()))
            {
                if (points0[index0].value > -1)
                {
                    if (points0[index0].original)
                    {
                        points0[index0].active = false;
                    }
                    result.emplace_back(points0[index0++]);
                    line_points.emplace_back(result.back());
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
                    line_points.emplace_back(result.back());
                    ++index0;
                    index0 %= count0;
                    index1 %= count1;
                    break;
                }
                index0 %= count0;
            }

            if (line_points.size() > 1)
            {
                output0.emplace_back(line_points.begin(), line_points.end());
            }
            ellipse_points.emplace_back(line_points.back());
            line_points.clear();

            while (result.size() < count2 && (result.size() < 3 || result.front() != result.back()))
            {
                if (points1[index1].value > -1)
                {
                    if (points1[index1].original)
                    {
                        points1[index1].active = false;
                    }
                    result.emplace_back(points1[index1++]);
                    ellipse_points.emplace_back(result.back());
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
                    ellipse_points.emplace_back(result.back());
                    ++index1;
                    index0 %= count0;
                    index1 %= count1;
                    break;
                }
                index1 %= count1;
            }

            if (ellipse_points.size() > 1)
            {
                const double angles[2] = {Geo::angle(center, ellipse_points.front()) - rad,
                                          Geo::angle(center, ellipse_points.back()) - rad};
                output1.emplace_back(center.x, center.y, a, b, angles[0], angles[1], false);
                output1.back().rotate(center.x, center.y, rad);
            }
            line_points.emplace_back(ellipse_points.back());
            ellipse_points.clear();
        }

        std::vector<Geo::MarkedPoint>::iterator it = points0.begin();
        while (it != points0.end())
        {
            if (!it->active || std::find(result.begin(), result.end(), *it) != result.end())
            {
                it = points0.erase(it);
            }
            else
            {
                ++it;
            }
        }
        it = points1.begin();
        while (it != points1.end())
        {
            if (!it->active || std::find(result.begin(), result.end(), *it) != result.end())
            {
                it = points1.erase(it);
            }
            else
            {
                ++it;
            }
        }

        if (std::count_if(points0.cbegin(), points0.cend(), [](const MarkedPoint &p) { return p.value < 0; }) == 0 ||
            std::count_if(points1.cbegin(), points1.cend(), [](const MarkedPoint &p) { return p.value < 0; }) == 0)
        {
            break;
        }

        count0 = points0.size();
        count1 = points1.size();
        count2 = count0 + count1;
    }
    return true;
}

bool Geo::polygon_ellipse_intersection(const Polygon &polygon, const Ellipse &ellipse, std::vector<Geo::Polyline> &output0,
                                       std::vector<Geo::Ellipse> &output1)
{
    if (ellipse.is_arc())
    {
        return false;
    }

    Geo::Polygon polygon0(polygon);
    polygon0.remove_repeated_points();
    polygon0.reorder_points(false);
    std::vector<MarkedPoint> points0, points1;
    for (const Point &point : polygon0)
    {
        points0.emplace_back(point.x, point.y);
    }
    const Geo::Point center(ellipse.center());
    const double a = ellipse.lengtha(), b = ellipse.lengthb(), rad = ellipse.angle();

    // 找到交点并计算其几何数
    for (size_t i = 1, count0 = points0.size(); i < count0; ++i)
    {
        Geo::Point point0, point1, pre_point = points0[i - 1];
        switch (Geo::is_intersected(pre_point, points0[i], ellipse, point0, point1, false))
        {
        case 2:
            if (Geo::distance(point0, pre_point) > Geo::distance(point1, pre_point))
            {
                std::swap(point0, point1);
            }
            points0.insert(points0.begin() + i++, MarkedPoint(point1.x, point1.y, false));
            points1.emplace_back(point1.x, point1.y, false);
            ++count0;
            if (Geo::cross(Geo::Point(points0[i]) - pre_point, ellipse.angle_tangency(Geo::angle(center, point1) - rad)) >= 0)
            {
                points0[i - 1].value = 1;
                points1.back().value = -1;
            }
            else
            {
                points0[i - 1].value = -1;
                points1.back().value = 1;
            }
        case 1:
            points0.insert(points0.begin() + i++, MarkedPoint(point0.x, point0.y, false));
            points1.emplace_back(point0.x, point0.y, false);
            ++count0;
            if (Geo::cross(Geo::Point(points0[i]) - pre_point, ellipse.angle_tangency(Geo::angle(center, point0) - rad)) >= 0)
            {
                points0[i - 1].value = 1;
                points1.back().value = -1;
            }
            else
            {
                points0[i - 1].value = -1;
                points1.back().value = 1;
            }
            break;
        default:
            break;
        }
    }

    if (points1.empty()) // 无交点
    {
        return false;
    }

    // 调整交点顺序,同一条边上的交点按照顺序排列
    sort_polygon_points(points0, polygon.front());
    std::sort(points1.begin(), points1.end(),
              [&](const MarkedPoint &p0, const MarkedPoint &p1) { return Geo::angle(center, p0) < Geo::angle(center, p1); });

    // 去除重复交点
    remove_circle_ellipse_repeated_intersections(points0);
    remove_circle_ellipse_repeated_intersections(points1);

    if (std::count_if(points0.begin(), points0.end(), [](const MarkedPoint &p) { return p.value < 0; }) == 0 ||
        std::count_if(points1.begin(), points1.end(), [](const MarkedPoint &p) { return p.value < 0; }) == 0)
    {
        return false; // 交点都是出点,即两多边形只有一个点相交
    }

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
    output0.clear(), output1.clear();

    while (count0 > 0 && count1 > 0)
    {
        size_t index0 = 0, index1 = 0;
        while (index0 < count0 && points0[index0].value < 1)
        {
            ++index0;
        }
        if (index0 >= count0)
        {
            break;
        }

        std::vector<Point> result, line_points, ellipse_points;
        while (result.size() < count2 && (result.size() < 3 || result.front() != result.back()))
        {
            while (result.size() < count2 && (result.size() < 3 || result.front() != result.back()))
            {
                if (points0[index0].value < 1)
                {
                    if (points0[index0].original)
                    {
                        points0[index0].active = false;
                    }
                    result.emplace_back(points0[index0++]);
                    line_points.emplace_back(result.back());
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
                    line_points.emplace_back(result.back());
                    ++index0;
                    index0 %= count0;
                    index1 %= count1;
                    break;
                }
                index0 %= count0;
            }

            if (line_points.size() > 1)
            {
                output0.emplace_back(line_points.begin(), line_points.end());
            }
            ellipse_points.emplace_back(line_points.back());
            line_points.clear();

            while (result.size() < count2 && (result.size() < 3 || result.front() != result.back()))
            {
                if (points1[index1].value < 1)
                {
                    if (points1[index1].original)
                    {
                        points1[index1].active = false;
                    }
                    result.emplace_back(points1[index1++]);
                    ellipse_points.emplace_back(result.back());
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
                    ellipse_points.emplace_back(result.back());
                    ++index1;
                    index0 %= count0;
                    index1 %= count1;
                    break;
                }
                index1 %= count1;
            }

            if (ellipse_points.size() > 1)
            {
                const double angles[2] = {Geo::angle(center, ellipse_points.front()) - rad,
                                          Geo::angle(center, ellipse_points.back()) - rad};
                output1.emplace_back(center.x, center.y, a, b, angles[0], angles[1], false);
                output1.back().rotate(center.x, center.y, rad);
            }
            line_points.emplace_back(ellipse_points.back());
            ellipse_points.clear();
        }

        std::vector<Geo::MarkedPoint>::iterator it = points0.begin();
        while (it != points0.end())
        {
            if (!it->active || std::find(result.begin(), result.end(), *it) != result.end())
            {
                it = points0.erase(it);
            }
            else
            {
                ++it;
            }
        }
        it = points1.begin();
        while (it != points1.end())
        {
            if (!it->active || std::find(result.begin(), result.end(), *it) != result.end())
            {
                it = points1.erase(it);
            }
            else
            {
                ++it;
            }
        }

        if (std::count_if(points0.cbegin(), points0.cend(), [](const MarkedPoint &p) { return p.value < 0; }) == 0 ||
            std::count_if(points1.cbegin(), points1.cend(), [](const MarkedPoint &p) { return p.value < 0; }) == 0)
        {
            break;
        }

        count0 = points0.size();
        count1 = points1.size();
        count2 = count0 + count1;
    }
    return true;
}

bool Geo::polygon_ellipse_difference(const Polygon &polygon, const Ellipse &ellipse, std::vector<Geo::Polyline> &output0,
                                     std::vector<Geo::Ellipse> &output1)
{
    if (ellipse.is_arc())
    {
        return false;
    }

    Geo::Polygon polygon0(polygon);
    polygon0.remove_repeated_points();
    polygon0.reorder_points(false);
    std::vector<MarkedPoint> points0, points1;
    for (const Point &point : polygon0)
    {
        points0.emplace_back(point.x, point.y);
    }
    const Geo::Point center(ellipse.center());
    const double a = ellipse.lengtha(), b = ellipse.lengthb(), rad = ellipse.angle();

    // 找到交点并计算其几何数
    for (size_t i = 1, count0 = points0.size(); i < count0; ++i)
    {
        Geo::Point point0, point1, pre_point = points0[i - 1];
        switch (Geo::is_intersected(pre_point, points0[i], ellipse, point0, point1, false))
        {
        case 2:
            if (Geo::distance(point0, pre_point) > Geo::distance(point1, pre_point))
            {
                std::swap(point0, point1);
            }
            points0.insert(points0.begin() + i++, MarkedPoint(point1.x, point1.y, false));
            points1.emplace_back(point1.x, point1.y, false);
            ++count0;
            if (Geo::cross(Geo::Point(points0[i]) - pre_point, ellipse.angle_tangency(Geo::angle(center, point1) - rad)) >= 0)
            {
                points0[i - 1].value = 1;
                points1.back().value = -1;
            }
            else
            {
                points0[i - 1].value = -1;
                points1.back().value = 1;
            }
        case 1:
            points0.insert(points0.begin() + i++, MarkedPoint(point0.x, point0.y, false));
            points1.emplace_back(point0.x, point0.y, false);
            ++count0;
            if (Geo::cross(Geo::Point(points0[i]) - pre_point, ellipse.angle_tangency(Geo::angle(center, point0) - rad)) >= 0)
            {
                points0[i - 1].value = 1;
                points1.back().value = -1;
            }
            else
            {
                points0[i - 1].value = -1;
                points1.back().value = 1;
            }
            break;
        default:
            break;
        }
    }

    if (points1.empty()) // 无交点
    {
        return false;
    }

    // 调整交点顺序,同一条边上的交点按照顺序排列
    sort_polygon_points(points0, polygon.front());
    std::sort(points1.begin(), points1.end(),
              [&](const MarkedPoint &p0, const MarkedPoint &p1) { return Geo::angle(center, p0) > Geo::angle(center, p1); }); // 顺时针排序

    // 去除重复交点
    remove_circle_ellipse_repeated_intersections(points0);
    remove_circle_ellipse_repeated_intersections(points1);

    if (std::count_if(points0.begin(), points0.end(), [](const MarkedPoint &p) { return p.value < 0; }) == 0 ||
        std::count_if(points1.begin(), points1.end(), [](const MarkedPoint &p) { return p.value < 0; }) == 0)
    {
        return false; // 交点都是出点,即两多边形只有一个点相交
    }

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
    output0.clear(), output1.clear();

    while (count0 > 0 && count1 > 0)
    {
        size_t index0 = 0, index1 = 0;
        while (index0 < count0 && points0[index0].value < 1)
        {
            ++index0;
        }
        if (index0 >= count0)
        {
            break;
        }

        std::vector<Point> result, line_points, ellipse_points;
        while (result.size() < count2 && (result.size() < 3 || result.front() != result.back()))
        {
            while (result.size() < count2 && (result.size() < 3 || result.front() != result.back()))
            {
                if (points0[index0].value > -1)
                {
                    if (points0[index0].original)
                    {
                        points0[index0].active = false;
                    }
                    result.emplace_back(points0[index0++]);
                    line_points.emplace_back(result.back());
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
                    line_points.emplace_back(result.back());
                    ++index0;
                    index0 %= count0;
                    index1 %= count1;
                    break;
                }
                index0 %= count0;
            }

            if (line_points.size() > 1)
            {
                output0.emplace_back(line_points.begin(), line_points.end());
            }
            ellipse_points.emplace_back(line_points.back());
            line_points.clear();

            while (result.size() < count2 && (result.size() < 3 || result.front() != result.back()))
            {
                if (points1[index1].value > -1)
                {
                    if (points1[index1].original)
                    {
                        points1[index1].active = false;
                    }
                    result.emplace_back(points1[index1++]);
                    ellipse_points.emplace_back(result.back());
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
                    ellipse_points.emplace_back(result.back());
                    ++index1;
                    index0 %= count0;
                    index1 %= count1;
                    break;
                }
                index1 %= count1;
            }

            if (ellipse_points.size() > 1)
            {
                const double angles[2] = {Geo::angle(center, ellipse_points.back()) - rad,
                                          Geo::angle(center, ellipse_points.front()) - rad};
                output1.emplace_back(center.x, center.y, a, b, angles[0], angles[1], false);
                output1.back().rotate(center.x, center.y, rad);
            }
            line_points.emplace_back(ellipse_points.back());
            ellipse_points.clear();
        }

        std::vector<Geo::MarkedPoint>::iterator it = points0.begin();
        while (it != points0.end())
        {
            if (!it->active || std::find(result.begin(), result.end(), *it) != result.end())
            {
                it = points0.erase(it);
            }
            else
            {
                ++it;
            }
        }
        it = points1.begin();
        while (it != points1.end())
        {
            if (!it->active || std::find(result.begin(), result.end(), *it) != result.end())
            {
                it = points1.erase(it);
            }
            else
            {
                ++it;
            }
        }

        if (std::count_if(points0.cbegin(), points0.cend(), [](const MarkedPoint &p) { return p.value < 0; }) == 0 ||
            std::count_if(points1.cbegin(), points1.cend(), [](const MarkedPoint &p) { return p.value < 0; }) == 0)
        {
            break;
        }

        count0 = points0.size();
        count1 = points1.size();
        count2 = count0 + count1;
    }
    return true;
}

bool Geo::ellipse_polygon_difference(const Ellipse &ellipse, const Polygon &polygon, std::vector<Geo::Ellipse> &output0,
                                     std::vector<Geo::Polyline> &output1)
{
    if (ellipse.is_arc())
    {
        return false;
    }

    Geo::Polygon polygon0(polygon);
    polygon0.remove_repeated_points();
    polygon0.reorder_points(true);
    std::vector<MarkedPoint> points0, points1;
    for (const Point &point : polygon0)
    {
        points1.emplace_back(point.x, point.y);
    }
    const Geo::Point center(ellipse.center());
    const double a = ellipse.lengtha(), b = ellipse.lengthb(), rad = ellipse.angle();

    // 找到交点并计算其几何数
    for (size_t i = 1, count1 = points1.size(); i < count1; ++i)
    {
        Geo::Point point0, point1, pre_point = points1[i - 1];
        switch (Geo::is_intersected(pre_point, points1[i], ellipse, point0, point1, false))
        {
        case 2:
            if (Geo::distance(point0, pre_point) > Geo::distance(point1, pre_point))
            {
                std::swap(point0, point1);
            }
            points0.emplace_back(point1.x, point1.y, false);
            points1.insert(points1.begin() + i++, MarkedPoint(point1.x, point1.y, false));
            ++count1;
            if (Geo::cross(ellipse.angle_tangency(Geo::angle(center, point1) - rad), Geo::Point(points1[i]) - pre_point) >= 0)
            {
                points0.back().value = -1;
                points1[i - 1].value = 1;
            }
            else
            {
                points0.back().value = 1;
                points1[i - 1].value = -1;
            }
        case 1:
            points0.emplace_back(point0.x, point0.y, false);
            points1.insert(points1.begin() + i++, MarkedPoint(point0.x, point0.y, false));
            ++count1;
            if (Geo::cross(ellipse.angle_tangency(Geo::angle(center, point0) - rad), Geo::Point(points1[i]) - pre_point) >= 0)
            {
                points0.back().value = -1;
                points1[i - 1].value = 1;
            }
            else
            {
                points0.back().value = 1;
                points1[i - 1].value = -1;
            }
            break;
        default:
            break;
        }
    }

    if (points0.empty()) // 无交点
    {
        return false;
    }

    // 调整交点顺序,同一条边上的交点按照顺序排列
    sort_polygon_points(points1, polygon.front());
    std::sort(points0.begin(), points0.end(),
              [&](const MarkedPoint &p0, const MarkedPoint &p1) { return Geo::angle(center, p0) < Geo::angle(center, p1); });

    // 去除重复交点
    remove_circle_ellipse_repeated_intersections(points0);
    remove_circle_ellipse_repeated_intersections(points1);

    if (std::count_if(points0.begin(), points0.end(), [](const MarkedPoint &p) { return p.value < 0; }) == 0 ||
        std::count_if(points1.begin(), points1.end(), [](const MarkedPoint &p) { return p.value < 0; }) == 0)
    {
        return false; // 交点都是出点,即两多边形只有一个点相交
    }

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
    output0.clear(), output1.clear();

    while (count0 > 0 && count1 > 0)
    {
        size_t index0 = 0, index1 = 0;
        while (index0 < count0 && points0[index0].value < 1)
        {
            ++index0;
        }
        if (index0 >= count0)
        {
            break;
        }

        std::vector<Point> result, ellipse_points, line_points;
        while (result.size() < count2 && (result.size() < 3 || result.front() != result.back()))
        {
            while (result.size() < count2 && (result.size() < 3 || result.front() != result.back()))
            {
                if (points0[index0].value > -1)
                {
                    if (points0[index0].original)
                    {
                        points0[index0].active = false;
                    }
                    result.emplace_back(points0[index0++]);
                    ellipse_points.emplace_back(result.back());
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
                    ellipse_points.emplace_back(result.back());
                    ++index0;
                    index0 %= count0;
                    index1 %= count1;
                    break;
                }
                index0 %= count0;
            }

            if (ellipse_points.size() > 1)
            {
                const double angles[2] = {Geo::angle(center, ellipse_points.front()) - rad,
                                          Geo::angle(center, ellipse_points.back()) - rad};
                output0.emplace_back(center.x, center.y, a, b, angles[0], angles[1], false);
                output0.back().rotate(center.x, center.y, rad);
            }
            line_points.emplace_back(ellipse_points.back());
            ellipse_points.clear();

            while (result.size() < count2 && (result.size() < 3 || result.front() != result.back()))
            {
                if (points1[index1].value > -1)
                {
                    if (points1[index1].original)
                    {
                        points1[index1].active = false;
                    }
                    result.emplace_back(points1[index1++]);
                    line_points.emplace_back(result.back());
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
                    line_points.emplace_back(result.back());
                    ++index1;
                    index0 %= count0;
                    index1 %= count1;
                    break;
                }
                index1 %= count1;
            }

            if (line_points.size() > 1)
            {
                output1.emplace_back(line_points.begin(), line_points.end());
            }
            ellipse_points.emplace_back(line_points.back());
            line_points.clear();
        }

        std::vector<Geo::MarkedPoint>::iterator it = points0.begin();
        while (it != points0.end())
        {
            if (!it->active || std::find(result.begin(), result.end(), *it) != result.end())
            {
                it = points0.erase(it);
            }
            else
            {
                ++it;
            }
        }
        it = points1.begin();
        while (it != points1.end())
        {
            if (!it->active || std::find(result.begin(), result.end(), *it) != result.end())
            {
                it = points1.erase(it);
            }
            else
            {
                ++it;
            }
        }

        if (std::count_if(points0.cbegin(), points0.cend(), [](const MarkedPoint &p) { return p.value < 0; }) == 0 ||
            std::count_if(points1.cbegin(), points1.cend(), [](const MarkedPoint &p) { return p.value < 0; }) == 0)
        {
            break;
        }

        count0 = points0.size();
        count1 = points1.size();
        count2 = count0 + count1;
    }
    return true;
}

bool Geo::polygon_ellipse_xor(const Polygon &polygon, const Ellipse &ellipse, std::vector<Geo::Polyline> &output0,
                              std::vector<Geo::Ellipse> &output1)
{
    if (ellipse.is_arc())
    {
        return false;
    }

    Geo::Polygon polygon0(polygon);
    polygon0.remove_repeated_points();
    polygon0.reorder_points(true);
    std::vector<MarkedPoint> points0, points1;
    for (const Point &point : polygon0)
    {
        points0.emplace_back(point.x, point.y);
    }
    const Geo::Point center(ellipse.center());
    const double a = ellipse.lengtha(), b = ellipse.lengthb(), rad = ellipse.angle();

    // 找到交点并计算其几何数
    for (size_t i = 1, count0 = points0.size(); i < count0; ++i)
    {
        Geo::Point point0, point1, pre_point = points0[i - 1];
        switch (Geo::is_intersected(pre_point, points0[i], ellipse, point0, point1, false))
        {
        case 2:
            if (Geo::distance(point0, pre_point) > Geo::distance(point1, pre_point))
            {
                std::swap(point0, point1);
            }
            points0.insert(points0.begin() + i++, MarkedPoint(point1.x, point1.y, false));
            points1.emplace_back(point1.x, point1.y, false);
            ++count0;
            if (Geo::cross(Geo::Point(points0[i]) - pre_point, ellipse.angle_tangency(Geo::angle(center, point1) - rad)) >= 0)
            {
                points0[i - 1].value = 1;
                points1.back().value = -1;
            }
            else
            {
                points0[i - 1].value = -1;
                points1.back().value = 1;
            }
        case 1:
            points0.insert(points0.begin() + i++, MarkedPoint(point0.x, point0.y, false));
            points1.emplace_back(point0.x, point0.y, false);
            ++count0;
            if (Geo::cross(Geo::Point(points0[i]) - pre_point, ellipse.angle_tangency(Geo::angle(center, point0) - rad)) >= 0)
            {
                points0[i - 1].value = 1;
                points1.back().value = -1;
            }
            else
            {
                points0[i - 1].value = -1;
                points1.back().value = 1;
            }
            break;
        default:
            break;
        }
    }

    if (points1.empty()) // 无交点
    {
        return false;
    }

    // 调整交点顺序,同一条边上的交点按照顺序排列
    sort_polygon_points(points0, polygon.front());
    std::sort(points1.begin(), points1.end(),
              [&](const MarkedPoint &p0, const MarkedPoint &p1) { return Geo::angle(center, p0) < Geo::angle(center, p1); });

    // 去除重复交点
    remove_circle_ellipse_repeated_intersections(points0);
    remove_circle_ellipse_repeated_intersections(points1);

    if (std::count_if(points0.begin(), points0.end(), [](const MarkedPoint &p) { return p.value < 0; }) == 0 ||
        std::count_if(points1.begin(), points1.end(), [](const MarkedPoint &p) { return p.value < 0; }) == 0)
    {
        return false; // 交点都是出点,即两多边形只有一个点相交
    }

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
    output0.clear(), output1.clear();
    std::vector<Geo::MarkedPoint> points2(points0.begin(), points0.end());
    std::vector<Geo::MarkedPoint> points3(points1.begin(), points1.end());
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

    while (count0 > 0 && count1 > 0)
    {
        size_t index0 = 0, index1 = 0;
        while (index0 < count0 && points0[index0].value < 1)
        {
            ++index0;
        }
        if (index0 >= count0)
        {
            break;
        }

        std::vector<Point> result, line_points, ellipse_points;
        while (result.size() < count2 && (result.size() < 3 || result.front() != result.back()))
        {
            while (result.size() < count2 && (result.size() < 3 || result.front() != result.back()))
            {
                if (points0[index0].value > -1)
                {
                    if (points0[index0].original)
                    {
                        points0[index0].active = false;
                    }
                    result.emplace_back(points0[index0++]);
                    line_points.emplace_back(result.back());
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
                    line_points.emplace_back(result.back());
                    ++index0;
                    index0 %= count0;
                    index1 %= count1;
                    break;
                }
                index0 %= count0;
            }

            if (line_points.size() > 1)
            {
                output0.emplace_back(line_points.begin(), line_points.end());
            }
            ellipse_points.emplace_back(line_points.back());
            line_points.clear();

            while (result.size() < count2 && (result.size() < 3 || result.front() != result.back()))
            {
                if (points1[index1].value > -1)
                {
                    if (points1[index1].original)
                    {
                        points1[index1].active = false;
                    }
                    result.emplace_back(points1[index1++]);
                    ellipse_points.emplace_back(result.back());
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
                    ellipse_points.emplace_back(result.back());
                    ++index1;
                    index0 %= count0;
                    index1 %= count1;
                    break;
                }
                index1 %= count1;
            }

            if (ellipse_points.size() > 1)
            {
                const double angles[2] = {Geo::angle(center, ellipse_points.front()) - rad,
                                          Geo::angle(center, ellipse_points.back()) - rad};
                output1.emplace_back(center.x, center.y, a, b, angles[0], angles[1], false);
                output1.back().rotate(center.x, center.y, rad);
            }
            line_points.emplace_back(ellipse_points.back());
            ellipse_points.clear();
        }

        std::vector<Geo::MarkedPoint>::iterator it = points0.begin();
        while (it != points0.end())
        {
            if (!it->active || std::find(result.begin(), result.end(), *it) != result.end())
            {
                it = points0.erase(it);
            }
            else
            {
                ++it;
            }
        }
        it = points1.begin();
        while (it != points1.end())
        {
            if (!it->active || std::find(result.begin(), result.end(), *it) != result.end())
            {
                it = points1.erase(it);
            }
            else
            {
                ++it;
            }
        }

        if (std::count_if(points0.cbegin(), points0.cend(), [](const MarkedPoint &p) { return p.value < 0; }) == 0 ||
            std::count_if(points1.cbegin(), points1.cend(), [](const MarkedPoint &p) { return p.value < 0; }) == 0)
        {
            break;
        }

        count0 = points0.size();
        count1 = points1.size();
        count2 = count0 + count1;
    }

    count0 = points2.size(), count1 = points3.size();
    count2 = count0 + count1;
    while (count0 > 0 && count1 > 0)
    {
        size_t index0 = 0, index1 = 0;
        while (index0 < count0 && points2[index0].value < 1)
        {
            ++index0;
        }
        if (index0 >= count0)
        {
            break;
        }

        std::vector<Point> result, line_points, ellipse_points;
        while (result.size() < count2 && (result.size() < 3 || result.front() != result.back()))
        {
            while (result.size() < count2 && (result.size() < 3 || result.front() != result.back()))
            {
                if (points2[index0].value > -1)
                {
                    if (points2[index0].original)
                    {
                        points2[index0].active = false;
                    }
                    result.emplace_back(points2[index0++]);
                    line_points.emplace_back(result.back());
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
                    line_points.emplace_back(result.back());
                    ++index0;
                    index0 %= count0;
                    index1 %= count1;
                    break;
                }
                index0 %= count0;
            }

            if (line_points.size() > 1)
            {
                output0.emplace_back(line_points.begin(), line_points.end());
            }
            ellipse_points.emplace_back(line_points.back());
            line_points.clear();

            while (result.size() < count2 && (result.size() < 3 || result.front() != result.back()))
            {
                if (points3[index1].value > -1)
                {
                    if (points3[index1].original)
                    {
                        points3[index1].active = false;
                    }
                    result.emplace_back(points3[index1++]);
                    ellipse_points.emplace_back(result.back());
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
                    ellipse_points.emplace_back(result.back());
                    ++index1;
                    index0 %= count0;
                    index1 %= count1;
                    break;
                }
                index1 %= count1;
            }

            if (ellipse_points.size() > 1)
            {
                const double angles[2] = {Geo::angle(center, ellipse_points.front()) - rad,
                                          Geo::angle(center, ellipse_points.back()) - rad};
                output1.emplace_back(center.x, center.y, a, b, angles[0], angles[1], false);
                output1.back().rotate(center.x, center.y, rad);
            }
            line_points.emplace_back(ellipse_points.back());
            ellipse_points.clear();
        }

        std::vector<Geo::MarkedPoint>::iterator it = points2.begin();
        while (it != points2.end())
        {
            if (!it->active || std::find(result.begin(), result.end(), *it) != result.end())
            {
                it = points2.erase(it);
            }
            else
            {
                ++it;
            }
        }
        it = points3.begin();
        while (it != points3.end())
        {
            if (!it->active || std::find(result.begin(), result.end(), *it) != result.end())
            {
                it = points3.erase(it);
            }
            else
            {
                ++it;
            }
        }

        if (std::count_if(points2.cbegin(), points2.cend(), [](const MarkedPoint &p) { return p.value < 0; }) == 0 ||
            std::count_if(points3.cbegin(), points3.cend(), [](const MarkedPoint &p) { return p.value < 0; }) == 0)
        {
            break;
        }

        count0 = points2.size();
        count1 = points3.size();
        count2 = count0 + count1;
    }

    return true;
}