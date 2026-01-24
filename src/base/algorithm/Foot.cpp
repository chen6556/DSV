#include <algorithm>
#include "base/Algorithm.hpp"
#include "base/Math.hpp"


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
        const double b = start.y - k * start.x;
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

bool Geo::foot_point(const Circle &circle, const Point &point, Point &output0, Point &output1)
{
    if (point == circle || Geo::distance(point, circle) == circle.radius)
    {
        return false;
    }
    const double dx = point.x - circle.x;
    const double dy = point.y - circle.y;
    const double dist = std::hypot(dx, dy);
    output0.x = circle.x + circle.radius * dx / dist;
    output0.y = circle.y + circle.radius * dy / dist;
    output1.x = circle.x - circle.radius * dx / dist;
    output1.y = circle.y - circle.radius * dy / dist;
    return true;
}

int Geo::foot_point(const Ellipse &ellipse, const Point &point, std::vector<Point> &output)
{
    if (Geo::distance(ellipse.c0(), point) + Geo::distance(ellipse.c1(), point) == std::max(ellipse.lengtha(), ellipse.lengthb()) * 2)
    {
        return 0;
    }
    const Geo::Point center = ellipse.center();
    const double angle = Geo::angle(ellipse.a0(), ellipse.a1());
    Geo::Point coord = Geo::to_coord(point, center.x, center.y, angle);
    const double a = Geo::distance(ellipse.a0(), ellipse.a1()) / 2;
    const double b = Geo::distance(ellipse.b0(), ellipse.b1()) / 2;
    const double aa = Geo::distance_square(ellipse.a0(), ellipse.a1()) / 4;
    const double bb = Geo::distance_square(ellipse.b0(), ellipse.b1()) / 4;
    Math::EllipseFootParameter parameter;
    parameter.a = b * coord.y;
    parameter.b = -a * coord.x;
    parameter.c = aa - bb;
    std::vector<double> ts;
    coord = Geo::to_coord(Geo::Point(0, 0), center.x, center.y, angle);
    double values[2] = {parameter.a, 0};
    for (double t = 0.0174; t <= Geo::PI * 2; t += 0.0175)
    {
        values[1] = parameter.a * std::cos(t) + parameter.b * std::sin(t) + parameter.c * std::sin(t) * std::cos(t);
        if (values[0] * values[1] <= 0)
        {
            double t0 = Math::solve_ellipse_foot(parameter, t);
            if (std::abs(parameter.a * std::cos(t0) + parameter.b * std::sin(t0) + parameter.c * std::sin(t0) * std::cos(t0)) >=
                Geo::EPSILON)
            {
                continue;
            }
            t0 = Geo::rad_to_PI(t0);
            if (std::find(ts.begin(), ts.end(), t0) == ts.end())
            {
                ts.push_back(t0);
                Geo::Point result(a * std::cos(t0), b * std::sin(t0));
                output.emplace_back(Geo::to_coord(result, coord.x, coord.y, -angle));
            }
        }
        values[0] = values[1];
    }
    if (ellipse.is_arc())
    {
        for (size_t i = 0, count = output.size(); i < count; ++i)
        {
            if (!Geo::is_inside(output[i], ellipse, true))
            {
                output.erase(output.begin() + i--);
                --count;
                ts.pop_back();
            }
        }
    }
    return ts.size();
}

