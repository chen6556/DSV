#include "io/File.hpp"
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

    for (int i = 0, count = groups["ContainerGroupCount"].toInt(); i < count; ++i)
    {
        QJsonObject obj = groups[std::to_string(i).c_str()].toObject();
        graph->append_group();
    
        QJsonArray containers = obj["ContainerGroup"].toArray();
        for (QJsonValueConstRef container : containers)
        {
            QJsonArray coordinates = container.toObject()["shape"].toArray();
            for (size_t i = 1, count = coordinates.size(); i < count; i += 2)
            {
                points.push_back(Geo::Point(coordinates[i - 1].toDouble(), coordinates[i].toDouble()));
            }
            graph->back().append(new Container(container.toObject()["text"].toString(), Geo::Polygon(points.cbegin(), points.cend())));
            graph->back().back()->memo()["id"] = container.toObject()["id"].toInt();
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
            graph->back().back()->memo()["id"] = container.toObject()["id"].toInt();
        }

        QJsonArray links = obj["LinkGroup"].toArray();
        Geo::Geometry *head, *tail;
        int tail_id, head_id;
        std::vector<Geo::Geometry*>::iterator container_it;
        for (QJsonValueConstRef link : links)
        {
            QJsonArray coordinates = link.toObject()["shape"].toArray();
            if (link.toObject().find("tail_id") == link.toObject().end() || link.toObject().find("head_id") == link.toObject().end())
            {
                continue;
            }
            head = tail = nullptr;

            container_it = std::find_if(graph->back().begin(), graph->back().end(), [&](const Geo::Geometry *c){return c->memo().has("id") && c->memo()["id"].to_int() == tail_id;});
            tail = (container_it == graph->back().end() ? nullptr : *container_it);

            container_it = std::find_if(graph->back().begin(), graph->back().end(), [&](const Geo::Geometry *c){return c->memo().has("id") && c->memo()["id"].to_int() == head_id;});
            head = (container_it == graph->back().end() ? nullptr : *container_it);

            if (tail == nullptr || head == nullptr)
            {
                continue;
            }
            
            for (size_t i = 1, count = coordinates.size(); i < count; i += 2)
            {
                points.push_back(Geo::Point(coordinates[i - 1].toDouble(), coordinates[i].toDouble()));
            }
            graph->back().append(new Link(Geo::Polyline(points.cbegin(), points.cend()), tail, head));
            tail->related().push_back(graph->back().back());
            head->related().push_back(graph->back().back());
            points.clear();
        }
        head = tail = nullptr;

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
    
        graph->back().memo()["layer_name"] = obj["LayerName"].toString().toStdString();
    }
}

