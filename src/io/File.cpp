#include "io/File.hpp"
#include "io/GlobalSetting.hpp"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <fstream>


void File::read(const QString &path, Graph *graph)
{
    QFile file(path);
    file.open(QIODevice::ReadOnly);
    QJsonParseError jerr;
    QJsonObject groups = QJsonDocument::fromJson(file.readAll(), &jerr).object();
    file.close();
    std::vector<Geo::Point> points;
    graph->clear();
    const int text_size = GlobalSetting::get_instance()->setting()["text_size"].toInt();

    for (int i = 0, count = groups["ContainerGroupCount"].toInt(); i < count; ++i)
    {
        QJsonObject obj = groups[std::to_string(i).c_str()].toObject();
        graph->append_group();

        QJsonArray texts = obj["Texts"].toArray();
        for (QJsonValueConstRef text : texts)
        {
            QJsonArray coordinates = text.toObject()["shape"].toArray();
            graph->back().append(new Text(coordinates[0].toDouble(), coordinates[1].toDouble(),
                text_size, text.toObject()["text"].toString()));
        }
    
        QJsonArray containers = obj["ContainerGroup"].toArray();
        for (QJsonValueConstRef container : containers)
        {
            QJsonArray coordinates = container.toObject()["shape"].toArray();
            for (size_t i = 1, count = coordinates.size(); i < count; i += 2)
            {
                points.push_back(Geo::Point(coordinates[i - 1].toDouble(), coordinates[i].toDouble()));
            }
            graph->back().append(new Container(container.toObject()["text"].toString(), Geo::Polygon(points.cbegin(), points.cend())));
            points.clear();
        }

        QJsonArray circlecontainers = obj["CircleContainerGroup"].toArray();
        for (QJsonValueConstRef container : circlecontainers)
        {
            QJsonArray coordinates = container.toObject()["shape"].toArray();
            if (coordinates.size() < 3)
            {
                continue;
            }
            graph->back().append(new CircleContainer(container.toObject()["text"].toString(), coordinates[0].toDouble(), coordinates[1].toDouble(), coordinates[2].toDouble()));
        }

        QJsonArray polylines = obj["PolylineGroup"].toArray();
        for (QJsonValueConstRef polyline : polylines)
        {
            QJsonArray coordinates = polyline.toObject()["shape"].toArray();
            for (size_t i = 1, count = coordinates.size(); i < count; i += 2)
            {
                points.push_back(Geo::Point(coordinates[i - 1].toDouble(), coordinates[i].toDouble()));
            }
            graph->back().append(new Geo::Polyline(points.cbegin(), points.cend()));
            points.clear();
        }
    
        QJsonArray beziers = obj["BezierGroup"].toArray();
        for (QJsonValueConstRef bezier : beziers)
        {
            QJsonArray coordinates = bezier.toObject()["shape"].toArray();
            for (size_t i = 1, count = coordinates.size(); i < count; i += 2)
            {
                points.push_back(Geo::Point(coordinates[i - 1].toDouble(), coordinates[i].toDouble()));
            }
            graph->back().append(new Geo::Bezier(points.begin(), points.end(), bezier.toObject()["order"].toInt()));
            points.clear();
        }

        graph->back().name = obj["LayerName"].toString();
    }
}

