#include <array>
#include <algorithm>
#include <EarCut/EarCut.hpp>
#include "base/Algorithm.hpp"


std::vector<unsigned int> Geo::ear_cut_to_indexs(const Geo::Polygon &polygon)
{
    std::vector<std::vector<std::array<double, 2>>> points;
    points.emplace_back();
    for (const Point &point : polygon)
    {
        points.front().emplace_back(std::array<double, 2>({point.x, point.y}));
    }
    return mapbox::earcut<unsigned int>(points);
}

std::vector<Geo::MarkedPoint> Geo::ear_cut_to_coords(const Geo::Polygon &polygon)
{
    std::vector<MarkedPoint> result;
    for (const unsigned int i : Geo::ear_cut_to_indexs(polygon))
    {
        result.emplace_back(polygon[i].x, polygon[i].y);
    }
    return result;
}

std::vector<Geo::Point> Geo::ear_cut_to_points(const Geo::Polygon &polygon)
{
    std::vector<Point> result;
    for (const unsigned int i : Geo::ear_cut_to_indexs(polygon))
    {
        result.emplace_back(polygon[i]);
    }
    return result;
}

std::vector<Geo::Triangle> Geo::ear_cut_to_triangles(const Geo::Polygon &polygon)
{
    std::vector<unsigned int> indexs;
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
    bool is_ear = false, is_cut = false;
    while (indexs.size() > 3)
    {
        is_cut = false;
        for (size_t pre = 0, cur = 0, nxt = 0, i = 0, count = indexs.size(); i < count; ++i)
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


bool Geo::merge_ear_cut_triangles(const std::vector<Geo::Triangle> &triangles, std::vector<Geo::Polygon> &polygons)
{
    if (triangles.empty())
    {
        return false;
    }

    polygons.clear();
    const size_t triangles_count = triangles.size();
    size_t merged_count = 1, index = 0;
    std::vector<bool> merged(triangles_count, false), current_triangles(triangles_count, false);
    Geo::Polygon points;

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

        for (size_t i = index; i < triangles_count; ++i)
        {
            if (merged[i])
            {
                continue;
            }

            for (size_t j = 1, count = points.size(); j < count; ++j)
            {
                int index0 = -1, index1 = -1;
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
                int index2 = 3 - index0 - index1;

                bool flag = true;
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
