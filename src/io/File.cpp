#include "io/File.hpp"
#include "io/GlobalSetting.hpp"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <fstream>


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
                output << text->center().coord().x << ',' << text->center().coord().y << std::endl;
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
                    output << point.coord().x << ',' << point.coord().y << ',';
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
                output << circlecontainer->center().coord().x << ',';
                output << circlecontainer->center().coord().y << ',';
                output << circlecontainer->radius();
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
                        output << text->center().coord().x << ',' << text->center().coord().y;
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
                            output << point.coord().x << ',' << point.coord().y << ',';
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
                        output << circlecontainer->center().coord().x << ',';
                        output << circlecontainer->center().coord().y << ',';
                        output << circlecontainer->radius();
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
                            output << point.coord().x << ',' << point.coord().y << ',';
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
                            output << ',' << point.coord().x << ',' << point.coord().y;
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
            case Geo::POLYLINE:
                polyline = dynamic_cast<const Geo::Polyline *>(geo);
                if (polyline->empty())
                {
                    continue;
                }
                output << "POLYLINE" << std::endl;
                for (const Geo::Point &point : *polyline)
                {
                    output << point.coord().x << ',' << point.coord().y << ',';
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
                    output << ',' << point.coord().x << ',' << point.coord().y;
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
                output << "PU" << text->center().coord().x << ',' << text->center().coord().y << ";PD";
                output << ";LB" << text->text().toStdString() << ';' << std::endl;
                text = nullptr;
                break;
            case Geo::Type::CONTAINER:
                container = dynamic_cast<const Container *>(geo);
                output << "PU" << container->shape().front().coord().x << ',' << container->shape().front().coord().y << ";PD";
                for (const Geo::Point &point : container->shape())
                {
                    output << point.coord().x << ',' << point.coord().y << ',';
                }
                output.seekp(-1, std::ios::cur);
                if (!container->text().isEmpty())
                {
                    output << ";PU" << container->center().coord().x << ',' << container->center().coord().y
                        << ";LB" << container->text().toStdString();
                }
                output << ';' << std::endl;
                container = nullptr;
                break;
            case Geo::Type::CIRCLECONTAINER:
                circlecontainer = dynamic_cast<const CircleContainer *>(geo);
                output << "PA" << circlecontainer->center().coord().x << ',' << circlecontainer->center().coord().y << ';';
                output << "CI" << circlecontainer->radius() << ';';
                if (circlecontainer->text().isEmpty())
                {
                    output << std::endl;
                }
                else
                {
                    output << "PU" << circlecontainer->center().coord().x << ',' << circlecontainer->center().coord().y
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
                        output << "PU" << text->center().coord().x << ',' << text->center().coord().y << ";PD";
                        output << ";LB" << text->text().toStdString() << ';' << std::endl;
                        text = nullptr;
                        break;
                    case Geo::Type::CONTAINER:
                        container = dynamic_cast<const Container *>(item);
                        output << "PU" << container->shape().front().coord().x << ',' << container->shape().front().coord().y << ";PD";
                        for (const Geo::Point &point : container->shape())
                        {
                            output << point.coord().x << ',' << point.coord().y << ',';
                        }
                        output.seekp(-1, std::ios::cur);
                        if (!container->text().isEmpty())
                        {
                            output << ";PU" << container->center().coord().x << ',' << container->center().coord().y
                                << ";LB" << container->text().toStdString();
                        }
                        output << ';' << std::endl;
                        container = nullptr;
                        break;
                    case Geo::Type::CIRCLECONTAINER:
                        circlecontainer = dynamic_cast<const CircleContainer *>(item);
                        output << "PU" << circlecontainer->center().coord().x << ',' << circlecontainer->center().coord().y << ';';
                        output << "CI" << circlecontainer->radius() << ';';
                        if (circlecontainer->text().isEmpty())
                        {
                            output << std::endl;
                        }
                        else
                        {
                            output << "PU" << circlecontainer->center().coord().x << ',' << circlecontainer->center().coord().y
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
                        output << "PU" << polyline->front().coord().x << ',' << polyline->front().coord().y << ";PD";
                        for (const Geo::Point &point : *polyline)
                        {
                            output << point.coord().x << ',' << point.coord().y << ',';
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
                        output << "PU" << bezier->front().coord().x << ',' << bezier->front().coord().y << ";PD";
                        for (const Geo::Point &point : bezier->shape())
                        {
                            output << point.coord().x << ',' << point.coord().y << ',';
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
                output << "PU" << polyline->front().coord().x << ',' << polyline->front().coord().y << ";PD";
                for (const Geo::Point &point : *polyline)
                {
                    output << point.coord().x << ',' << point.coord().y << ',';
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
                output << "PU" << bezier->front().coord().x << ',' << bezier->front().coord().y << ";PD";
                for (const Geo::Point &point : bezier->shape())
                {
                    output << point.coord().x << ',' << point.coord().y << ',';
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