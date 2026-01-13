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
    const Geo::Arc *arc = nullptr;

    std::ofstream output(path);
    for (const ContainerGroup &group : graph->container_groups())
    {
        if (group.name.isEmpty())
        {
            output << "GROUP" << '\n';
        }
        else
        {
            output << "GROUP<" << group.name.toStdString() << '>' << '\n';
        }

        for (const Geo::Geometry *geo : group)
        {
            switch (geo->type())
            {
            case Geo::Type::TEXT:
                text = static_cast<const Text *>(geo);
                output << "TEXT<" << text->text().toStdString() << '>' << '\n';
                output << text->center().x << ',' << text->center().y << '\n';
                text = nullptr;
                break;
            case Geo::Type::POLYGON:
                polygon = static_cast<const Geo::Polygon *>(geo);
                output << "POLYGON" << '\n';
                for (const Geo::Point &point : *polygon)
                {
                    output << point.x << ',' << point.y << ',';
                }
                output.seekp(-1, std::ios::cur);
                output << '\n';
                polygon = nullptr;
                break;
            case Geo::Type::CIRCLE:
                circle = static_cast<const Geo::Circle *>(geo);
                output << "CIRCLE" << '\n';
                output << circle->x << ',';
                output << circle->y << ',';
                output << circle->radius;
                output << '\n';
                circle = nullptr;
                break;
            case Geo::Type::ELLIPSE:
                ellipse = static_cast<const Geo::Ellipse *>(geo);
                output << "ELLIPSE" << '\n';
                output << ellipse->a0().x << ',' << ellipse->a0().y << ',';
                output << ellipse->a1().x << ',' << ellipse->a1().y << ',';
                output << ellipse->b0().x << ',' << ellipse->b0().y << ',';
                output << ellipse->b1().x << ',' << ellipse->b1().y << ',';
                output << ellipse->arc_angle0() << ',' << ellipse->arc_angle1() << '\n';
                ellipse = nullptr;
                break;
            case Geo::Type::COMBINATION:
                output << "COMBINATION" << '\n';
                for (const Geo::Geometry *item : *static_cast<const Combination *>(geo))
                {
                    switch (item->type())
                    {
                    case Geo::Type::TEXT:
                        text = static_cast<const Text *>(item);
                        output << "TEXT<" << text->text().toStdString() << '>' << '\n';
                        output << text->center().x << ',' << text->center().y;
                        text = nullptr;
                        break;
                    case Geo::Type::POLYGON:
                        polygon = static_cast<const Geo::Polygon *>(item);
                        output << "POLYGON" << '\n';
                        for (const Geo::Point &point : *polygon)
                        {
                            output << point.x << ',' << point.y << ',';
                        }
                        output.seekp(-1, std::ios::cur);
                        output << '\n';
                        polygon = nullptr;
                        break;
                    case Geo::Type::CIRCLE:
                        circle = static_cast<const Geo::Circle *>(item);
                        output << "CIRCLE" << '\n';
                        output << circle->x << ',';
                        output << circle->y << ',';
                        output << circle->radius;
                        output << '\n';
                        circle = nullptr;
                        break;
                    case Geo::Type::ELLIPSE:
                        ellipse = static_cast<const Geo::Ellipse *>(item);
                        output << "ELLIPSE" << '\n';
                        output << ellipse->a0().x << ',' << ellipse->a0().y << ',';
                        output << ellipse->a1().x << ',' << ellipse->a1().y << ',';
                        output << ellipse->b0().x << ',' << ellipse->b0().y << ',';
                        output << ellipse->b1().x << ',' << ellipse->b1().y << '\n';
                        ellipse = nullptr;
                        break;
                    case Geo::Type::POLYLINE:
                        polyline = static_cast<const Geo::Polyline *>(item);
                        if (polyline->empty())
                        {
                            continue;
                        }
                        output << "POLYLINE" << '\n';
                        for (const Geo::Point &point : *polyline)
                        {
                            output << point.x << ',' << point.y << ',';
                        }
                        output.seekp(-1, std::ios_base::cur);
                        output << '\n';
                        polyline = nullptr;
                        break;
                    case Geo::Type::BEZIER:
                        bezier = static_cast<const Geo::Bezier *>(item);
                        if (bezier->empty())
                        {
                            continue;
                        }
                        output << "BEZIER" << '\n';
                        output << bezier->order();
                        for (const Geo::Point &point : *bezier)
                        {
                            output << ',' << point.x << ',' << point.y;
                        }
                        output << '\n';
                        bezier = nullptr;
                        break;
                    case Geo::Type::BSPLINE:
                        bspline = static_cast<const Geo::BSpline *>(item);
                        if (bspline->empty())
                        {
                            continue;
                        }
                        output << "BSPLINE" << '\n';
                        output << (dynamic_cast<const Geo::CubicBSpline *>(bspline) == nullptr ? '2' : '3');
                        for (const Geo::Point &point : bspline->path_points)
                        {
                            output << ',' << point.x << ',' << point.y;
                        }
                        output << '\n';
                        bspline = nullptr;
                        break;
                    case Geo::Type::ARC:
                        arc = static_cast<const Geo::Arc *>(item);
                        if (arc->empty())
                        {
                            continue;
                        }
                        output << "ARC" << '\n';
                        for (const Geo::Point &point : arc->control_points)
                        {
                            output << point.x << ',' << point.y << ',';
                        }
                        output.seekp(-1, std::ios_base::cur);
                        output << '\n';
                        arc = nullptr;
                        break;
                    case Geo::Type::POINT:
                        {
                            const Geo::Point *point = static_cast<const Geo::Point *>(item);
                            output << "POINT" << '\n';
                            output << point->x << ',' << point->y << '\n';
                        }
                        break;
                    default:
                        break;
                    }
                    output << "END" << '\n';
                }
                break;
            case Geo::Type::POLYLINE:
                polyline = static_cast<const Geo::Polyline *>(geo);
                if (polyline->empty())
                {
                    continue;
                }
                output << "POLYLINE" << '\n';
                for (const Geo::Point &point : *polyline)
                {
                    output << point.x << ',' << point.y << ',';
                }
                output.seekp(-1, std::ios_base::cur);
                output << '\n';
                polyline = nullptr;
                break;
            case Geo::Type::BEZIER:
                bezier = static_cast<const Geo::Bezier *>(geo);
                if (bezier->empty())
                {
                    continue;
                }
                output << "BEZIER" << '\n';
                output << bezier->order();
                for (const Geo::Point &point : *bezier)
                {
                    output << ',' << point.x << ',' << point.y;
                }
                output << '\n';
                bezier = nullptr;
                break;
            case Geo::Type::BSPLINE:
                bspline = static_cast<const Geo::BSpline *>(geo);
                if (bspline->empty())
                {
                    continue;
                }
                output << "BSPLINE" << '\n';
                output << (dynamic_cast<const Geo::CubicBSpline *>(bspline) == nullptr ? '2' : '3');
                for (const Geo::Point &point : bspline->path_points)
                {
                    output << ',' << point.x << ',' << point.y;
                }
                output << '\n';
                bspline = nullptr;
                break;
            case Geo::Type::ARC:
                arc = static_cast<const Geo::Arc *>(geo);
                if (arc->empty())
                {
                    continue;
                }
                output << "ARC" << '\n';
                for (const Geo::Point &point : arc->control_points)
                {
                    output << point.x << ',' << point.y << ',';
                }
                output.seekp(-1, std::ios_base::cur);
                output << '\n';
                arc = nullptr;
                break;
            case Geo::Type::POINT:
                {
                    const Geo::Point *point = static_cast<const Geo::Point *>(geo);
                    output << "POINT" << '\n';
                    output << point->x << ',' << point->y << '\n';
                }
                break;
            default:
                break;
            }
            output << "END" << '\n';
        }

        output << "END" << '\n';
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
    const Geo::Arc *arc = nullptr;
    const double x_ratio = 40, y_ratio = 40;

    std::ofstream output(path);
    output << "IN;PA;SP1;" << '\n';
    for (const ContainerGroup &group : graph->container_groups())
    {
        for (const Geo::Geometry *geo : group)
        {
            switch (geo->type())
            {
            case Geo::Type::TEXT:
                text = static_cast<const Text *>(geo);
                output << "PU" << text->center().x * x_ratio << ',' << text->center().y * y_ratio << ";PD";
                output << ";LB" << QString(text->text()).remove(';').toStdString() << ';' << '\n';
                text = nullptr;
                break;
            case Geo::Type::POLYGON:
                polygon = static_cast<const Geo::Polygon *>(geo);
                output << "PU" << polygon->front().x * x_ratio << ',' << polygon->front().y * y_ratio << ";PD";
                for (const Geo::Point &point : *polygon)
                {
                    output << point.x * x_ratio << ',' << point.y * y_ratio << ',';
                }
                output.seekp(-1, std::ios::cur);
                output << ';' << '\n';
                polygon = nullptr;
                break;
            case Geo::Type::CIRCLE:
                circle = static_cast<const Geo::Circle *>(geo);
                output << "PU" << circle->x * x_ratio << ',' << circle->y * y_ratio << ';';
                output << "CI" << circle->radius * x_ratio << ';';
                output << '\n';
                circle = nullptr;
                break;
            case Geo::Type::ELLIPSE:
                ellipse = static_cast<const Geo::Ellipse *>(geo);
                {
                    const Geo::Polyline &points = ellipse->shape();
                    output << "PU" << points.front().x * x_ratio << ',' << points.front().y * y_ratio << ";PD";
                    for (const Geo::Point &point : points)
                    {
                        output << point.x * x_ratio << ',' << point.y * y_ratio << ',';
                    }
                }
                output.seekp(-1, std::ios::cur);
                output << ';' << '\n';
                ellipse = nullptr;
                break;
            case Geo::Type::COMBINATION:
                output << "Block;" << '\n';
                for (Geo::Geometry *item : *static_cast<const Combination *>(geo))
                {
                    switch (item->type())
                    {
                    case Geo::Type::TEXT:
                        text = static_cast<const Text *>(geo);
                        output << "PU" << text->center().x * x_ratio << ',' << text->center().y * y_ratio << ";PD";
                        output << ";LB" << QString(text->text()).remove(';').toStdString() << ';' << '\n';
                        text = nullptr;
                        break;
                    case Geo::Type::POLYGON:
                        polygon = static_cast<const Geo::Polygon *>(item);
                        output << "PU" << polygon->front().x * x_ratio << ',' << polygon->front().y * y_ratio << ";PD";
                        for (const Geo::Point &point : *polygon)
                        {
                            output << point.x * x_ratio << ',' << point.y * y_ratio << ',';
                        }
                        output.seekp(-1, std::ios::cur);
                        output << ';' << '\n';
                        polygon = nullptr;
                        break;
                    case Geo::Type::CIRCLE:
                        circle = static_cast<const Geo::Circle *>(item);
                        output << "PU" << circle->x * x_ratio << ',' << circle->y * y_ratio << ';';
                        output << "CI" << circle->radius * x_ratio << ';';
                        output << '\n';
                        circle = nullptr;
                        break;
                    case Geo::Type::ELLIPSE:
                        ellipse = static_cast<const Geo::Ellipse *>(item);
                        {
                            const Geo::Polygon &points = ellipse->shape();
                            output << "PU" << points.front().x * x_ratio << ',' << points.front().y * y_ratio << ";PD";
                            for (const Geo::Point &point : points)
                            {
                                output << point.x * x_ratio << ',' << point.y * y_ratio << ',';
                            }
                        }
                        output.seekp(-1, std::ios::cur);
                        output << ';' << '\n';
                        ellipse = nullptr;
                        break;
                    case Geo::Type::POLYLINE:
                        polyline = static_cast<const Geo::Polyline *>(item);
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
                        output << ';' << '\n';
                        polyline = nullptr;
                        break;
                    case Geo::Type::BEZIER:
                        bezier = static_cast<const Geo::Bezier *>(item);
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
                        output << ';' << '\n';
                        bezier = nullptr;
                        break;
                    case Geo::Type::BSPLINE:
                        bspline = static_cast<const Geo::BSpline *>(item);
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
                        output << ';' << '\n';
                        bspline = nullptr;
                        break;
                    case Geo::Type::ARC:
                        arc = static_cast<const Geo::Arc *>(item);
                        if (arc->empty())
                        {
                            break;
                        }
                        output << "PU" << arc->control_points[0].x * x_ratio << ',' << arc->control_points[0].y * y_ratio << ";AT";
                        output << arc->control_points[1].x * x_ratio << ',' << arc->control_points[1].y * y_ratio << ',';
                        output << arc->control_points[2].x * x_ratio << ',' << arc->control_points[2].y * y_ratio << ';' << '\n';
                        arc = nullptr;
                        break;
                    case Geo::Type::POINT:
                        {
                            const Geo::Point *point = static_cast<const Geo::Point *>(item);
                            output << "PU" << point->x * x_ratio << ',' << point->y * y_ratio << ";PD";
                            output << point->x * x_ratio << ',' << point->y * y_ratio << ';' << '\n';
                        }
                        break;
                    default:
                        break;
                    }
                }
                output << "BlockEnd;" << '\n';
                break;
            case Geo::Type::POLYLINE:
                polyline = static_cast<const Geo::Polyline *>(geo);
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
                output << ';' << '\n';
                polyline = nullptr;
                break;
            case Geo::Type::BEZIER:
                bezier = static_cast<const Geo::Bezier *>(geo);
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
                output << ';' << '\n';
                bezier = nullptr;
                break;
            case Geo::Type::BSPLINE:
                bspline = static_cast<const Geo::BSpline *>(geo);
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
                output << ';' << '\n';
                bspline = nullptr;
                break;
            case Geo::Type::ARC:
                arc = static_cast<const Geo::Arc *>(geo);
                if (arc->empty())
                {
                    break;
                }
                output << "PU" << arc->control_points[0].x * x_ratio << ',' << arc->control_points[0].y * y_ratio << ";AT";
                output << arc->control_points[1].x * x_ratio << ',' << arc->control_points[1].y * y_ratio << ',';
                output << arc->control_points[2].x * x_ratio << ',' << arc->control_points[2].y * y_ratio << ';' << '\n';
                arc = nullptr;
                break;
            case Geo::Type::POINT:
                {
                    const Geo::Point *point = static_cast<const Geo::Point *>(geo);
                    output << "PU" << point->x * x_ratio << ',' << point->y * y_ratio << ";PD";
                    output << point->x * x_ratio << ',' << point->y * y_ratio << ';' << '\n';
                }
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