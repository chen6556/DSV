#include "base/Editer.hpp"
#include "io/GlobalSetting.hpp"


Editer::Editer(Graph *graph)
    : _graph(graph)
{
    init();
}

Editer::Editer(Graph *graph, const QString &path)
    : _graph(graph), _file_path(path)
{
    init();
}

Editer::~Editer()
{
    _graph = nullptr;
    for (Graph *g : _backup)
    {
        delete g;
    }
    for (Geo::Geometry *geo : _paste_table)
    {
        delete geo;
    }
}

void Editer::init()
{
    if (_graph != nullptr)
    {
        for (ContainerGroup &group : _graph->container_groups())
        {
            for (Geo::Geometry *geo : group)
            {
                geo->is_selected = false;
            }
        }
        if (_graph->container_groups().empty())
        {
            _graph->append_group();
        }

        while (!_backup.empty())
        {
            delete _backup.back();
            _backup.pop_back();
        }
    }
}

void Editer::store_backup()
{
    _backup.push_back(_graph->clone());
    if (_backup.size() > GlobalSetting::get_instance()->setting()["backup_times"].toInt())
    {
        delete _backup.front();
        _backup.pop_front();
    }
    _graph->modified = true;
}

void Editer::load_graph(Graph *graph, const QString &path)
{
    if (graph != nullptr)
    {
        _graph = graph;
        _current_group = 0;
        init();
        _file_path = path;
    }
}

void Editer::load_graph(Graph *graph)
{
    if (graph != nullptr)
    {
        _graph = graph;
        _current_group = 0;
        init();
        _file_path.clear();
    }
}

void Editer::delete_graph()
{
    if (_graph != nullptr)
    {
        delete _graph;
        _graph = nullptr;
        _current_group = 0;
        _file_path.clear();
        while (!_backup.empty())
        {
            delete _backup.back();
            _backup.pop_back();
        }
    }
}

Graph *Editer::graph()
{
    return _graph;
}

const Graph *Editer::graph() const
{
    return _graph;
}

const bool Editer::modified() const
{
    return _graph->modified;
}

void Editer::reset_modified(const bool value)
{
    _graph->modified = value;
}

const QString &Editer::path() const
{
    return _file_path;
}

void Editer::set_path(const QString &path)
{
    _file_path = path;
}

std::vector<Geo::Point> &Editer::point_cache()
{
    return _point_cache;
}

const std::vector<Geo::Point> &Editer::point_cache() const
{
    return _point_cache;
}

const size_t &Editer::current_group() const
{
    return _current_group;
}

void Editer::set_current_group(const size_t index)
{
    assert(index < _graph->container_groups().size());
    _current_group = index;
}

const size_t Editer::groups_count() const
{
    return _graph->container_groups().size();
}

void Editer::set_view_ratio(const double value)
{
    _view_ratio = value;
}



void Editer::append_points()
{
    if (_point_cache.empty())
    {
        return;
    }
    if (_graph == nullptr)
    {
        _graph = new Graph;
        _graph->append_group();
    }

    store_backup();
    if (_point_cache.size() < 4)
    {
        _point_cache.clear();
        return;
    }

    if (_point_cache.size() > 3 && Geo::distance(_point_cache.front(), _point_cache.back()) <= 8 / _view_ratio) // Container
    {
        _point_cache.pop_back();
        _point_cache.pop_back();
        _graph->append(new Container(Geo::Polygon(_point_cache.cbegin(), _point_cache.cend())), _current_group);
    }
    else
    {
        _point_cache.pop_back();
        _point_cache.pop_back();
        _graph->append(new Geo::Polyline(_point_cache.cbegin(), _point_cache.cend()), _current_group);
    }
    _point_cache.clear();
}

void Editer::append(const Geo::Circle &circle)
{
    if (circle.empty())
    {
        return;
    }
    if (_graph == nullptr)
    {
        _graph = new Graph;
        _graph->append_group();
    }
    store_backup();
    _graph->append(new CircleContainer(circle), _current_group);
}

void Editer::append(const Geo::AABBRect &rect)
{
    if (rect.empty())
    {
        return;
    }
    if (_graph == nullptr)
    {
        _graph = new Graph;
        _graph->append_group();
    }
    store_backup();
    _graph->append(new Container(rect), _current_group);
}

void Editer::append_bezier(const size_t order)
{
    if (_point_cache.size() < order + 2)
    {
        return _point_cache.clear();
    }
    store_backup();
    _point_cache.pop_back();
    while ((_point_cache.size() - 1) % order > 0)
    {
        _point_cache.pop_back();
    }
    _graph->append(new Geo::Bezier(_point_cache.begin(), _point_cache.end(), order), _current_group);
    _point_cache.clear();
}

void Editer::append_text(const double x, const double y)
{
    if (_graph == nullptr)
    {
        _graph = new Graph;
        _graph->append_group();
    }
    store_backup();
    _graph->append(new Text(x, y, GlobalSetting::get_instance()->setting()["text_size"].toInt()), _current_group);
}