int Geo::foot_point(const Point &point, const Bezier &bezier, std::vector<Point> &output,
                    std::vector<std::tuple<size_t, double, double, double>> *tvalues)
{
    const int order = bezier.order();
    std::vector<int> nums(order, 1);
    if (order == 3)
    {
        nums[1] = 2;
    }
    std::vector<int> nums1(order + 1, 1);
    if (order == 2)
    {
        nums1[1] = 2;
    }
    else
    {
        nums1[1] = nums1[2] = 3;
    }

    std::vector<Geo::Point> result;
    std::vector<std::tuple<size_t, double, double, double>> temp;
    for (size_t i = 0, end = bezier.size() - order; i < end; i += order)
    {
        double t0 = 0, t1 = Geo::Bezier::default_step;
        double angles[2] = {1, 1};
        angles[0] = Geo::angle(bezier[i], point, bezier[i], bezier[i + 1]);
        std::vector<std::tuple<double, double>> pairs;
        while (t1 <= 1)
        {
            Geo::Point head, tail;
            for (int j = 0; j < order; ++j)
            {
                head += (bezier[i + j] * (nums[j] * std::pow(1 - t1, order - j - 1) * std::pow(t1, j)));
                tail += (bezier[i + j + 1] * (nums[j] * std::pow(1 - t1, order - j - 1) * std::pow(t1, j)));
            }
            Geo::Point coord;
            for (int j = 0; j <= order; ++j)
            {
                coord += (bezier[j + i] * (nums1[j] * std::pow(1 - t1, order - j) * std::pow(t1, j)));
            }
            head.rotate(coord.x, coord.y, Geo::PI / 2);
            tail.rotate(coord.x, coord.y, Geo::PI / 2);
            angles[1] = Geo::cross(coord, point, head, tail);
            if (angles[0] * angles[1] <= 0)
            {
                pairs.emplace_back(t0, t1);
            }
            angles[0] = angles[1];
            t0 = t1;
            t1 += Geo::Bezier::default_step;
        }
        for (auto [t0, t1] : pairs)
        {
            double last_t = -1;
            {
                Geo::Point coord;
                for (int j = 0; j <= order; ++j)
                {
                    coord += (bezier[i + j] * (nums1[j] * std::pow(1 - t0, order - j) * std::pow(t0, j)));
                }
                Geo::Point head, tail;
                for (int j = 0; j < order; ++j)
                {
                    head += (bezier[i + j] * (nums[j] * std::pow(1 - t0, order - j - 1) * std::pow(t0, j)));
                    tail += (bezier[i + j + 1] * (nums[j] * std::pow(1 - t0, order - j - 1) * std::pow(t0, j)));
                }
                head.rotate(coord.x, coord.y, Geo::PI / 2);
                tail.rotate(coord.x, coord.y, Geo::PI / 2);
                angles[0] = Geo::cross(coord, point, head, tail);
            }
            {
                Geo::Point coord;
                for (int j = 0; j <= order; ++j)
                {
                    coord += (bezier[i + j] * (nums1[j] * std::pow(1 - t1, order - j) * std::pow(t1, j)));
                }
                Geo::Point head, tail;
                for (int j = 0; j < order; ++j)
                {
                    head += (bezier[i + j] * (nums[j] * std::pow(1 - t1, order - j - 1) * std::pow(t1, j)));
                    tail += (bezier[i + j + 1] * (nums[j] * std::pow(1 - t1, order - j - 1) * std::pow(t1, j)));
                }
                head.rotate(coord.x, coord.y, Geo::PI / 2);
                tail.rotate(coord.x, coord.y, Geo::PI / 2);
                angles[1] = Geo::cross(coord, point, head, tail);
            }
            double t = (t0 + t1) / 2;
            while (t1 - t0 > 1e-14 && last_t != t && angles[0] * angles[1] != 0)
            {
                Geo::Point coord;
                for (int j = 0; j <= order; ++j)
                {
                    coord += (bezier[i + j] * (nums1[j] * std::pow(1 - t, order - j) * std::pow(t, j)));
                }
                Geo::Point head, tail;
                for (int j = 0; j < order; ++j)
                {
                    head += (bezier[i + j] * (nums[j] * std::pow(1 - t, order - j - 1) * std::pow(t, j)));
                    tail += (bezier[i + j + 1] * (nums[j] * std::pow(1 - t, order - j - 1) * std::pow(t, j)));
                }
                head.rotate(coord.x, coord.y, Geo::PI / 2);
                tail.rotate(coord.x, coord.y, Geo::PI / 2);
                const double angle = Geo::angle(coord, point, head, tail);
                if (angle * angles[0] <= 0)
                {
                    t1 = t;
                    angles[1] = angle;
                }
                else if (angle * angles[1] <= 0)
                {
                    t0 = t;
                    angles[0] = angle;
                }
                last_t = t;
                t = (t0 + t1) / 2;
            }

            if (angles[0] == 0)
            {
                t1 = t0;
            }
            else if (angles[1] != 0)
            {
                t1 = (t0 + t1) / 2;
            }
            Geo::Point coord;
            for (int j = 0; j <= order; ++j)
            {
                coord += (bezier[i + j] * (nums1[j] * std::pow(1 - t1, order - j) * std::pow(t1, j)));
            }
            Geo::Point head, tail;
            for (int j = 0; j < order; ++j)
            {
                head += (bezier[i + j] * (nums[j] * std::pow(1 - t1, order - j - 1) * std::pow(t1, j)));
                tail += (bezier[i + j + 1] * (nums[j] * std::pow(1 - t1, order - j - 1) * std::pow(t1, j)));
            }
            head.rotate(coord.x, coord.y, Geo::PI / 2);
            tail.rotate(coord.x, coord.y, Geo::PI / 2);
            if (std::abs(Geo::angle(coord, point, head, tail)) < 1e-4 ||
                std::abs(std::abs(Geo::angle(coord, point, head, tail)) - Geo::PI) < 1e-4)
            {
                result.emplace_back(coord);
                temp.emplace_back(i, t1, coord.x, coord.y);
            }
        }
    }

    output.insert(output.end(), result.begin(), result.end());
    if (tvalues != nullptr)
    {
        tvalues->insert(tvalues->end(), temp.begin(), temp.end());
    }
    return result.size();
}

