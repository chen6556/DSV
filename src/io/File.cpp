#include <fstream>

#include "io/File.hpp"
#include "io/GlobalSetting.hpp"
#include "base/Algorithm.hpp"


void File::write_dsv(const std::string &path, const Graph *graph)
{
    const Geo::Polygon *polygon = nullptr;
    const Geo::Circle *circle = nullptr;
    const Geo::Ellipse *ellipse = nullptr;
    const Text *text = nullptr;
    const Geo::Polyline *polyline = nullptr;
    const Geo::Bezier *bezier = nullptr;
    const Geo::BSpline *bspline = nullptr;

    std::ofstream output(path);
    for (const ContainerGroup &group : graph->container_groups())
    {
        if (group.name.isEmpty())
        {
            output << "GROUP" << std::endl;
        }
        else
        {
            output << "GROUP<" << group.name.toStdString() << '>' << std::endl;
        }

        for (const Geo::Geometry *geo : group)
        {
            switch (geo->type())
            {
            case Geo::Type::TEXT:
                text = dynamic_cast<const Text *>(geo);
                output << "TEXT<" << text->text().toStdString() << '>' << std::endl;
                output << text->center().x << ',' << text->center().y << std::endl;
                text = nullptr;
                break;
            case Geo::Type::POLYGON:
                polygon = dynamic_cast<const Geo::Polygon *>(geo);
                output << "POLYGON" << std::endl;
                for (const Geo::Point &point : *polygon)
                {
                    output << point.x << ',' << point.y << ',';
                }
                output.seekp(-1, std::ios::cur);
                output << std::endl;
                polygon = nullptr;
                break;
            case Geo::Type::CIRCLE:
                circle = dynamic_cast<const Geo::Circle *>(geo);
                output << "CIRCLE" << std::endl;
                output << circle->x << ',';
                output << circle->y << ',';
                output << circle->radius;
                output << std::endl;
                circle = nullptr;
                break;
            case Geo::Type::ELLIPSE:
                ellipse = dynamic_cast<const Geo::Ellipse *>(geo);
                output << "ELLIPSE" << std::endl;
                output << ellipse->a0().x << ',' << ellipse->a0().y << ',';
                output << ellipse->a1().x << ',' << ellipse->a1().y << ',';
                output << ellipse->b0().x << ',' << ellipse->b0().y << ',';
                output << ellipse->b1().x << ',' << ellipse->b1().y << std::endl;
                ellipse = nullptr;
                break;
            case Geo::Type::COMBINATION:
                output << "COMBINATION" << std::endl;
                for (const Geo::Geometry *item : *dynamic_cast<const Combination *>(geo))
                {
                    switch (item->type())
                    {
                    case Geo::Type::TEXT:
                        text = dynamic_cast<const Text *>(item);
                        output << "TEXT<" << text->text().toStdString() << '>' << std::endl;
                        output << text->center().x << ',' << text->center().y;
                        text = nullptr;
                        break;
                    case Geo::Type::POLYGON:
                        polygon = dynamic_cast<const Geo::Polygon *>(item);
                        output << "POLYGON" << std::endl;
                        for (const Geo::Point &point : *polygon)
                        {
                            output << point.x << ',' << point.y << ',';
                        }
                        output.seekp(-1, std::ios::cur);
                        output << std::endl;
                        polygon = nullptr;
                        break;
                    case Geo::Type::CIRCLE:
                        circle = dynamic_cast<const Geo::Circle *>(item);
                        output << "CIRCLE" << std::endl;
                        output << circle->x << ',';
                        output << circle->y << ',';
                        output << circle->radius;
                        output << std::endl;
                        circle = nullptr;
                        break;
                    case Geo::Type::ELLIPSE:
                        ellipse = dynamic_cast<const Geo::Ellipse *>(item);
                        output << "ELLIPSE" << std::endl;
                        output << ellipse->a0().x << ',' << ellipse->a0().y << ',';
                        output << ellipse->a1().x << ',' << ellipse->a1().y << ',';
                        output << ellipse->b0().x << ',' << ellipse->b0().y << ',';
                        output << ellipse->b1().x << ',' << ellipse->b1().y << std::endl;
                        ellipse = nullptr;
                        break;
                    case Geo::Type::POLYLINE:
                        polyline = dynamic_cast<const Geo::Polyline *>(item);
                        if (polyline->empty())
                        {
                            continue;
                        }
                        output << "POLYLINE" << std::endl;
                        for (const Geo::Point &point : *polyline)
                        {
                            output << point.x << ',' << point.y << ',';
                        }
                        output.seekp(-1, std::ios_base::cur);
                        output << std::endl;
                        polyline = nullptr;
                        break;
                    case Geo::Type::BEZIER:
                        bezier = dynamic_cast<const Geo::Bezier *>(item);
                        if (bezier->empty())
                        {
                            continue;
                        }
                        output << "BEZIER" << std::endl;
                        output << bezier->order();
                        for (const Geo::Point &point : *bezier)
                        {
                            output << ',' << point.x << ',' << point.y;
                        }
                        output << std::endl;
                        bezier = nullptr;
                        break;
                    case Geo::Type::BSPLINE:
                        bspline = dynamic_cast<const Geo::BSpline *>(item);
                        if (bspline->empty())
                        {
                            continue;
                        }
                        output << "BSPLINE" << std::endl;
                        output << (dynamic_cast<const Geo::CubicBSpline *>(bspline) == nullptr ? '2' : '3');
                        for (const Geo::Point &point : bspline->path_points)
                        {
                            output << ',' << point.x << ',' << point.y;
                        }
                        output << std::endl;
                        bspline = nullptr;
                        break;
                    default:
                        break;
                    }
                    output << "END" << std::endl;
                }
                break;
            case Geo::Type::POLYLINE:
                polyline = dynamic_cast<const Geo::Polyline *>(geo);
                if (polyline->empty())
                {
                    continue;
                }
                output << "POLYLINE" << std::endl;
                for (const Geo::Point &point : *polyline)
                {
                    output << point.x << ',' << point.y << ',';
                }
                output.seekp(-1, std::ios_base::cur);
                output << std::endl;
                polyline = nullptr;
                break;
            case Geo::Type::BEZIER:
                bezier = dynamic_cast<const Geo::Bezier *>(geo);
                if (bezier->empty())
                {
                    continue;
                }
                output << "BEZIER" << std::endl;
                output << bezier->order();
                for (const Geo::Point &point : *bezier)
                {
                    output << ',' << point.x << ',' << point.y;
                }
                output << std::endl;
                bezier = nullptr;
                break;
            case Geo::Type::BSPLINE:
                bspline = dynamic_cast<const Geo::BSpline *>(geo);
                if (bspline->empty())
                {
                    continue;
                }
                output << "BSPLINE" << std::endl;
                output << (dynamic_cast<const Geo::CubicBSpline *>(bspline) == nullptr ? '2' : '3');
                for (const Geo::Point &point : bspline->path_points)
                {
                    output << ',' << point.x << ',' << point.y;
                }
                output << std::endl;
                bspline = nullptr;
                break;
            default:
                break;
            }
            output << "END" << std::endl;
        }

        output << "END" << std::endl;
    }
    output.close();
}

