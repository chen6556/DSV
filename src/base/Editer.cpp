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
                geo->memo()["is_selected"] = false;
            }
        }
        if (_graph->container_groups().empty())
        {
            _graph->append_group();
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
}

void Editer::load_graph(Graph *graph, const QString &path)
{
    if (graph != nullptr)
    {
        _graph = graph;
        init();
        _file_path = path;
    }
}

void Editer::load_graph(Graph *graph)
{
    if (graph != nullptr)
    {
        _graph = graph;
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
        _file_path.clear();
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

const bool &Editer::modified() const
{
    return _modified;
}

void Editer::reset_modified()
{
    _modified = false;
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
        Geo::Geometry *tail_geo = select(_point_cache.front(), false), *head_geo = select(_point_cache.back());
        if (tail_geo != nullptr && head_geo != nullptr && tail_geo != head_geo) // Link
        {
            _graph->append(new Link(Geo::Polyline(), tail_geo, head_geo), _current_group);
        }
        _point_cache.clear();
        _modified = true;
        return;
    }

    if (_point_cache.size() > 3 && Geo::distance(_point_cache.front(), _point_cache.back()) <= 8) // Container
    {
        _point_cache.pop_back();
        _point_cache.pop_back();
        _graph->append(new Container(Geo::Polygon(_point_cache.cbegin(), _point_cache.cend())), _current_group);
    }
    else
    {
        Geo::Geometry *tail_geo = select(_point_cache.front(), false), *head_geo = select(_point_cache.back());
        if (tail_geo != nullptr && head_geo != nullptr && tail_geo != head_geo) // Link
        {
            _point_cache.pop_back();
            _point_cache.pop_back();
            _graph->append(new Link(_point_cache.cbegin() + 1, _point_cache.cend(), tail_geo, head_geo), _current_group);
        }
        else // Polyline
        {
            if (tail_geo != nullptr)
            {
                tail_geo->memo()["is_selected"] = false;
            }
            if (head_geo != nullptr)
            {
                tail_geo->memo()["is_selected"] = false;
            }
            _point_cache.pop_back();
            _point_cache.pop_back();
            _graph->append(new Geo::Polyline(_point_cache.cbegin(), _point_cache.cend()), _current_group);
        }
    }
    _point_cache.clear();

    _modified = true;
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
    _modified = true;
}

void Editer::append(const Geo::Rectangle &rect)
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
    _modified = true;
}

void Editer::append_bezier(const size_t order)
{
    store_backup();
    _point_cache.pop_back();
    while ((_point_cache.size() - 1) % order > 0)
    {
        _point_cache.pop_back();
    }
    _graph->append(new Geo::Bezier(_point_cache.begin(), _point_cache.end(), order), _current_group);
    Geo::Bezier *b = reinterpret_cast<Geo::Bezier *>(_graph->back().back());
    _point_cache.clear();
}

