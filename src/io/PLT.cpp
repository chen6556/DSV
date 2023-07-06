#include "io/PLT.hpp"


PLTSpirit::PLTSpirit()
{
    const Scanner end = Scanner(';');
    const Scanner coord = Scanners::num << Scanner(',') << Scanners::num << Scanner::optional(',');
    const Scanner skip = Scanners::alpha_b << Scanners::alpha_b << Scanner::optional(!end) << end;
    const Scanner positive_num = Scanners::digits << Scanner::optional(Scanner('.') << Scanners::digits);

    bind(Scanner('\n'), &PLTSpirit::pass);
    bind(Scanner("PU"), &PLTSpirit::pu);
    bind(Scanner("PD"), &PLTSpirit::pd);
    bind(coord, &PLTSpirit::coord);
    bind(end, &PLTSpirit::exec);
    
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
    const size_t pos = value.find(',');
    const double x = std::atof(value.substr(0, pos).c_str());
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



void PLTSpirit::store_points()
{
    if (_points.front() == _points.back())
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
    _graph->container_groups().back().append(new CircleContainer(QString(), 
        _points.back().coord().x, _points.back().coord().y, _radius));
    _radius = -1;
    _points.pop_back();
}


void PLTSpirit::exec(const std::string &value)
{
    switch (_cur_cmd)
    {
    case Command::PD:
    case Command::CI:
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
    case Command::PD:
        store_points();
        break;
    case Command::CI:
        store_circle();
        break;
    default:
        break;
    }
}