void File::write_plt(const std::string &path, const Graph *graph)
{
    const Text *text = nullptr;
    const Geo::Polygon *polygon = nullptr;
    const Geo::Circle *circle = nullptr;
    const Geo::Ellipse *ellipse = nullptr;
    const Geo::Polyline *polyline = nullptr;
    const Geo::Bezier *bezier = nullptr;
    const Geo::BSpline *bspline = nullptr;
    const double x_ratio = 40, y_ratio = 40;

    std::ofstream output(path);
    output << "IN;PA;SP1;" << std::endl;
    for (const ContainerGroup &group : graph->container_groups())
    {
        for (const Geo::Geometry *geo : group)
        {
            switch (geo->type())
            {
            case Geo::Type::TEXT:
                text = dynamic_cast<const Text *>(geo);
                output << "PU" << text->center().x * x_ratio << ',' << text->center().y * y_ratio << ";PD";
                output << ";LB" << text->text().toStdString() << ';' << std::endl;
                text = nullptr;
                break;
            case Geo::Type::POLYGON:
                polygon = dynamic_cast<const Geo::Polygon *>(geo);
                output << "PU" << polygon->front().x * x_ratio << ',' << polygon->front().y * y_ratio << ";PD";
                for (const Geo::Point &point : *polygon)
                {
                    output << point.x * x_ratio << ',' << point.y * y_ratio << ',';
                }
                output.seekp(-1, std::ios::cur);
                output << ';' << std::endl;
                polygon = nullptr;
                break;
            case Geo::Type::CIRCLE:
                circle = dynamic_cast<const Geo::Circle *>(geo);
                output << "PU" << circle->x * x_ratio << ',' << circle->y * y_ratio << ';';
                output << "CI" << circle->radius * x_ratio << ';';
                output << std::endl;
                circle = nullptr;
                break;
            case Geo::Type::ELLIPSE:
                ellipse = dynamic_cast<const Geo::Ellipse *>(geo);
                {
                    const Geo::Polygon &points = ellipse->shape();
                    output << "PU" << points.front().x * x_ratio << ',' << points.front().y * y_ratio << ";PD";
                    for (const Geo::Point &point : points)
                    {
                        output << point.x * x_ratio << ',' << point.y * y_ratio << ',';
                    }
                }
                output.seekp(-1, std::ios::cur);
                output << ';' << std::endl;
                ellipse = nullptr;
                break;
            case Geo::Type::COMBINATION:
                output << "Block;" << std::endl;
                for (Geo::Geometry *item : *dynamic_cast<const Combination *>(geo))
                {
                    switch (item->type())
                    {
                    case Geo::Type::TEXT:
                        text = dynamic_cast<const Text *>(geo);
                        output << "PU" << text->center().x * x_ratio << ',' << text->center().y * y_ratio << ";PD";
                        output << ";LB" << text->text().toStdString() << ';' << std::endl;
                        text = nullptr;
                        break;
                    case Geo::Type::POLYGON:
                        polygon = dynamic_cast<const Geo::Polygon *>(item);
                        output << "PU" << polygon->front().x * x_ratio << ',' << polygon->front().y * y_ratio << ";PD";
                        for (const Geo::Point &point : *polygon)
                        {
                            output << point.x * x_ratio << ',' << point.y * y_ratio << ',';
                        }
                        output.seekp(-1, std::ios::cur);
                        output << ';' << std::endl;
                        polygon = nullptr;
                        break;
                    case Geo::Type::CIRCLE:
                        circle = dynamic_cast<const Geo::Circle *>(item);
                        output << "PU" << circle->x * x_ratio << ',' << circle->y * y_ratio << ';';
                        output << "CI" << circle->radius * x_ratio << ';';
                        output << std::endl;
                        circle = nullptr;
                        break;
                    case Geo::Type::ELLIPSE:
                        ellipse = dynamic_cast<const Geo::Ellipse *>(item);
                        {
                            const Geo::Polygon &points = ellipse->shape();
                            output << "PU" << points.front().x * x_ratio << ',' << points.front().y * y_ratio << ";PD";
                            for (const Geo::Point &point : points)
                            {
                                output << point.x * x_ratio << ',' << point.y * y_ratio << ',';
                            }
                        }
                        output.seekp(-1, std::ios::cur);
                        output << ';' << std::endl;
                        ellipse = nullptr;
                        break;
                    case Geo::Type::POLYLINE:
                        polyline = dynamic_cast<const Geo::Polyline *>(item);
                        if (polyline->empty())
                        {
                            break;
                        }
                        output << "PU" << polyline->front().x * x_ratio << ',' << polyline->front().y * y_ratio << ";PD";
                        for (const Geo::Point &point : *polyline)
                        {
                            output << point.x * y_ratio << ',' << point.y * y_ratio << ',';
                        }
                        output.seekp(-1, std::ios::cur);
                        output << ';' << std::endl;
                        polyline = nullptr;
                        break;
                    case Geo::Type::BEZIER:
                        bezier = dynamic_cast<const Geo::Bezier *>(item);
                        if (bezier->empty())
                        {
                            break;
                        }
                        if (bezier->order() == 3)
                        {
                            output << "PU" << bezier->front().x * x_ratio << ',' << bezier->front().y * y_ratio << ";BZ";
                            for (const Geo::Point &point : *bezier)
                            {
                                output << point.x * x_ratio << ',' << point.y * y_ratio << ',';
                            }
                        }
                        else
                        {
                            output << "PU" << bezier->front().x * x_ratio << ',' << bezier->front().y * y_ratio << ";PD";
                            for (const Geo::Point &point : bezier->shape())
                            {
                                output << point.x * x_ratio << ',' << point.y * y_ratio << ',';
                            }
                        }
                        output.seekp(-1, std::ios::cur);
                        output << ';' << std::endl;
                        bezier = nullptr;
                        break;
                    case Geo::Type::BSPLINE:
                        bspline = dynamic_cast<const Geo::BSpline *>(item);
                        if (bspline->empty())
                        {
                            break;
                        }
                        output << "PU" << bspline->front().x * x_ratio << ',' << bspline->front().y * y_ratio << ";PD";
                        for (const Geo::Point &point : bspline->shape())
                        {
                            output << point.x * x_ratio << ',' << point.y * y_ratio << ',';
                        }
                        output.seekp(-1, std::ios::cur);
                        output << ';' << std::endl;
                        bspline = nullptr;
                        break;
                    default:
                        break;
                    }
                }
                output << "BlockEnd;" << std::endl;
                break;
            case Geo::Type::POLYLINE:
                polyline = dynamic_cast<const Geo::Polyline *>(geo);
                if (polyline->empty())
                {
                    break;
                }
                output << "PU" << polyline->front().x * x_ratio << ',' << polyline->front().y * y_ratio << ";PD";
                for (const Geo::Point &point : *polyline)
                {
                    output << point.x * x_ratio << ',' << point.y * y_ratio << ',';
                }
                output.seekp(-1, std::ios::cur);
                output << ';' << std::endl;
                polyline = nullptr;
                break;
            case Geo::Type::BEZIER:
                bezier = dynamic_cast<const Geo::Bezier *>(geo);
                if (bezier->empty())
                {
                    break;
                }
                if (bezier->order() == 3)
                {
                    output << "PU" << bezier->front().x * x_ratio << ',' << bezier->front().y * y_ratio << ";BZ";
                    for (const Geo::Point &point : *bezier)
                    {
                        output << point.x * x_ratio << ',' << point.y * y_ratio << ',';
                    }
                }
                else
                {
                    output << "PU" << bezier->front().x * x_ratio << ',' << bezier->front().y * y_ratio << ";PD";
                    for (const Geo::Point &point : bezier->shape())
                    {
                        output << point.x * x_ratio << ',' << point.y * y_ratio << ',';
                    }
                }
                output.seekp(-1, std::ios::cur);
                output << ';' << std::endl;
                bezier = nullptr;
                break;
            case Geo::Type::BSPLINE:
                bspline = dynamic_cast<const Geo::BSpline *>(geo);
                if (bspline->empty())
                {
                    break;
                }
                output << "PU" << bspline->front().x * x_ratio << ',' << bspline->front().y * y_ratio << ";PD";
                for (const Geo::Point &point : bspline->shape())
                {
                    output << point.x * x_ratio << ',' << point.y * y_ratio << ',';
                }
                output.seekp(-1, std::ios::cur);
                output << ';' << std::endl;
                bspline = nullptr;
                break;
            default:
                break;
            }
        }
    }
    output << "PU;";
    output.close();
}

void File::write(const QString &path, const Graph *graph, const FileType type)
{
    switch (type)
    {
    case FileType::DSV:
        write_dsv(path.toLocal8Bit().toStdString(), graph);
        break;
    case FileType::PLT:
        write_plt(path.toLocal8Bit().toStdString(), graph);
        break;
    default:
        break;
    }
}