void Editer::translate_points(Geo::Geometry *points, const double x0, const double y0, const double x1, const double y1, const bool change_shape)
{
    const double catch_distance = GlobalSetting::get_instance()->setting()["catch_distance"].toDouble();
    switch (points->memo()["Type"].to_int())
    {
    case 0:
        {
            Container *temp = reinterpret_cast<Container *>(points);
            size_t count = 0;
            if (change_shape && !points->shape_fixed())
            {
                for (Geo::Point &point : temp->shape())
                {
                    ++count;
                    if (Geo::distance(x0, y0, point.coord().x, point.coord().y) <= catch_distance ||
                        Geo::distance(x1, y1, point.coord().x, point.coord().y) <= catch_distance)
                    {
                        if (temp->size() == 5)
                        {
                            if (temp->shape()[0].coord().y == temp->shape()[1].coord().y
                                && temp->shape()[2].coord().y == temp->shape()[3].coord().y
                                && temp->shape()[0].coord().x == temp->shape()[3].coord().x
                                && temp->shape()[2].coord().x == temp->shape()[1].coord().x)
                            {
                                point.translate(x1 - x0, y1 - y0);
                                if (count % 2 != 0)
                                {
                                    temp->shape()[count == 1 ? 3 : 1].coord().x = point.coord().x;
                                    temp->shape()[count].coord().y = point.coord().y;
                                }
                                else
                                {
                                    temp->shape()[count - 2].coord().y = point.coord().y;
                                    temp->shape()[count].coord().x = point.coord().x;
                                    if (count == 4)
                                    {
                                        temp->shape().front().coord().x = temp->shape().back().coord().x;
                                    }
                                }
                            }
                            else if (temp->shape()[0].coord().x == temp->shape()[1].coord().x
                                && temp->shape()[2].coord().x == temp->shape()[3].coord().x
                                && temp->shape()[0].coord().y == temp->shape()[3].coord().y
                                && temp->shape()[2].coord().y == temp->shape()[1].coord().y)
                            {
                                point.translate(x1 - x0, y1 - y0);
                                if (count % 2 == 0)
                                {
                                    temp->shape()[count - 2].coord().x = point.coord().x;
                                    temp->shape()[count].coord().y = point.coord().y;
                                    if (count == 4)
                                    {
                                        temp->shape().front().coord().y = temp->shape().back().coord().y;
                                    }
                                }
                                else
                                {
                                    temp->shape()[count == 1 ? 3 : 1].coord().y = point.coord().y;
                                    temp->shape()[count].coord().x = point.coord().x;
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
                        if (count == 1)
                        {
                            temp->shape().back().coord() = temp->shape().front().coord();
                        }
                        _modified = true;
                        return;
                    }
                }
            }
            temp->translate(x1 - x0, y1 - y0);
        }
        break;
    case 1:
        {
            CircleContainer *temp = reinterpret_cast<CircleContainer *>(points);
            if (change_shape && !points->shape_fixed() &&
                (std::abs(temp->radius() - Geo::distance(temp->center(), Geo::Point(x0, y0))) <= catch_distance ||
                 std::abs(temp->radius() - Geo::distance(temp->center(), Geo::Point(x1, y1))) <= catch_distance))
            {
                temp->radius() = Geo::distance(temp->center(), Geo::Point(x1, y1));
            }
            else
            {
                temp->translate(x1 - x0, y1 - y0);
            }
        }
        break;
    case 2:
        {
            Link *temp = reinterpret_cast<Link *>(points);
            if (change_shape && !points->shape_fixed())
            {
                for (Geo::Point &point : *temp)
                {
                    if (Geo::distance(x0, y0, point.coord().x, point.coord().y) <= catch_distance ||
                        Geo::distance(x1, y1, point.coord().x, point.coord().y) <= catch_distance)
                    {
                        point.translate(x1 - x0, y1 - y0);
                        _modified = true;
                        return;
                    }
                }
            }
            temp->translate(x1 - x0, y1 - y0);
        }
        break;
    case 20:
        {
            Geo::Polyline *temp = reinterpret_cast<Geo::Polyline *>(points);
            if (change_shape && !points->shape_fixed())
            {
                for (Geo::Point &point : *temp)
                {
                    if (Geo::distance(x0, y0, point.coord().x, point.coord().y) <= catch_distance ||
                        Geo::distance(x1, y1, point.coord().x, point.coord().y) <= catch_distance)
                    {
                        point.translate(x1 - x0, y1 - y0);
                        _modified = true;
                        return;
                    }
                }
            }
            temp->translate(x1 - x0, y1 - y0);
        }
        break;
    case 21:
        {
            Geo::Bezier *temp = reinterpret_cast<Geo::Bezier *>(points);
            if (temp->memo()["is_selected"].to_bool())
            {
                for (size_t i = 0, count = temp->size(); i < count; ++i)
                {
                    if (Geo::distance(x0, y0, (*temp)[i].coord().x, (*temp)[i].coord().y) <= catch_distance ||
                        Geo::distance(x1, y1, (*temp)[i].coord().x, (*temp)[i].coord().y) <= catch_distance)
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
                                for (int j = i - 2; j  > 2; j -= 2)
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
                                for (int j = i; j  > 2; j -= 2)
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
                                for (int j = i - 1; j  > 2; j -= 2)
                                {
                                    (*temp)[j - 2] = (*temp)[j - 1] + ((*temp)[j - 1] - (*temp)[j]).normalize() * Geo::distance((*temp)[j - 2], (*temp)[j - 1]);
                                }
                            }
                        }
                        temp->update_shape();
                        _modified = true;
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
    _modified = true;
}

bool Editer::remove_selected()
{
    if (_graph == nullptr || _graph->empty() || selected_count() == 0)
    {
        return false;
    }
    store_backup();

    for (std::vector<Geo::Geometry *>::iterator it = _graph->container_group(_current_group).begin(); it != _graph->container_group(_current_group).end();)
    { // Polyline Container CircleContainer
        if ((*it)->memo()["is_selected"].to_bool())
        {
            switch ((*it)->memo()["Type"].to_int())
            {
            case 0:
            case 1:
                for (Geo::Geometry *link : (*it)->related())
                {
                    if (dynamic_cast<Link *>(link) != nullptr)
                    {
                        link->memo()["is_selected"] = true;
                        if (reinterpret_cast<Link *>(link)->tail() == *it)
                        {
                            reinterpret_cast<Link *>(link)->set_tail(nullptr);
                        }
                        else
                        {
                            reinterpret_cast<Link *>(link)->set_head(nullptr);
                        }
                    }
                }
                it = _graph->container_group(_current_group).remove(it);
                break;
            case 20:
            case 21:
                it = _graph->container_group(_current_group).remove(it);
                break;
            default:
                break;
            }
        }
        else
        {
            ++it;
        }
    }

    for (std::vector<Geo::Geometry *>::iterator it = _graph->container_group(_current_group).begin(); it != _graph->container_group(_current_group).end();)
    {
        if ((*it)->memo()["is_selected"].to_bool() && dynamic_cast<Link *>(*it) != nullptr)
        {
            if (reinterpret_cast<Link *>(*it)->head() != nullptr)
            {
                reinterpret_cast<Link *>(*it)->head()->related().erase(std::find(reinterpret_cast<Link *>(*it)->head()->related().begin(),
                    reinterpret_cast<Link *>(*it)->head()->related().end(), reinterpret_cast<Link *>(*it)));
            }
            if (reinterpret_cast<Link *>(*it)->tail() != nullptr)
            {
                reinterpret_cast<Link *>(*it)->tail()->related().erase(std::find(reinterpret_cast<Link *>(*it)->tail()->related().begin(),
                    reinterpret_cast<Link *>(*it)->tail()->related().end(), reinterpret_cast<Link *>(*it)));
            }
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
    std::list<Link *> copyed_links;
    std::list<Link *>::iterator tail_it, head_it;
    int container_id = 0, circlecontainer_id = 0;
    for (const Geo::Geometry *container : _graph->container_group(_current_group))
    {
        if (container->memo()["is_selected"].to_bool())
        {
            switch (container->memo()["Type"].to_int())
            {
            case 0:
                _paste_table.push_back(reinterpret_cast<Container *>(const_cast<Geo::Geometry *>(container))->clone());
                _paste_table.back()->memo()["id"] = ++container_id * 10;
                for (Geo::Geometry *link : container->related())
                {
                    if (dynamic_cast<Link *>(link) != nullptr)
                    {
                        copyed_links.push_back(reinterpret_cast<Link *>(link));
                        if (reinterpret_cast<Link *>(link)->tail() == container)
                        {
                            link->memo()["tail_id"] = container_id * 10;
                        }
                        else
                        {
                            link->memo()["head_id"] = container_id * 10;
                        }
                    }
                }
                break;
            case 1:
                _paste_table.push_back(reinterpret_cast<CircleContainer *>(const_cast<Geo::Geometry *>(container))->clone());
                _paste_table.back()->memo()["id"] = ++circlecontainer_id * 10 + 1;
                for (Geo::Geometry *link : container->related())
                {
                    if (dynamic_cast<Link *>(link) != nullptr)
                    {
                        copyed_links.push_back(reinterpret_cast<Link *>(link));
                        if (reinterpret_cast<Link *>(link)->tail() == container)
                        {
                            link->memo()["tail_id"] = circlecontainer_id * 10 + 1;
                        }
                        else
                        {
                            link->memo()["head_id"] = circlecontainer_id * 10 + 1;
                        }
                    }
                }
                break;
            case 20:
                _paste_table.push_back(reinterpret_cast<Geo::Polyline *>(const_cast<Geo::Geometry *>(container))->clone());
                break;
            case 21:
                _paste_table.push_back(reinterpret_cast<Geo::Bezier *>(const_cast<Geo::Geometry *>(container))->clone());
                break;
            default:
                break;
            }
        }
    }

    while (!copyed_links.empty())
    {
        if (std::count(copyed_links.cbegin(), copyed_links.cend(), copyed_links.back()) >= 2)
        {
            copyed_links.erase(std::find(copyed_links.begin(), copyed_links.end(), copyed_links.back()));
            container_id = 0;
            _paste_table.push_back(copyed_links.back()->clone());
            /* for (Geo::Geometry *geo : _paste_table)
            {
                if (geo->memo().has("id"))
                {
                    if (geo->memo()["id"].to_int() == copyed_links.back()->memo()["tail_id"].to_int())
                    {
                        reinterpret_cast<Link*>(_paste_table.back())->set_tail(geo);
                        ++container_id;
                    }
                    else if (geo->memo()["id"].to_int() == copyed_links.back()->memo()["head_id"].to_int())
                    {
                        reinterpret_cast<Link*>(_paste_table.back())->set_head(geo);
                        ++container_id;
                    }
                    if (container_id == 2)
                    {
                        break;
                    }
                }
            } */
        }
        copyed_links.pop_back();
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
    std::list<Link *> copyed_links;
    int container_id = 0, circlecontainer_id = 0;
    for (std::vector<Geo::Geometry *>::iterator it = _graph->container_group(_current_group).begin(); it != _graph->container_group(_current_group).end();)
    {
        if ((*it)->memo()["is_selected"].to_bool())
        {
            switch ((*it)->memo()["Type"].to_int())
            {
            case 0:
                _paste_table.push_back(reinterpret_cast<Container *>(const_cast<Geo::Geometry *>(*it))->clone());
                _paste_table.back()->memo()["id"] = ++container_id * 10;
                for (Geo::Geometry *link : (*it)->related())
                {
                    if (dynamic_cast<Link *>(link) != nullptr)
                    {
                        copyed_links.push_back(reinterpret_cast<Link *>(link));
                        if (reinterpret_cast<Link *>(link)->tail() == *it)
                        {
                            link->memo()["tail_id"] = container_id * 10;
                        }
                        else
                        {
                            link->memo()["head_id"] = container_id * 10;
                        }
                        link->memo()["remove"] = true;
                    }
                }
                break;
            case 1:
                _paste_table.push_back(reinterpret_cast<CircleContainer *>(const_cast<Geo::Geometry *>(*it))->clone());
                _paste_table.back()->memo()["id"] = ++circlecontainer_id * 10 + 1;
                for (Geo::Geometry *link : (*it)->related())
                {
                    if (dynamic_cast<Link *>(link) != nullptr)
                    {
                        copyed_links.push_back(reinterpret_cast<Link *>(link));
                        if (reinterpret_cast<Link *>(link)->tail() == *it)
                        {
                            link->memo()["tail_id"] = circlecontainer_id * 10 + 1;
                        }
                        else
                        {
                            link->memo()["head_id"] = circlecontainer_id * 10 + 1;
                        }
                        link->memo()["remove"] = true;
                    }
                }
                break;
            case 20:
                _paste_table.push_back(reinterpret_cast<Geo::Polyline *>(const_cast<Geo::Geometry *>(*it))->clone());
                break;
            case 21:
                _paste_table.push_back(reinterpret_cast<Geo::Bezier *>(const_cast<Geo::Geometry *>(*it))->clone());
                break;
            default:
                break;
            }
            it = _graph->container_group(_current_group).remove(it);
        }
        else
        {
            ++it;
        }
    }

    while (!copyed_links.empty())
    {
        if (std::count(copyed_links.cbegin(), copyed_links.cend(), copyed_links.back()) >= 2)
        {
            copyed_links.erase(std::find(copyed_links.begin(), copyed_links.end(), copyed_links.back()));
            container_id = 0;
            _paste_table.push_back(reinterpret_cast<Link *>(copyed_links.back())->clone());
            /*  for (Geo::Geometry *geo : _paste_table)
             {
                 if (geo->memo().has("id"))
                 {
                     if (geo->memo()["id"].to_int() == copyed_links.back()->memo()["tail_id"].to_int())
                     {
                         reinterpret_cast<Link*>(_paste_table.back())->set_tail(geo);
                         ++container_id;
                     }
                     else if (geo->memo()["id"].to_int() == copyed_links.back()->memo()["head_id"].to_int())
                     {
                         reinterpret_cast<Link*>(_paste_table.back())->set_head(geo);
                         ++container_id;
                     }
                     if (container_id == 2)
                     {
                         break;
                     }
                 }
             } */
        }
        copyed_links.pop_back();
    }
    for (std::vector<Geo::Geometry *>::iterator it = _graph->container_group(_current_group).begin(); it != _graph->container_group(_current_group).end();)
    {
        if ((*it)->memo().has("remove") && (*it)->memo()["remove"].to_bool())
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

bool Editer::paste(const double tx, const double ty)
{
    if (_paste_table.empty() || _graph == nullptr)
    {
        return false;
    }
    store_backup();
    reset_selected_mark();

    std::vector<Geo::Geometry *> pasted_containers;
    for (Geo::Geometry *geo : _paste_table)
    {
        switch (geo->memo()["Type"].to_int())
        {
        case 0:
            _graph->container_group(_current_group).append(reinterpret_cast<Container *>(geo)->clone());
            break;
        case 1:
            _graph->container_group(_current_group).append(reinterpret_cast<CircleContainer *>(geo)->clone());
            break;
        case 2:
            _graph->container_group(_current_group).append(reinterpret_cast<Link *>(geo)->clone());
            break;
        case 20:
            _graph->container_group(_current_group).append(reinterpret_cast<Geo::Polyline *>(geo)->clone());
            break;
        case 21:
            _graph->container_group(_current_group).append(reinterpret_cast<Geo::Bezier *>(geo)->clone());
            break;
        default:
            break;
        }
        _graph->container_group(_current_group).back()->translate(tx, ty);
        pasted_containers.push_back(_graph->container_group(_current_group).back());
    }

    size_t count;
    for (Geo::Geometry *link : pasted_containers)
    {
        if (dynamic_cast<Link *>(link) == nullptr)
        {
            continue;
        }
        count = 0;
        for (Geo::Geometry *geo : pasted_containers)
        {
            if (geo->memo().has("id"))
            {
                if (geo->memo()["id"].to_int() == reinterpret_cast<Link *>(link)->memo()["tail_id"].to_int())
                {
                    reinterpret_cast<Link *>(link)->set_tail(geo);
                    geo->related().push_back(link);
                    ++count;
                }
                else if (geo->memo()["id"].to_int() == reinterpret_cast<Link *>(link)->memo()["head_id"].to_int())
                {
                    reinterpret_cast<Link *>(link)->set_head(geo);
                    geo->related().push_back(link);
                    ++count;
                }
                if (count == 2)
                {
                    break;
                }
            }
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

    std::vector<Link *> head_links, tail_links;
    Container *c = nullptr;
    CircleContainer *cc = nullptr;
    Link *l = nullptr;
    Geo::Polyline *p = nullptr;
    Geo::Bezier *b = nullptr;
    for (std::vector<Geo::Geometry *>::reverse_iterator it = _graph->container_group(_current_group).rbegin(),
                                                        end = _graph->container_group(_current_group).rend();
         it != end; ++it)
    {
        switch ((*it)->memo()["Type"].to_int())
        {
        case 0:
            c = reinterpret_cast<Container *>(*it);
            if (Geo::is_inside(point, c->shape(), true))
            {
                c->memo()["is_selected"] = true;
                _graph->container_group(_current_group).pop(it);
                _graph->container_group(_current_group).append(c);
                return c;
            }
            c = nullptr;
            break;
        case 1:
            cc = reinterpret_cast<CircleContainer *>(*it);
            if (Geo::is_inside(point, cc->shape(), true))
            {
                cc->memo()["is_selected"] = true;
                _graph->container_group(_current_group).pop(it);
                _graph->container_group(_current_group).append(cc);
                return cc;
            }
            cc = nullptr;
            break;
        case 2:
            l = reinterpret_cast<Link *>(*it);
            if (l->size() > 0)
            {
                for (size_t i = 1, count = l->size(); i < count; ++i)
                {
                    if (Geo::distance(point, (*l)[i - 1], (*l)[i]) <= 2)
                    {
                        l->memo()["is_selected"] = true;
                        _graph->container_group(_current_group).pop(it);
                        _graph->container_group(_current_group).append(l);
                        return l;
                    }
                }
                if (dynamic_cast<CircleContainer *>(l->tail()) != nullptr)
                {
                    if (Geo::distance(point, reinterpret_cast<CircleContainer *>(l->tail())->center(), l->front()) <= 2)
                    {
                        l->memo()["is_selected"] = true;
                        _graph->container_group(_current_group).pop(it);
                        _graph->container_group(_current_group).append(l);
                        return l;
                    }
                }
                else
                {
                    if (Geo::distance(point, l->tail()->bounding_rect().center(), l->front()) <= 2)
                    {
                        l->memo()["is_selected"] = true;
                        _graph->container_group(_current_group).pop(it);
                        _graph->container_group(_current_group).append(l);
                        return l;
                    }
                }
                if (dynamic_cast<CircleContainer *>(l->head()) != nullptr)
                {
                    if (Geo::distance(point, l->back(), reinterpret_cast<CircleContainer *>(l->head())->center()) <= 2)
                    {
                        l->memo()["is_selected"] = true;
                        _graph->container_group(_current_group).pop(it);
                        _graph->container_group(_current_group).append(l);
                        return l;
                    }
                }
                else
                {
                    if (Geo::distance(point, l->back(), l->head()->bounding_rect().center()) <= 2)
                    {
                        l->memo()["is_selected"] = true;
                        _graph->container_group(_current_group).pop(it);
                        _graph->container_group(_current_group).append(l);
                        return l;
                    }
                }
            }
            else
            {
                if (dynamic_cast<CircleContainer *>(l->tail()) != nullptr)
                {
                    if (dynamic_cast<CircleContainer *>(l->head()) != nullptr)
                    {
                        if (Geo::distance(point, reinterpret_cast<CircleContainer *>(l->tail())->center(), reinterpret_cast<CircleContainer *>(l->head())->center()) <= 2)
                        {
                            l->memo()["is_selected"] = true;
                            _graph->container_group(_current_group).pop(it);
                            _graph->container_group(_current_group).append(l);
                            return l;
                        }
                    }
                    else
                    {
                        if (Geo::distance(point, reinterpret_cast<CircleContainer *>(l->tail())->center(), l->head()->bounding_rect().center()) <= 2)
                        {
                            l->memo()["is_selected"] = true;
                            _graph->container_group(_current_group).pop(it);
                            _graph->container_group(_current_group).append(l);
                            return l;
                        }
                    }
                }
                else
                {
                    if (dynamic_cast<CircleContainer *>(l->head()) != nullptr)
                    {
                        if (Geo::distance(point, l->tail()->bounding_rect().center(), reinterpret_cast<CircleContainer *>(l->head())->center()) <= 2)
                        {
                            l->memo()["is_selected"] = true;
                            _graph->container_group(_current_group).pop(it);
                            _graph->container_group(_current_group).append(l);
                            return l;
                        }
                    }
                    else
                    {
                        if (Geo::distance(point, l->tail()->bounding_rect().center(), l->head()->bounding_rect().center()) <= 2)
                        {
                            l->memo()["is_selected"] = true;
                            _graph->container_group(_current_group).pop(it);
                            _graph->container_group(_current_group).append(l);
                            return l;
                        }
                    }
                }
            }
            l = nullptr;
            break;
        case 20:
            p = reinterpret_cast<Geo::Polyline *>(*it);
            for (size_t i = 1, count = p->size(); i < count; ++i)
            {
                if (Geo::distance(point, (*p)[i - 1], (*p)[i]) <= 2)
                {
                    p->memo()["is_selected"] = true;
                    _graph->container_group(_current_group).pop(it);
                    _graph->container_group(_current_group).append(p);
                    return p;
                }
            }
            p = nullptr;
            break;
        case 21:
            b = reinterpret_cast<Geo::Bezier*>(*it);
            if (b->memo()["is_selected"].to_bool())
            {
                for (const Geo::Point &inner_point : *b)
                {
                    if (Geo::distance(point, inner_point) <= 3)
                    {
                        return b;
                    }
                }
            }
            for (size_t i = 1, count = b->shape().size(); i < count; ++i)
            {
                if (Geo::distance(point, b->shape()[i - 1], b->shape()[i]) <= 2)
                {
                    b->memo()["is_selected"] = true;
                    _graph->container_group(_current_group).pop(it);
                    _graph->container_group(_current_group).append(b);
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
    Geo::Geometry *p = select(Geo::Point(x, y), reset_others);
    return p;
}

std::vector<Geo::Geometry *> Editer::select(const Geo::Rectangle &rect)
{
    std::vector<Geo::Geometry *> result;
    if (rect.empty() || _graph == nullptr || _graph->empty())
    {
        return result;
    }

    Link *link = nullptr;
    for (Geo::Geometry *container : _graph->container_group(_current_group))
    {
        switch (container->memo()["Type"].to_int())
        {
        case 0:
            if (Geo::is_intersected(rect, reinterpret_cast<Container *>(container)->shape()))
            {
                container->memo()["is_selected"] = true;
                result.push_back(container);
            }
            else
            {
                container->memo()["is_selected"] = false;
            }
            break;
        case 1:
            if (Geo::is_intersected(rect, reinterpret_cast<CircleContainer *>(container)->shape()))
            {
                container->memo()["is_selected"] = true;
                result.push_back(container);
            }
            else
            {
                container->memo()["is_selected"] = false;
            }
            break;
        case 2:
            link = reinterpret_cast<Link *>(container);
            if (link->size() > 0)
            {
                if (link->size() > 1 && Geo::is_intersected(rect, *link))
                {
                    link->memo()["is_selected"] = true;
                    result.push_back(link);
                }
                if (dynamic_cast<CircleContainer *>(link->tail()) != nullptr)
                {
                    if (Geo::is_intersected(rect, reinterpret_cast<CircleContainer *>(link->tail())->center(), link->front()))
                    {
                        link->memo()["is_selected"] = true;
                        result.push_back(link);
                    }
                }
                else
                {
                    if (Geo::is_intersected(rect, link->tail()->bounding_rect().center(), link->front()))
                    {
                        link->memo()["is_selected"] = true;
                        result.push_back(link);
                    }
                }
                if (dynamic_cast<CircleContainer *>(link->head()) != nullptr)
                {
                    if (Geo::is_intersected(rect, link->back(), reinterpret_cast<CircleContainer *>(link->head())->center()))
                    {
                        link->memo()["is_selected"] = true;
                        result.push_back(link);
                    }
                }
                else
                {
                    if (Geo::is_intersected(rect, link->back(), link->head()->bounding_rect().center()))
                    {
                        link->memo()["is_selected"] = true;
                        result.push_back(link);
                    }
                }
            }
            else
            {
                if (dynamic_cast<CircleContainer *>(link->tail()) != nullptr)
                {
                    if (dynamic_cast<CircleContainer *>(link->head()) != nullptr)
                    {
                        if (Geo::is_intersected(rect, reinterpret_cast<CircleContainer *>(link->tail())->center(), reinterpret_cast<CircleContainer *>(link->head())->center()))
                        {
                            link->memo()["is_selected"] = true;
                            result.push_back(link);
                        }
                    }
                    else
                    {
                        if (Geo::is_intersected(rect, reinterpret_cast<CircleContainer *>(link->tail())->center(), link->head()->bounding_rect().center()))
                        {
                            link->memo()["is_selected"] = true;
                            result.push_back(link);
                        }
                    }
                }
                else
                {
                    if (dynamic_cast<CircleContainer *>(link->head()) != nullptr)
                    {
                        if (Geo::is_intersected(rect, link->tail()->bounding_rect().center(), reinterpret_cast<CircleContainer *>(link->head())->center()))
                        {
                            link->memo()["is_selected"] = true;
                            result.push_back(link);
                        }
                    }
                    else
                    {
                        if (Geo::is_intersected(rect, link->tail()->bounding_rect().center(), link->head()->bounding_rect().center()))
                        {
                            link->memo()["is_selected"] = true;
                            result.push_back(link);
                        }
                    }
                }
            }
            link = nullptr;
            break;
        case 20:
            if (Geo::is_intersected(rect, *reinterpret_cast<Geo::Polyline *>(container)))
            {
                container->memo()["is_selected"] = true;
                result.push_back(container);
            }
            else
            {
                container->memo()["is_selected"] = false;
            }
            break;
        case 21:
            if (Geo::is_intersected(rect, reinterpret_cast<Geo::Bezier *>(container)->shape()))
            {
                container->memo()["is_selected"] = true;
                result.push_back(container);
            }
            else
            {
                container->memo()["is_selected"] = false;
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
        if (container->memo()["is_selected"].to_bool())
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
                           { return c->memo()["is_selected"].to_bool(); });
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
        container->memo()["is_selected"] = value;
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




void Editer::remove_group(const size_t index)
{
    assert(index < _graph->container_groups().size());
    _graph->container_groups().erase(_graph->begin() + index);
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



void Editer::rotate(const double angle, const bool unitary)
{
    const double rad = angle * Geo::PI / 180;
    Geo::Coord coord;
    if (unitary)
    {
        coord = _graph->container_group(_current_group).bounding_rect().center().coord();
        for (Geo::Geometry *geo : _graph->container_group(_current_group))
        {
            geo->rotate(coord.x, coord.y, rad);
        }
    }
    else
    {
        for (Geo::Geometry *geo : _graph->container_group(_current_group))
        {
            if (geo->memo()["is_selected"].to_bool())
            {
                coord = geo->bounding_rect().center().coord();
                geo->rotate(coord.x, coord.y, rad);
            }
        }
    }
}

void Editer::flip(const bool direction, const bool unitary)
{
    Geo::Coord coord;
    if (unitary)
    {
        coord = _graph->container_group(_current_group).bounding_rect().center().coord();
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
    else
    {
        for (Geo::Geometry *geo : _graph->container_group(_current_group))
        {
            if (geo->memo()["is_selected"].to_bool())
            {
                coord = geo->bounding_rect().center().coord();
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
}

bool Editer::auto_aligning(Geo::Geometry *points, std::list<QLineF> &reflines, const bool current_group_only)
{   
    if (points == nullptr || _graph == nullptr || _graph->empty())
    {
        return false;
    }

    const Geo::Rectangle rect(points->bounding_rect());
    Geo::Coord center(rect.center().coord());
    double left = rect.left(), top = rect.top(), right = rect.right(), bottom = rect.bottom();
    const double heigh = top - bottom, width = right - left;
    Geo::Geometry *dst = nullptr;
    double temp, distance = DBL_MAX;

    if (current_group_only)
    {
        for (Geo::Geometry *geo : _graph->container_group(_current_group))
        {
            if (geo->memo()["Type"].to_int() / 10 == 2 || geo->memo()["Type"].to_int() == 2 || geo == points)
            {
                continue;
            }

            switch (geo->memo()["Type"].to_int())
            {
            case 0:
                temp = Geo::distance(center, reinterpret_cast<Container *>(geo)->shape());
                break;
            case 1:
                temp = Geo::distance(center, reinterpret_cast<CircleContainer *>(geo)->center());
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
                if (geo->memo()["Type"].to_int() / 10 == 2 || geo->memo()["Type"].to_int() == 2 || geo == points)
                {
                    continue;
                }

                switch (geo->memo()["Type"].to_int())
                {
                case 0:
                    temp = Geo::distance(center, reinterpret_cast<Container *>(geo)->shape());
                    break;
                case 1:
                    temp = Geo::distance(center, reinterpret_cast<CircleContainer *>(geo)->center());
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

    if (dst == nullptr)
    {
        return false;
    }

    const size_t count = reflines.size();
    const Geo::Rectangle dst_rect(dst->bounding_rect());
    const Geo::Coord dst_center(dst_rect.center().coord());
    const double dst_left = dst_rect.left(), dst_top = dst_rect.top(), dst_right = dst_rect.right(), dst_bottom = dst_rect.bottom();
    const double dst_heigh = dst_top - dst_bottom, dst_width = dst_right - dst_left;

    if (std::abs(dst_center.x - center.x) < 2)
    {
        reflines.emplace_back(QLineF(dst_center.x, std::min(top, dst_top), dst_center.x, std::max(bottom, dst_bottom)));
        points->translate(dst_center.x - center.x, 0);
        left += (dst_center.x - center.x);
        right += (dst_center.x - center.x);
        center.x = dst_center.x;
    }
    if (std::abs(dst_center.y - center.y) < 2)
    {
        reflines.emplace_back(QLineF(std::min(left, dst_left), dst_center.y, std::max(right, dst_right), dst_center.y));
        points->translate(0, dst_center.y - center.y);
        top += (dst_center.y - center.y);
        bottom += (dst_center.y - center.y);
        center.y = dst_center.y;
    }
    if (std::abs(dst_top - top) < 2)
    {
        reflines.emplace_back(QLineF(std::min(left, dst_left), dst_top, std::max(right, dst_right), dst_top));
        points->translate(0, dst_top - top);
        center.y += (dst_top - top);
        bottom += (dst_top - top);
        top = dst_top;
    }
    if (std::abs(dst_bottom - top) < 2)
    {
        reflines.emplace_back(QLineF(std::min(left, dst_left), dst_bottom, std::max(right, dst_right), dst_bottom));
        points->translate(0, dst_bottom - top);
        center.y += (dst_bottom - top);
        bottom += (dst_bottom - top);
        top = dst_bottom;
    }
    if (std::abs(dst_top - bottom) < 2)
    {
        reflines.emplace_back(QLineF(std::min(left, dst_left), dst_top, std::max(right, dst_right), dst_top));
        points->translate(0, dst_top - bottom);
        center.y += (dst_top - bottom);
        top += (dst_top - bottom);
        bottom = dst_top;
    }
    if (std::abs(dst_bottom - bottom) < 2)
    {
        reflines.emplace_back(QLineF(std::min(left, dst_left), dst_bottom, std::max(right, dst_right), dst_bottom));
        points->translate(0, dst_bottom - bottom);
        center.y += (dst_bottom - bottom);
        top += (dst_bottom - bottom);
        bottom = dst_bottom;
    }
    if (std::abs(dst_left - left) < 2)
    {
        reflines.emplace_back(QLineF(dst_left, std::max(bottom, dst_bottom), dst_left, std::min(top, dst_top)));
        points->translate(dst_left - left, 0);
        center.x += (dst_left - left);
        right += (dst_left - left);
        left = dst_left;
    }
    if (std::abs(dst_right - left) < 2)
    {
        reflines.emplace_back(QLineF(dst_right, std::max(bottom, dst_bottom), dst_right, std::min(top, dst_top)));
        points->translate(dst_right - left, 0);
        center.x += (dst_right - left);
        right += (dst_right - left);
        left = dst_right;
    }
    if (std::abs(dst_left - right) < 2)
    {
        reflines.emplace_back(QLineF(dst_left, std::max(bottom, dst_bottom), dst_left, std::min(top, dst_top)));
        points->translate(dst_left - right, 0);
        center.x += (dst_left - right);
        left += (dst_left - right);
        right = dst_left;
    }
    if (std::abs(dst_right - right) < 2)
    {
        reflines.emplace_back(QLineF(dst_right, std::max(bottom, dst_bottom), dst_right, std::min(top, dst_top)));
        points->translate(dst_right - right, 0);
        center.x += (dst_right - right);
        left += (dst_right - right);
        right = dst_right;
    }
    
    return count != reflines.size();
}

bool Editer::auto_aligning(Geo::Geometry *points, const double x, const double y, std::list<QLineF> &reflines, const bool current_group_only)
{
    if (points == nullptr || _graph == nullptr || _graph->empty())
    {
        return false;
    }

    const Geo::Rectangle rect(points->bounding_rect());
    const Geo::Point anchor(x, y);
    Geo::Coord center(rect.center().coord());
    double left = rect.left(), top = rect.top(), right = rect.right(), bottom = rect.bottom();
    const double heigh = top - bottom, width = right - left;
    Geo::Geometry *dst = nullptr;
    double temp, distance = DBL_MAX;

    if (current_group_only)
    {
        for (Geo::Geometry *geo : _graph->container_group(_current_group))
        {
            if (geo->memo()["Type"].to_int() / 10 == 2 || geo->memo()["Type"].to_int() == 2 || geo == points)
            {
                continue;
            }

            switch (geo->memo()["Type"].to_int())
            {
            case 0:
                temp = Geo::distance(anchor, reinterpret_cast<Container *>(geo)->shape());
                break;
            case 1:
                temp = Geo::distance(anchor, reinterpret_cast<CircleContainer *>(geo)->center());
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
                if (geo->memo()["Type"].to_int() / 10 == 2 || geo->memo()["Type"].to_int() == 2 || geo == points)
                {
                    continue;
                }

                switch (geo->memo()["Type"].to_int())
                {
                case 0:
                    temp = Geo::distance(anchor, reinterpret_cast<Container *>(geo)->shape());
                    break;
                case 1:
                    temp = Geo::distance(anchor, reinterpret_cast<CircleContainer *>(geo)->center());
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

    if (dst == nullptr)
    {
        return false;
    }

    const size_t count = reflines.size();
    const Geo::Rectangle dst_rect(dst->bounding_rect());
    const Geo::Coord dst_center(dst_rect.center().coord());
    const double dst_left = dst_rect.left(), dst_top = dst_rect.top(), dst_right = dst_rect.right(), dst_bottom = dst_rect.bottom();
    const double dst_heigh = dst_top - dst_bottom, dst_width = dst_right - dst_left;

    if (std::abs(dst_center.x - center.x) < 2)
    {
        reflines.emplace_back(QLineF(dst_center.x, std::min(top, dst_top), dst_center.x, std::max(bottom, dst_bottom)));
        points->translate(dst_center.x - center.x, 0);
        left += (dst_center.x - center.x);
        right += (dst_center.x - center.x);
        center.x = dst_center.x;
    }
    if (std::abs(dst_center.y - center.y) < 2)
    {
        reflines.emplace_back(QLineF(std::min(left, dst_left), dst_center.y, std::max(right, dst_right), dst_center.y));
        points->translate(0, dst_center.y - center.y);
        top += (dst_center.y - center.y);
        bottom += (dst_center.y - center.y);
        center.y = dst_center.y;
    }
    if (std::abs(dst_top - top) < 2)
    {
        reflines.emplace_back(QLineF(std::min(left, dst_left), dst_top, std::max(right, dst_right), dst_top));
        points->translate(0, dst_top - top);
        center.y += (dst_top - top);
        bottom += (dst_top - top);
        top = dst_top;
    }
    if (std::abs(dst_bottom - top) < 2)
    {
        reflines.emplace_back(QLineF(std::min(left, dst_left), dst_bottom, std::max(right, dst_right), dst_bottom));
        points->translate(0, dst_bottom - top);
        center.y += (dst_bottom - top);
        bottom += (dst_bottom - top);
        top = dst_bottom;
    }
    if (std::abs(dst_top - bottom) < 2)
    {
        reflines.emplace_back(QLineF(std::min(left, dst_left), dst_top, std::max(right, dst_right), dst_top));
        points->translate(0, dst_top - bottom);
        center.y += (dst_top - bottom);
        top += (dst_top - bottom);
        bottom = dst_top;
    }
    if (std::abs(dst_bottom - bottom) < 2)
    {
        reflines.emplace_back(QLineF(std::min(left, dst_left), dst_bottom, std::max(right, dst_right), dst_bottom));
        points->translate(0, dst_bottom - bottom);
        center.y += (dst_bottom - bottom);
        top += (dst_bottom - bottom);
        bottom = dst_bottom;
    }
    if (std::abs(dst_left - left) < 2)
    {
        reflines.emplace_back(QLineF(dst_left, std::max(bottom, dst_bottom), dst_left, std::min(top, dst_top)));
        points->translate(dst_left - left, 0);
        center.x += (dst_left - left);
        right += (dst_left - left);
        left = dst_left;
    }
    if (std::abs(dst_right - left) < 2)
    {
        reflines.emplace_back(QLineF(dst_right, std::max(bottom, dst_bottom), dst_right, std::min(top, dst_top)));
        points->translate(dst_right - left, 0);
        center.x += (dst_right - left);
        right += (dst_right - left);
        left = dst_right;
    }
    if (std::abs(dst_left - right) < 2)
    {
        reflines.emplace_back(QLineF(dst_left, std::max(bottom, dst_bottom), dst_left, std::min(top, dst_top)));
        points->translate(dst_left - right, 0);
        center.x += (dst_left - right);
        left += (dst_left - right);
        right = dst_left;
    }
    if (std::abs(dst_right - right) < 2)
    {
        reflines.emplace_back(QLineF(dst_right, std::max(bottom, dst_bottom), dst_right, std::min(top, dst_top)));
        points->translate(dst_right - right, 0);
        center.x += (dst_right - right);
        left += (dst_right - right);
        right = dst_right;
    }
    
    return count != reflines.size();
}

bool Editer::coord_aligning(Geo::Coord &coord, std::list<QLineF> &reflines, const bool current_group_only)
{
    if (_graph == nullptr || _graph->empty())
    {
        return false;
    }

    const size_t count = reflines.size();
    const Geo::Point anchor(coord);
    Geo::Geometry *dst = nullptr;
    double temp, distance = DBL_MAX;

    if (current_group_only)
    {
        for (Geo::Geometry *geo : _graph->container_group(_current_group))
        {
            if (geo->memo()["Type"].to_int() / 10 == 2 || geo->memo()["Type"].to_int() == 2
                || geo->memo()["is_selected"].to_bool())
            {
                continue;
            }

            switch (geo->memo()["Type"].to_int())
            {
            case 0:
                temp = Geo::distance(anchor, reinterpret_cast<Container *>(geo)->shape());
                break;
            case 1:
                temp = Geo::distance(anchor, reinterpret_cast<CircleContainer *>(geo)->center());
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
                if (geo->memo()["Type"].to_int() / 10 == 2 || geo->memo()["Type"].to_int() == 2
                    || geo->memo()["is_selected"].to_bool())
                {
                    continue;
                }

                switch (geo->memo()["Type"].to_int())
                {
                case 0:
                    temp = Geo::distance(anchor, reinterpret_cast<Container *>(geo)->shape());
                    break;
                case 1:
                    temp = Geo::distance(anchor, reinterpret_cast<CircleContainer *>(geo)->center());
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

    if (dst == nullptr)
    {
        return false;
    }

    const Geo::Rectangle dst_rect(dst->bounding_rect());
    const Geo::Coord dst_center(dst_rect.center().coord());
    const double dst_left = dst_rect.left(), dst_top = dst_rect.top(), dst_right = dst_rect.right(), dst_bottom = dst_rect.bottom();
    const double dst_heigh = dst_top - dst_bottom, dst_width = dst_right - dst_left;

    if (std::abs(dst_center.x - coord.x) < 2)
    {
        reflines.emplace_back(QLineF(dst_center.x, std::min(coord.y, dst_top), dst_center.x, std::max(coord.y, dst_bottom)));
        coord.x = dst_center.x;
    }
    if (std::abs(dst_center.y - coord.y) < 2)
    {
        reflines.emplace_back(QLineF(std::min(coord.x, dst_left), dst_center.y, std::max(coord.x, dst_right), dst_center.y));
        coord.y = dst_center.y;
    }
    if (std::abs(dst_top - coord.y) < 2)
    {
        reflines.emplace_back(QLineF(std::min(coord.x, dst_left), dst_top, std::max(coord.x, dst_right), dst_top));
        coord.y = dst_top;
    }
    if (std::abs(dst_bottom - coord.y) < 2)
    {
        reflines.emplace_back(QLineF(std::min(coord.x, dst_left), dst_bottom, std::max(coord.x, dst_right), dst_bottom));
        coord.y = dst_bottom;
    }
    if (std::abs(dst_left - coord.x) < 2)
    {
        reflines.emplace_back(QLineF(dst_left, std::max(coord.y, dst_bottom), dst_left, std::min(coord.y, dst_top)));
        coord.x = dst_left;
    }
    if (std::abs(dst_right - coord.x) < 2)
    {
        reflines.emplace_back(QLineF(dst_right, std::max(coord.y, dst_bottom), dst_right, std::min(coord.y, dst_top)));
        coord.x = dst_right;
    }
    
    return count != reflines.size();
}




void Editer::auto_layering()
{
    if (_graph == nullptr || _graph->empty())
    {
        return;
    }

    std::vector<Geo::Geometry *> all_containers, all_polylines;
    std::vector<Link *> all_links;
    for (ContainerGroup &group : _graph->container_groups())
    {
        while (!group.empty())
        {
            switch (group.back()->memo()["Type"].to_int())
            {
            case 2:
                all_links.emplace_back(reinterpret_cast<Link *>(group.pop_back()));
                break;
            case 20:
            case 21:
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
            switch (item->memo()["Type"].to_int())
            {
            case 20:
                _graph->back().append(reinterpret_cast<Geo::Polyline *>(item));
                break;
            case 21:
                _graph->back().append(reinterpret_cast<Geo::Bezier *>(item));
                break;
            default:
                break;
            }
        }
        return;
    }

    std::sort(all_containers.begin(), all_containers.end(), [](const Geo::Geometry *a, const Geo::Geometry *b)
        {return a->bounding_rect().area() < b->bounding_rect().area();});
    _graph->append_group();
    for (size_t i = all_containers.size() - 1; _graph->back().empty() && i > 0; --i)
    {
        switch (all_containers[i]->memo()["Type"].to_int())
        {
        case 0:
            _graph->back().append(reinterpret_cast<Container *>(all_containers[i]));
            all_containers.erase(all_containers.begin() + i);
            break;
        case 1:
            _graph->back().append(reinterpret_cast<CircleContainer *>(all_containers[i]));
            all_containers.erase(all_containers.begin() + i);
            break;
        default:
            break;
        }
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
            switch (all_containers[i]->memo()["Type"].to_int())
            {
            case 0:
                container = reinterpret_cast<Container *>(all_containers[i]);
                for (Geo::Geometry *geo : _graph->back())
                {
                    switch (geo->memo()["Type"].to_int())
                    {
                    case 0:
                        if (Geo::is_intersected(container->shape(), reinterpret_cast<Container *>(geo)->shape()))
                        {
                            flag = false;
                        }
                        break;
                    case 1:
                        if (Geo::is_inside(reinterpret_cast<CircleContainer *>(geo)->center(), container->shape()))
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
            case 1:
                circle_container = reinterpret_cast<CircleContainer *>(all_containers[i]);
                for (Geo::Geometry *geo : _graph->back())
                {
                    switch (geo->memo()["Type"].to_int())
                    {
                    case 0:
                        if (Geo::is_inside(circle_container->center(), reinterpret_cast<Container *>(geo)->shape()))
                        {
                            flag = false;
                        }
                        break;
                    case 1:
                        if (Geo::is_intersected(circle_container->shape(), reinterpret_cast<CircleContainer *>(geo)->shape()))
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

        for (size_t i = 0, count = all_links.size(); i < count; ++i)
        {
            for (Geo::Geometry *geo : _graph->back())
            {
                if (geo == all_links[i]->head())
                {
                    _graph->back().append(all_links[i--]);
                    --count;
                    it = std::find(all_containers.begin(), all_containers.end(), all_links[i]->tail());
                    if (it != all_containers.end())
                    {
                        switch ((*it)->memo()["Type"].to_int())
                        {
                        case 0:
                            _graph->back().append(reinterpret_cast<Container *>(*it));
                            break;
                        case 1:
                            _graph->back().append(reinterpret_cast<CircleContainer *>(*it));
                            break;
                        default:
                            break;
                        }
                        all_containers.erase(it);
                    }
                }
                else if (geo == all_links[i]->tail())
                {
                    _graph->back().append(all_links[i--]);
                    --count;
                    it = std::find(all_containers.begin(), all_containers.end(), all_links[i]->head());
                    if (it != all_containers.end())
                    {
                        switch ((*it)->memo()["Type"].to_int())
                        {
                        case 0:
                            _graph->back().append(reinterpret_cast<Container *>(*it));
                            break;
                        case 1:
                            _graph->back().append(reinterpret_cast<CircleContainer *>(*it));
                            break;
                        default:
                            break;
                        }
                        all_containers.erase(it);
                    }
                }
            }
        }
    
        _graph->append_group();
    }
    
    _graph->append_group();
    for (Geo::Geometry *geo : all_polylines)
    {
        if (dynamic_cast<Geo::Bezier *>(geo) != nullptr)
        {
            _graph->back().append(reinterpret_cast<Geo::Bezier *>(geo));
        }
        else
        {
            _graph->back().append(reinterpret_cast<Geo::Polyline *>(geo));
        }
    }

    for (size_t i = 0, count = _graph->container_groups().size(); i < count; ++i)
    {
        if (_graph->container_group(i).empty())
        {
            _graph->container_groups().erase(_graph->container_groups().begin() + i--);
            --count;
        }
    }
}