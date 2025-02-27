#include "base/Editer.hpp"
#include "base/Algorithm.hpp"
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
    _backup.clear();
    for (Geo::Geometry *geo : _paste_table)
    {
        delete geo;
    }
}

void Editer::init()
{
    GlobalSetting::get_instance()->graph = _graph;
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

        _backup.clear();
        _backup.set_graph(_graph);
    }
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
    else
    {
        GlobalSetting::get_instance()->graph = nullptr;
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
    else
    {
        GlobalSetting::get_instance()->graph = nullptr;
    }
}

void Editer::delete_graph()
{
    if (_graph != nullptr)
    {
        delete _graph;
        _graph = nullptr;
        GlobalSetting::get_instance()->graph = nullptr;
        _current_group = 0;
        _file_path.clear();
        _backup.clear();
    }
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

std::vector<std::tuple<double, double>> &Editer::edited_shape()
{
    return _edited_shape;
}

const size_t Editer::current_group() const
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
    Geo::Polygon *polygon = nullptr;
    Geo::Circle *circle = nullptr;
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
            if (Geo::distance_square(point, dynamic_cast<const Geo::AABBRect *>(t)->center()) <= catch_distance * catch_distance * 120)
            {
                t->is_selected = true;
                return t;
            }
            break;
        case Geo::Type::POLYGON:
            polygon = dynamic_cast<Geo::Polygon *>(*it);
            if (Geo::is_inside(point, *polygon, true))
            {
                polygon->is_selected = true;
                return polygon;
            }
            for (size_t i = 1, count = polygon->size(); i < count; ++i)
            {
                if (Geo::distance_square(point, (*polygon)[i], (*polygon)[i - 1]) <= catch_distance * catch_distance)
                {
                    polygon->is_selected = true;
                    return polygon;
                }
            }
            polygon = nullptr;
            break;
        case Geo::Type::CIRCLE:
            circle = dynamic_cast<Geo::Circle *>(*it);
            if (Geo::distance_square(point, *circle) <= std::pow(catch_distance + circle->radius, 2))
            {
                circle->is_selected = true;
                return circle;
            }
            circle = nullptr;
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
                        if (Geo::distance_square(point, dynamic_cast<const Geo::AABBRect *>(item)->center()) <= catch_distance * catch_distance * 100)
                        {
                            cb->is_selected = true;
                            return cb;
                        }
                        break;
                    case Geo::Type::POLYGON:
                        if (Geo::is_inside(point, *dynamic_cast<Geo::Polygon *>(item), true))
                        {
                            cb->is_selected = true;
                            return cb;
                        }
                        break;
                    case Geo::Type::CIRCLE:
                        if (Geo::is_inside(point, *dynamic_cast<Geo::Circle *>(item), true))
                        {
                            cb->is_selected = true;
                            return cb;
                        }
                        break;
                    case Geo::Type::POLYLINE:
                        p = dynamic_cast<Geo::Polyline *>(item);
                        for (size_t i = 1, count = p->size(); i < count; ++i)
                        {
                            if (Geo::distance_square(point, (*p)[i - 1], (*p)[i]) <= catch_distance * catch_distance)
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
                            if (Geo::distance_square(point, b->shape()[i - 1], b->shape()[i]) <= catch_distance * catch_distance)
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
                if (Geo::distance_square(point, (*p)[i - 1], (*p)[i]) <= catch_distance * catch_distance)
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
                    if (Geo::distance_square(point, inner_point) <= catch_distance * catch_distance * 2.25)
                    {
                        return b;
                    }
                }
            }
            for (size_t i = 1, count = b->shape().size(); i < count; ++i)
            {
                if (Geo::distance_square(point, b->shape()[i - 1], b->shape()[i]) <= catch_distance * catch_distance)
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
        case Geo::Type::POLYGON:
            if (Geo::is_intersected(rect, *dynamic_cast<Geo::Polygon *>(container)))
            {
                container->is_selected = true;
                result.push_back(container);
            }
            else
            {
                container->is_selected = false;
            }
            break;
        case Geo::Type::CIRCLE:
            if (Geo::is_intersected(rect, *dynamic_cast<Geo::Circle *>(container)))
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
                    case Geo::Type::POLYGON:
                        if (Geo::is_intersected(rect, *dynamic_cast<Geo::Polygon *>(item), true))
                        {
                            end = true;
                        }
                        break;
                    case Geo::Type::CIRCLE:
                        if (Geo::is_intersected(rect, *dynamic_cast<Geo::Circle *>(item), true))
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
                           [](const Geo::Geometry *c) { return c->is_selected; });
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

void Editer::undo()
{
    _backup.undo();
}

void Editer::set_backup_count(const size_t count)
{
    _backup.set_count(count);
}

void Editer::push_backup_command(UndoStack::Command *command)
{
    return _backup.push_command(command);
}


void Editer::remove_group(const size_t index)
{
    assert(index < _graph->container_groups().size());
    _backup.push_command(new UndoStack::GroupCommand(index, false, _graph->container_group(index)));
    _graph->remove_group(index);
}

void Editer::append_group(const size_t index)
{
    if (index >= _graph->container_groups().size())
    {
        _graph->append_group();
        _backup.push_command(new UndoStack::GroupCommand(_graph->size() - 1, true));
    }
    else
    {
        _graph->insert_group(index);
        _backup.push_command(new UndoStack::GroupCommand(index, true));
    }
}

void Editer::reorder_group(size_t from, size_t to)
{
    if (from < to)
    {
        _backup.push_command(new UndoStack::ReorderGroupCommand(from, to++));
    }
    else if (from > to)
    {
        _backup.push_command(new UndoStack::ReorderGroupCommand(from++, to));
    }
    else
    {
        return;
    }

    if (to >= _graph->size())
    {
        _graph->append_group();
    }
    else
    {
        _graph->insert_group(to);
    }
    _graph->container_group(from).transfer(_graph->container_group(to));
    _graph->remove_group(from);
}

bool Editer::group_is_visible(const size_t index) const
{
    return _graph->container_group(index).visible();
}

void Editer::show_group(const size_t index)
{
    _graph->container_group(index).show();
}

void Editer::hide_group(const size_t index)
{
    _graph->container_group(index).hide();
}

QString Editer::group_name(const size_t index) const
{
    return _graph->container_group(index).name;
}

void Editer::set_group_name(const size_t index, const QString &name)
{
    _backup.push_command(new UndoStack::RenameGroupCommand(index, _graph->container_group(index).name));
    _graph->container_group(index).name = name;
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
        GlobalSetting::get_instance()->graph = _graph;
        _graph->append_group();
        _backup.set_graph(_graph);
    }

    if (_point_cache.size() < 4)
    {
        _point_cache.clear();
        return;
    }

    if (_point_cache.size() > 3 && Geo::distance(_point_cache.front(), _point_cache.back()) <= 8 / _view_ratio) // Container
    {
        _point_cache.pop_back();
        _point_cache.pop_back();
        _graph->append(new Container<Geo::Polygon>(Geo::Polygon(_point_cache.cbegin(), _point_cache.cend())), _current_group);
    }
    else
    {
        _point_cache.pop_back();
        _point_cache.pop_back();
        _graph->append(new Geo::Polyline(_point_cache.cbegin(), _point_cache.cend()), _current_group);
    }
    _point_cache.clear();

    _graph->modified = true;
    _backup.push_command(new UndoStack::ObjectCommand(_graph->container_group(_current_group).back(), 
        _current_group, _graph->container_group(_current_group).size(), true));
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
        GlobalSetting::get_instance()->graph = _graph;
        _graph->append_group();
        _backup.set_graph(_graph);
    }
    _graph->append(new Container<Geo::Circle>(circle), _current_group);

    _graph->modified = true;
    _backup.push_command(new UndoStack::ObjectCommand(_graph->container_group(_current_group).back(), 
        _current_group, _graph->container_group(_current_group).size(), true));
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
        GlobalSetting::get_instance()->graph = _graph;
        _graph->append_group();
        _backup.set_graph(_graph);
    }
    _graph->append(new Container<Geo::Polygon>(rect), _current_group);

    _graph->modified = true;
    _backup.push_command(new UndoStack::ObjectCommand(_graph->container_group(_current_group).back(), 
        _current_group, _graph->container_group(_current_group).size(), true));
}

void Editer::append_bezier(const size_t order)
{
    if (_graph == nullptr)
    {
        _graph = new Graph;
        GlobalSetting::get_instance()->graph = _graph;
        _graph->append_group();
        _backup.set_graph(_graph);
    }
    if (_point_cache.size() < order + 2)
    {
        return _point_cache.clear();
    }
    _point_cache.pop_back();
    while ((_point_cache.size() - 1) % order > 0)
    {
        _point_cache.pop_back();
    }
    _graph->append(new Geo::Bezier(_point_cache.begin(), _point_cache.end(), order), _current_group);
    _point_cache.clear();

    _graph->modified = true;
    _backup.push_command(new UndoStack::ObjectCommand(_graph->container_group(_current_group).back(), 
        _current_group, _graph->container_group(_current_group).size(), true));
}