void Editer::translate_points(Geo::Geometry *points, const double x0, const double y0, const double x1, const double y1, const bool change_shape)
{
    const double catch_distance = GlobalSetting::get_instance()->setting()["catch_distance"].toDouble();
    switch (points->type())
    {
    case Geo::Type::CONTAINER:
        {
            Container *temp = dynamic_cast<Container *>(points);
            size_t count = 0;
            if (change_shape && !points->shape_fixed)
            {
                for (Geo::Point &point : temp->shape())
                {
                    ++count;
                    if (Geo::distance(x0, y0, point.x, point.y) <= catch_distance ||
                        Geo::distance(x1, y1, point.x, point.y) <= catch_distance)
                    {
                        if (temp->size() == 5)
                        {
                            if (temp->shape()[0].y == temp->shape()[1].y && temp->shape()[2].y == temp->shape()[3].y && temp->shape()[0].x == temp->shape()[3].x && temp->shape()[2].x == temp->shape()[1].x)
                            {
                                point.translate(x1 - x0, y1 - y0);
                                if (count % 2 != 0)
                                {
                                    temp->shape()[count == 1 ? 3 : 1].x = point.x;
                                    temp->shape()[count].y = point.y;
                                }
                                else
                                {
                                    temp->shape()[count - 2].y = point.y;
                                    temp->shape()[count].x = point.x;
                                    if (count == 4)
                                    {
                                        temp->front().x = temp->back().x;
                                    }
                                }
                            }
                            else if (temp->shape()[0].x == temp->shape()[1].x && temp->shape()[2].x == temp->shape()[3].x && temp->shape()[0].y == temp->shape()[3].y && temp->shape()[2].y == temp->shape()[1].y)
                            {
                                point.translate(x1 - x0, y1 - y0);
                                if (count % 2 == 0)
                                {
                                    temp->shape()[count - 2].x = point.x;
                                    temp->shape()[count].y = point.y;
                                    if (count == 4)
                                    {
                                        temp->front().y = temp->back().y;
                                    }
                                }
                                else
                                {
                                    temp->shape()[count == 1 ? 3 : 1].y = point.y;
                                    temp->shape()[count].x = point.x;
                                }
                            }
                            else
                            {
                                point.translate(x1 - x0, y1 - y0);
                            }
                        }
                        else
                        {
                            point.translate(x1 - x0, y1 - y0);
                        }
                        temp->back() = temp->front();
                        _graph->modified = true;
                        return;
                    }
                }
            }
            temp->translate(x1 - x0, y1 - y0);
        }
        break;
    case Geo::Type::CIRCLECONTAINER:
        {
            CircleContainer *temp = dynamic_cast<CircleContainer *>(points);
            if (change_shape && !points->shape_fixed &&
                (std::abs(temp->radius - Geo::distance(*temp, Geo::Point(x0, y0))) <= catch_distance ||
                std::abs(temp->radius - Geo::distance(*temp, Geo::Point(x1, y1))) <= catch_distance))
            {
                temp->radius = Geo::distance(*temp, Geo::Point(x1, y1));
            }
            else
            {
                temp->translate(x1 - x0, y1 - y0);
            }
        }
        break;
    case Geo::Type::TEXT:
    case Geo::Type::COMBINATION:
        points->translate(x1 - x0, y1 - y0);
        break;
    case Geo::Type::POLYLINE:
        {
            Geo::Polyline *temp = dynamic_cast<Geo::Polyline *>(points);
            if (change_shape && !points->shape_fixed)
            {
                for (Geo::Point &point : *temp)
                {
                    if (Geo::distance(x0, y0, point.x, point.y) <= catch_distance ||
                        Geo::distance(x1, y1, point.x, point.y) <= catch_distance)
                    {
                        point.translate(x1 - x0, y1 - y0);
                        _graph->modified = true;
                        return;
                    }
                }
            }
            temp->translate(x1 - x0, y1 - y0);
        }
        break;
    case Geo::Type::BEZIER:
        {
            Geo::Bezier *temp = dynamic_cast<Geo::Bezier *>(points);
            if (temp->is_selected)
            {
                for (size_t i = 0, count = temp->size(); i < count; ++i)
                {
                    if (Geo::distance(x0, y0, (*temp)[i].x, (*temp)[i].y) <= catch_distance ||
                        Geo::distance(x1, y1, (*temp)[i].x, (*temp)[i].y) <= catch_distance)
                    {
                        (*temp)[i].translate(x1 - x0, y1 - y0);
                        if (i > 2 && i % temp->order() == 1)
                        {
                            (*temp)[i - 2] = (*temp)[i - 1] + ((*temp)[i - 1] - (*temp)[i]).normalize() * Geo::distance((*temp)[i - 2], (*temp)[i - 1]);
                            if (temp->order() == 2)
                            {
                                for (int j = i; j + 2 < count; j += 2)
                                {
                                    (*temp)[j + 2] = (*temp)[j + 1] + ((*temp)[j + 1] - (*temp)[j]).normalize() * Geo::distance((*temp)[j + 1], (*temp)[j + 2]);
                                }
                                for (int j = i - 2; j > 2; j -= 2)
                                {
                                    (*temp)[j - 2] = (*temp)[j - 1] + ((*temp)[j - 1] - (*temp)[j]).normalize() * Geo::distance((*temp)[j - 2], (*temp)[j - 1]);
                                }
                            }
                        }
                        else if (i + 2 < temp->size() && i % temp->order() == temp->order() - 1)
                        {
                            (*temp)[i + 2] = (*temp)[i + 1] + ((*temp)[i + 1] - (*temp)[i]).normalize() * Geo::distance((*temp)[i + 1], (*temp)[i + 2]);
                            if (temp->order() == 2)
                            {
                                for (int j = i + 2; j + 2 < count; j += 2)
                                {
                                    (*temp)[j + 2] = (*temp)[j + 1] + ((*temp)[j + 1] - (*temp)[j]).normalize() * Geo::distance((*temp)[j + 1], (*temp)[j + 2]);
                                }
                                for (int j = i; j > 2; j -= 2)
                                {
                                    (*temp)[j - 2] = (*temp)[j - 1] + ((*temp)[j - 1] - (*temp)[j]).normalize() * Geo::distance((*temp)[j - 2], (*temp)[j - 1]);
                                }
                            }
                        }
                        else if (i % temp->order() == 0 && i > 0 && i < count - 1)
                        {
                            (*temp)[i - 1].translate(x1 - x0, y1 - y0);
                            (*temp)[i + 1].translate(x1 - x0, y1 - y0);
                            if (temp->order() == 2)
                            {
                                for (int j = i + 1; j + 2 < count; j += 2)
                                {
                                    (*temp)[j + 2] = (*temp)[j + 1] + ((*temp)[j + 1] - (*temp)[j]).normalize() * Geo::distance((*temp)[j + 1], (*temp)[j + 2]);
                                }
                                for (int j = i - 1; j > 2; j -= 2)
                                {
                                    (*temp)[j - 2] = (*temp)[j - 1] + ((*temp)[j - 1] - (*temp)[j]).normalize() * Geo::distance((*temp)[j - 2], (*temp)[j - 1]);
                                }
                            }
                        }
                        temp->update_shape();
                        _graph->modified = true;
                        return;
                    }
                }
            }
            points->translate(x1 - x0, y1 - y0);
        }
        break;
    default:
        break;
    }
    _graph->modified = true;
}

bool Editer::remove_selected()
{
    if (_graph == nullptr || _graph->empty() || selected_count() == 0)
    {
        return false;
    }
    store_backup();

    for (std::vector<Geo::Geometry *>::iterator it = _graph->container_group(_current_group).begin(); it != _graph->container_group(_current_group).end();)
    {
        if ((*it)->is_selected)
        {
            it = _graph->container_group(_current_group).remove(it);
        }
        else
        {
            ++it;
        }
    }
    return true;
}

bool Editer::copy_selected()
{
    while (!_paste_table.empty())
    {
        delete _paste_table.back();
        _paste_table.pop_back();
    }
    if (_graph == nullptr || _graph->empty() || selected_count() == 0)
    {
        return false;
    }

    for (const Geo::Geometry *container : _graph->container_group(_current_group))
    {
        if (container->is_selected)
        {
            _paste_table.push_back(container->clone());
        }
    }

    reset_selected_mark();
    return true;
}

bool Editer::cut_selected()
{
    while (!_paste_table.empty())
    {
        delete _paste_table.back();
        _paste_table.pop_back();
    }
    if (_graph == nullptr || _graph->empty() || selected_count() == 0)
    {
        return false;
    }
    store_backup();

    for (std::vector<Geo::Geometry *>::iterator it = _graph->container_group(_current_group).begin(); it != _graph->container_group(_current_group).end();)
    {
        if ((*it)->is_selected)
        {
            _paste_table.push_back((*it)->clone());
            it = _graph->container_group(_current_group).remove(it);
        }
        else
        {
            ++it;
        }
    }

    return true;
}

bool Editer::paste(const double tx, const double ty)
{
    if (_paste_table.empty() || _graph == nullptr)
    {
        return false;
    }
    store_backup();
    reset_selected_mark();

    for (Geo::Geometry *geo : _paste_table)
    {
        _graph->container_group(_current_group).append(geo->clone());
        _graph->container_group(_current_group).back()->translate(tx, ty);
    }
    return true;
}

