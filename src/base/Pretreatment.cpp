#include "base/Pretreatment.hpp"
#include "io/GlobalSetting.hpp"


Pretreatment::Pretreatment(Graph *graph)
    : _graph(graph)
{
    split();
    connect_lines(GlobalSetting::get_instance()->setting()["connect_distance"].toDouble());
    if (GlobalSetting::get_instance()->setting()["combine"].toBool())
    {
        combine();
    }
    finish();
};


void Pretreatment::split()
{
    for (ContainerGroup &group : _graph->container_groups())
    {
        while (!group.empty())
        {
            switch (group.back()->memo()["Type"].to_int())
            {
            case 0:
                _all_containers.emplace_back(reinterpret_cast<Container *>(group.pop_back()));
                break;
            case 1:
                _all_circles.emplace_back(reinterpret_cast<CircleContainer *>(group.pop_back()));
                break;
            case 2:
                _all_links.emplace_back(reinterpret_cast<Link *>(group.pop_back()));
                break;
            case 20:
                _all_polylines.emplace_back(reinterpret_cast<Geo::Polyline *>(group.pop_back()));
                break;
            case 21:
                _all_beziers.emplace_back(reinterpret_cast<Geo::Bezier *>(group.pop_back()));
                break;
            default:
                break;
            }
        }
    }

    _graph->clear();
    _graph->append_group();
}

void Pretreatment::connect_lines(const double value)
{
    bool flag = false;
    for (size_t i = 0, count0 = _all_polylines.size(); i < count0; ++i)
    {
        flag = false;
        for (size_t j = i + 1; j < count0; ++j)
        {
            if (_all_polylines[i]->front() == _all_polylines[j]->front() ||
                Geo::distance(_all_polylines[i]->front(), _all_polylines[j]->front()) < value)
            {
                std::reverse(_all_polylines[j]->begin(), _all_polylines[j]->end());
                _all_polylines[i]->insert(0, *_all_polylines[j]);
                delete _all_polylines[j];
                _all_polylines.erase(_all_polylines.begin() + j);
                --count0;
                --i;
                flag = true;
                break;
            }
            else if (_all_polylines[i]->front() == _all_polylines[j]->back() ||
                Geo::distance(_all_polylines[i]->front(), _all_polylines[j]->back()) < value)
            {
                _all_polylines[i]->insert(0, *_all_polylines[j]);
                delete _all_polylines[j];
                _all_polylines.erase(_all_polylines.begin() + j);
                --count0;
                --i;
                flag = true;
                break;
            }
            else if (_all_polylines[i]->back() == _all_polylines[j]->front() ||
                Geo::distance(_all_polylines[i]->back(), _all_polylines[j]->front()) < value)
            {
                _all_polylines[i]->append(*_all_polylines[j]);
                delete _all_polylines[j];
                _all_polylines.erase(_all_polylines.begin() + j);
                --count0;
                --i;
                flag = true;
                break;
            }
            else if (_all_polylines[i]->back() == _all_polylines[j]->back() ||
                Geo::distance(_all_polylines[i]->back(), _all_polylines[j]->back()) < value)
            {
                std::reverse(_all_polylines[j]->begin(), _all_polylines[j]->end());
                _all_polylines[i]->append(*_all_polylines[j]);
                delete _all_polylines[j];
                _all_polylines.erase(_all_polylines.begin() + j);
                --count0;
                --i;
                flag = true;
                break;
            }
        }
    
        if (flag)
        {
            continue;
        }
        for (size_t j = 0, count1 = _all_beziers.size(); j < count1; ++j)
        {
            if (_all_polylines[i]->front() == _all_beziers[j]->front() ||
                Geo::distance(_all_polylines[i]->front(), _all_beziers[j]->front()) < value)
            {
                std::reverse(_all_beziers[j]->begin(), _all_beziers[j]->end());
                _all_beziers[j]->update_shape();
                _all_polylines[i]->insert(0, _all_beziers[j]->shape());
                delete _all_beziers[j];
                _all_beziers.erase(_all_beziers.begin() + j);
                --i;
                break;
            }
            else if (_all_polylines[i]->front() == _all_beziers[j]->back() ||
                Geo::distance(_all_polylines[i]->front(), _all_beziers[j]->back()) < value)
            {
                _all_beziers[j]->update_shape();
                _all_polylines[i]->insert(0, _all_beziers[j]->shape());
                delete _all_beziers[j];
                _all_beziers.erase(_all_beziers.begin() + j);
                --i;
                break;
            }
            else if (_all_polylines[i]->back() == _all_beziers[j]->front() ||
                Geo::distance(_all_polylines[i]->back(), _all_beziers[j]->front()) < value)
            {
                _all_beziers[j]->update_shape();
                _all_polylines[i]->append(_all_beziers[j]->shape());
                delete _all_beziers[j];
                _all_beziers.erase(_all_beziers.begin() + j);
                --i;
                break;
            }
            else if (_all_polylines[i]->back() == _all_beziers[j]->back() ||
                Geo::distance(_all_polylines[i]->back(), _all_beziers[j]->back()) < value)
            {
                std::reverse(_all_beziers[j]->begin(), _all_beziers[j]->end());
                _all_beziers[j]->update_shape();
                _all_polylines[i]->append(_all_beziers[j]->shape());
                delete _all_beziers[j];
                _all_beziers.erase(_all_beziers.begin() + j);
                --i;
                break;
            }
        }
    }

    for (size_t i = 0, count = _all_polylines.size(); i < count; ++i)
    {
        if (_all_polylines[i]->front() == _all_polylines[i]->back() ||
            Geo::distance(_all_polylines[i]->front(), _all_polylines[i]->back()) < value)
        {
            _all_containers.emplace_back(new Container(Geo::Polygon(*_all_polylines[i])));
            delete _all_polylines[i];
            _all_polylines.erase(_all_polylines.begin() + i--);
            --count;
        }
    }

    std::sort(_all_containers.begin(), _all_containers.end(),
        [](const Container *a, const Container *b) { return a->area() > b->area(); });
    std::sort(_all_circles.begin(), _all_circles.end(),
        [](const CircleContainer *a, const CircleContainer *b) { return a->area() > b->area(); });
    std::sort(_all_polylines.begin(), _all_polylines.end(),
        [](const Geo::Polyline *a, const Geo::Polyline *b) { return a->length() < b->length(); });
}

