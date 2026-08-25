#include "ExtendClip.hpp"
#include "Split.hpp"


bool Geo::clip(const CubicBezier &bezier, const bool head, const double length, CubicBezier &result)
{
    if (length < 0 || bezier.length() <= length)
    {
        return false;
    }
    if (length == 0)
    {
        result = bezier;
        return true;
    }
    if (std::vector<std::tuple<size_t, double>> pos; head)
    {   
        if (split(bezier, length, pos))
        {
            CubicBezier *b = bezier.range(std::get<size_t>(pos.front()), std::get<double>(pos.front()),
                                          bezier.control_points.size() / 3 - 1, 1.0);
            result = *b;
            delete b;
        }
        else
        {
            return false;
        }
    }
    else
    {
        std::vector<Point> controls(bezier.control_points.rbegin(), bezier.control_points.rend());
        if (CubicBezier temp(controls.begin(), controls.end(), false); split(temp, length, pos))
        {
            CubicBezier *b = temp.range(std::get<size_t>(pos.front()), std::get<double>(pos.front()),
                                        temp.control_points.size() / 3 - 1, 1.0);
            controls.assign(b->control_points.rbegin(), b->control_points.rend());
            delete b;
            result = CubicBezier(controls.begin(), controls.end(), false);
        }
        else
        {
            return false;
        }
    }
    return true;
}