bool Editer::connect(std::list<Geo::Geometry *> objects, const double connect_distance)
{
    if (_graph == nullptr || objects.empty())
    {
        return false;
    }

    std::vector<Geo::Polyline*> polylines;
    std::vector<size_t> indexs;
    ContainerGroup &group = _graph->container_group(_current_group);
    for (Geo::Geometry *object : objects)
    {
        if (object->type() == Geo::Type::POLYLINE || object->type() == Geo::Type::BEZIER)
        {
            polylines.push_back(dynamic_cast<Geo::Polyline *>(object));
            indexs.push_back(std::distance(group.begin(), std::find(group.begin(), group.end(), object)));
        }
    }

    store_backup();
    bool flag;
    const size_t num = indexs.size();
    for (size_t i = 0, count = indexs.size(); i < count; ++i)
    {
        flag = false;
        for (size_t j = i + 1; j < count; ++j)
        {
            if (Geo::distance(polylines[i]->front(), polylines[j]->front()) < connect_distance)
            {
                std::reverse(polylines[j]->begin(), polylines[j]->end());
                if (polylines[i]->type() == Geo::Type::POLYLINE)
                {
                    if (polylines[j]->type() == Geo::Type::POLYLINE)
                    {
                        polylines[i]->insert(0, *polylines[j]);
                    }
                    else
                    {
                        dynamic_cast<Geo::Bezier *>(polylines[j])->update_shape();
                        polylines[i]->insert(0, dynamic_cast<Geo::Bezier *>(polylines[j])->shape());
                    }
                }
                else
                {
                    Geo::Polyline *polyline = new Geo::Polyline(dynamic_cast<Geo::Bezier *>(polylines[i])->shape());
                    polyline->is_selected = true;
                    if (polylines[j]->type() == Geo::Type::POLYLINE)
                    {
                        polyline->insert(0, *polylines[j]);
                    }
                    else
                    {
                        dynamic_cast<Geo::Bezier *>(polylines[j])->update_shape();
                        polyline->insert(0, dynamic_cast<Geo::Bezier *>(polylines[j])->shape());
                    }
                    group.remove(indexs[i]);
                    group.insert(indexs[i], polyline);
                    polylines[i] = polyline;
                }
                flag = true;
            }
            else if (Geo::distance(polylines[i]->front(), polylines[j]->back()) < connect_distance)
            {
                if (polylines[i]->type() == Geo::Type::POLYLINE)
                {
                    if (polylines[j]->type() == Geo::Type::POLYLINE)
                    {
                        polylines[i]->insert(0, *polylines[j]);
                    }
                    else
                    {
                        polylines[i]->insert(0, dynamic_cast<Geo::Bezier *>(polylines[j])->shape());
                    }
                }
                else
                {
                    Geo::Polyline *polyline = new Geo::Polyline(dynamic_cast<Geo::Bezier *>(polylines[i])->shape());
                    polyline->is_selected = true;
                    if (polylines[j]->type() == Geo::Type::POLYLINE)
                    {
                        polyline->insert(0, *polylines[j]);
                    }
                    else
                    {
                        polyline->insert(0, dynamic_cast<Geo::Bezier *>(polylines[j])->shape());
                    }
                    group.remove(indexs[i]);
                    group.insert(indexs[i], polyline);
                    polylines[i] = polyline;
                }
                flag = true;
            }
            else if (Geo::distance(polylines[i]->back(), polylines[j]->front()) < connect_distance)
            {
                if (polylines[i]->type() == Geo::Type::POLYLINE)
                {
                    if (polylines[j]->type() == Geo::Type::POLYLINE)
                    {
                        polylines[i]->append(*polylines[j]);
                    }
                    else
                    {
                        polylines[i]->append(dynamic_cast<Geo::Bezier *>(polylines[j])->shape());
                    }
                }
                else
                {
                    Geo::Polyline *polyline = new Geo::Polyline(dynamic_cast<Geo::Bezier *>(polylines[i])->shape());
                    polyline->is_selected = true;
                    if (polylines[j]->type() == Geo::Type::POLYLINE)
                    {
                        polyline->append(*polylines[j]);
                    }
                    else
                    {
                        polyline->append(dynamic_cast<Geo::Bezier *>(polylines[j])->shape());
                    }
                    group.remove(indexs[i]);
                    group.insert(indexs[i], polyline);
                    polylines[i] = polyline;
                }
                flag = true;
            }
            else if (Geo::distance(polylines[i]->back(), polylines[j]->back()) < connect_distance)
            {
                std::reverse(polylines[j]->begin(), polylines[j]->end());
                if (polylines[i]->type() == Geo::Type::POLYLINE)
                {
                    if (polylines[j]->type() == Geo::Type::POLYLINE)
                    {
                        polylines[i]->append(*polylines[j]);
                    }
                    else
                    {
                        dynamic_cast<Geo::Bezier *>(polylines[j])->update_shape();
                        polylines[i]->append(dynamic_cast<Geo::Bezier *>(polylines[j])->shape());
                    }
                }
                else
                {
                    Geo::Polyline *polyline = new Geo::Polyline(dynamic_cast<Geo::Bezier *>(polylines[i])->shape());
                    polyline->is_selected = true;
                    if (polylines[j]->type() == Geo::Type::POLYLINE)
                    {
                        polyline->append(*polylines[j]);
                    }
                    else
                    {
                        dynamic_cast<Geo::Bezier *>(polylines[j])->update_shape();
                        polyline->append(dynamic_cast<Geo::Bezier *>(polylines[j])->shape());
                    }
                    group.remove(indexs[i]);
                    group.insert(indexs[i], polyline);
                    polylines[i] = polyline;
                }
                flag = true;
            }
            if (flag)
            {
                polylines.erase(polylines.begin() + j);
                group.remove(indexs[j]);
                for (size_t k = j + 1; k < count; ++k)
                {
                    --indexs[k];
                }
                indexs.erase(indexs.begin() + j);
                --i;
                --count;
                break;
            }
        }
    }

    if (num != indexs.size())
    {
        return true;
    }
    else
    {
        delete _backup.back();
        _backup.pop_back();
        return false;
    }
}

bool Editer::close_polyline(std::list<Geo::Geometry *> objects)
{
    if (_graph == nullptr || objects.empty())
    {
        return false;
    }

    store_backup();
    Container *container = nullptr;
    ContainerGroup &group = _graph->container_group(_current_group);
    size_t index;
    for (Geo::Geometry *object : objects)
    {
        switch (object->type())
        {
        case Geo::Type::POLYLINE:
            if (dynamic_cast<Geo::Polyline *>(object)->size() < 3)
            {
                continue;
            }
            container = new Container(*dynamic_cast<Geo::Polyline *>(object));
            container->is_selected = true;
            index = std::distance(group.begin(), std::find(group.begin(), group.end(), object));
            group.remove(index);
            group.insert(index, container);
            break;
        case Geo::Type::BEZIER:
            if (dynamic_cast<Geo::Bezier *>(object)->shape().size() < 3)
            {
                continue;
            }
            container = new Container(dynamic_cast<Geo::Bezier *>(object)->shape());
            container->is_selected = true;
            index = std::distance(group.begin(), std::find(group.begin(), group.end(), object));
            group.remove(index);
            group.insert(index, container);
            break;
        default:
            break;
        }
    }

    if (container == nullptr)
    {
        delete _backup.back();
        _backup.pop_back();
        return false;
    }
    else
    {
        return true;
    }
}

bool Editer::combinate(std::list<Geo::Geometry *> objects)
{
    if (_graph == nullptr || objects.size() < 2)
    {
        return false;
    }

    size_t count = 0;
    std::vector<Geo::Geometry *> filtered_objects;
    for (Geo::Geometry *object : objects)
    {
        if (object->type() == Geo::Type::CONTAINER || object->type() == Geo::Type::TEXT
            || object->type() == Geo::Type::CIRCLECONTAINER || object->type() == Geo::Type::COMBINATION
            || object->type() == Geo::Type::POLYLINE ||  object->type() == Geo::Type::BEZIER)
        {
            filtered_objects.push_back(object);
        }
    }
    if (filtered_objects.size() < 2)
    {
        return false;
    }

    store_backup();
    Combination *combination = new Combination();
    std::reverse(objects.begin(), objects.end());
    Combination *temp = nullptr;
    ContainerGroup &group = _graph->container_group(_current_group);
    for (Geo::Geometry *object : filtered_objects)
    {
        if (object->type() == Geo::Type::COMBINATION)
        {
            temp = dynamic_cast<Combination *>(group.pop(std::find(group.rbegin(), group.rend(), object)));
            combination->append(temp);
            delete temp;
        }
        else
        {
            combination->append(group.pop(std::find(group.rbegin(), group.rend(), object)));
        }
    }

    std::reverse(combination->begin(), combination->end());
    combination->is_selected = true;
    combination->update_border();
    _graph->container_group(_current_group).append(combination);
    return true;
}

bool Editer::split(std::list<Geo::Geometry *> objects)
{
    if (_graph == nullptr || objects.empty())
    {
        return false;
    }

    std::vector<Combination *> combiantions;  
    for (Geo::Geometry *object : objects)
    {
        if (object->type() == Geo::Type::COMBINATION)
        {
            combiantions.push_back(dynamic_cast<Combination *>(object));
        }
    }
    if (combiantions.empty())
    {
        return false;
    }

    store_backup();
    std::reverse(combiantions.begin(), combiantions.end());
    ContainerGroup &group = _graph->container_group(_current_group);
    for (Combination *combination : combiantions)
    {
        group.pop(std::find(group.rbegin(), group.rend(), combination));
        group.append(*dynamic_cast<ContainerGroup *>(combination));
        delete combination;
    }
    return true;
}

bool Editer::mirror(std::list<Geo::Geometry *> objects, const Geo::Geometry *line, const bool copy)
{
    if (objects.empty() || line->type() != Geo::Type::POLYLINE)
    {
        return false;
    }

    const Geo::Point &point0 = dynamic_cast<const Geo::Polyline *>(line)->front();
    const Geo::Point &point1 = dynamic_cast<const Geo::Polyline *>(line)->back();
    const double a = point0.y - point1.y;
    const double b = point1.x - point0.x;
    const double c = point0.x * point1.y - point1.x * point0.y;
    const double d = a * a + b * b;
    const double mat[6] = {(b * b - a * a) / d, -2 * a * b / d, -2 * a * c / d,
        -2 *a * b / d, (a * a - b * b) / d, -2 * b * c / d};

    store_backup();

    if (copy)
    {
        for (Geo::Geometry *obj : objects)
        {
            _graph->container_group(_current_group).append(obj->clone());
            _graph->container_group(_current_group).back()->transform(mat);
            _graph->container_group(_current_group).back()->is_selected = true;
        }
    }
    else
    {
        for (Geo::Geometry *obj : objects)
        {
            obj->transform(mat);
            obj->is_selected = true;
        }
    }
    return true;
}