void Pretreatment::combine()
{
    bool flag;
    for (size_t i = 0, count = _all_containers.size(); i < count; ++i)
    {
        flag = false;
        for (size_t j = i + 1; j < count; ++j)
        {
            if (std::all_of(_all_containers[j]->cbegin(), _all_containers[j]->cend(),
                [&](const Geo::Point &point) { return Geo::is_inside(point, _all_containers[i]->shape(), true); }))
            {
                if (!flag)
                {
                    flag = true;
                    _all_combinations.emplace_back(new Combination());
                    _all_combinations.back()->append(_all_containers[i]);
                }
                _all_combinations.back()->append(_all_containers[j]);
                _all_containers.erase(_all_containers.begin() + j--);
                --count;
            }
        }

        for (size_t j = 0, num = _all_circles.size(); j < num; ++j)
        {
            if (Geo::is_inside(_all_circles[j]->center(), _all_containers[i]->shape()) &&
                Geo::distance(_all_circles[j]->center(), _all_containers[i]->shape()) <= _all_circles[j]->radius())
            {
                if (!flag)
                {
                    flag = true;
                    _all_combinations.emplace_back(new Combination());
                    _all_combinations.back()->append(_all_containers[i]);
                }
                _all_combinations.back()->append(_all_circles[j]);
                _all_circles.erase(_all_circles.begin() + j--);
                --num;
            }
        }

        if (flag)
        {
            _all_containers.erase(_all_containers.begin() + i--);
            --count;
        }
    }

    for (size_t i = 0, count = _all_circles.size(); i < count; ++i)
    {
        flag = false;
        for (size_t j = i + 1; j < count; ++j)
        {
            if (_all_circles[j]->radius() < _all_circles[i]->radius() &&
                Geo::distance(_all_circles[i]->center(), _all_circles[j]->center()) <= _all_circles[j]->radius())
            {
                if (!flag)
                {
                    flag = true;
                    _all_combinations.emplace_back(new Combination());
                    _all_combinations.back()->append(_all_circles[i]);
                }
                _all_combinations.back()->append(_all_circles[j]);
                _all_circles.erase(_all_circles.begin() + j--);
                --count;
            }
        }

        for (size_t j = 0, num = _all_containers.size(); j < num; ++j)
        {
            if (std::all_of(_all_containers[j]->begin(), _all_containers[j]->end(),
                [&](const Geo::Point &point) { return Geo::is_inside(point, _all_circles[i]->shape(), true); }))
            {
                if (!flag)
                {
                    flag = true;
                    _all_combinations.emplace_back(new Combination());
                    _all_combinations.back()->append(_all_circles[i]);
                }
                _all_combinations.back()->append(_all_containers[j]);
                _all_containers.erase(_all_containers.begin() + j--);
                --num;
            }
        }

        if (flag)
        {
            _all_circles.erase(_all_circles.begin() + i--);
            --count;
        }
    }

    for (size_t i = 0, count = _all_combinations.size(); i < count; ++i)
    {
        for (size_t j = 0, num = _all_polylines.size(); j < num; ++j)
        {
            if (dynamic_cast<Container *>(_all_combinations[i]->front()) != nullptr &&
                std::all_of(_all_polylines[j]->begin(), _all_polylines[j]->end(),
                    [&](const Geo::Point &point) { return Geo::is_inside(point, reinterpret_cast<Container *>(_all_combinations[i]->front())->shape(), true); }))
            {
                _all_combinations[i]->append(_all_polylines[j]);
                _all_polylines.erase(_all_polylines.begin() + j--);
                --num;
            }
            else if (dynamic_cast<CircleContainer *>(_all_combinations[i]->front()) != nullptr &&
                std::all_of(_all_polylines[j]->begin(), _all_polylines[j]->end(),
                    [&](const Geo::Point &point) { return Geo::is_inside(point, dynamic_cast<CircleContainer *>(_all_combinations[i]->front())->shape(), true); }))
            {
                _all_combinations[i]->insert(0, _all_polylines[j]);
                _all_polylines.erase(_all_polylines.begin() + j--);
                --num;
            }
        }

        for (size_t j = 0, num = _all_beziers.size(); j < num; ++j)
        {
            if (dynamic_cast<Container *>(_all_combinations[i]->front()) != nullptr &&
                std::all_of(_all_beziers[j]->begin(), _all_beziers[j]->end(),
                    [&](const Geo::Point &point) { return Geo::is_inside(point, reinterpret_cast<Container *>(_all_combinations[i]->front())->shape(), true); }))
            {
                _all_combinations[i]->append(_all_beziers[j]);
                _all_beziers.erase(_all_beziers.begin() + j--);
                --num;
            }
            else if (dynamic_cast<CircleContainer *>(_all_combinations[i]->front()) != nullptr &&
                std::all_of(_all_beziers[j]->begin(), _all_beziers[j]->end(),
                    [&](const Geo::Point &point) { return Geo::is_inside(point, dynamic_cast<CircleContainer *>(_all_combinations[i]->front())->shape(), true); }))
            {
                _all_combinations[i]->insert(0, _all_beziers[j]);
                _all_beziers.erase(_all_beziers.begin() + j--);
                --num;
            }
        }
    }

    for (Combination *combination : _all_combinations)
    {
        for (Geo::Geometry *geo : *combination)
        {
            geo->memo()["is_selected"] = true;
        }
    }
}

void Pretreatment::finish()
{
    for (Link *link : _all_links)
    {
        _graph->back().append(link);
    }
    for (Container *container : _all_containers)
    {
        _graph->back().append(container);
    }
    for (CircleContainer *circle : _all_circles)
    {
        _graph->back().append(circle);
    }
    for (Geo::Polyline *polyline : _all_polylines)
    {
        _graph->back().append(polyline);
    }
    for (Geo::Bezier *bezier : _all_beziers)
    {
        _graph->back().append(bezier);
    }
    for (Combination *combination : _all_combinations)
    {
        combination->update_border();
        _graph->back().append(combination);
    }
    _all_links.clear();
    _all_containers.clear();
    _all_circles.clear();
    _all_polylines.clear();
    _all_beziers.clear();
    _all_combinations.clear();
}