#include "io/PLTSpirit.hpp"


PLTSpirit::PLTSpirit()
{
    const Scanner end = Scanner(';');
    const Scanner coord = Scanners::num << (Scanner(',') | Scanners::space) << Scanners::num << Scanner::optional(',');
    const Scanner skip = Scanners::alphas << Scanner::optional(!end) << end;

    bind(Scanner('\n'), &PLTSpirit::pass);
    bind(Scanner("PU"), &PLTSpirit::pu);
    bind(Scanner("PD"), &PLTSpirit::pd);
    bind(coord, &PLTSpirit::coord);
    bind(Scanners::num, &PLTSpirit::radius);
    bind(end, &PLTSpirit::exec);
    bind(Scanner("CI"), &PLTSpirit::ci);
    bind(Scanner("AA"), &PLTSpirit::aa);
    bind(Scanner("AR"), &PLTSpirit::ar);
    
    bind(skip, &PLTSpirit::pass);
}

void PLTSpirit::load_graph(Graph *g)
{
    _graph = g;
    _graph->append_group();
}

void PLTSpirit::pu(const std::string &value)
{
    _last_cmd = _cur_cmd;
    _cur_cmd = Command::PU;
    store_points();
}

void PLTSpirit::pd(const std::string &value)
{
    _last_cmd = _cur_cmd;
    _cur_cmd = Command::PD;
}

void PLTSpirit::ci(const std::string &value)
{
    _last_cmd = _cur_cmd;
    _cur_cmd = Command::CI;
}

void PLTSpirit::pa(const std::string &value)
{
    _relative_coord = false;
}

void PLTSpirit::pr(const std::string &value)
{
    _relative_coord = true;
}

void PLTSpirit::coord(const std::string &value)
{
    size_t pos = 0;
    while (('0' <= value[pos] && value[pos] <= '9') || value[pos] == '.' || value[pos] == '-')
    {
        ++pos;
    }
    const double x = std::atof(value.substr(0, pos).c_str());
    
    if (('0' > value[pos + 1] || value[pos + 1] > '9') && value[pos + 1] != '.' && value[pos + 1] != '-')
    {
        while (('0' > value[pos] || value[pos] > '9') && value[pos] != '.' && value[pos] != '-')
        {
            ++pos;
        }
    }
    const double y = std::atof(value.substr(pos + 1).c_str());
    
    if (_relative_coord)
    {
        _points.emplace_back(_last_coord.x + x, _last_coord.y + y);
    }
    else
    {
        _points.emplace_back(Geo::Point(x, y));
    }
    _last_coord = _points.back().coord();
}

void PLTSpirit::radius(const std::string &value)
{
    _radius = std::atof(value.c_str());
}

void PLTSpirit::aa(const std::string &value)
{
    _last_cmd = _cur_cmd;
    _cur_cmd = Command::AA;
}

void PLTSpirit::ar(const std::string &value)
{
    _last_cmd = _cur_cmd;
    _cur_cmd = Command::AR;
}


void PLTSpirit::store_points()
{
    if (_points.empty())
    {
        return;
    }
    if (_points.front() == _points.back() && _points.size() >= 3)
    {
        _graph->container_groups().back().append(new Container(Geo::Polygon(_points.cbegin(), _points.cend())));
    }
    else
    {
        _graph->container_groups().back().append(new Geo::Polyline(_points.cbegin(), _points.cend()));
    }
    _points.clear();
}

void PLTSpirit::store_circle()
{
    if (_points.size() == 1)
    {
        _graph->container_groups().back().append(new CircleContainer(QString(), 
            _points.back().coord().x, _points.back().coord().y, _radius));
        _radius = -1;
        _points.pop_back();
    }
    else
    {
        Geo::Polygon polygon;
        const Geo::Point center(_points.front());
        Geo::Vector dir(0, _points.back().coord().x);
        const double step = _points.back().coord().y >= 15 ? _points.back().coord().y : _points.back().coord().y * 180 / Geo::PI;
        double angle = 0;
        while (angle < 360)
        {
            polygon.append(center + dir);
            dir.rotate(0, 0, step * Geo::PI / 180.0);
            angle += step;
        }
        polygon.insert(0, center + Geo::Vector(0, _points.back().coord().x));
        _graph->container_groups().back().append(new Container(polygon));
        _points.clear();
    }
}

void PLTSpirit::store_arc()
{
    if (_cur_cmd == Command::AR)
    {
        _points.front() += _points[1];
    }

    double step;
    if (_points.size() == 3)
    {
        _radius = _points.back().coord().x;
        step = _points.back().coord().y >= 15 ? _points.back().coord().y : _points.back().coord().y * 180 / Geo::PI;
    }
    else
    {
        step = 1;
    }

    if (_radius < 0)
    {
        step = -step;
    }

    Geo::Polyline *polyline = new Geo::Polyline();
    const Geo::Point center(_points.front());
    Geo::Vector dir = _points[1] - _points.front();

    double angle = step;
    if (_radius >= 0)
    {
        while (angle <= _radius)
        {
            polyline->append(center + dir);
            dir.rotate(0, 0, step * Geo::PI / 180.0);
            angle += step;
        }
    }
    else
    {
        while (angle >= _radius)
        {
            polyline->append(center + dir);
            dir.rotate(0, 0, step * Geo::PI / 180.0);
            angle += step;
        }
    }
    polyline->append(center + dir);
    
    _graph->container_groups().back().append(polyline);
    _points.clear();
    _radius = -1;
}



void PLTSpirit::exec(const std::string &value)
{
    switch (_cur_cmd)
    {
    case Command::CI:
    case Command::AA:
    case Command::AR:
        _exec = true;
        break;
    default:
        break;
    }
}

void PLTSpirit::exec()
{
    switch (_cur_cmd)
    {
    case Command::CI:
        store_circle();
        break;
    case Command::AA:
    case Command::AR:
        store_arc();
        break;
    default:
        break;
    }
}