bool Editer::offset(std::list<Geo::Geometry *> objects, const double distance)
{
    store_backup();
    const size_t count = _graph->container_group(_current_group).size();
    Container *container = nullptr;
    CircleContainer *circlecontainer = nullptr;
    Geo::Polygon shape0;
    Geo::Polyline shape1;
    for (Geo::Geometry *object : objects)
    {
        switch (object->type())
        {
        case Geo::Type::CONTAINER:
            container = dynamic_cast<Container *>(object);
            if (Geo::offset(container->shape(), shape0, distance))
            {
                _graph->append(new Container(container->text(), shape0), _current_group);
            }
            break;
        case Geo::Type::CIRCLECONTAINER:
            circlecontainer = dynamic_cast<CircleContainer *>(object);
            if (distance >= 0 || -distance < circlecontainer->radius)
            {
                _graph->append(new CircleContainer(circlecontainer->text(),
                    circlecontainer->x, circlecontainer->y,
                    circlecontainer->radius + distance), _current_group);
            }
            break;
        case Geo::Type::POLYLINE:
            if (Geo::offset(*dynamic_cast<const Geo::Polyline *>(object), shape1, distance))
            {
                _graph->append(shape1.clone(), _current_group);
            }
            break;
        default:
            break;
        }
        object->is_selected = false;
    }

    if (count == _graph->container_group(_current_group).size())
    {
        delete _backup.back();
        _backup.pop_back();
        _graph->modified = true;
        return false;
    }
    else
    {
        return true;
    }
}

bool Editer::scale(std::list<Geo::Geometry *> objects, const double k)
{
    if (objects.empty() || k == 0 || k == 1)
    {
        return false;
    }

    const size_t count = _graph->container_group(_current_group).size();
    double top = -DBL_MAX, bottom = DBL_MAX, left = DBL_MAX, right = -DBL_MAX;
    bool flag = false;
    Geo::AABBRect rect;
    for (Geo::Geometry *object : objects)
    {
        switch (object->type())
        {
        case Geo::Type::CONTAINER:
        case Geo::Type::CIRCLECONTAINER:
        case Geo::Type::POLYLINE:
        case Geo::Type::BEZIER:
        case Geo::Type::COMBINATION:
            rect = object->bounding_rect();
            top = std::max(top, rect.top());
            bottom = std::min(bottom, rect.bottom());
            left = std::min(left, rect.left());
            right = std::max(right, rect.right());
            flag = true;
            break;
        default:
            object->is_selected = false;
            break;
        }
    }

    if (!flag)
    {
        return false;
    }
    store_backup();
    const double x = (left + right) / 2, y = (top + bottom) / 2;
    for (Geo::Geometry *object : objects)
    {
        switch (object->type())
        {
        case Geo::Type::CONTAINER:
        case Geo::Type::CIRCLECONTAINER:
        case Geo::Type::POLYLINE:
        case Geo::Type::COMBINATION:
            object->scale(x, y, k);
            break;
        default:
            break;
        }
    }

    return true;
}

bool Editer::polygon_union(Container *container0, Container *container1)
{
    if (_graph == nullptr || _graph->empty() || container0 == nullptr || container1 == nullptr  || container0 == container1)
    {
        return false;
    }

    store_backup();

    std::vector<Geo::Polygon> shapes;
    if (Geo::polygon_union(container0->shape(), container1->shape(), shapes))
    {
        container0->shape() = shapes.front();
        ContainerGroup &group = _graph->container_group(_current_group);
        group.remove(std::find(group.begin(), group.end(), container1));
        for (size_t i = 1, count = shapes.size(); i < count; ++i)
        {
            group.append(new Container(shapes[i]));
        }
        return true;
    }
    else
    {
        return false;
    }
}

bool Editer::polygon_intersection(Container *container0, Container *container1)
{
    if (_graph == nullptr || _graph->empty() || container0 == nullptr || container1 == nullptr || container0 == container1)
    {
        return false;
    }

    store_backup();

    std::vector<Geo::Polygon> shapes;
    if (Geo::polygon_intersection(container0->shape(), container1->shape(), shapes))
    {
        container0->shape() = shapes.front();
        ContainerGroup &group = _graph->container_group(_current_group);
        group.remove(std::find(group.rbegin(), group.rend(), container1));
        for (size_t i = 1, count = shapes.size(); i < count; ++i)
        {
            group.append(new Container(shapes[i]));
        }
        return true;
    }
    else
    {
        return false;
    }
}

bool Editer::polygon_difference(Container *container0, const Container *container1)
{
    if (container0 == nullptr || container1 == nullptr || container0 == container1)
    {
        return false;
    }

    std::vector<Geo::Polygon> shapes;
    if (Geo::polygon_difference(container0->shape(), container1->shape(), shapes))
    {
        store_backup();
        container0->shape() = shapes.front();
        for (size_t i = 1, count = shapes.size(); i < count; ++i)
        {
            _graph->container_group(_current_group).append(new Container(shapes[i]));
        }
        return true;
    }
    else
    {
        return false;
    }
}

bool Editer::fillet(Container *container, const Geo::Point &point, const double radius)
{
    Geo::Polygon &polygon = container->shape();
    std::vector<Geo::Point>::const_iterator it = std::find(polygon.begin(), polygon.end(), point);
    if (it == polygon.end())
    {
        return false;
    }
    const size_t index1 = std::distance(polygon.cbegin(), it);
    const size_t index0 = index1 > 0 ? index1 - 1 : polygon.size() - 2;
    const size_t index2 = index1 + 1;

    Geo::Polyline arc;
    if (Geo::angle_to_arc(polygon[index0], polygon[index1], polygon[index2], radius, arc))
    {
        store_backup();
        polygon.remove(index1);
        polygon.insert(index1, arc);
        return true;
    }
    else
    {
        return false;
    }
}

bool Editer::fillet(Geo::Polyline *polyline, const Geo::Point &point, const double radius)
{
    if (point == polyline->front() || point == polyline->back())
    {
        return false;
    }
    std::vector<Geo::Point>::const_iterator it = std::find(polyline->begin(), polyline->end(), point);
    if (it == polyline->end())
    {
        return false;
    }
    const size_t index = std::distance(polyline->cbegin(), it);
    Geo::Polyline arc;
    if (Geo::angle_to_arc((*polyline)[index - 1], (*polyline)[index], (*polyline)[index + 1], radius, arc))
    {
        store_backup();
        polyline->remove(index);
        polyline->insert(index, arc);
        return true;
    }
    else
    {
        return false;
    }
}


bool Editer::line_array(std::list<Geo::Geometry *> objects, int x, int y, double x_space, double y_space)
{
    if (objects.empty() || x == 0 || y == 0 || (x == 1 && y == 1))
    {
        return false;
    }

    double left = FLT_MAX, right = -FLT_MAX, top = -FLT_MAX, bottom = FLT_MAX;
    Geo::AABBRect rect;
    for (Geo::Geometry *object : objects)
    {
        rect = object->bounding_rect();
        left = std::min(rect.left(), left);
        right = std::max(rect.right(), right);
        top = std::max(rect.top(), top);
        bottom = std::min(rect.bottom(), bottom);
    }

    store_backup();

    x_space += (right - left);
    if (x < 0)
    {
        x_space = -x_space;
        x = -x;
    }
    y_space += (top - bottom);
    if (y < 0)
    {
        y_space = -y_space;
        y = -y;
    }

    for (int i = 0; i < x; ++i)
    {
        for (int j = 0; j < y; ++j)
        {
            if (i == 0 && j == 0)
            {
                continue;
            }
            for (Geo::Geometry *object : objects)
            {
                _graph->container_group(_current_group).append(object->clone());
                _graph->container_group(_current_group).back()->translate(x_space * i, y_space * j);
                _graph->container_group(_current_group).back()->is_selected = true;
            }
        }
    }
    return true;
}

bool Editer::ring_array(std::list<Geo::Geometry *> objects, const double x, const double y, const int n)
{
    if (n <= 1 || objects.empty())
    {
        return false;
    }

    store_backup();

    for (Geo::Geometry *obj : objects)
    {
        obj->is_selected = true;
    }

    for (int i = 1; i < n; ++i)
    {
        for (Geo::Geometry *obj : objects)
        {
            _graph->container_group(_current_group).append(obj->clone());
            _graph->container_group(_current_group).back()->rotate(
                x, y, 2 * Geo::PI * i / n);
            _graph->container_group(_current_group).back()->is_selected = true;
        }
    }

    return true;
}