int Geo::foot_point(const Point &point, const BSpline &bspline, std::vector<Point> &output,
                    std::vector<std::tuple<double, double, double>> *tvalues)
{
    std::vector<std::tuple<double, double>> pairs;
    {
        const Geo::Point origin(0, 0);
        const std::vector<double> &knots = bspline.knots();
        double t0 = knots[0], t1 = knots[0] + 1e-3;
        double angles[2] = {Geo::angle(bspline.at(t0), point, origin, bspline.vertical(t0)), 1};
        while (t1 <= knots.back())
        {
            angles[1] = Geo::angle(bspline.at(t1), point, origin, bspline.vertical(t1));
            if (angles[0] * angles[1] <= 0)
            {
                pairs.emplace_back(t0, t1);
            }
            angles[0] = angles[1];
            t0 = t1;
            t1 += 1e-3;
        }
    }
    std::vector<Geo::Point> result;
    std::vector<std::tuple<double, double, double>> temp;
    for (auto [t0, t1] : pairs)
    {
        const Geo::Point origin(0, 0);
        double angles[2] = {Geo::angle(bspline.at(t0), point, origin, bspline.vertical(t0)),
                            Geo::angle(bspline.at(t1), point, origin, bspline.vertical(t1))};
        double last_t = -1, t = (t0 + t1) / 2;
        while (t1 - t0 > 1e-14 && last_t != t && angles[0] * angles[1] != 0)
        {
            const double angle = Geo::angle(bspline.at(t), point, origin, bspline.vertical(t));
            if (angle * angles[0] <= 0)
            {
                t1 = t;
                angles[1] = angle;
            }
            else if (angle * angles[1] <= 0)
            {
                t0 = t;
                angles[0] = angle;
            }
            last_t = t;
            t = (t0 + t1) / 2;
        }

        if (angles[0] == 0)
        {
            t1 = t0;
        }
        else if (angles[1] != 0)
        {
            t1 = (t0 + t1) / 2;
        }
        if (const Geo::Point coord = bspline.at(t1);
            std::abs(Geo::angle(coord, point, origin, bspline.vertical(t1))) < 1e-4 ||
            std::abs(std::abs(Geo::angle(coord, point, origin, bspline.vertical(t1))) - Geo::PI) < 1e-4)
        {
            result.emplace_back(coord);
            temp.emplace_back(t1, coord.x, coord.y);
        }
    }

    output.insert(output.end(), result.begin(), result.end());
    if (tvalues != nullptr)
    {
        tvalues->insert(tvalues->end(), temp.begin(), temp.end());
    }
    return result.size();
}
