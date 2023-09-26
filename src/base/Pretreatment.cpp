#include "base/Pretreatment.hpp"
#include "io/GlobalSetting.hpp"


Pretreatment::Pretreatment(Graph *graph)
    : _graph(graph)
{
    split();    
    connect_lines(GlobalSetting::get_instance()->setting()["connect_distance"].toDouble());

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
            if (_all_polylines[i]->front() == _all_beziers[j]->front())
            {
                std::reverse(_all_beziers[j]->begin(), _all_beziers[j]->end());
                _all_beziers[j]->update_shape();
                _all_polylines[i]->insert(0, _all_beziers[j]->shape());
                delete _all_beziers[j];
                _all_beziers.erase(_all_beziers.begin() + j);
                --i;
                break;
            }
            else if (_all_polylines[i]->front() == _all_beziers[j]->back())
            {
                _all_beziers[j]->update_shape();
                _all_polylines[i]->insert(0, _all_beziers[j]->shape());
                delete _all_beziers[j];
                _all_beziers.erase(_all_beziers.begin() + j);
                --i;
                break;
            }
            else if (_all_polylines[i]->back() == _all_beziers[j]->front())
            {
                _all_beziers[j]->update_shape();
                _all_polylines[i]->append(_all_beziers[j]->shape());
                delete _all_beziers[j];
                _all_beziers.erase(_all_beziers.begin() + j);
                --i;
                break;
            }
            else if (_all_polylines[i]->back() == _all_beziers[j]->back())
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
        [](const Container *a, const Container *b) { return a->area() < b->area(); });
    std::sort(_all_circles.begin(), _all_circles.end(), 
        [](const CircleContainer *a, const CircleContainer *b) { return a->area() < b->area(); });
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
    _all_links.clear();
    _all_containers.clear();
    _all_circles.clear();
    _all_polylines.clear();
    _all_beziers.clear();
}