Geo::Geometry *Editer::select(const Geo::Point &point, const bool reset_others)
{
    if (_graph == nullptr || _graph->empty())
    {
        return nullptr;
    }
    if (reset_others)
    {
        reset_selected_mark();
    }

    const double catch_distance = 2 / _view_ratio;
    Text *t = nullptr;
    Container *c = nullptr;
    CircleContainer *cc = nullptr;
    Geo::Polyline *p = nullptr;
    Geo::Bezier *b = nullptr;
    Combination *cb = nullptr;
    for (std::vector<Geo::Geometry *>::reverse_iterator it = _graph->container_group(_current_group).rbegin(),
                                                        end = _graph->container_group(_current_group).rend();
         it != end; ++it)
    {
        switch ((*it)->type())
        {
        case Geo::Type::TEXT:
            t = dynamic_cast<Text *>(*it);
            if (Geo::distance(point, dynamic_cast<const Geo::AABBRect *>(t)->center()) <= catch_distance * 10)
            {
                t->is_selected = true;
                // _graph->container_group(_current_group).pop(it);
                // _graph->container_group(_current_group).append(t);
                return t;
            }
            break;
        case Geo::Type::CONTAINER:
            c = dynamic_cast<Container *>(*it);
            if (Geo::is_inside(point, c->shape(), true))
            {
                c->is_selected = true;
                // _graph->container_group(_current_group).pop(it);
                // _graph->container_group(_current_group).append(c);
                return c;
            }
            c = nullptr;
            break;
        case Geo::Type::CIRCLECONTAINER:
            cc = dynamic_cast<CircleContainer *>(*it);
            if (Geo::is_inside(point, cc->shape(), true))
            {
                cc->is_selected = true;
                // _graph->container_group(_current_group).pop(it);
                // _graph->container_group(_current_group).append(cc);
                return cc;
            }
            cc = nullptr;
            break;
        case Geo::Type::COMBINATION:
            cb = dynamic_cast<Combination *>(*it);
            if (Geo::is_inside(point, cb->border(), true))
            {
                for (Geo::Geometry *item : *cb)
                {
                    switch (item->type())
                    {
                    case Geo::Type::TEXT:
                        if (Geo::distance(point, dynamic_cast<const Geo::AABBRect *>(item)->center()) <= catch_distance * 10)
                        {
                            cb->is_selected = true;
                            return cb;
                        }
                        break;
                    case Geo::Type::CONTAINER:
                        if (Geo::is_inside(point, dynamic_cast<Container *>(item)->shape(), true))
                        {
                            cb->is_selected = true;
                            return cb;
                        }
                        break;
                    case Geo::Type::CIRCLECONTAINER:
                        if (Geo::is_inside(point, dynamic_cast<CircleContainer *>(item)->shape(), true))
                        {
                            cb->is_selected = true;
                            return cb;
                        }
                        break;
                    case Geo::Type::POLYLINE:
                        p = dynamic_cast<Geo::Polyline *>(item);
                        for (size_t i = 1, count = p->size(); i < count; ++i)
                        {
                            if (Geo::distance(point, (*p)[i - 1], (*p)[i]) <= catch_distance)
                            {
                                cb->is_selected = true;
                                return cb;
                            }
                        }
                        p = nullptr;
                        break;
                    case Geo::Type::BEZIER:
                        b = dynamic_cast<Geo::Bezier *>(item);
                        for (size_t i = 1, count = b->shape().size(); i < count; ++i)
                        {
                            if (Geo::distance(point, b->shape()[i - 1], b->shape()[i]) <= catch_distance)
                            {
                                cb->is_selected = true;
                                return cb;
                            }
                        }
                        b = nullptr;
                        break;
                    default:
                        break;
                    }
                }
            }
            cb = nullptr;
            break;
        case Geo::Type::POLYLINE:
            p = dynamic_cast<Geo::Polyline *>(*it);
            for (size_t i = 1, count = p->size(); i < count; ++i)
            {
                if (Geo::distance(point, (*p)[i - 1], (*p)[i]) <= catch_distance)
                {
                    p->is_selected = true;
                    return p;
                }
            }
            p = nullptr;
            break;
        case Geo::Type::BEZIER:
            b = dynamic_cast<Geo::Bezier *>(*it);
            if (b->is_selected)
            {
                for (const Geo::Point &inner_point : *b)
                {
                    if (Geo::distance(point, inner_point) <= catch_distance * 1.5)
                    {
                        return b;
                    }
                }
            }
            for (size_t i = 1, count = b->shape().size(); i < count; ++i)
            {
                if (Geo::distance(point, b->shape()[i - 1], b->shape()[i]) <= catch_distance)
                {
                    b->is_selected = true;
                    return b;
                }
            }
            b = nullptr;
            break;
        default:
            break;
        }
    }

    return nullptr;
}

Geo::Geometry *Editer::select(const double x, const double y, const bool reset_others)
{
    return select(Geo::Point(x, y), reset_others);
}

std::vector<Geo::Geometry *> Editer::select(const Geo::AABBRect &rect)
{
    std::vector<Geo::Geometry *> result;
    if (rect.empty() || _graph == nullptr || _graph->empty())
    {
        return result;
    }

    for (Geo::Geometry *container : _graph->container_group(_current_group))
    {
        switch (container->type())
        {
        case Geo::Type::TEXT:
            if (Geo::is_inside(dynamic_cast<const Geo::AABBRect *>(container)->center(), rect))
            {
                container->is_selected = true;
                result.push_back(container);
            }
            else
            {
                container->is_selected = false;
            }
            break;
        case Geo::Type::CONTAINER:
            if (Geo::is_intersected(rect, dynamic_cast<Container *>(container)->shape()))
            {
                container->is_selected = true;
                result.push_back(container);
            }
            else
            {
                container->is_selected = false;
            }
            break;
        case Geo::Type::CIRCLECONTAINER:
            if (Geo::is_intersected(rect, dynamic_cast<CircleContainer *>(container)->shape()))
            {
                container->is_selected = true;
                result.push_back(container);
            }
            else
            {
                container->is_selected = false;
            }
            break;
        case Geo::Type::COMBINATION:
            if (Geo::is_intersected(rect, dynamic_cast<Combination *>(container)->border(), true))
            {
                bool end = false;
                for (Geo::Geometry *item : *dynamic_cast<Combination *>(container))
                {
                    switch (item->type())
                    {
                    case Geo::Type::TEXT:
                        if (Geo::is_inside(dynamic_cast<const Geo::AABBRect *>(item)->center(), rect))
                        {
                            end = true;
                        }
                        break;
                    case Geo::Type::CONTAINER:
                        if (Geo::is_intersected(rect, dynamic_cast<Container *>(item)->shape(), true))
                        {
                            end = true;
                        }
                        break;
                    case Geo::Type::CIRCLECONTAINER:
                        if (Geo::is_intersected(rect, dynamic_cast<CircleContainer *>(item)->shape(), true))
                        {
                            end = true;
                        }
                        break;
                    case Geo::Type::POLYLINE:
                        if (Geo::is_intersected(rect, *dynamic_cast<Geo::Polyline *>(item)))
                        {
                            end = true;
                        }
                        break;
                    case Geo::Type::BEZIER:
                        if (Geo::is_intersected(rect, dynamic_cast<Geo::Bezier *>(item)->shape()))
                        {
                            end = true;
                        }
                        break;
                    }
                    if (end)
                    {
                        break;
                    }
                }
                if (end)
                {
                    container->is_selected = true;
                    result.push_back(container);
                }
                else
                {
                    container->is_selected = false;
                }
            }
            break;
        case Geo::Type::POLYLINE:
            if (Geo::is_intersected(rect, *dynamic_cast<Geo::Polyline *>(container)))
            {
                container->is_selected = true;
                result.push_back(container);
            }
            else
            {
                container->is_selected = false;
            }
            break;
        case Geo::Type::BEZIER:
            if (Geo::is_intersected(rect, dynamic_cast<Geo::Bezier *>(container)->shape()))
            {
                container->is_selected = true;
                result.push_back(container);
            }
            else
            {
                container->is_selected = false;
            }
            break;
        default:
            break;
        }
    }

    return result;
}

