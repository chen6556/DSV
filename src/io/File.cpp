#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <fstream>

#include "io/File.hpp"
#include "io/GlobalSetting.hpp"


void File::write_dsv(const std::string &path, const Graph *graph)
{
    const Container *container = nullptr;
    const CircleContainer *circlecontainer = nullptr;
    const Text *text = nullptr;
    const Geo::Polyline *polyline = nullptr;
    const Geo::Bezier *bezier = nullptr;

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
            case Geo::Type::CONTAINER:
                container = dynamic_cast<const Container *>(geo);
                if (container->text().isEmpty())
                {
                    output << "POLYGON" << std::endl;
                }
                else
                {
                    output << "POLYGON<" << container->text().toStdString() << '>' << std::endl;
                }
                for (const Geo::Point &point : container->shape())
                {
                    output << point.x << ',' << point.y << ',';
                }
                output.seekp(-1, std::ios::cur);
                output << std::endl;
                container = nullptr;
                break;
            case Geo::Type::CIRCLECONTAINER:
                circlecontainer = dynamic_cast<const CircleContainer *>(geo);
                if (circlecontainer->text().isEmpty())
                {
                    output << "CIRCLE" << std::endl;
                }
                else
                {
                    output << "CIRCLE<" << circlecontainer->text().toStdString() << '>' << std::endl;
                }
                output << circlecontainer->x << ',';
                output << circlecontainer->y << ',';
                output << circlecontainer->radius;
                output << std::endl;
                circlecontainer = nullptr;
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
                    case Geo::Type::CONTAINER:
                        container = dynamic_cast<const Container *>(item);
                        if (container->text().isEmpty())
                        {
                            output << "POLYGON" << std::endl;
                        }
                        else
                        {
                            output << "POLYGON<" << container->text().toStdString() << '>' << std::endl;
                        }
                        for (const Geo::Point &point : container->shape())
                        {
                            output << point.x << ',' << point.y << ',';
                        }
                        output.seekp(-1, std::ios::cur);
                        output << std::endl;
                        container = nullptr;
                        break;
                    case Geo::Type::CIRCLECONTAINER:
                        circlecontainer = dynamic_cast<const CircleContainer *>(item);
                        if (circlecontainer->text().isEmpty())
                        {
                            output << "CIRCLE" << std::endl;
                        }
                        else
                        {
                            output << "CIRCLE<" << circlecontainer->text().toStdString() << '>' << std::endl;
                        }
                        output << circlecontainer->x << ',';
                        output << circlecontainer->y << ',';
                        output << circlecontainer->radius;
                        output << std::endl;
                        circlecontainer = nullptr;
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
    const Container *container = nullptr;
    const CircleContainer *circlecontainer = nullptr;
    const Geo::Polyline *polyline = nullptr;
    const Geo::Bezier *bezier = nullptr;
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
            case Geo::Type::CONTAINER:
                container = dynamic_cast<const Container *>(geo);
                output << "PU" << container->shape().front().x * x_ratio << ',' << container->shape().front().y * y_ratio << ";PD";
                for (const Geo::Point &point : container->shape())
                {
                    output << point.x * x_ratio << ',' << point.y * y_ratio << ',';
                }
                output.seekp(-1, std::ios::cur);
                if (!container->text().isEmpty())
                {
                    output << ";PU" << container->center().x * x_ratio<< ',' << container->center().y * y_ratio
                        << ";LB" << container->text().toStdString();
                }
                output << ';' << std::endl;
                container = nullptr;
                break;
            case Geo::Type::CIRCLECONTAINER:
                circlecontainer = dynamic_cast<const CircleContainer *>(geo);
                output << "PU" << circlecontainer->x * x_ratio << ',' << circlecontainer->y * y_ratio << ';';
                output << "CI" << circlecontainer->radius * x_ratio << ';';
                if (circlecontainer->text().isEmpty())
                {
                    output << std::endl;
                }
                else
                {
                    output << "PU" << circlecontainer->x * x_ratio << ',' << circlecontainer->y * y_ratio
                        << ";LB" << circlecontainer->text().toStdString() << ';' << std::endl;
                }
                circlecontainer = nullptr;
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
                    case Geo::Type::CONTAINER:
                        container = dynamic_cast<const Container *>(item);
                        output << "PU" << container->shape().front().x * x_ratio << ',' << container->shape().front().y * y_ratio << ";PD";
                        for (const Geo::Point &point : container->shape())
                        {
                            output << point.x * x_ratio << ',' << point.y * y_ratio << ',';
                        }
                        output.seekp(-1, std::ios::cur);
                        if (!container->text().isEmpty())
                        {
                            output << ";PU" << container->center().x * x_ratio << ',' << container->center().y * y_ratio
                                << ";LB" << container->text().toStdString();
                        }
                        output << ';' << std::endl;
                        container = nullptr;
                        break;
                    case Geo::Type::CIRCLECONTAINER:
                        circlecontainer = dynamic_cast<const CircleContainer *>(item);
                        output << "PU" << circlecontainer->x * x_ratio << ',' << circlecontainer->y * y_ratio << ';';
                        output << "CI" << circlecontainer->radius * x_ratio << ';';
                        if (circlecontainer->text().isEmpty())
                        {
                            output << std::endl;
                        }
                        else
                        {
                            output << "PU" << circlecontainer->x * x_ratio << ',' << circlecontainer->y * y_ratio
                                << ";LB" << circlecontainer->text().toStdString() << ';' << std::endl;
                        }
                        circlecontainer = nullptr;
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
                        const_cast<Geo::Bezier *>(bezier)->update_shape();
                        output << "PU" << bezier->front().x * x_ratio << ',' << bezier->front().y * y_ratio << ";PD";
                        for (const Geo::Point &point : bezier->shape())
                        {
                            output << point.x * x_ratio << ',' << point.y * y_ratio << ',';
                        }
                        output.seekp(-1, std::ios::cur);
                        output << ';' << std::endl;
                        bezier = nullptr;
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
                const_cast<Geo::Bezier *>(bezier)->update_shape();
                output << "PU" << bezier->front().x * x_ratio << ',' << bezier->front().y * y_ratio << ";PD";
                for (const Geo::Point &point : bezier->shape())
                {
                    output << point.x * x_ratio << ',' << point.y * y_ratio << ',';
                }
                output.seekp(-1, std::ios::cur);
                output << ';' << std::endl;
                bezier = nullptr;
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