void File::write_json(const QString &path, Graph *graph)
{
    QJsonObject obj;
    Container *container = nullptr;
    CircleContainer *circlecontainer = nullptr;
    Geo::Polyline *polyline = nullptr;
    Geo::Bezier *bezier = nullptr;
    int index = 0;

    for (const ContainerGroup &group : graph->container_groups())
    {
        QJsonArray container_objs, circlecontainer_objs, link_objs, polyline_objs, bezier_objs, text_objs;
        QJsonObject container_group;

        for (Geo::Geometry *geo : group)
        {
            QJsonObject obj2;
            QJsonArray array;
            switch (geo->type())
            {
            case Geo::Type::TEXT:
                array.append(dynamic_cast<Text *>(geo)->center().coord().x);
                array.append(dynamic_cast<Text *>(geo)->center().coord().y);
                obj2.insert("shape", array);
                obj2.insert("text", dynamic_cast<Text *>(geo)->text());
                text_objs.append(obj2);
                break;
            case Geo::Type::CONTAINER:
                container = dynamic_cast<Container *>(geo);
                for (const Geo::Point &point : container->shape())
                {
                    array.append(point.coord().x);
                    array.append(point.coord().y);
                }
                obj2.insert("shape", array);
                obj2.insert("text", container->text());
                container_objs.append(obj2);
                container = nullptr;
                break;
            case Geo::Type::CIRCLECONTAINER:
                circlecontainer = dynamic_cast<CircleContainer *>(geo);
                array.append(circlecontainer->center().coord().x);
                array.append(circlecontainer->center().coord().y);
                array.append(circlecontainer->radius());
                obj2.insert("shape", array);
                obj2.insert("text", circlecontainer->text());
                circlecontainer_objs.append(obj2);
                circlecontainer = nullptr;
                break;
            case Geo::Type::COMBINATION:
                for (Geo::Geometry *item : *dynamic_cast<Combination *>(geo))
                {
                    QJsonObject obj2;
                    QJsonArray array;
                    switch (item->type())
                    {
                    case Geo::Type::TEXT:
                        array.append(dynamic_cast<Text *>(geo)->center().coord().x);
                        array.append(dynamic_cast<Text *>(geo)->center().coord().y);
                        obj2.insert("shape", array);
                        obj2.insert("text", dynamic_cast<Text *>(geo)->text());
                        text_objs.append(obj2);
                        break;
                    case Geo::Type::CONTAINER:
                        container = dynamic_cast<Container *>(item);
                        for (const Geo::Point &point : container->shape())
                        {
                            array.append(point.coord().x);
                            array.append(point.coord().y);
                        }
                        obj2.insert("shape", array);
                        obj2.insert("text", container->text());
                        container_objs.append(obj2);
                        container = nullptr;
                        break;
                    case Geo::Type::CIRCLECONTAINER:
                        circlecontainer = dynamic_cast<CircleContainer *>(item);
                        array.append(circlecontainer->center().coord().x);
                        array.append(circlecontainer->center().coord().y);
                        array.append(circlecontainer->radius());
                        obj2.insert("shape", array);
                        obj2.insert("text", circlecontainer->text());
                        circlecontainer_objs.append(obj2);
                        circlecontainer = nullptr;
                        break;
                    case Geo::Type::POLYLINE:
                        polyline = dynamic_cast<Geo::Polyline *>(item);
                        if (polyline->empty())
                        {
                            break;
                        }
                        for (const Geo::Point &point : *polyline)
                        {
                            array.append(point.coord().x);
                            array.append(point.coord().y);
                        }
                        obj2.insert("shape", array);
                        polyline_objs.append(obj2);
                        polyline = nullptr;
                        break;
                    case Geo::Type::BEZIER:
                        bezier = dynamic_cast<Geo::Bezier *>(item);
                        if (bezier->empty())
                        {
                            break;
                        }
                        for (const Geo::Point &point : *bezier)
                        {
                            array.append(point.coord().x);
                            array.append(point.coord().y);
                        }
                        obj2.insert("shape", array);
                        obj2.insert("order", static_cast<int>(bezier->order()));
                        bezier_objs.append(obj2);
                        bezier = nullptr;
                        break;
                    default:
                        break;
                    }
                }
                break;
            case Geo::POLYLINE:
                polyline = dynamic_cast<Geo::Polyline *>(geo);
                if (polyline->empty())
                {
                    break;
                }
                for (const Geo::Point &point : *polyline)
                {
                    array.append(point.coord().x);
                    array.append(point.coord().y);
                }
                obj2.insert("shape", array);
                polyline_objs.append(obj2);
                polyline = nullptr;
                break;
            case Geo::Type::BEZIER:
                bezier = dynamic_cast<Geo::Bezier *>(geo);
                if (bezier->empty())
                {
                    break;
                }
                for (const Geo::Point &point : *bezier)
                {
                    array.append(point.coord().x);
                    array.append(point.coord().y);
                }
                obj2.insert("shape", array);
                obj2.insert("order", static_cast<int>(bezier->order()));
                bezier_objs.append(obj2);
                bezier = nullptr;
                break;
            default:
                break;
            }
        }
        
        container_group.insert("Text", text_objs);
        container_group.insert("ContainerGroup", container_objs);
        container_group.insert("CircleContainerGroup", circlecontainer_objs);
        container_group.insert("LinkGroup", link_objs);
        container_group.insert("PolylineGroup", polyline_objs);
        container_group.insert("BezierGroup", bezier_objs);
        container_group.insert("LayerName", group.name);

        obj.insert(std::to_string(index++).c_str(), container_group);
    }
    obj.insert("ContainerGroupCount", index);

    QJsonDocument doc;
    doc.setObject(obj);
    QFile file(path);
    file.open(QIODevice::WriteOnly);
    file.write(doc.toJson());
    file.close();
}