void File::write_json(const QString &path, Graph *graph)
{
    const Geo::Point point = graph->bounding_rect()[0];
    graph->translate(10-point.coord().x, 10-point.coord().y);
    QJsonObject obj;
    Container *container = nullptr;
    CircleContainer *circlecontainer = nullptr;
    Link *link = nullptr;
    Geo::Polyline *polyline = nullptr;
    Geo::Bezier *bezier = nullptr;
    int container_id, circlecontainer_id, index = 0;

    for (const ContainerGroup &group : graph->container_groups())
    {
        container_id = circlecontainer_id = 0;
        QJsonArray container_objs, circlecontainer_objs, link_objs, polyline_objs, bezier_objs;
        QJsonObject container_group;

        for (Geo::Geometry *geo : group)
        {
            QJsonObject obj2;
            QJsonArray array;
            switch (geo->memo()["Type"].to_int())
            {
            case 0:
                container = reinterpret_cast<Container *>(geo);
                for (const Geo::Point &point : container->shape())
                {
                    array.append(point.coord().x);
                    array.append(point.coord().y);
                }
                obj2.insert("shape", array);
                obj2.insert("text", container->text());
                obj2.insert("id", ++container_id * 10);
                container_objs.append(obj2);
                container->memo()["id"] = container_id * 10;
                container = nullptr;
                break;
            case 1:
                circlecontainer = reinterpret_cast<CircleContainer *>(geo);
                array.append(circlecontainer->center().coord().x);
                array.append(circlecontainer->center().coord().y);
                array.append(circlecontainer->radius());
                obj2.insert("shape", array);
                obj2.insert("text", circlecontainer->text());
                obj2.insert("id", ++circlecontainer_id * 10 + 1);
                circlecontainer_objs.append(obj2);
                circlecontainer->memo()["id"] = circlecontainer_id * 10 + 1;
                circlecontainer = nullptr;
                break;
            case 2:
                link = reinterpret_cast<Link *>(geo);
                if (link->empty())
                {
                    break;
                }
                for (const Geo::Point &point : *link)
                {
                    array.append(point.coord().x);
                    array.append(point.coord().y);
                }
                obj2.insert("shape", array);
                obj2.insert("tail_id", link->tail()->memo()["id"].to_int());
                obj2.insert("head_id", link->head()->memo()["id"].to_int());
                link_objs.append(obj2);
                link = nullptr;
                break;
            case 3:
                for (Geo::Geometry *item : *reinterpret_cast<Combination *>(geo))
                {
                    QJsonObject obj2;
                    QJsonArray array;
                    switch (item->memo()["Type"].to_int())
                    {
                    case 0:
                        container = reinterpret_cast<Container *>(item);
                        for (const Geo::Point &point : container->shape())
                        {
                            array.append(point.coord().x);
                            array.append(point.coord().y);
                        }
                        obj2.insert("shape", array);
                        obj2.insert("text", container->text());
                        obj2.insert("id", ++container_id * 10);
                        container_objs.append(obj2);
                        container->memo()["id"] = container_id * 10;
                        container = nullptr;
                        break;
                    case 1:
                        circlecontainer = reinterpret_cast<CircleContainer *>(item);
                        array.append(circlecontainer->center().coord().x);
                        array.append(circlecontainer->center().coord().y);
                        array.append(circlecontainer->radius());
                        obj2.insert("shape", array);
                        obj2.insert("text", circlecontainer->text());
                        obj2.insert("id", ++circlecontainer_id * 10 + 1);
                        circlecontainer_objs.append(obj2);
                        circlecontainer->memo()["id"] = circlecontainer_id * 10 + 1;
                        circlecontainer = nullptr;
                        break;
                    case 20:
                        polyline = reinterpret_cast<Geo::Polyline *>(item);
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
                    case 21:
                        bezier = reinterpret_cast<Geo::Bezier *>(item);
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
            case 20:
                polyline = reinterpret_cast<Geo::Polyline *>(geo);
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
            case 21:
                bezier = reinterpret_cast<Geo::Bezier *>(geo);
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
        
        container_group.insert("ContainerGroup", container_objs);
        container_group.insert("CircleContainerGroup", circlecontainer_objs);
        container_group.insert("LinkGroup", link_objs);
        container_group.insert("PolylineGroup", polyline_objs);
        container_group.insert("BezierGroup", bezier_objs);
        container_group.insert("LayerName", group.memo()["layer_name"].to_string().c_str());

        obj.insert(std::to_string(index++).c_str(), container_group);
    }
    obj.insert("ContainerGroupCount", index);

    graph->translate(point.coord().x-10, point.coord().y-10);
    QJsonDocument doc;
    doc.setObject(obj);
    QFile file(path);
    file.open(QIODevice::WriteOnly);
    file.write(doc.toJson());
    file.close();
}

void File::wirte_plt(const std::string &path, Graph *graph)
{
    const Geo::Point point = graph->bounding_rect()[0];
    graph->translate(10-point.coord().x, 10-point.coord().y);
    Container *container = nullptr;
    CircleContainer *circlecontainer = nullptr;
    Link *link = nullptr;
    Geo::Polyline *polyline = nullptr;
    Geo::Bezier *bezier = nullptr;

    std::ofstream output(path);
    output << "IN;PA;SP1;" << std::endl;
    for (const ContainerGroup &group : graph->container_groups())
    {
        for (Geo::Geometry *geo : group)
        {
            switch (geo->memo()["Type"].to_int())
            {
            case 0:
                container = reinterpret_cast<Container *>(geo);
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
            case 1:
                circlecontainer = reinterpret_cast<CircleContainer *>(geo);
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
            case 2:
                link = reinterpret_cast<Link *>(geo);
                if (link->empty())
                {
                    break;
                }
                output << "PU" << link->front().coord().x << ',' << link->front().coord().y << ";PD";
                for (const Geo::Point &point : *link)
                {
                    output << point.coord().x << ',' << point.coord().y << ',';
                }
                output.seekp(-1, std::ios::cur);
                output << ';' << std::endl;
                link = nullptr;
                break;
            case 3:
                output << "Block;" << std::endl;
                for (Geo::Geometry *item : *reinterpret_cast<Combination *>(geo))
                {
                    switch (item->memo()["Type"].to_int())
                    {
                    case 0:
                        container = reinterpret_cast<Container *>(item);
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
                    case 1:
                        circlecontainer = reinterpret_cast<CircleContainer *>(item);
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
                    case 20:
                        polyline = reinterpret_cast<Geo::Polyline *>(item);
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
                    case 21:
                        bezier = reinterpret_cast<Geo::Bezier *>(item);
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
            case 20:
                polyline = reinterpret_cast<Geo::Polyline *>(geo);
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
            case 21:
                bezier = reinterpret_cast<Geo::Bezier *>(geo);
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

    graph->translate(point.coord().x-10, point.coord().y-10);
}

void File::write(const QString &path, Graph *graph, const FileType type)
{
    switch (type)
    {
    case FileType::JSON:
        return write_json(path, graph);
    case FileType::PLT:
        return wirte_plt(path.toStdString(), graph);
    default:
        break;
    }
}