std::list<Geo::Geometry *> Editer::selected() const
{
    std::list<Geo::Geometry *> result;
    if (_graph == nullptr || _graph->empty())
    {
        return result;
    }

    for (Geo::Geometry *container : _graph->container_group(_current_group))
    {
        if (container->is_selected)
        {
            result.push_back(container);
        }
    }

    return result;
}

const size_t Editer::selected_count() const
{
    size_t count = 0;
    if (_graph == nullptr || _graph->empty())
    {
        return count;
    }
    count += std::count_if(_graph->container_group(_current_group).cbegin(), _graph->container_group(_current_group).cend(),
                           [](const Geo::Geometry *c)
                           { return c->is_selected; });
    return count;
}

void Editer::reset_selected_mark(const bool value)
{
    if (_graph == nullptr || _graph->empty())
    {
        return;
    }
    for (Geo::Geometry *container : _graph->container_group(_current_group))
    {
        container->is_selected = value;
    }
}

void Editer::load_backup()
{
    if (!_backup.empty())
    {
        delete _graph;
        _graph = _backup.back();
        _backup.pop_back();
    }
}

void Editer::up(Geo::Geometry *item)
{
    for (size_t i = 0, count = _graph->container_group(_current_group).size() - 1; i < count; ++i)
    {
        if (_graph->container_group(_current_group)[i] == item)
        {
            _graph->container_group(_current_group).pop(i);
            _graph->container_group(_current_group).insert(++i, item);
            break;
        }
    }
}

void Editer::down(Geo::Geometry *item)
{
    for (size_t i = 1, count = _graph->container_group(_current_group).size(); i < count; ++i)
    {
        if (_graph->container_group(_current_group)[i] == item)
        {
            _graph->container_group(_current_group).pop(i);
            _graph->container_group(_current_group).insert(--i, item);
            break;
        }
    }
}




void Editer::remove_group(const size_t index)
{
    assert(index < _graph->container_groups().size());
    _graph->remove_group(index);
}

void Editer::append_group(const size_t index)
{
    if (index >= _graph->container_groups().size())
    {
        _graph->append_group();
    }
    else
    {
        _graph->insert_group(index);
    }
}



void Editer::rotate(std::list<Geo::Geometry *> objects, const double angle, const bool unitary, const bool all_layers)
{
    store_backup();
    const double rad = angle * Geo::PI / 180;
    Geo::Point coord;
    if (unitary)
    {
        if (all_layers)
        {
            coord = _graph->bounding_rect().center();
            for (ContainerGroup &group : _graph->container_groups())
            {
                for (Geo::Geometry *geo : group)
                {
                    geo->rotate(coord.x, coord.y, rad);
                }
            }
        }
        else
        {
            coord = _graph->container_group(_current_group).bounding_rect().center();
            for (Geo::Geometry *geo : _graph->container_group(_current_group))
            {
                geo->rotate(coord.x, coord.y, rad);
            }
        }
    }
    else
    {
        for (Geo::Geometry *geo : objects)
        {
            coord = geo->bounding_rect().center();
            geo->rotate(coord.x, coord.y, rad);
        }
    }
}

void Editer::flip(std::list<Geo::Geometry *> objects, const bool direction, const bool unitary, const bool all_layers)
{
    store_backup();
    Geo::Point coord;
    if (unitary)
    {
        if (all_layers)
        {
            coord = _graph->bounding_rect().center();
            for (ContainerGroup &group : _graph->container_groups())
            {
                for (Geo::Geometry *geo : group)
                {
                    if (direction)
                    {
                        geo->translate(-coord.x, 0);
                        geo->transform(-1, 0, 0, 0, 1, 0);
                        geo->translate(coord.x, 0);
                    }
                    else
                    {
                        geo->translate(0, -coord.y);
                        geo->transform(1, 0, 0, 0, -1, 0);
                        geo->translate(0, coord.y);
                    }
                }
            }
        }
        else
        {
            coord = _graph->container_group(_current_group).bounding_rect().center();
            for (Geo::Geometry *geo : _graph->container_group(_current_group))
            {
                if (direction)
                {
                    geo->translate(-coord.x, 0);
                    geo->transform(-1, 0, 0, 0, 1, 0);
                    geo->translate(coord.x, 0);
                }
                else
                {
                    geo->translate(0, -coord.y);
                    geo->transform(1, 0, 0, 0, -1, 0);
                    geo->translate(0, coord.y);
                }
            }
        } 
    }
    else
    {
        for (Geo::Geometry *geo : objects)
        {
            coord = geo->bounding_rect().center();
            if (direction)
            {
                geo->translate(-coord.x, 0);
                geo->transform(-1, 0, 0, 0, 1, 0);
                geo->translate(coord.x, 0);
            }
            else
            {
                geo->translate(0, -coord.y);
                geo->transform(1, 0, 0, 0, -1, 0);
                geo->translate(0, coord.y);
            }
        }
    }
}