void Editer::append_text(const double x, const double y)
{
    if (_graph == nullptr)
    {
        _graph = new Graph;
        GlobalSetting::get_instance()->graph = _graph;
        _graph->append_group();
        _backup.set_graph(_graph);
    }
    _graph->append(new Text(x, y, GlobalSetting::get_instance()->setting["text_size"].toInt()), _current_group);

    _graph->modified = true;
    _backup.push_command(new UndoStack::ObjectCommand(_graph->container_group(_current_group).back(), 
        _current_group, _graph->container_group(_current_group).size(), true));
}

void Editer::translate_points(Geo::Geometry *points, const double x0, const double y0, const double x1, const double y1, const bool change_shape)
{
    const double catch_distance = GlobalSetting::get_instance()->setting["catch_distance"].toDouble();
    GlobalSetting::get_instance()->translated_points = true;
    switch (points->type())
    {
    case Geo::Type::POLYGON:
        {
            Geo::Polygon *temp = dynamic_cast<Geo::Polygon *>(points);
            if (change_shape && !points->shape_fixed)
            {
                size_t count = 0, index = SIZE_MAX;
                double distance, min_distance = DBL_MAX;
                for (Geo::Point &point : *temp)
                {
                    distance = std::min(Geo::distance_square(x0, y0, point.x, point.y), Geo::distance_square(x1, y1, point.x, point.y));
                    if (distance <= catch_distance * catch_distance && distance < min_distance)
                    {
                        index = count;
                        min_distance = distance;
                    }
                    ++count;
                }
                if (index < SIZE_MAX)
                {
                    if (_edited_shape.empty())
                    {
                        for (const Geo::Point &point : *temp)
                        {
                            _edited_shape.emplace_back(point.x, point.y);
                        }
                    }

                    Geo::Point &point = (*temp)[index];
                    if (temp->size() == 5)
                    {
                        if ((*temp)[0].y == (*temp)[1].y && (*temp)[2].y == (*temp)[3].y
                            && (*temp)[0].x == (*temp)[3].x && (*temp)[2].x == (*temp)[1].x)
                        {
                            point.translate(x1 - x0, y1 - y0);
                            if (index % 2 == 0)
                            {
                                (*temp)[index == 0 ? 3 : 1].x = point.x;
                                (*temp)[index + 1].y = point.y;
                            }
                            else
                            {
                                (*temp)[index - 1].y = point.y;
                                (*temp)[index + 1].x = point.x;
                                if (index == 3)
                                {
                                    temp->front().x = temp->back().x;
                                }
                            }
                        }
                        else if ((*temp)[0].x == (*temp)[1].x && (*temp)[2].x == (*temp)[3].x
                            && (*temp)[0].y == (*temp)[3].y && (*temp)[2].y == (*temp)[1].y)
                        {
                            point.translate(x1 - x0, y1 - y0);
                            if (index % 2 == 0)
                            {
                                (*temp)[index == 0 ? 3 : 1].y = point.y;
                                (*temp)[index + 1].x = point.x;                                
                            }
                            else
                            {
                                (*temp)[index - 1].x = point.x;
                                (*temp)[index + 1].y = point.y;
                                if (index == 3)
                                {
                                    temp->front().y = temp->back().y;
                                }
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
            temp->translate(x1 - x0, y1 - y0);
        }
        break;
    case Geo::Type::CIRCLE:
        {
            Geo::Circle *temp = dynamic_cast<Geo::Circle *>(points);
            if (change_shape && !points->shape_fixed &&
                (std::abs(temp->radius - Geo::distance(*temp, Geo::Point(x0, y0))) <= catch_distance ||
                std::abs(temp->radius - Geo::distance(*temp, Geo::Point(x1, y1))) <= catch_distance))
            {
                if (_edited_shape.empty())
                {
                    _edited_shape.emplace_back(temp->x, temp->y);
                    _edited_shape.emplace_back(temp->radius, 0);
                }
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
                size_t count = 0, index = SIZE_MAX;
                double distance, min_distance = DBL_MAX;
                for (Geo::Point &point : *temp)
                {
                    distance = std::min(Geo::distance_square(x0, y0, point.x, point.y), Geo::distance_square(x1, y1, point.x, point.y));
                    if (distance <= catch_distance * catch_distance && distance < min_distance)
                    {
                        index = count;
                        min_distance = distance;
                    }
                    ++count;
                }
                if (index < SIZE_MAX)
                {
                    if (_edited_shape.empty())
                    {
                        for (const Geo::Point &point : *temp)
                        {
                            _edited_shape.emplace_back(point.x, point.y);
                        }
                    }

                    (*temp)[index].translate(x1 - x0, y1 - y0);
                    _graph->modified = true;
                    return;
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
                size_t count = temp->size(), index = SIZE_MAX;
                double distance, min_distance = DBL_MAX;
                for (size_t i = 0; i < count; ++i)
                {
                    distance = std::min(Geo::distance_square(x0, y0, (*temp)[i].x, (*temp)[i].y), Geo::distance_square(x1, y1, (*temp)[i].x, (*temp)[i].y));
                    if (distance <= catch_distance * catch_distance && distance < min_distance)
                    {
                        index = i;
                        min_distance = distance;
                    }
                }
                if (index < SIZE_MAX)
                {
                    if (_edited_shape.empty())
                    {
                        for (const Geo::Point &point : *temp)
                        {
                            _edited_shape.emplace_back(point.x, point.y);
                        }
                    }

                    (*temp)[index].translate(x1 - x0, y1 - y0);
                    if (index > 2 && index % temp->order() == 1)
                    {
                        (*temp)[index - 2] = (*temp)[index - 1] + ((*temp)[index - 1] - (*temp)[index]).normalize() * Geo::distance((*temp)[index - 2], (*temp)[index - 1]);
                        if (temp->order() == 2)
                        {
                            for (int j = index; j + 2 < count; j += 2)
                            {
                                (*temp)[j + 2] = (*temp)[j + 1] + ((*temp)[j + 1] - (*temp)[j]).normalize() * Geo::distance((*temp)[j + 1], (*temp)[j + 2]);
                            }
                            for (int j = index - 2; j > 2; j -= 2)
                            {
                                (*temp)[j - 2] = (*temp)[j - 1] + ((*temp)[j - 1] - (*temp)[j]).normalize() * Geo::distance((*temp)[j - 2], (*temp)[j - 1]);
                            }
                        }
                    }
                    else if (index + 2 < temp->size() && index % temp->order() == temp->order() - 1)
                    {
                        (*temp)[index + 2] = (*temp)[index + 1] + ((*temp)[index + 1] - (*temp)[index]).normalize() * Geo::distance((*temp)[index + 1], (*temp)[index + 2]);
                        if (temp->order() == 2)
                        {
                            for (int j = index + 2; j + 2 < count; j += 2)
                            {
                                (*temp)[j + 2] = (*temp)[j + 1] + ((*temp)[j + 1] - (*temp)[j]).normalize() * Geo::distance((*temp)[j + 1], (*temp)[j + 2]);
                            }
                            for (int j = index; j > 2; j -= 2)
                            {
                                (*temp)[j - 2] = (*temp)[j - 1] + ((*temp)[j - 1] - (*temp)[j]).normalize() * Geo::distance((*temp)[j - 2], (*temp)[j - 1]);
                            }
                        }
                    }
                    else if (index % temp->order() == 0 && index > 0 && index < count - 1)
                    {
                        (*temp)[index - 1].translate(x1 - x0, y1 - y0);
                        (*temp)[index + 1].translate(x1 - x0, y1 - y0);
                        if (temp->order() == 2)
                        {
                            for (int j = index + 1; j + 2 < count; j += 2)
                            {
                                (*temp)[j + 2] = (*temp)[j + 1] + ((*temp)[j + 1] - (*temp)[j]).normalize() * Geo::distance((*temp)[j + 1], (*temp)[j + 2]);
                            }
                            for (int j = index - 1; j > 2; j -= 2)
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

    std::vector<std::tuple<Geo::Geometry *, size_t, size_t>> items;
    for (size_t i = _graph->container_group(_current_group).size() - 1; i > 0; --i)
    {
        if (_graph->container_group(_current_group)[i]->is_selected)
        {
            items.emplace_back(_graph->container_group(_current_group).pop(i), _current_group, i);
        }
    }
    if (_graph->container_group(_current_group).front()->is_selected)
    {
        items.emplace_back(_graph->container_group(_current_group).pop_front(), _current_group, 0);
    }

    _graph->modified = true;
    _backup.push_command(new UndoStack::ObjectCommand(items, false));
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

    std::vector<std::tuple<Geo::Geometry *, size_t, size_t>> items;
    for (size_t i = _graph->container_group(_current_group).size() - 1; i > 0; --i)
    {
        if (_graph->container_group(_current_group)[i]->is_selected)
        {
            items.emplace_back(_graph->container_group(_current_group).pop(i), _current_group, i);
            _paste_table.push_back(std::get<0>(items.back())->clone());
        }
    }
    if (_graph->container_group(_current_group).front()->is_selected)
    {
        items.emplace_back(_graph->container_group(_current_group).pop_front(), _current_group, 0);
        _paste_table.push_back(std::get<0>(items.back())->clone());
    }
    std::reverse(_paste_table.begin(), _paste_table.end());

    _graph->modified = true;
    _backup.push_command(new UndoStack::ObjectCommand(items, false));

    return true;
}

bool Editer::paste(const double tx, const double ty)
{
    if (_paste_table.empty() || _graph == nullptr)
    {
        return false;
    }

    reset_selected_mark();
    std::vector<std::tuple<Geo::Geometry *, size_t, size_t>> items;
    size_t index = _graph->container_group(_current_group).size();
    for (Geo::Geometry *geo : _paste_table)
    {
        _graph->container_group(_current_group).append(geo->clone());
        _graph->container_group(_current_group).back()->translate(tx, ty);
        items.emplace_back(_graph->container_group(_current_group).back(), _current_group, index++);
    }

    _graph->modified = true;
    _backup.push_command(new UndoStack::ObjectCommand(items, true));

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
    std::vector<bool> merged;
    ContainerGroup &group = _graph->container_group(_current_group);
    for (Geo::Geometry *object : objects)
    {
        if (object->type() == Geo::Type::POLYLINE || object->type() == Geo::Type::BEZIER)
        {
            polylines.push_back(dynamic_cast<Geo::Polyline *>(object));
            indexs.push_back(std::distance(group.begin(), std::find(group.begin(), group.end(), object)));
        }
    }
    merged.assign(polylines.size(), false);

    Geo::Polyline *polyline = nullptr;
    size_t index = 0;
    for (size_t i = 0, count = indexs.size(); i < count; ++i)
    {
        for (size_t j = i + 1; j < count; ++j)
        {
            if (Geo::distance_square(polylines[i]->front(), polylines[j]->front()) < connect_distance * connect_distance)
            {
                polyline = new Geo::Polyline();
                if (polylines[i]->type() == Geo::Type::POLYLINE)
                {
                    polyline->append(polylines[i]->begin(), polylines[i]->end());
                }
                else
                {
                    static_cast<Geo::Bezier *>(polylines[i])->update_shape();
                    polyline->append(static_cast<Geo::Bezier *>(polylines[i])->shape().begin(),
                        static_cast<Geo::Bezier *>(polylines[i])->shape().end());
                }
                std::reverse(polyline->begin(), polyline->end());

                if (polylines[j]->type() == Geo::Type::POLYLINE)
                {
                    polyline->append(polylines[j]->begin(), polylines[j]->end());
                }
                else
                {
                    static_cast<Geo::Bezier *>(polylines[j])->update_shape();
                    polyline->append(static_cast<Geo::Bezier *>(polylines[j])->shape().begin(),
                        static_cast<Geo::Bezier *>(polylines[j])->shape().end());
                }

                merged[i] = true;
                merged[j] = false;
                index = i;
                i = count;
                break;
            }
            else if (Geo::distance_square(polylines[i]->front(), polylines[j]->back()) < connect_distance * connect_distance)
            {
                polyline = new Geo::Polyline();
                if (polylines[j]->type() == Geo::Type::POLYLINE)
                {
                    polyline->append(polylines[j]->begin(), polylines[j]->end());
                }
                else
                {
                    static_cast<Geo::Bezier *>(polylines[j])->update_shape();
                    polyline->append(static_cast<Geo::Bezier *>(polylines[j])->shape().begin(),
                        static_cast<Geo::Bezier *>(polylines[j])->shape().end());
                }

                if (polylines[i]->type() == Geo::Type::POLYLINE)
                {
                    polyline->append(polylines[i]->begin(), polylines[i]->end());
                }
                else
                {
                    static_cast<Geo::Bezier *>(polylines[i])->update_shape();
                    polyline->append(static_cast<Geo::Bezier *>(polylines[i])->shape().begin(),
                        static_cast<Geo::Bezier *>(polylines[i])->shape().end());
                }

                merged[i] = true;
                merged[j] = false;
                index = i;
                i = count;
                break;
            }
            else if (Geo::distance_square(polylines[i]->back(), polylines[j]->front()) < connect_distance * connect_distance )
            {
                polyline = new Geo::Polyline();
                if (polylines[i]->type() == Geo::Type::POLYLINE)
                {
                    polyline->append(polylines[i]->begin(), polylines[i]->end());
                }
                else
                {
                    static_cast<Geo::Bezier *>(polylines[i])->update_shape();
                    polyline->append(static_cast<Geo::Bezier *>(polylines[i])->shape().begin(),
                        static_cast<Geo::Bezier *>(polylines[i])->shape().end());
                }

                if (polylines[j]->type() == Geo::Type::POLYLINE)
                {
                    polyline->append(polylines[j]->begin(), polylines[j]->end());
                }
                else
                {
                    static_cast<Geo::Bezier *>(polylines[j])->update_shape();
                    polyline->append(static_cast<Geo::Bezier *>(polylines[j])->shape().begin(),
                        static_cast<Geo::Bezier *>(polylines[j])->shape().end());
                }

                merged[i] = true;
                merged[j] = false;
                index = i;
                i = count;
                break;
            }
            else if (Geo::distance_square(polylines[i]->back(), polylines[j]->back()) < connect_distance * connect_distance)
            {
                polyline = new Geo::Polyline();
                if (polylines[i]->type() == Geo::Type::POLYLINE)
                {
                    polyline->append(polylines[i]->begin(), polylines[i]->end());
                }
                else
                {
                    static_cast<Geo::Bezier *>(polylines[i])->update_shape();
                    polyline->append(static_cast<Geo::Bezier *>(polylines[i])->shape().begin(),
                        static_cast<Geo::Bezier *>(polylines[i])->shape().end());
                }

                if (polylines[j]->type() == Geo::Type::POLYLINE)
                {
                    std::vector<Geo::Point> points(polylines[j]->rbegin(), polylines[j]->rend());
                    polyline->append(points.begin(), points.end());
                }
                else
                {
                    static_cast<Geo::Bezier *>(polylines[j])->update_shape();
                    std::vector<Geo::Point> points(static_cast<Geo::Bezier *>(polylines[j])->shape().rbegin(),
                        static_cast<Geo::Bezier *>(polylines[j])->shape().rend());
                    polyline->append(points.begin(), points.end());
                }

                merged[i] = true;
                merged[j] = false;
                index = i;
                i = count;
                break;
            }
        }
    }

    if (polyline == nullptr)
    {
        return false;
    }

    for (size_t i = 0, count = polylines.size(); i < count;)
    {
        if (merged[i])
        {
            ++i;
            continue;
        }

        if (Geo::distance_square(polyline->front(), polylines[i]->front()) < connect_distance * connect_distance)
        {
            if (polylines[i]->type() == Geo::Type::POLYLINE)
            {
                std::vector<Geo::Point> points(polylines[i]->rbegin(), polylines[i]->rend());
                polyline->insert(0, points.begin(), points.end());
            }
            else
            {
                static_cast<Geo::Bezier *>(polylines[i])->update_shape();
                std::vector<Geo::Point> points(static_cast<Geo::Bezier *>(polylines[i])->shape().rbegin(),
                    static_cast<Geo::Bezier *>(polylines[i])->shape().rend());
                polyline->insert(0, points.begin(), points.end());
            }
            
            merged[i] = true;
            i = 0;
        }
        else if (Geo::distance_square(polyline->front(), polylines[i]->back()) < connect_distance * connect_distance)
        {
            if (polylines[i]->type() == Geo::Type::POLYLINE)
            {
                polyline->insert(0, polylines[i]->begin(), polylines[i]->end());
            }
            else
            {
                static_cast<Geo::Bezier *>(polylines[i])->update_shape();
                polyline->insert(0, static_cast<Geo::Bezier *>(polylines[i])->shape().begin(),
                    static_cast<Geo::Bezier *>(polylines[i])->shape().end());
            }

            merged[i] = true;
            i = 0;
        }
        else if (Geo::distance_square(polyline->back(), polylines[i]->front()) < connect_distance * connect_distance)
        {
            if (polylines[i]->type() == Geo::Type::POLYLINE)
            {
                polyline->append(polylines[i]->begin(), polylines[i]->end());
            }
            else
            {
                static_cast<Geo::Bezier *>(polylines[i])->update_shape();
                polyline->append(static_cast<Geo::Bezier *>(polylines[i])->shape().begin(),
                    static_cast<Geo::Bezier *>(polylines[i])->shape().end());
            }

            merged[i] = true;
            i = 0;
        }
        else if (Geo::distance_square(polyline->back(), polylines[i]->back()) < connect_distance * connect_distance)
        {
            if (polylines[i]->type() == Geo::Type::POLYLINE)
            {
                std::vector<Geo::Point> points(polylines[i]->rbegin(), polylines[i]->rend());
                polyline->append(points.begin(), points.end());
            }
            else
            {
                static_cast<Geo::Bezier *>(polylines[i])->update_shape();
                std::vector<Geo::Point> points(static_cast<Geo::Bezier *>(polylines[i])->shape().rbegin(),
                    static_cast<Geo::Bezier *>(polylines[i])->shape().rend());
                polyline->append(points.begin(), points.end());
            }

            merged[i] = true;
            i = 0;
        }
        else
        {
            ++i;
        }
    }

    for (size_t i = polylines.size() - 1; i > 0; --i)
    {
        if (!merged[i])
        {
            indexs.erase(indexs.begin() + i);
        }
    }
    if (!merged.front())
    {
        indexs.erase(indexs.begin());
    }
    std::sort(indexs.begin(), indexs.end(), std::greater<size_t>());
    std::vector<std::tuple<Geo::Polyline *, size_t>> items;
    for (size_t i : indexs)
    {
        items.emplace_back(static_cast<Geo::Polyline *>(group.pop(i)), i);
    }
    std::reverse(items.begin(), items.end());

    if (group.size() <= index)
    {
        group.append(polyline);
    }
    else
    {
        group.insert(index, polyline);
    }

    _graph->modified = true;
    _backup.push_command(new UndoStack::ConnectCommand(items, polyline, _current_group));
    return true;
}

bool Editer::close_polyline(std::list<Geo::Geometry *> objects)
{
    if (_graph == nullptr || objects.empty())
    {
        return false;
    }

    std::vector<std::tuple<Geo::Geometry *, size_t, size_t>> add_items, remove_items;
    Container<Geo::Polygon> *container = nullptr;
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
            container = new Container<Geo::Polygon>(*dynamic_cast<Geo::Polyline *>(object));
            container->is_selected = true;
            index = std::distance(group.begin(), std::find(group.begin(), group.end(), object));
            add_items.emplace_back(container, _current_group, index);
            remove_items.emplace_back(group.pop(index), _current_group, index);
            group.insert(index, container);
            break;
        case Geo::Type::BEZIER:
            if (dynamic_cast<Geo::Bezier *>(object)->shape().size() < 3)
            {
                continue;
            }
            container = new Container<Geo::Polygon>(dynamic_cast<Geo::Bezier *>(object)->shape());
            container->is_selected = true;
            index = std::distance(group.begin(), std::find(group.begin(), group.end(), object));
            add_items.emplace_back(container, _current_group, index);
            remove_items.emplace_back(group.pop(index), _current_group, index);
            group.insert(index, container);
            break;
        default:
            break;
        }
    }

    if (container == nullptr)
    {
        return false;
    }
    else
    {
        _graph->modified = true;
        _backup.push_command(new UndoStack::ObjectCommand(add_items, remove_items));

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
        if (object->type() == Geo::Type::POLYGON || object->type() == Geo::Type::TEXT
            || object->type() == Geo::Type::CIRCLE || object->type() == Geo::Type::COMBINATION
            || object->type() == Geo::Type::POLYLINE ||  object->type() == Geo::Type::BEZIER)
        {
            filtered_objects.push_back(object);
        }
    }
    if (filtered_objects.size() < 2)
    {
        return false;
    }

    Combination *combination = new Combination();
    std::reverse(objects.begin(), objects.end());
    Combination *temp = nullptr;
    ContainerGroup &group = _graph->container_group(_current_group);
    std::vector<std::tuple<Combination *, size_t, std::vector<Geo::Geometry *>>> items;
    for (Geo::Geometry *object : filtered_objects)
    {
        if (object->type() == Geo::Type::COMBINATION)
        {
            std::vector<Geo::Geometry *>::iterator it = std::find(group.begin(), group.end(), object);
            size_t index = std::distance(group.begin(), it);
            temp = dynamic_cast<Combination *>(group.pop(it));
            items.emplace_back(temp, index, std::vector<Geo::Geometry *>(temp->begin(), temp->end()));
            combination->append(temp);
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

    _backup.push_command(new UndoStack::CombinateCommand(combination, items, _current_group));
    _graph->modified = true;

    return true;
}

bool Editer::split(std::list<Geo::Geometry *> objects)
{
    if (_graph == nullptr || objects.empty())
    {
        return false;
    }

    std::vector<std::tuple<Combination *, size_t>> combiantions;
    ContainerGroup &group = _graph->container_group(_current_group);
    for (Geo::Geometry *object : objects)
    {
        if (object->type() == Geo::Type::COMBINATION)
        {
            combiantions.emplace_back(static_cast<Combination *>(object),
                std::distance(group.begin(), std::find(group.begin(), group.end(), object)));
        }
    }
    if (combiantions.empty())
    {
        return false;
    }

    std::reverse(combiantions.begin(), combiantions.end());
    _backup.push_command(new UndoStack::CombinateCommand(combiantions, _current_group));
    for (std::tuple<Combination *, size_t> &combination : combiantions)
    {
        std::reverse(std::get<0>(combination)->begin(), std::get<0>(combination)->end());
        group.pop(std::find(group.rbegin(), group.rend(), std::get<0>(combination)));
        group.append(*static_cast<ContainerGroup *>(std::get<0>(combination)));
    }

    _graph->modified = true;

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

    if (copy)
    {
        std::vector<std::tuple<Geo::Geometry *, size_t, size_t>> items;
        size_t index = _graph->container_group(_current_group).size();
        for (Geo::Geometry *obj : objects)
        {
            _graph->container_group(_current_group).append(obj->clone());
            _graph->container_group(_current_group).back()->transform(mat);
            _graph->container_group(_current_group).back()->is_selected = true;
            items.emplace_back(_graph->container_group(_current_group).back(), _current_group, index++);
        }
        _backup.push_command(new UndoStack::ObjectCommand(items, true));
    }
    else
    {
        for (Geo::Geometry *obj : objects)
        {
            obj->transform(mat);
            obj->is_selected = true;
        }
        _backup.push_command(new UndoStack::TransformCommand(objects.begin(), objects.end(), mat));
    }

    _graph->modified = true;

    return true;
}

bool Editer::mirror(std::list<Geo::Geometry *> objects, const double x, const double y, const bool copy)
{
    if (objects.empty())
    {
        return false;
    }

    if (copy)
    {
        std::vector<std::tuple<Geo::Geometry *, size_t, size_t>> items;
        size_t index = _graph->container_group(_current_group).size();
        for (Geo::Geometry *obj : objects)
        {
            _graph->container_group(_current_group).append(obj->clone());
            _graph->container_group(_current_group).back()->rotate(x, y, Geo::PI);
            _graph->container_group(_current_group).back()->is_selected = true;
            items.emplace_back(_graph->container_group(_current_group).back(), _current_group, index++);
        }
        _backup.push_command(new UndoStack::ObjectCommand(items, true));
    }
    else
    {
        for (Geo::Geometry *obj : objects)
        {
            obj->rotate(x, y, Geo::PI);
            obj->is_selected = true;
        }
        _backup.push_command(new UndoStack::RotateCommand(objects.begin(), objects.end(), x, y, Geo::PI, true));
    }

    _graph->modified = true;

    return true;
}

bool Editer::offset(std::list<Geo::Geometry *> objects, const double distance)
{
    const size_t count = _graph->container_group(_current_group).size();
    Container<Geo::Polygon> *container = nullptr;
    Container<Geo::Circle> *circlecontainer = nullptr;
    Geo::Polygon shape0;
    Geo::Polyline shape1;
    std::vector<std::tuple<Geo::Geometry *, size_t, size_t>> items;
    size_t index = count;
    for (Geo::Geometry *object : objects)
    {
        switch (object->type())
        {
        case Geo::Type::POLYGON:
            container = dynamic_cast<Container<Geo::Polygon> *>(object);
            if (Geo::offset(container->shape(), shape0, distance))
            {
                _graph->append(new Container<Geo::Polygon>(container->text(), shape0), _current_group);
                items.emplace_back(_graph->container_group(_current_group).back(), _current_group, index++);
            }
            break;
        case Geo::Type::CIRCLE:
            circlecontainer = dynamic_cast<Container<Geo::Circle> *>(object);
            if (distance >= 0 || -distance < circlecontainer->radius)
            {
                _graph->append(new Container<Geo::Circle>(circlecontainer->text(),
                    circlecontainer->x, circlecontainer->y,
                    circlecontainer->radius + distance), _current_group);
                items.emplace_back(_graph->container_group(_current_group).back(), _current_group, index++);
            }
            break;
        case Geo::Type::POLYLINE:
            if (Geo::offset(*dynamic_cast<const Geo::Polyline *>(object), shape1, distance))
            {
                _graph->append(shape1.clone(), _current_group);
                items.emplace_back(_graph->container_group(_current_group).back(), _current_group, index++);
            }
            break;
        default:
            break;
        }
        object->is_selected = false;
    }

    if (count == _graph->container_group(_current_group).size())
    {
        return false;
    }
    else
    {
        _graph->modified = true;
        _backup.push_command(new UndoStack::ObjectCommand(items, true));
        return true;
    }
}

bool Editer::scale(std::list<Geo::Geometry *> objects, const bool unitary, const double k)
{
    if (objects.empty() || k == 0 || k == 1)
    {
        return false;
    }

    const size_t count = _graph->container_group(_current_group).size();

    if (unitary)
    {
        double top = -DBL_MAX, bottom = DBL_MAX, left = DBL_MAX, right = -DBL_MAX;
        bool flag = false;
        Geo::AABBRect rect;
        for (Geo::Geometry *object : objects)
        {
            switch (object->type())
            {
            case Geo::Type::POLYGON:
            case Geo::Type::CIRCLE:
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

        const double x = (left + right) / 2, y = (top + bottom) / 2;
        for (Geo::Geometry *object : objects)
        {
            switch (object->type())
            {
            case Geo::Type::POLYGON:
            case Geo::Type::CIRCLE:
            case Geo::Type::POLYLINE:
            case Geo::Type::COMBINATION:
                object->scale(x, y, k);
                break;
            default:
                break;
            }
        }

        _backup.push_command(new UndoStack::ScaleCommand(objects.begin(), objects.end(), x, y, k, unitary));
    }
    else
    {
        bool flag = false;
        Geo::AABBRect rect;
        for (Geo::Geometry *object : objects)
        {
            switch (object->type())
            {
            case Geo::Type::POLYGON:
            case Geo::Type::CIRCLE:
            case Geo::Type::POLYLINE:
            case Geo::Type::BEZIER:
            case Geo::Type::COMBINATION:
                rect = object->bounding_rect();
                object->scale((rect.left() + rect.right()) / 2, (rect.top() + rect.bottom()) / 2, k);
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

        _backup.push_command(new UndoStack::ScaleCommand(objects.begin(), objects.end(), 0, 0, k, unitary));
    }
    
    _graph->modified = true;

    return true;
}

bool Editer::polygon_union(Container<Geo::Polygon> *container0, Container<Geo::Polygon> *container1)
{
    if (_graph == nullptr || _graph->empty() || container0 == nullptr || container1 == nullptr  || container0 == container1)
    {
        return false;
    }

    std::vector<Geo::Polygon> shapes;
    if (Geo::polygon_union(container0->shape(), container1->shape(), shapes))
    {
        ContainerGroup &group = _graph->container_group(_current_group);
        std::vector<std::tuple<Geo::Geometry *, size_t, size_t>> add_items, remove_items;
        size_t index0 = std::distance(group.begin(), std::find(group.begin(), group.end(), container0));
        size_t index1 = std::distance(group.begin(), std::find(group.begin(), group.end(), container1));
        remove_items.emplace_back(container0, _current_group, index0);
        remove_items.emplace_back(container1, _current_group, index1);
        group.pop(index1);
        group.pop(index0);

        if (index0 == group.size())
        {
            group.append(new Container<Geo::Polygon>(shapes.front()));
        }
        else
        {
            group.insert(index0, new Container<Geo::Polygon>(shapes.front()));
        }
        add_items.emplace_back(group[index0], _current_group, index0);

        index0 = group.size();
        for (size_t i = 1, count = shapes.size(); i < count; ++i)
        {
            group.append(new Container<Geo::Polygon>(shapes[i]));
            add_items.emplace_back(group.back(), _current_group, index0++);
        }

        _graph->modified = true;
        _backup.push_command(new UndoStack::ObjectCommand(add_items, remove_items));
        
        return true;
    }
    else
    {
        return false;
    }
}

bool Editer::polygon_intersection(Container<Geo::Polygon> *container0, Container<Geo::Polygon> *container1)
{
    if (_graph == nullptr || _graph->empty() || container0 == nullptr || container1 == nullptr || container0 == container1)
    {
        return false;
    }

    std::vector<Geo::Polygon> shapes;
    if (Geo::polygon_intersection(container0->shape(), container1->shape(), shapes))
    {
        ContainerGroup &group = _graph->container_group(_current_group);
        std::vector<std::tuple<Geo::Geometry *, size_t, size_t>> add_items, remove_items;
        size_t index0 = std::distance(group.begin(), std::find(group.begin(), group.end(), container0));
        size_t index1 = std::distance(group.begin(), std::find(group.begin(), group.end(), container1));
        remove_items.emplace_back(container0, _current_group, index0);
        remove_items.emplace_back(container1, _current_group, index1);
        group.pop(index1);
        group.pop(index0);

        if (index0 == group.size())
        {
            group.append(new Container<Geo::Polygon>(shapes.front()));
        }
        else
        {
            group.insert(index0, new Container<Geo::Polygon>(shapes.front()));
        }
        add_items.emplace_back(group[index0], _current_group, index0);

        index0 = group.size();        
        for (size_t i = 1, count = shapes.size(); i < count; ++i)
        {
            group.append(new Container<Geo::Polygon>(shapes[i]));
            add_items.emplace_back(group.back(), _current_group, index0++);
        }

        _graph->modified = true;
        _backup.push_command(new UndoStack::ObjectCommand(add_items, remove_items));

        return true;
    }
    else
    {
        return false;
    }
}

bool Editer::polygon_difference(Container<Geo::Polygon> *container0, const Container<Geo::Polygon> *container1)
{
    if (container0 == nullptr || container1 == nullptr || container0 == container1)
    {
        return false;
    }

    std::vector<Geo::Polygon> shapes;
    if (Geo::polygon_difference(container0->shape(), container1->shape(), shapes))
    {
        ContainerGroup &group = _graph->container_group(_current_group);
        std::vector<std::tuple<Geo::Geometry *, size_t, size_t>> add_items, remove_items;
        size_t index0 = std::distance(group.begin(), std::find(group.begin(), group.end(), container0));
        remove_items.emplace_back(container0, _current_group, index0);
        group.pop(index0);

        if (index0 == group.size())
        {
            group.append(new Container<Geo::Polygon>(shapes.front()));
        }
        else
        {
            group.insert(index0, new Container<Geo::Polygon>(shapes.front()));
        }
        add_items.emplace_back(group[index0], _current_group, index0);

        index0 = group.size();
        for (size_t i = 1, count = shapes.size(); i < count; ++i)
        {
            group.append(new Container<Geo::Polygon>(shapes[i]));
            add_items.emplace_back(group.back(), _current_group, index0++);
        }

        _graph->modified = true;
        _backup.push_command(new UndoStack::ObjectCommand(add_items, remove_items));

        return true;
    }
    else
    {
        return false;
    }
}

bool Editer::fillet(Container<Geo::Polygon> *container, const Geo::Point &point, const double radius)
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
        std::vector<std::tuple<double, double>> shape;
        for (const Geo::Point &point : polygon)
        {
            shape.emplace_back(point.x, point.y);
        }
        _backup.push_command(new UndoStack::ChangeShapeCommand(container, shape));

        polygon.remove(index1);
        polygon.insert(index1, arc);

        _graph->modified = true;

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
        std::vector<std::tuple<double, double>> shape;
        for (const Geo::Point &point : *polyline)
        {
            shape.emplace_back(point.x, point.y);
        }
        _backup.push_command(new UndoStack::ChangeShapeCommand(polyline, shape));

        polyline->remove(index);
        polyline->insert(index, arc);

        _graph->modified = true;

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

    std::vector<std::tuple<Geo::Geometry *, size_t, size_t>> items;
    size_t index = _graph->container_group(_current_group).size();
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
                items.emplace_back(_graph->container_group(_current_group).back(), _current_group, index++);
            }
        }
    }

    _graph->modified = true;
    _backup.push_command(new UndoStack::ObjectCommand(items, true));

    return true;
}

bool Editer::ring_array(std::list<Geo::Geometry *> objects, const double x, const double y, const int n)
{
    if (n <= 1 || objects.empty())
    {
        return false;
    }

    for (Geo::Geometry *obj : objects)
    {
        obj->is_selected = true;
    }

    std::vector<std::tuple<Geo::Geometry *, size_t, size_t>> items;
    size_t index = _graph->container_group(_current_group).size();
    for (int i = 1; i < n; ++i)
    {
        for (Geo::Geometry *obj : objects)
        {
            _graph->container_group(_current_group).append(obj->clone());
            _graph->container_group(_current_group).back()->rotate(
                x, y, 2 * Geo::PI * i / n);
            _graph->container_group(_current_group).back()->is_selected = true;
            items.emplace_back(_graph->container_group(_current_group).back(), _current_group, index++);
        }
    }

    _graph->modified = true;
    _backup.push_command(new UndoStack::ObjectCommand(items, true));

    return true;
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

void Editer::rotate(std::list<Geo::Geometry *> objects, const double angle, const bool unitary, const bool all_layers)
{
    const double rad = angle * Geo::PI / 180;
    Geo::Point coord;
    if (objects.empty())
    {
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
                    objects.insert(objects.end(), group.begin(), group.end());
                }
            }
            else
            {
                coord = _graph->container_group(_current_group).bounding_rect().center();
                for (Geo::Geometry *geo : _graph->container_group(_current_group))
                {
                    geo->rotate(coord.x, coord.y, rad);
                }
                objects.assign(_graph->container_group(_current_group).begin(), 
                    _graph->container_group(_current_group).end());
            }
        }
        else
        {
            if (all_layers)
            {
                for (ContainerGroup &group : _graph->container_groups())
                {
                    for (Geo::Geometry *geo : group)
                    {
                        coord = geo->bounding_rect().center();
                        geo->rotate(coord.x, coord.y, rad);
                    }
                    objects.insert(objects.end(), group.begin(), group.end());
                }
            }
            else
            {
                for (Geo::Geometry *geo : _graph->container_group(_current_group))
                {
                    coord = geo->bounding_rect().center();
                    geo->rotate(coord.x, coord.y, rad);
                }
                objects.assign(_graph->container_group(_current_group).begin(), 
                    _graph->container_group(_current_group).end());
            }
        }
    }
    else
    {
        if (unitary)
        {
            Geo::AABBRect rect;
            double left = DBL_MAX, top = -DBL_MAX, right = -DBL_MAX, bottom = DBL_MAX;
            for (Geo::Geometry *geo : objects)
            {
                rect = geo->bounding_rect();
                left = std::min(left, rect.left());
                top = std::max(top, rect.top());
                right = std::max(right, rect.right());
                bottom = std::min(bottom, rect.bottom());
            }
            coord.x = (left + right) / 2;
            coord.y = (top + bottom) / 2;

            for (Geo::Geometry *geo : objects)
            {
                geo->rotate(coord.x, coord.y, rad);
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

    _graph->modified = true;
    _backup.push_command(new UndoStack::RotateCommand(objects.begin(), objects.end(), coord.x, coord.y, rad, unitary));
}

void Editer::flip(std::list<Geo::Geometry *> objects, const bool direction, const bool unitary, const bool all_layers)
{
    Geo::Point coord;
    if (objects.empty())
    {
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
                    objects.insert(objects.end(), group.begin(), group.end());
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
                objects.assign(_graph->container_group(_current_group).begin(),
                    _graph->container_group(_current_group).end());
            }
        }
        else
        {
            if (all_layers)
            {
                for (ContainerGroup &group : _graph->container_groups())
                {
                    for (Geo::Geometry *geo : group)
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
                    objects.insert(objects.end(), group.begin(), group.end());
                }
            }
            else
            {
                for (Geo::Geometry *geo : _graph->container_group(_current_group))
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
                objects.assign(_graph->container_group(_current_group).begin(),
                    _graph->container_group(_current_group).end());
            }
        }
    }
    else
    {
        if (unitary)
        {
            Geo::AABBRect rect;
            double left = DBL_MAX, top = -DBL_MAX, right = -DBL_MAX, bottom = DBL_MAX;
            for (Geo::Geometry *geo : objects)
            {
                rect = geo->bounding_rect();
                left = std::min(left, rect.left());
                top = std::max(top, rect.top());
                right = std::max(right, rect.right());
                bottom = std::min(bottom, rect.bottom());
            }
            coord.x = (left + right) / 2;
            coord.y = (top + bottom) / 2;

            for (Geo::Geometry *geo : objects)
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

    _graph->modified = true;
    _backup.push_command(new UndoStack::FlipCommand(objects.begin(), objects.end(), coord.x, coord.y, direction, unitary));
}


bool Editer::auto_aligning(Geo::Geometry *src, const Geo::Geometry *dst, std::list<QLineF> &reflines)
{
    if (src == nullptr || dst == nullptr || !(src->type() == Geo::Type::POLYGON || src->type() == Geo::Type::CIRCLE)
        || !(dst->type() == Geo::Type::POLYGON || dst->type() == Geo::Type::CIRCLE))
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
    if (dst == nullptr || !(dst->type() == Geo::Type::POLYGON || dst->type() == Geo::Type::CIRCLE))
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
            if (!(geo->type() == Geo::Type::POLYGON || geo->type() == Geo::Type::CIRCLE) || geo == points)
            {
                continue;
            }

            switch (geo->type())
            {
            case Geo::Type::POLYGON:
                temp = Geo::distance(center, *dynamic_cast<Geo::Polygon *>(geo));
                break;
            case Geo::Type::CIRCLE:
                temp = Geo::distance(center, *dynamic_cast<Geo::Circle *>(geo));
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
                if (!(geo->type() == Geo::Type::POLYGON || geo->type() == Geo::Type::CIRCLE) || geo == points)
                {
                    continue;
                }

                switch (geo->type())
                {
                case Geo::Type::POLYGON:
                    temp = Geo::distance(center, *dynamic_cast<Geo::Polygon *>(geo));
                    break;
                case Geo::Type::CIRCLE:
                    temp = Geo::distance(center, *dynamic_cast<Geo::Circle *>(geo));
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
            if (!(geo->type() == Geo::Type::POLYGON || geo->type() == Geo::Type::CIRCLE) || geo == points)
            {
                continue;
            }

            switch (geo->type())
            {
            case Geo::Type::POLYGON:
                temp = Geo::distance(anchor, *dynamic_cast<Geo::Polygon *>(geo));
                break;
            case Geo::Type::CIRCLE:
                temp = Geo::distance(anchor, *dynamic_cast<Geo::Circle *>(geo));
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
                if (!(geo->type() == Geo::Type::POLYGON || geo->type() == Geo::Type::CIRCLE) || geo == points)
                {
                    continue;
                }

                switch (geo->type())
                {
                case Geo::Type::POLYGON:
                    temp = Geo::distance(anchor, *dynamic_cast<Geo::Polygon *>(geo));
                    break;
                case Geo::Type::CIRCLE:
                    temp = Geo::distance(anchor, *dynamic_cast<Geo::Circle *>(geo));
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
            if (!(geo->type() == Geo::Type::POLYGON || geo->type() == Geo::Type::CIRCLE) || geo->is_selected)
            {
                continue;
            }

            switch (geo->type())
            {
            case Geo::Type::POLYGON:
                temp = Geo::distance(anchor, *dynamic_cast<Geo::Polygon *>(geo));
                break;
            case Geo::Type::CIRCLE:
                temp = Geo::distance(anchor, *dynamic_cast<Geo::Circle *>(geo));
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
                if (!(geo->type() == Geo::Type::POLYGON || geo->type() == Geo::Type::CIRCLE) || geo->is_selected)
                {
                    continue;
                }

                switch (geo->type())
                {
                case Geo::Type::POLYGON:
                    temp = Geo::distance(anchor, *dynamic_cast<Geo::Polygon *>(geo));
                    break;
                case Geo::Type::CIRCLE:
                    temp = Geo::distance(anchor, *dynamic_cast<Geo::Circle *>(geo));
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

void Editer::auto_combinate()
{
    if (_graph == nullptr || _graph->empty())
    {
        return;
    }

    std::vector<Geo::Geometry *> all_containers;
    std::vector<Geo::Polyline *> all_polylines;
    for (ContainerGroup &group : _graph->container_groups())
    {
        while (!group.empty())
        {
            switch (group.back()->type())
            {
            case Geo::Type::POLYLINE:
            case Geo::Type::BEZIER:
                all_polylines.push_back(static_cast<Geo::Polyline *>(group.pop_back()));
                break;
            default:
                all_containers.push_back(group.pop_back());
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
        {
            if (dynamic_cast<const Geo::Polygon *>(a) == nullptr)
            {
                if (dynamic_cast<const Geo::Polygon *>(b) == nullptr)
                {
                    return static_cast<const Geo::Circle *>(a)->area() > static_cast<const Geo::Circle *>(b)->area();
                }
                else
                {
                    return static_cast<const Geo::Circle *>(a)->area() > static_cast<const Geo::Polygon *>(b)->area();
                }
            }
            else
            {
                if (dynamic_cast<const Geo::Polygon *>(b) == nullptr)
                {
                    return static_cast<const Geo::Polygon *>(a)->area() > static_cast<const Geo::Circle *>(b)->area();
                }
                else
                {
                    return static_cast<const Geo::Polygon *>(a)->area() > static_cast<const Geo::Polygon *>(b)->area();
                }
            }
        });
    _graph->append_group();
    for (size_t i = 0, count = all_containers.size(); _graph->back().empty() && i < count; ++i)
    {
        _graph->back().append(all_containers[i]);
        all_containers.erase(all_containers.begin() + i);
    }

    bool flag;
    Geo::Polygon *polygon = nullptr;
    Geo::Circle *circle = nullptr;
    for (size_t i = 0, count = all_containers.size(); i < count; ++i)
    {
        flag = true;
        switch (all_containers[i]->type())
        {
        case Geo::Type::POLYGON:
            polygon = static_cast<Geo::Polygon *>(all_containers[i]);
            for (Geo::Geometry *geo : _graph->back())
            {
                switch (geo->type())
                {
                case Geo::Type::POLYGON:
                    if (Geo::is_intersected(*polygon, *static_cast<Geo::Polygon *>(geo)))
                    {
                        flag = false;
                    }
                    break;
                case Geo::Type::CIRCLE:
                    if (Geo::is_inside(*static_cast<Geo::Circle *>(geo), *polygon))
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
                _graph->back().append(polygon);
                all_containers.erase(all_containers.begin() + i--);
                --count;
            }
            break;
        case Geo::Type::CIRCLE:
            circle = static_cast<Geo::Circle *>(all_containers[i]);
            for (Geo::Geometry *geo : _graph->back())
            {
                switch (geo->type())
                {
                case Geo::Type::POLYGON:
                    if (Geo::is_inside(*circle, *static_cast<Geo::Polygon *>(geo)))
                    {
                        flag = false;
                    }
                    break;
                case Geo::Type::CIRCLE:
                    if (Geo::is_intersected(*circle, *static_cast<Geo::Circle *>(geo)))
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
                _graph->back().append(circle);
                all_containers.erase(all_containers.begin() + i--);
                --count;
            }
            break;
        default:
            break;
        }
    }
    for (std::vector<Geo::Geometry *>::iterator it = _graph->back().begin(); it != _graph->back().end(); ++it)
    {
        polygon = static_cast<Geo::Polygon *>(*it);
        (*it) = new Combination({polygon});
    }

    for (Geo::Geometry *item : all_containers)
    {
        switch (item->type())
        {
        case Geo::Type::POLYGON:
            polygon = static_cast<Geo::Polygon *>(item);
            for (Geo::Geometry *combination : _graph->back())
            {
                flag = false;
                for (Geo::Geometry *item : *static_cast<Combination *>(combination))
                {
                    if (dynamic_cast<Geo::Polygon *>(item) != nullptr)
                    {
                        if (Geo::is_intersected(*polygon, *static_cast<Geo::Polygon *>(item)))
                        {
                            static_cast<Combination *>(combination)->append(polygon);
                            static_cast<Combination *>(combination)->update_border();
                            flag = true;
                            break;
                        }
                    }
                    else if (dynamic_cast<Geo::Circle *>(item) != nullptr)
                    {
                        if (Geo::is_intersected(*polygon, *static_cast<Geo::Circle *>(item)))
                        {
                            static_cast<Combination *>(combination)->append(polygon);
                            static_cast<Combination *>(combination)->update_border();
                            flag = true;
                            break;
                        }
                    }
                }
                if (flag)
                {
                    break;
                }
            }
            break;
        case Geo::Type::CIRCLE:
            circle = static_cast<Geo::Circle *>(item);
            for (Geo::Geometry *combination : _graph->back())
            {
                flag = false;
                for (Geo::Geometry *item : *static_cast<Combination *>(combination))
                {
                    if (dynamic_cast<Geo::Polygon *>(item) != nullptr)
                    {
                        if (Geo::is_inside(*circle, *static_cast<Geo::Polygon *>(item)))
                        {
                            static_cast<Combination *>(combination)->append(circle);
                            static_cast<Combination *>(combination)->update_border();
                            flag = true;
                            break;
                        }
                    }
                    else if (dynamic_cast<Geo::Circle *>(item) != nullptr)
                    {
                        if (Geo::is_inside(*circle, *static_cast<Geo::Circle *>(item)))
                        {
                            static_cast<Combination *>(combination)->append(circle);
                            static_cast<Combination *>(combination)->update_border();
                            flag = true;
                            break;
                        }
                    }
                }
                if (flag)
                {
                    break;
                }
            }
            break;
        }
    }

    std::vector<bool> flags(all_polylines.size(), false);
    size_t index = 0;
    for (Geo::Polyline *polyline : all_polylines)
    {
        if (dynamic_cast<Geo::Bezier *>(polyline) == nullptr)
        {
            for (Geo::Geometry *combination : _graph->back())
            {
                for (Geo::Geometry *item : *static_cast<Combination *>(combination))
                {
                    if (dynamic_cast<Geo::Polygon *>(item) != nullptr)
                    {
                        if (Geo::is_intersected(*polyline, *static_cast<Geo::Polygon *>(item)))
                        {
                            static_cast<Combination *>(combination)->append(polyline);
                            static_cast<Combination *>(combination)->update_border();
                            flags[index] = true;
                            break;
                        }
                    }
                    else if (dynamic_cast<Geo::Circle *>(item) != nullptr)
                    {
                        if (Geo::is_intersected(*polyline, *static_cast<Geo::Circle *>(item)))
                        {
                            static_cast<Combination *>(combination)->append(polyline);
                            static_cast<Combination *>(combination)->update_border();
                            flags[index] = true;
                            break;
                        }
                    }
                }
                if (flags[index])
                {
                    break;
                }
            }
        }
        else
        {
            Geo::Bezier *bezier = static_cast<Geo::Bezier *>(polyline);
            for (Geo::Geometry *combination : _graph->back())
            {
                for (Geo::Geometry *item : *static_cast<Combination *>(combination))
                {
                    if (dynamic_cast<Geo::Polygon *>(item) != nullptr)
                    {
                        if (Geo::is_intersected(bezier->shape(), *static_cast<Geo::Polygon *>(item)))
                        {
                            static_cast<Combination *>(combination)->append(bezier);
                            static_cast<Combination *>(combination)->update_border();
                            flags[index] = true;
                            break;
                        }
                    }
                    else if (dynamic_cast<Geo::Circle *>(item) != nullptr)
                    {
                        if (Geo::is_intersected(bezier->shape(), *static_cast<Geo::Circle*>(item)))
                        {
                            static_cast<Combination *>(combination)->append(bezier);
                            static_cast<Combination *>(combination)->update_border();
                            flags[index] = true;
                            break;
                        }
                    }
                }
                if (flags[index])
                {
                    break;
                }
            }
        }
        ++index;
    }

    for (std::vector<Geo::Geometry *>::iterator it = _graph->back().begin(); it != _graph->back().end(); ++it)
    {
        if (dynamic_cast<Combination *>(*it) != nullptr &&
            static_cast<Combination *>(*it)->size() == 1)
        {
            Combination *combination = static_cast<Combination *>(*it);
            *it = combination->pop_back();
            delete combination;
        }
    }

    for (size_t i = 0, count = flags.size(); i < count; ++i)
    {
        if (!flags[i])
        {
            _graph->back().append(all_polylines[i]);
        }
    }
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
            case Geo::Type::TEXT:
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
    Geo::Polygon *polygon = nullptr;
    Geo::Circle *circle = nullptr;
    std::vector<Geo::Geometry *>::iterator it;
    while (!all_containers.empty())
    {
        for (size_t i = 0, count = all_containers.size(); i < count; ++i)
        {
            flag = true;
            switch (all_containers[i]->type())
            {
            case Geo::Type::POLYGON:
                polygon = static_cast<Geo::Polygon *>(all_containers[i]);
                for (Geo::Geometry *geo : _graph->back())
                {
                    switch (geo->type())
                    {
                    case Geo::Type::POLYGON:
                        if (Geo::is_intersected(*polygon, *static_cast<Geo::Polygon *>(geo)))
                        {
                            flag = false;
                        }
                        break;
                    case Geo::Type::CIRCLE:
                        if (Geo::is_inside(*static_cast<Geo::Circle *>(geo), *polygon))
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
                    _graph->back().append(polygon);
                    all_containers.erase(all_containers.begin() + i--);
                    --count;
                }
                break;
            case Geo::Type::CIRCLE:
                circle = static_cast<Geo::Circle *>(all_containers[i]);
                for (Geo::Geometry *geo : _graph->back())
                {
                    switch (geo->type())
                    {
                    case Geo::Type::POLYGON:
                        if (Geo::is_inside(*circle, *static_cast<Geo::Polygon *>(geo)))
                        {
                            flag = false;
                        }
                        break;
                    case Geo::Type::CIRCLE:
                        if (Geo::is_intersected(*circle, *static_cast<Geo::Circle *>(geo)))
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
                    _graph->back().append(circle);
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

void Editer::auto_connect()
{
    if (_graph == nullptr || _graph->empty())
    {
        return;
    }

    Geo::Polyline *polyline0, *polyline1;
    Geo::Bezier *bezier0, *bezier1;
    for (size_t i = 0, count = _graph->container_group().size(); i < count; ++i)
    {
        if (dynamic_cast<Geo::Polygon *>(_graph->container_group()[i]) != nullptr)
        {
            continue;
        }

        bezier0 = dynamic_cast<Geo::Bezier *>(_graph->container_group()[i]);
        if (bezier0 == nullptr)
        {
            polyline0 = dynamic_cast<Geo::Polyline *>(_graph->container_group()[i]);
            if (polyline0 == nullptr)
            {
                continue;
            }

            for (size_t j = i + 1; j < count; ++j)
            {
                if (dynamic_cast<Geo::Polygon *>(_graph->container_group()[j]) != nullptr)
                {
                    continue;
                }

                bezier1 = dynamic_cast<Geo::Bezier *>(_graph->container_group()[j]);
                if (bezier1 == nullptr)
                {
                    polyline1 = dynamic_cast<Geo::Polyline *>(_graph->container_group()[j]);
                    if (polyline1 == nullptr)
                    {
                        continue;
                    }

                    if (polyline0->front() == polyline1->front() && polyline0->back() == polyline1->back())
                    {
                        polyline1->flip();
                        polyline0->insert(0, polyline1->begin(), polyline1->end());
                        _graph->container_group().remove(j);
                        --count;
                        break;
                    }
                    else if (polyline0->front() == polyline1->back() && polyline0->back() == polyline1->front())
                    {
                        polyline0->insert(0, polyline1->begin(), polyline1->end());
                        _graph->container_group().remove(j);
                        --count;
                        break;
                    }
                }
                else
                {
                    if (polyline0->front() == bezier1->front() && polyline0->back() == bezier1->back())
                    {
                        polyline1->flip();
                        polyline0->insert(0, bezier1->shape().begin(), bezier1->shape().end());
                        _graph->container_group().remove(j);
                        --count;
                        break;
                    }
                    else if (polyline0->front() == bezier1->back() && polyline0->back() == bezier1->front())
                    {
                        polyline0->insert(0, bezier1->shape().begin(), bezier1->shape().end());
                        _graph->container_group().remove(j);
                        --count;
                        break;
                    }
                }
            }
        }
        else
        {
            polyline0 = nullptr;
            for (size_t j = i + 1; j < count; ++j)
            {
                if (dynamic_cast<Geo::Polygon *>(_graph->container_group()[j]) != nullptr)
                {
                    continue;
                }

                polyline1 = dynamic_cast<Geo::Polyline *>(_graph->container_group()[j]);
                if (polyline1 == nullptr)
                {
                    continue;
                }
                if ((bezier0->front() == polyline1->front() && bezier0->back() == polyline1->back()) ||
                    (bezier0->front() == polyline1->back() && bezier0->back() == polyline1->front()))
                {
                    polyline0 = new Geo::Polyline(bezier0->shape());
                    _graph->container_group().insert(i, polyline0);
                    _graph->container_group().remove(i + 1);
                    bezier0 = nullptr;

                    bezier1 = dynamic_cast<Geo::Bezier *>(_graph->container_group()[j]);
                    if (bezier1 == nullptr)
                    {
                        if (polyline0->front() == polyline1->front() && polyline0->back() == polyline1->back())
                        {
                            polyline1->flip();
                            polyline0->insert(0, polyline1->begin(), polyline1->end());
                            _graph->container_group().remove(j);
                            --count;
                            break;
                        }
                        else if (polyline0->front() == polyline1->back() && polyline0->back() == polyline1->front())
                        {
                            polyline0->insert(0, polyline1->begin(), polyline1->end());
                            _graph->container_group().remove(j);
                            --count;
                            break;
                        }
                    }
                    else
                    {
                        if (polyline0->front() == bezier1->front() && polyline0->back() == bezier1->back())
                        {
                            polyline1->flip();
                            polyline0->insert(0, bezier1->shape().begin(), bezier1->shape().end());
                            _graph->container_group().remove(j);
                            --count;
                            break;
                        }
                        else if (polyline0->front() == bezier1->back() && polyline0->back() == bezier1->front())
                        {
                            polyline0->insert(0, bezier1->shape().begin(), bezier1->shape().end());
                            _graph->container_group().remove(j);
                            --count;
                            break;
                        }
                    }
                }
            }
        }

        if (polyline0 != nullptr && polyline0->front() == polyline0->back())
        {
            _graph->container_group().insert(i, new Container<Geo::Polygon>(
                Geo::Polygon(polyline0->begin(), polyline0->end())));
            _graph->container_group().remove(i + 1);
            continue;
        }
        else if (bezier0 != nullptr && bezier0->front() == bezier0->back())
        {
            _graph->container_group().insert(i, new Container<Geo::Polygon>(
                Geo::Polygon(bezier0->shape().begin(), bezier0->shape().end())));
            _graph->container_group().remove(i + 1);
            continue;
        }

        if (bezier0 == nullptr)
        {
            for (size_t j = i + 1; j < count; ++j)
            {
                if (dynamic_cast<Geo::Polygon *>(_graph->container_group()[j]) != nullptr)
                {
                    continue;
                }
                bezier1 = dynamic_cast<Geo::Bezier *>(_graph->container_group()[j]);
                if (bezier1 == nullptr)
                {
                    polyline1 = dynamic_cast<Geo::Polyline *>(_graph->container_group()[j]);
                    if (polyline1 == nullptr)
                    {
                        continue;
                    }

                    if (polyline0->front() == polyline1->front())
                    {
                        polyline1->flip();
                        polyline0->insert(0, polyline1->begin(), polyline1->end());
                        _graph->container_group().remove(j);
                        --count;
                        j = i;
                    }
                    else if (polyline0->front() == polyline1->back())
                    {
                        polyline0->insert(0, polyline1->begin(), polyline1->end());
                        _graph->container_group().remove(j);
                        --count;
                        j = i;
                    }
                    else if (polyline0->back() == polyline1->front())
                    {
                        polyline0->append(polyline1->begin(), polyline1->end());
                        _graph->container_group().remove(j);
                        --count;
                        j = i;
                    }
                    else if (polyline0->back() == polyline1->back())
                    {
                        polyline1->flip();
                        polyline0->append(polyline1->begin(), polyline1->end());
                        _graph->container_group().remove(j);
                        --count;
                        j = i;
                    }
                }
                else
                {
                    if (polyline0->front() == bezier1->front())
                    {
                        bezier1->flip();
                        bezier1->update_shape();
                        polyline0->insert(0, bezier1->shape().cbegin(), bezier1->shape().cend());
                        _graph->container_group().remove(j);
                        --count;
                        j = i;
                    }
                    else if (polyline0->front() == bezier1->back())
                    {
                        polyline0->insert(0, bezier1->shape().begin(), bezier1->shape().end());
                        _graph->container_group().remove(j);
                        --count;
                        j = i;
                    }
                    else if (polyline0->back() == bezier1->front())
                    {
                        polyline0->append(bezier1->shape().begin(), bezier1->shape().end());
                        _graph->container_group().remove(j);
                        --count;
                        j = i;
                    }
                    else if (polyline0->back() == bezier1->back())
                    {
                        bezier1->flip();
                        bezier1->update_shape();
                        polyline0->append(bezier1->shape().begin(), bezier1->shape().end());
                        _graph->container_group().remove(j);
                        --count;
                        j = i;
                    }
                }
            }
        }
        else
        {
            for (size_t j = i + 1; j < count; ++j)
            {
                if (dynamic_cast<Geo::Polygon *>(_graph->container_group()[j]) != nullptr)
                {
                    continue;
                }
                polyline1 = dynamic_cast<Geo::Polyline *>(_graph->container_group()[j]);
                if (polyline1 == nullptr)
                {
                    continue;
                }

                if (bezier0->front() == polyline1->front() || bezier0->front() == polyline1->back()
                    || bezier0->back() == polyline1->front() || bezier0->back() == polyline1->back())
                {
                    polyline0 = new Geo::Polyline(bezier0->shape());
                    bezier0 = nullptr;
                    _graph->container_group().insert(i, polyline0);
                    _graph->container_group().remove(i + 1);
                    break;
                }
            }
            if (bezier0 != nullptr)
            {
                continue;
            }
            for (size_t j = i + 1; j < count; ++j)
            {
                if (dynamic_cast<Geo::Polygon *>(_graph->container_group()[j]) != nullptr)
                {
                    continue;
                }
                bezier1 = dynamic_cast<Geo::Bezier *>(_graph->container_group()[j]);
                if (bezier1 == nullptr)
                {
                    polyline1 = dynamic_cast<Geo::Polyline *>(_graph->container_group()[j]);
                    if (polyline1 == nullptr)
                    {
                        continue;
                    }

                    if (polyline0->front() == polyline1->front())
                    {
                        polyline1->flip();
                        polyline0->insert(0, polyline1->begin(), polyline1->end());
                        _graph->container_group().remove(j);
                        --count;
                        j = i;
                    }
                    else if (polyline0->front() == polyline1->back())
                    {
                        polyline0->insert(0, polyline1->begin(), polyline1->end());
                        _graph->container_group().remove(j);
                        --count;
                        j = i;
                    }
                    else if (polyline0->back() == polyline1->front())
                    {
                        polyline0->append(polyline1->begin(), polyline1->end());
                        _graph->container_group().remove(j);
                        --count;
                        j = i;
                    }
                    else if (polyline0->back() == polyline1->back())
                    {
                        polyline1->flip();
                        polyline0->append(polyline1->begin(), polyline1->end());
                        _graph->container_group().remove(j);
                        --count;
                        j = i;
                    }
                }
                else
                {
                    if (polyline0->front() == bezier1->front())
                    {
                        bezier1->flip();
                        bezier1->update_shape();
                        polyline0->insert(0, bezier1->shape().cbegin(), bezier1->shape().cend());
                        _graph->container_group().remove(j);
                        --count;
                        j = i;
                    }
                    else if (polyline0->front() == bezier1->back())
                    {
                        polyline0->insert(0, bezier1->shape().begin(), bezier1->shape().end());
                        _graph->container_group().remove(j);
                        --count;
                        j = i;
                    }
                    else if (polyline0->back() == bezier1->front())
                    {
                        polyline0->append(bezier1->shape().begin(), bezier1->shape().end());
                        _graph->container_group().remove(j);
                        --count;
                        j = i;
                    }
                    else if (polyline0->back() == bezier1->back())
                    {
                        bezier1->flip();
                        bezier1->update_shape();
                        polyline0->append(bezier1->shape().begin(), bezier1->shape().end());
                        _graph->container_group().remove(j);
                        --count;
                        j = i;
                    }
                }
            }
        }

        if (polyline0->front() == polyline0->back())
        {
            _graph->container_group().insert(i, new Container<Geo::Polygon>(
                Geo::Polygon(polyline0->begin(), polyline0->end())));
            _graph->container_group().remove(i + 1);
        }
    }
}