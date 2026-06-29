#include "base/Algorithm.hpp"


double Geo::archimedean_spiral_length(const double a, const double b, const double theta)
{
    const double t = a + b * theta;
    return (t * std::sqrt(t * t + b * b) / (b + b) + b * std::log(std::abs(t + std::sqrt(t * t + b * b))) / 2) -
           (a * std::sqrt(a * a + b * b) / (b + b) + b * std::log(a + std::sqrt(a * a + b * b)) / 2);
}

std::vector<Geo::Point> Geo::archimedean_spiral_points(const Geo::Point &center, const double inner_radius, const double outer_radius,
                                                       const double step, const size_t turns, const bool clockwise)
{
    assert(inner_radius > 0 && outer_radius > inner_radius && step > 0 && turns > 0);
    const double b = (outer_radius - inner_radius) / (Geo::PI * 2 * turns);
    std::vector<Geo::Point> points;
    points.emplace_back(center.x + inner_radius, center.y);
    const double total_angle = Geo::PI * 2 * turns;
    const double total_length = archimedean_spiral_length(inner_radius, b, total_angle);
    double start_angle = 0;
    for (size_t i = 0, count = total_length / step; i < count; ++i)
    {
        double angle0 = start_angle, angle1 = total_angle;
        const double start_length = archimedean_spiral_length(inner_radius, b, angle0);
        double length = archimedean_spiral_length(inner_radius, b, angle1) - start_length;
        while (std::abs(length - step) > Geo::EPSILON && std::abs(angle1 - angle0) > Geo::EPSILON)
        {
            if (length < step)
            {
                angle0 = (angle0 + angle1) / 2;
            }
            else
            {
                angle1 = (angle0 + angle1) / 2;
            }
            length = archimedean_spiral_length(inner_radius, b, (angle0 + angle1) / 2) - start_length;
        }
        start_angle = (angle0 + angle1) / 2;
        if (clockwise)
        {
            points.emplace_back(center.x + (inner_radius + b * start_angle) * std::cos(-start_angle),
                                center.y + (inner_radius + b * start_angle) * std::sin(-start_angle));
        }
        else
        {
            points.emplace_back(center.x + (inner_radius + b * start_angle) * std::cos(start_angle),
                                center.y + (inner_radius + b * start_angle) * std::sin(start_angle));
        }
    }
    points.emplace_back(center.x + inner_radius + b * total_angle, center.y);
    return points;
}

std::vector<Geo::Point> Geo::archimedean_spiral_points(const Geo::Point &center, const double inner_radius, const double outer_radius,
                                                       const size_t n, const size_t turns, const bool clockwise)
{
    assert(inner_radius > 0 && outer_radius > inner_radius && n > 0 && turns > 0);
    const double b = (outer_radius - inner_radius) / (Geo::PI * 2 * turns);
    std::vector<Geo::Point> points;
    points.emplace_back(center.x + inner_radius, center.y);
    const double total_angle = Geo::PI * 2 * turns;
    const double total_length = archimedean_spiral_length(inner_radius, b, total_angle);
    const double step = total_length / n;
    double start_angle = 0;
    for (size_t i = 0; i < n; ++i)
    {
        double angle0 = start_angle, angle1 = total_angle;
        const double start_length = archimedean_spiral_length(inner_radius, b, angle0);
        double length = archimedean_spiral_length(inner_radius, b, angle1) - start_length;
        while (std::abs(length - step) > Geo::EPSILON && std::abs(angle1 - angle0) > Geo::EPSILON)
        {
            if (length < step)
            {
                angle0 = (angle0 + angle1) / 2;
            }
            else
            {
                angle1 = (angle0 + angle1) / 2;
            }
            length = archimedean_spiral_length(inner_radius, b, (angle0 + angle1) / 2) - start_length;
        }
        start_angle = (angle0 + angle1) / 2;
        if (clockwise)
        {
            points.emplace_back(center.x + (inner_radius + b * start_angle) * std::cos(-start_angle),
                                center.y + (inner_radius + b * start_angle) * std::sin(-start_angle));
        }
        else
        {
            points.emplace_back(center.x + (inner_radius + b * start_angle) * std::cos(start_angle),
                                center.y + (inner_radius + b * start_angle) * std::sin(start_angle));
        }
    }
    points.emplace_back(center.x + inner_radius + b * total_angle, center.y);
    return points;
}

std::vector<Geo::Point> Geo::archimedean_spiral_bezier(const std::vector<Geo::Point> &path)
{
    std::vector<Geo::Point> points;
    points.emplace_back(path.front());
    Geo::Point mid0((path[1] + path[2]) / 2), mid1((path[0] + path[1]) / 2);
    points.emplace_back(mid1 + (mid1 - mid0).normalize() * Geo::distance(mid0, mid1) / 3);
    for (size_t i = 1, count = path.size() - 1; i < count; ++i)
    {
        mid0 = mid1;
        mid1 = (path[i] + path[i + 1]) / 2;
        points.emplace_back(path[i] + (mid0 - mid1).normalize() * Geo::distance(mid0, mid1) / 3);
        points.emplace_back(path[i]);
        points.emplace_back(path[i] + (mid1 - mid0).normalize() * Geo::distance(mid0, mid1) / 3);
    }
    points.emplace_back(mid1 + (mid1 - mid0).normalize() * Geo::distance(mid0, mid1) / 3);
    points.emplace_back(path.back());
    return points;
}