bool Editer::auto_aligning(Geo::Geometry *src, const Geo::Geometry *dst, std::list<QLineF> &reflines)
{
    if (src == nullptr || dst == nullptr || !(src->type() == Geo::Type::CONTAINER || src->type() == Geo::Type::CIRCLECONTAINER)
        || !(dst->type() == Geo::Type::CONTAINER || dst->type() == Geo::Type::CIRCLECONTAINER))
    {
        return false;
    }

    const Geo::AABBRect rect(src->bounding_rect());
    Geo::Point center(rect.center());
    double left = rect.left(), top = rect.top(), right = rect.right(), bottom = rect.bottom();
    const double height = top - bottom, width = right - left;

    const size_t count = reflines.size();
    const Geo::AABBRect dst_rect(dst->bounding_rect());
    const Geo::Point dst_center(dst_rect.center());
    const double dst_left = dst_rect.left(), dst_top = dst_rect.top(), dst_right = dst_rect.right(), dst_bottom = dst_rect.bottom();
    const double dst_height = dst_top - dst_bottom, dst_width = dst_right - dst_left;
    const double align_distance = 2.0 / _view_ratio;

    if (std::abs(dst_left - center.x) < align_distance)
    {
        reflines.emplace_back(QLineF(dst_left, std::max(top, dst_top), dst_left, std::min(bottom, dst_bottom)));
        src->translate(dst_left - center.x, 0);
        left += (dst_left - center.x);
        right += (dst_left - center.x);
        center.x = dst_left;
    }
    if (std::abs(dst_center.x - center.x) < align_distance)
    {
        reflines.emplace_back(QLineF(dst_center.x, std::max(top, dst_top), dst_center.x, std::min(bottom, dst_bottom)));
        src->translate(dst_center.x - center.x, 0);
        left += (dst_center.x - center.x);
        right += (dst_center.x - center.x);
        center.x = dst_center.x;
    }
    if (std::abs(dst_right - center.x) < align_distance)
    {
        reflines.emplace_back(QLineF(dst_right, std::max(top, dst_top), dst_right, std::min(bottom, dst_bottom)));
        src->translate(dst_right - center.x, 0);
        left += (dst_right - center.x);
        right += (dst_right - center.x);
        center.x = dst_right;
    }
    
    if (std::abs(dst_top - center.y) < align_distance)
    {
        reflines.emplace_back(QLineF(std::min(left, dst_left), dst_top, std::max(right, dst_right), dst_top));
        src->translate(0, dst_top - center.y);
        top += (dst_top - center.y);
        bottom += (dst_top - center.y);
        center.y = dst_top;
    }
    if (std::abs(dst_center.y - center.y) < align_distance)
    {
        reflines.emplace_back(QLineF(std::min(left, dst_left), dst_center.y, std::max(right, dst_right), dst_center.y));
        src->translate(0, dst_center.y - center.y);
        top += (dst_center.y - center.y);
        bottom += (dst_center.y - center.y);
        center.y = dst_center.y;
    }
    if (std::abs(dst_bottom - center.y) < align_distance)
    {
        reflines.emplace_back(QLineF(std::min(left, dst_left), dst_bottom, std::max(right, dst_right), dst_bottom));
        src->translate(0, dst_bottom - center.y);
        top += (dst_bottom - center.y);
        bottom += (dst_bottom - center.y);
        center.y = dst_bottom;
    }
    
    if (std::abs(dst_top - top) < align_distance)
    {
        reflines.emplace_back(QLineF(std::min(left, dst_left), dst_top, std::max(right, dst_right), dst_top));
        src->translate(0, dst_top - top);
        center.y += (dst_top - top);
        bottom += (dst_top - top);
        top = dst_top;
    }
    if (std::abs(dst_center.y - top) < align_distance)
    {
        reflines.emplace_back(QLineF(std::min(left, dst_left), dst_center.y, std::max(right, dst_right), dst_center.y));
        src->translate(0, dst_center.y - top);
        center.y += (dst_center.y - top);
        bottom += (dst_center.y - top);
        top = dst_center.y;
    }
    if (std::abs(dst_bottom - top) < align_distance)
    {
        reflines.emplace_back(QLineF(std::min(left, dst_left), dst_bottom, std::max(right, dst_right), dst_bottom));
        src->translate(0, dst_bottom - top);
        center.y += (dst_bottom - top);
        bottom += (dst_bottom - top);
        top = dst_bottom;
    }
    
    if (std::abs(dst_top - bottom) < align_distance)
    {
        reflines.emplace_back(QLineF(std::min(left, dst_left), dst_top, std::max(right, dst_right), dst_top));
        src->translate(0, dst_top - bottom);
        center.y += (dst_top - bottom);
        top += (dst_top - bottom);
        bottom = dst_top;
    }
    if (std::abs(dst_center.y - bottom) < align_distance)
    {
        reflines.emplace_back(QLineF(std::min(left, dst_left), dst_center.y, std::max(right, dst_right), dst_center.y));
        src->translate(0, dst_center.y - bottom);
        center.y += (dst_center.y - bottom);
        top += (dst_center.y - bottom);
        bottom = dst_center.y;
    }
    if (std::abs(dst_bottom - bottom) < align_distance)
    {
        reflines.emplace_back(QLineF(std::min(left, dst_left), dst_bottom, std::max(right, dst_right), dst_bottom));
        src->translate(0, dst_bottom - bottom);
        center.y += (dst_bottom - bottom);
        top += (dst_bottom - bottom);
        bottom = dst_bottom;
    }
    
    if (std::abs(dst_left - left) < align_distance)
    {
        reflines.emplace_back(QLineF(dst_left, std::min(bottom, dst_bottom), dst_left, std::max(top, dst_top)));
        src->translate(dst_left - left, 0);
        center.x += (dst_left - left);
        right += (dst_left - left);
        left = dst_left;
    }
    if (std::abs(dst_center.x - left) < align_distance)
    {
        reflines.emplace_back(QLineF(dst_center.x, std::min(bottom, dst_bottom), dst_center.x, std::max(top, dst_top)));
        src->translate(dst_center.x - left, 0);
        center.x += (dst_center.x - left);
        right += (dst_center.x - left);
        left = dst_center.x;
    }
    if (std::abs(dst_right - left) < align_distance)
    {
        reflines.emplace_back(QLineF(dst_right, std::min(bottom, dst_bottom), dst_right, std::max(top, dst_top)));
        src->translate(dst_right - left, 0);
        center.x += (dst_right - left);
        right += (dst_right - left);
        left = dst_right;
    }
    
    if (std::abs(dst_left - right) < align_distance)
    {
        reflines.emplace_back(QLineF(dst_left, std::min(bottom, dst_bottom), dst_left, std::max(top, dst_top)));
        src->translate(dst_left - right, 0);
        center.x += (dst_left - right);
        left += (dst_left - right);
        right = dst_left;
    }
    if (std::abs(dst_center.x - right) < align_distance)
    {
        reflines.emplace_back(QLineF(dst_center.x, std::min(bottom, dst_bottom), dst_center.x, std::max(top, dst_top)));
        src->translate(dst_center.x - right, 0);
        center.x += (dst_center.x - right);
        left += (dst_center.x - right);
        right = dst_center.x;
    }
    if (std::abs(dst_right - right) < align_distance)
    {
        reflines.emplace_back(QLineF(dst_right, std::min(bottom, dst_bottom), dst_right, std::max(top, dst_top)));
        src->translate(dst_right - right, 0);
        center.x += (dst_right - right);
        left += (dst_right - right);
        right = dst_right;
    }

    return count != reflines.size();
}

bool Editer::auto_aligning(Geo::Point &coord, const Geo::Geometry *dst, std::list<QLineF> &reflines)
{
    if (dst == nullptr || !(dst->type() == Geo::Type::CONTAINER || dst->type() == Geo::Type::CIRCLECONTAINER))
    {
        return false;
    }

    const size_t count = reflines.size();
    const Geo::AABBRect dst_rect(dst->bounding_rect());
    const Geo::Point dst_center(dst_rect.center());
    const double dst_left = dst_rect.left(), dst_top = dst_rect.top(), dst_right = dst_rect.right(), dst_bottom = dst_rect.bottom();
    const double dst_height = dst_top - dst_bottom, dst_width = dst_right - dst_left;
    const double align_distance = 2.0 / _view_ratio;

    if (std::abs(dst_center.x - coord.x) < align_distance)
    {
        reflines.emplace_back(QLineF(dst_center.x, std::max(coord.y, dst_top), dst_center.x, std::min(coord.y, dst_bottom)));
        coord.x = dst_center.x;
    }
    if (std::abs(dst_center.y - coord.y) < align_distance)
    {
        reflines.emplace_back(QLineF(std::min(coord.x, dst_left), dst_center.y, std::max(coord.x, dst_right), dst_center.y));
        coord.y = dst_center.y;
    }
    if (std::abs(dst_top - coord.y) < align_distance)
    {
        reflines.emplace_back(QLineF(std::min(coord.x, dst_left), dst_top, std::max(coord.x, dst_right), dst_top));
        coord.y = dst_top;
    }
    if (std::abs(dst_bottom - coord.y) < align_distance)
    {
        reflines.emplace_back(QLineF(std::min(coord.x, dst_left), dst_bottom, std::max(coord.x, dst_right), dst_bottom));
        coord.y = dst_bottom;
    }
    if (std::abs(dst_left - coord.x) < align_distance)
    {
        reflines.emplace_back(QLineF(dst_left, std::min(coord.y, dst_bottom), dst_left, std::max(coord.y, dst_top)));
        coord.x = dst_left;
    }
    if (std::abs(dst_right - coord.x) < align_distance)
    {
        reflines.emplace_back(QLineF(dst_right, std::min(coord.y, dst_bottom), dst_right, std::max(coord.y, dst_top)));
        coord.x = dst_right;
    }

    return count != reflines.size();
}

bool Editer::auto_aligning(Geo::Geometry *points, std::list<QLineF> &reflines, const bool current_group_only)
{
    if (points == nullptr || _graph == nullptr || _graph->empty())
    {
        return false;
    }

    const Geo::Point center(points->bounding_rect().center());
    Geo::Geometry *dst = nullptr;
    double temp, distance = DBL_MAX;

    if (current_group_only)
    {
        for (Geo::Geometry *geo : _graph->container_group(_current_group))
        {
            if (!(geo->type() == Geo::Type::CONTAINER || geo->type() == Geo::Type::CIRCLECONTAINER) || geo == points)
            {
                continue;
            }

            switch (geo->type())
            {
            case Geo::Type::CONTAINER:
                temp = Geo::distance(center, dynamic_cast<Container *>(geo)->shape());
                break;
            case Geo::Type::CIRCLECONTAINER:
                temp = Geo::distance(center, *dynamic_cast<CircleContainer *>(geo));
                break;
            default:
                break;
            }
            
            if (temp < distance)
            {
                dst = geo;
                distance = temp;
            }
        }
    }
    else
    {
        for (ContainerGroup &group : _graph->container_groups())
        {
            for (Geo::Geometry *geo : group)
            {
                if (!(geo->type() == Geo::Type::CONTAINER || geo->type() == Geo::Type::CIRCLECONTAINER) || geo == points)
                {
                    continue;
                }

                switch (geo->type())
                {
                case Geo::Type::CONTAINER:
                    temp = Geo::distance(center, dynamic_cast<Container *>(geo)->shape());
                    break;
                case Geo::Type::CIRCLECONTAINER:
                    temp = Geo::distance(center, *dynamic_cast<CircleContainer *>(geo));
                    break;
                default:
                    break;
                }
                
                if (temp < distance)
                {
                    dst = geo;
                    distance = temp;
                }
            }
        }
    }
    
    bool flag = false;
    if (points != _catched_points && auto_aligning(points, _catched_points, reflines))
    {
        flag = true;
    }
    else
    {
        _catched_points = nullptr;
    }
    if (dst != _catched_points && auto_aligning(points, dst, reflines))
    {
        flag = true;
        if (_catched_points == nullptr)
        {
            _catched_points = dst;
        }
    }
    return flag;
}

