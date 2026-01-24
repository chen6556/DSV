#include <algorithm>
#include "base/Algorithm.hpp"


bool Geo::tangency_point(const Point &point, const Circle &circle, Point &output0, Point &output1)
{
    if (Geo::distance_square(point, circle) <= std::pow(circle.radius, 2))
    {
        return false;
    }
    const Geo::Point point1(-100,
                            (std::pow(circle.radius, 2) - (point.x - circle.x) * (-100 - circle.x)) / (point.y - circle.y) + circle.y);
    const Geo::Point point2(100, (std::pow(circle.radius, 2) - (point.x - circle.x) * (100 - circle.x)) / (point.y - circle.y) + circle.y);
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
    Geo::Point coord = Geo::to_coord(point, center.x, center.y, angle);
    const double aa = Geo::distance_square(ellipse.a0(), ellipse.a1()) / 4;
    const double bb = Geo::distance_square(ellipse.b0(), ellipse.b1()) / 4;
    const double a1 = coord.x / aa, b1 = coord.y / bb;
    const double a = std::pow(a1, 2) * aa + std::pow(b1, 2) * bb;
    if (b1 != 0)
    {
        const double b = -2 * a1 * aa;
        const double c = (1 - std::pow(b1, 2) * bb) * aa;
        output0.x = (-b - std::sqrt(std::pow(b, 2) - 4 * a * c)) / (2 * a);
        output1.x = (-b + std::sqrt(std::pow(b, 2) - 4 * a * c)) / (2 * a);
        output0.y = (1 - a1 * output0.x) / b1;
        output1.y = (1 - a1 * output1.x) / b1;
    }
    else
    {
        const double b = -2 * b1 * bb;
        const double c = (1 - std::pow(a1, 2) * aa) * bb;
        output0.y = (-b - std::sqrt(std::pow(b, 2) - 4 * a * c)) / (2 * a);
        output1.y = (-b + std::sqrt(std::pow(b, 2) - 4 * a * c)) / (2 * a);
        output0.x = (1 - b1 * output0.y) / a1;
        output1.x = (1 - b1 * output1.y) / a1;
    }
    coord = Geo::to_coord(Geo::Point(0, 0), center.x, center.y, angle);
    output0 = Geo::to_coord(output0, coord.x, coord.y, -angle);
    output1 = Geo::to_coord(output1, coord.x, coord.y, -angle);
    return true;
}

int Geo::tangency_point(const Point &point, const CubicBezier &bezier, std::vector<Point> &output,
                        std::vector<std::tuple<size_t, double, double, double>> *tvalues)
{
    const int order = 3;
    const int nums[3] = {1, 2, 1};
    const int nums1[4] = {1, 3, 3, 1};
    std::vector<Geo::Point> result;
    std::vector<std::tuple<size_t, double, double, double>> temp;
    for (size_t i = 0, end = bezier.size() - order; i < end; i += order)
    {
        double t0 = 0, t1 = Geo::CubicBezier::default_step;
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
            angles[1] = Geo::cross(coord, point, head, tail);
            if (angles[0] * angles[1] <= 0)
            {
                pairs.emplace_back(t0, t1);
            }
            angles[0] = angles[1];
            t0 = t1;
            t1 += Geo::CubicBezier::default_step;
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

int Geo::tangency_point(const Point &point, const BSpline &bspline, std::vector<Point> &output,
                        std::vector<std::tuple<double, double, double>> *tvalues)
{
    std::vector<std::tuple<double, double>> pairs;
    {
        const Geo::Point origin(0, 0);
        const std::vector<double> &knots = bspline.knots();
        double t0 = knots[0], t1 = knots[0] + 1e-3;
        double angles[2] = {Geo::angle(bspline.at(t0), point, origin, bspline.tangent(t0)), 1};
        while (t1 <= knots.back())
        {
            angles[1] = Geo::angle(bspline.at(t1), point, origin, bspline.tangent(t1));
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
        double angles[2] = {Geo::angle(bspline.at(t0), point, origin, bspline.tangent(t0)),
                            Geo::angle(bspline.at(t1), point, origin, bspline.tangent(t1))};
        double last_t = -1, t = (t0 + t1) / 2;
        while (t1 - t0 > 1e-14 && last_t != t && angles[0] * angles[1] != 0)
        {
            const double angle = Geo::angle(bspline.at(t), point, origin, bspline.tangent(t));
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
            std::abs(Geo::angle(coord, point, origin, bspline.tangent(t1))) < 1e-4 ||
            std::abs(std::abs(Geo::angle(coord, point, origin, bspline.tangent(t1))) - Geo::PI) < 1e-4)
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