void File::write_plt(const std::string &path, Graph *graph)
{
    Text *text = nullptr;
    Container *container = nullptr;
    CircleContainer *circlecontainer = nullptr;
    Geo::Polyline *polyline = nullptr;
    Geo::Bezier *bezier = nullptr;

    std::ofstream output(path);
    output << "IN;PA;SP1;" << std::endl;
    for (const ContainerGroup &group : graph->container_groups())
    {
        for (Geo::Geometry *geo : group)
        {
            switch (geo->type())
            {
            case Geo::Type::TEXT:
                text = dynamic_cast<Text *>(geo);
                output << "PU" << text->center().coord().x << ',' << text->center().coord().y << ";PD";
                output << ";LB" << text->text().toStdString() << ';' << std::endl;
                text = nullptr;
                break;
            case Geo::Type::CONTAINER:
                container = dynamic_cast<Container *>(geo);
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
                circlecontainer = dynamic_cast<CircleContainer *>(geo);
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
                for (Geo::Geometry *item : *dynamic_cast<Combination *>(geo))
                {
                    switch (item->type())
                    {
                    case Geo::Type::TEXT:
                        text = dynamic_cast<Text *>(geo);
                        output << "PU" << text->center().coord().x << ',' << text->center().coord().y << ";PD";
                        output << ";LB" << text->text().toStdString() << ';' << std::endl;
                        text = nullptr;
                        break;
                    case Geo::Type::CONTAINER:
                        container = dynamic_cast<Container *>(item);
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
                        circlecontainer = dynamic_cast<CircleContainer *>(item);
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
                        polyline = dynamic_cast<Geo::Polyline *>(item);
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
                        bezier = dynamic_cast<Geo::Bezier *>(item);
                        if (bezier->empty())
                        {
                            break;
                        }
                        bezier->update_shape();
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
                polyline = dynamic_cast<Geo::Polyline *>(geo);
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
                bezier = dynamic_cast<Geo::Bezier *>(geo);
                if (bezier->empty())
                {
                    break;
                }
                bezier->update_shape();
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

void File::write(const QString &path, Graph *graph, const FileType type)
{
    const double k = graph->ratio();
    const Geo::Point point = graph->bounding_rect()[0];
    graph->translate(10-point.coord().x, 10-point.coord().y);
    graph->scale(10, 10, 1.0 / k);
    switch (type)
    {
    case FileType::JSON:
        write_json(path, graph);
        break;
    case FileType::PLT:
        write_plt(path.toStdString(), graph);
        break;
    default:
        break;
    }
    graph->scale(10, 10, k);
    graph->translate(point.coord().x - 10,  point.coord().y - 10);
}