bool Editer::auto_aligning(Geo::Geometry *points, const double x, const double y, std::list<QLineF> &reflines, const bool current_group_only)
{
    if (points == nullptr || _graph == nullptr || _graph->empty())
    {
        return false;
    }

    const Geo::Point anchor(x, y);
    Geo::Geometry *dst = nullptr;
    double temp, distance = DBL_MAX;

    if (current_group_only)
    {
        for (Geo::Geometry *geo : _graph->container_group(_current_group))
        {
            if (!(geo->type() == Geo::Type::CONTAINER || geo->type() == Geo::Type::CIRCLECONTAINER) || geo == points)
            {
                continue;
            }

            switch (geo->type())
            {
            case Geo::Type::CONTAINER:
                temp = Geo::distance(anchor, dynamic_cast<Container *>(geo)->shape());
                break;
            case Geo::Type::CIRCLECONTAINER:
                temp = Geo::distance(anchor, *dynamic_cast<CircleContainer *>(geo));
                break;
            default:
                break;
            }
            
            if (temp < distance)
            {
                dst = geo;
                distance = temp;
            }
        }
    }
    else
    {
        for (ContainerGroup &group : _graph->container_groups())
        {
            for (Geo::Geometry *geo : group)
            {
                if (!(geo->type() == Geo::Type::CONTAINER || geo->type() == Geo::Type::CIRCLECONTAINER) || geo == points)
                {
                    continue;
                }

                switch (geo->type())
                {
                case Geo::Type::CONTAINER:
                    temp = Geo::distance(anchor, dynamic_cast<Container *>(geo)->shape());
                    break;
                case Geo::Type::CIRCLECONTAINER:
                    temp = Geo::distance(anchor, *dynamic_cast<CircleContainer *>(geo));
                    break;
                default:
                    break;
                }
                
                if (temp < distance)
                {
                    dst = geo;
                    distance = temp;
                }
            }
        }
    }

    _catched_points = nullptr;
    bool flag = false;
    if (points != _catched_points && auto_aligning(points, _catched_points, reflines))
    {
        flag = true;
    }
    else
    {
        _catched_points = nullptr;
    }
    if (dst != _catched_points && auto_aligning(points, dst, reflines))
    {
        flag = true;
        if (_catched_points == nullptr)
        {
            _catched_points = dst;
        }
    }
    return flag;
}

bool Editer::auto_aligning(Geo::Point &coord, std::list<QLineF> &reflines, const bool current_group_only)
{
    if (_graph == nullptr || _graph->empty())
    {
        return false;
    }

    const Geo::Point anchor(coord);
    Geo::Geometry *dst = nullptr;
    double temp, distance = DBL_MAX;

    if (current_group_only)
    {
        for (Geo::Geometry *geo : _graph->container_group(_current_group))
        {
            if (!(geo->type() == Geo::Type::CONTAINER || geo->type() == Geo::Type::CIRCLECONTAINER) || geo->is_selected)
            {
                continue;
            }

            switch (geo->type())
            {
            case Geo::Type::CONTAINER:
                temp = Geo::distance(anchor, dynamic_cast<Container *>(geo)->shape());
                break;
            case Geo::Type::CIRCLECONTAINER:
                temp = Geo::distance(anchor, *dynamic_cast<CircleContainer *>(geo));
                break;
            default:
                break;
            }
            
            if (temp < distance)
            {
                dst = geo;
                distance = temp;
            }
        }
    }
    else
    {
        for (ContainerGroup &group : _graph->container_groups())
        {
            for (Geo::Geometry *geo : group)
            {
                if (!(geo->type() == Geo::Type::CONTAINER || geo->type() == Geo::Type::CIRCLECONTAINER) || geo->is_selected)
                {
                    continue;
                }

                switch (geo->type())
                {
                case Geo::Type::CONTAINER:
                    temp = Geo::distance(anchor, dynamic_cast<Container *>(geo)->shape());
                    break;
                case Geo::Type::CIRCLECONTAINER:
                    temp = Geo::distance(anchor, *dynamic_cast<CircleContainer *>(geo));
                    break;
                default:
                    break;
                }
                
                if (temp < distance)
                {
                    dst = geo;
                    distance = temp;
                }
            }
        }
    }

    _catched_points = nullptr;
    bool flag = false;
    if (auto_aligning(coord, _catched_points, reflines))
    {
        flag = true;
    }
    else
    {
        _catched_points = nullptr;
    }
    if (dst != _catched_points && auto_aligning(coord, dst, reflines))
    {
        flag = true;
        if (_catched_points == nullptr)
        {
            _catched_points = dst;
        }
    }
    return flag;
}





void Editer::auto_layering()
{
    if (_graph == nullptr || _graph->empty())
    {
        return;
    }

    std::vector<Geo::Geometry *> all_containers, all_polylines;
    for (ContainerGroup &group : _graph->container_groups())
    {
        while (!group.empty())
        {
            switch (group.back()->type())
            {
            case Geo::Type::POLYLINE:
            case Geo::Type::BEZIER:
                all_polylines.emplace_back(group.pop_back());
                break;
            default:
                all_containers.emplace_back(group.pop_back());
                break;
            }
        }
    }
    _graph->clear();

    if (all_containers.empty())
    {
        _graph->append_group();
        for (Geo::Geometry *item : all_polylines)
        {
            _graph->back().append(item);
        }
        return;
    }

    std::sort(all_containers.begin(), all_containers.end(), [](const Geo::Geometry *a, const Geo::Geometry *b)
              { return a->bounding_rect().area() > b->bounding_rect().area(); });
    _graph->append_group();
    for (size_t i = 0, count = all_containers.size(); _graph->back().empty() && i < count; ++i)
    {
        _graph->back().append(all_containers[i]);
        all_containers.erase(all_containers.begin() + i);
    }
    
    bool flag;
    Container *container = nullptr;
    CircleContainer *circle_container = nullptr;
    std::vector<Geo::Geometry *>::iterator it;
    while (!all_containers.empty())
    {
        for (size_t i = 0, count = all_containers.size(); i < count; ++i)
        {
            flag = true;
            switch (all_containers[i]->type())
            {
            case Geo::Type::CONTAINER:
                container = dynamic_cast<Container *>(all_containers[i]);
                for (Geo::Geometry *geo : _graph->back())
                {
                    switch (geo->type())
                    {
                    case Geo::Type::CONTAINER:
                        if (Geo::is_intersected(container->shape(), dynamic_cast<Container *>(geo)->shape()))
                        {
                            flag = false;
                        }
                        break;
                    case Geo::Type::CIRCLECONTAINER:
                        if (Geo::is_inside(*dynamic_cast<CircleContainer *>(geo), container->shape()))
                        {
                            flag = false;
                        }
                        break;
                    default:
                        break;
                    }
                    if (!flag)
                    {
                        break;
                    }
                }
                if (flag)
                {
                    _graph->back().append(container);
                    all_containers.erase(all_containers.begin() + i--);
                    --count;
                }
                break;
            case Geo::Type::CIRCLECONTAINER:
                circle_container = dynamic_cast<CircleContainer *>(all_containers[i]);
                for (Geo::Geometry *geo : _graph->back())
                {
                    switch (geo->type())
                    {
                    case Geo::Type::CONTAINER:
                        if (Geo::is_inside(*circle_container, dynamic_cast<Container *>(geo)->shape()))
                        {
                            flag = false;
                        }
                        break;
                    case Geo::Type::CIRCLECONTAINER:
                        if (Geo::is_intersected(circle_container->shape(), dynamic_cast<CircleContainer *>(geo)->shape()))
                        {
                            flag = false;
                        }
                        break;
                    default:
                        break;
                    }
                    if (!flag)
                    {
                        break;
                    }
                }
                if (flag)
                {
                    _graph->back().append(circle_container);
                    all_containers.erase(all_containers.begin() + i--);
                    --count;
                }
                break;
            default:
                break;
            }
        }
        _graph->append_group();
    }
    
    _graph->append_group();
    for (Geo::Geometry *geo : all_polylines)
    {
        _graph->back().append(geo);
    }

    for (size_t i = 0, count = _graph->container_groups().size(); i < count; ++i)
    {
        if (_graph->container_group(i).empty())
        {
            _graph->remove_group(i--);
            --count;
        }
    }
}