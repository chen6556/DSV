#include <sstream>

#include "base/Algorithm.hpp"
#include "io/PLTParser.hpp"
#include "io/Parser/ParserGen2.hpp"
#include "io/GlobalSetting.hpp"


namespace PLTParser
{
const double Importer::plotter_unit = 0.025;

void Importer::load_graph(Graph *g)
{
    _graph = g;
    _graph->append_group();
}

void Importer::reset()
{
    _points.clear();
    _parameters.clear();
    _polygon_cache.clear();
    _last_coord.x = _last_coord.y = 0;
    _rotate_coord = 0;
    _ip[0] = _ip[1] = _ip[4] = _ip[5] = 0;
    _ip[2] = _ip[3] = 1;
    _sc[0] = _sc[2] = 0;
    _sc[1] = _sc[3] = 1;
    _x_ratio = _y_ratio = Importer::plotter_unit;
    _pen_down = _relative_coord = _polygon_mode = false;
    _texts.clear();
}


void Importer::store_points()
{
    if (_polygon_mode)
    {
        return;
    }
    if (_points.size() <= 1)
    {
        _points.clear();
        return;
    }

    if (_points.front() == _points.back() && _points.size() >= 3)
    {
        _graph->container_groups().back().append(new Container<Geo::Polygon>(Geo::Polygon(_points.cbegin(), _points.cend())));
    }
    else
    {
        _graph->container_groups().back().append(new Geo::Polyline(_points.cbegin(), _points.cend()));
    }
    _points.clear();
}

void Importer::store_arc()
{
    const Geo::Point center(_parameters[0], _parameters[1]);
    Geo::Vector dir(_last_coord.x - _parameters[0], _last_coord.y - _parameters[1]);
    const double radius = std::sqrt(std::pow(_parameters[0] - _last_coord.x, 2) + 
        std::pow(_parameters[1] - _last_coord.y, 2));

    double angle = Geo::degree_to_rad(_parameters[2]);
    double step = radius > 3000 ? Geo::PI / 360 : Geo::PI / 180;
    if (angle >= 0)
    {
        dir.rotate(0, 0, step);
        angle -= step;
        while (angle > 0)
        {
            _points.emplace_back(center + dir);
            angle -= step;
            dir.rotate(0, 0, step);
        }
    }
    else
    {
        dir.rotate(0, 0, -step);
        angle += step;
        while (angle < 0)
        {
            _points.emplace_back(center + dir);
            angle += step;
            dir.rotate(0, 0, -step);
        }
    }
    dir.x = _last_coord.x - _parameters[0], dir.y = _last_coord.y - _parameters[1];
    dir.rotate(0, 0, Geo::degree_to_rad(_parameters[2]));
    _points.emplace_back(center + dir);
    _last_coord = _points.back();

    _parameters.clear();
}


void Importer::ip()
{
    store_points();
    if (_parameters.size() > 4)
    {
        _parameters.erase(_parameters.begin() + 4, _parameters.end());
    }
    if (_parameters.size() >= 2)
    {
        if (_parameters.size() >= 4)
        {
            _ip[2] = _parameters[2];
            _ip[3] = _parameters[3];
        }
        else
        {
            _ip[2] = _parameters[0] + _ip[2] - _ip[0];
            _ip[3] = _parameters[1] + _ip[3] - _ip[1];
        }
        _ip[0] = _parameters[0];
        _ip[1] = _parameters[1];
        switch (_rotate_coord)
        {
        case 90:
            std::swap(_ip[0], _ip[1]);
            _ip[0] = -_ip[0];
            std::swap(_ip[2], _ip[3]);
            _ip[2] = -_ip[2];
            break;
        case 180:
            _ip[0] = -_ip[0];
            _ip[1] = -_ip[1];
            _ip[2] = -_ip[2];
            _ip[3] = -_ip[3];
            break;
        case 270:
            std::swap(_ip[0], _ip[1]);
            _ip[1] = -_ip[1];
            std::swap(_ip[2], _ip[3]);
            _ip[3] = -_ip[3];
            break;
        default:
            break;
        }
        _x_ratio = (_ip[2] - _ip[0]) / (_sc[1] - _sc[0]) * Importer::plotter_unit;
        _y_ratio = (_ip[3] - _ip[1]) / (_sc[3] - _sc[2]) * Importer::plotter_unit;
        _ip[4] = std::min(_ip[0], _ip[2]) * Importer::plotter_unit;
        _ip[5] = std::min(_ip[1], _ip[3]) * Importer::plotter_unit;
    }
    _parameters.clear();
}

void Importer::sc()
{
    if (_parameters.size() == 4)
    {
        for (int i = 0; i < 4; ++i)
        {
            _sc[i] = _parameters[i];
        }
        _x_ratio = (_ip[2] - _ip[0]) / (_sc[1] - _sc[0]) * Importer::plotter_unit;
        _y_ratio = (_ip[3] - _ip[1]) / (_sc[3] - _sc[2]) * Importer::plotter_unit;
    }
    else if (_parameters.size() >= 5)
    {
        switch (static_cast<int>(_parameters[4]))
        {
        case 0:
            for (int i = 0; i < 4; ++i)
            {
                _sc[i] = _parameters[i];
            }
            _x_ratio = (_ip[2] - _ip[0]) / (_sc[1] - _sc[0]) * Importer::plotter_unit;
            _y_ratio = (_ip[3] - _ip[1]) / (_sc[3] - _sc[2]) * Importer::plotter_unit;
            break;
        case 2:
            _x_ratio = _parameters[1] * Importer::plotter_unit;
            _y_ratio = _parameters[3] * Importer::plotter_unit;
            break;
        default:
            _sc[0] = _sc[2] = 0;
            _sc[1] = _sc[3] = 1;
            _x_ratio = _y_ratio = Importer::plotter_unit;
            break;
        }
    }
    else
    {
        _sc[0] = _sc[2] = 0;
        _sc[1] = _sc[3] = 1;
        _x_ratio = _y_ratio = Importer::plotter_unit;
    }
    _parameters.clear();
}

void Importer::ro()
{
    store_points();
    _rotate_coord = _parameters.front();
    _parameters.clear();
    switch (_rotate_coord)
    {
    case 90:
        std::swap(_ip[0], _ip[1]);
        _ip[0] = -_ip[0];
        std::swap(_ip[2], _ip[3]);
        _ip[2] = -_ip[2];
        _ip[4] = std::min(_ip[0], _ip[2]) * Importer::plotter_unit;
        _ip[5] = std::min(_ip[1], _ip[3]) * Importer::plotter_unit;
        break;
    case 180:
        _ip[0] = -_ip[0];
        _ip[1] = -_ip[1];
        _ip[2] = -_ip[2];
        _ip[3] = -_ip[3];
        _ip[4] = std::min(_ip[0], _ip[2]) * Importer::plotter_unit;
        _ip[5] = std::min(_ip[1], _ip[3]) * Importer::plotter_unit;
        break;
    case 270:
        std::swap(_ip[0], _ip[1]);
        _ip[1] = -_ip[1];
        std::swap(_ip[2], _ip[3]);
        _ip[3] = -_ip[3];
        _ip[4] = std::min(_ip[0], _ip[2]) * Importer::plotter_unit;
        _ip[5] = std::min(_ip[1], _ip[3]) * Importer::plotter_unit;
        break;
    default:
        _rotate_coord = 0;
        break;
    }
}

void Importer::pu()
{
    _pen_down = false;
    store_points();
}

void Importer::pd()
{
    _pen_down = true;
    if (!_polygon_mode && (_points.empty() || _points.back() != _last_coord))
    {
        _points.emplace_back(_last_coord);
    }
}

void Importer::sp(const int value)
{
    store_points();
}

void Importer::x_coord(const double value)
{
    switch (_rotate_coord)
    {
    case 90:
        if (_relative_coord)
        {
            _points.emplace_back(0, _last_coord.x + value * _x_ratio);
        }
        else
        {
            _points.emplace_back(0, _ip[4] + value * _x_ratio);
        }
        break;
    case 180:
        if (_relative_coord)
        {
            _points.emplace_back(_last_coord.x - value * _x_ratio, 0);
        }
        else
        {
            _points.emplace_back(_ip[4] - value * _x_ratio, 0);
        }
        break;
    case 270:
        if (_relative_coord)
        {
            _points.emplace_back(0, _last_coord.x - value * _x_ratio);
        }
        else
        {
            _points.emplace_back(0, _ip[4] - value * _x_ratio);
        }
        break;
    default:
        if (_relative_coord)
        {
            _points.emplace_back(_last_coord.x + value * _x_ratio, 0);
        }
        else
        {
            _points.emplace_back(_ip[4] + value * _x_ratio, 0);
        }
        break;
    }
}

void Importer::y_coord(const double value)
{
    switch (_rotate_coord)
    {
    case 90:
        if (_relative_coord)
        {
            _points.back().x = _last_coord.y - value * _y_ratio;
        }
        else
        {
            _points.back().x = _ip[5] - value * _y_ratio;
        }
        break;
    case 180:
        if (_relative_coord)
        {
            _points.back().y = _last_coord.y - value * _y_ratio;
        }
        else
        {
            _points.back().y = _ip[5] - value * _y_ratio;
        }
        break;
    case 270:
        if (_relative_coord)
        {
            _points.back().x = _last_coord.y + value * _y_ratio;
        }
        else
        {
            _points.back().x = _ip[5] + value * _y_ratio;
        }
        break;
    default:
        if (_relative_coord)
        {
            _points.back().y = _last_coord.y + value * _y_ratio;
        }
        else
        {
            _points.back().y = _ip[5] + value * _y_ratio;
        }
        break;
    }

    if (_points.size() > 1 && _points.back() == _last_coord)
    {
        _points.pop_back();
    }
    else
    {
        _last_coord = _points.back();
    }
    if (!_pen_down && _points.size() > 1)
    {
        _points.erase(_points.begin(), _points.end() - 1);
    }
}

void Importer::parameter(const double value)
{
    _parameters.emplace_back(value);
}


void Importer::br()
{
    store_points();
    _points.emplace_back(_last_coord);
    for (size_t i = 1, count = _parameters.size(); i < count; i += 2)
    {
        _points.emplace_back(_parameters[i - 1] * _x_ratio + _last_coord.x, _parameters[i] * _y_ratio + _last_coord.y);
        _last_coord = _points.back();
    }
    _graph->container_groups().back().append(new Geo::Bezier(_points.cbegin(), _points.cend(), 3));
    _points.clear();
    _parameters.clear();
}

void Importer::bz()
{
    store_points();
    _points.emplace_back(_last_coord);
    for (size_t i = 1, count = _parameters.size(); i < count; i += 2)
    {
        _points.emplace_back(_parameters[i - 1] * _x_ratio, _parameters[i] * _y_ratio);
    }
    _last_coord = _points.back();
    _graph->container_groups().back().append(new Geo::Bezier(_points.cbegin(), _points.cend(), 3));
    _points.clear();
    _parameters.clear();
}

void Importer::ci()
{
    if (_parameters.size() > 1)
    {
        _parameters.pop_back();
    }
    _graph->container_groups().back().append(new Container<Geo::Circle>(QString(),
        _points.front().x, _points.front().y, _parameters.back() * _x_ratio));
    _points.clear();
    _parameters.clear();
}

void Importer::pa()
{
    _relative_coord = false;
}

void Importer::pr()
{
    _relative_coord = true;
}

void Importer::aa()
{
    _parameters[0] *= _x_ratio;
    _parameters[1] *= _y_ratio;
    store_arc();
}

void Importer::ar()
{
    _parameters[0] *= _x_ratio;
    _parameters[1] *= _y_ratio;
    _parameters[0] += _last_coord.x;
    _parameters[1] += _last_coord.y;
    store_arc();
}

void Importer::ea()
{
    if (_parameters.size() >= 2)
    {
        _parameters.erase(_parameters.begin(), _parameters.begin() + _parameters.size() - 2);
        _graph->container_groups().back().append(new Container<Geo::Polygon>(Geo::AABBRect(_last_coord.x, _last_coord.y,
            _parameters.front() * _x_ratio + _ip[4], _parameters.back() * _y_ratio + _ip[5])));
    }
    _points.clear();
    _parameters.clear();
}

void Importer::er()
{
    if (_parameters.size() >= 2)
    {
        _parameters.erase(_parameters.begin(), _parameters.begin() + _parameters.size() - 2);
        _graph->container_groups().back().append(new Container<Geo::Polygon>(Geo::AABBRect(_last_coord.x, _last_coord.y,
            _parameters.front() * _x_ratio + _last_coord.x, _parameters.back() * _y_ratio + _last_coord.y)));
    }
    _points.clear();
    _parameters.clear();
}

void Importer::pm(const int value)
{
    switch (value)
    {
    case 0:
        _polygon_cache.clear();
        _polygon_mode = true;
        break;
    case 2:
        _polygon_mode = false;
    case 1:
        if (!_points.empty())
        {
            _polygon_cache.emplace_back(_points.begin(), _points.end());
        }
        break;
    default:
        break;
    }
    _points.clear();
}

void Importer::pm()
{
    return pm(0);
}

void Importer::ep()
{
    for (Geo::Polygon &polygon : _polygon_cache)
    {
        _graph->container_group().append(new Container<Geo::Polygon>(polygon));
    }
    _polygon_cache.clear();
}

void Importer::store_text(const std::string &text)
{
    std::string str(text);
    size_t pos;
    for (char c = 1; c < 32; ++c)
    {
        pos = str.find(c);
        if (pos != std::string::npos)
        {
            --c;
            str.erase(pos, 1);
        }
    }

    _texts.emplace_back(str, _last_coord);
    _points.clear();
}

void Importer::print_symbol(const std::string& str)
{
    qDebug() << str;
}

void Importer::end()
{
    store_points();
    const int text_size = GlobalSetting::get_instance()->setting["text_size"].toInt();
    std::vector<Geo::Geometry *> group(_graph->container_group().begin(), _graph->container_group().end());
    std::sort(group.begin(), group.end(), [](const Geo::Geometry *a, const Geo::Geometry *b)
              { return a->bounding_rect().area() < b->bounding_rect().area(); });
    for (Txt &text : _texts)
    {
        for (Geo::Geometry *geo : group)
        {
            if (geo->type() == Geo::Type::POLYGON && Geo::is_inside(text.pos, dynamic_cast<Container<Geo::Polygon> *>(geo)->shape(), true))
            {
                if (dynamic_cast<Container<Geo::Polygon> *>(geo)->text().isEmpty())
                {
                    dynamic_cast<Container<Geo::Polygon> *>(geo)->set_text(QString::fromUtf8(text.txt));
                }
                else
                {
                    dynamic_cast<Container<Geo::Polygon> *>(geo)->set_text(dynamic_cast<Container<Geo::Polygon> *>(geo)->text() + '\n' + QString::fromUtf8(text.txt));
                }
                text.marked = true;
                break;
            }
            else if (geo->type() == Geo::Type::CIRCLE && Geo::is_inside(text.pos, dynamic_cast<Container<Geo::Circle> *>(geo)->shape(), true))
            {
                if (dynamic_cast<Container<Geo::Circle> *>(geo)->text().isEmpty())
                {
                    dynamic_cast<Container<Geo::Circle> *>(geo)->set_text(QString::fromUtf8(text.txt));
                }
                else
                {
                    dynamic_cast<Container<Geo::Circle> *>(geo)->set_text(dynamic_cast<Container<Geo::Circle> *>(geo)->text() + '\n' + QString::fromUtf8(text.txt));
                }
                text.marked = true;
                break;
            }
        }
        if (!text.marked)
        {
            _graph->container_group().append(new Text(text.pos.x, text.pos.y, text_size, QString::fromUtf8(text.txt)));
        }
    }
    _texts.clear();
}


Importer importer;

Action<double> x_coord_a(&importer, &Importer::x_coord);
Action<double> y_coord_a(&importer, &Importer::y_coord);
Action<double> parameter_a(&importer, &Importer::parameter);
Action<void> pu_a(&importer, &Importer::pu);
Action<void> pd_a(&importer, &Importer::pd);
Action<void> pa_a(&importer, &Importer::pa);
Action<void> pr_a(&importer, &Importer::pr);
Action<int> sp_a(&importer, &Importer::sp);
Action<void> br_a(&importer, &Importer::br);
Action<void> bz_a(&importer, &Importer::bz);
Action<void> ci_a(&importer, &Importer::ci);
Action<void> aa_a(&importer, &Importer::aa);
Action<void> ar_a(&importer, &Importer::ar);
Action<void> ea_a(&importer, &Importer::ea);
Action<void> er_a(&importer, &Importer::er);
Action<int> pm_int_a(&importer, &Importer::pm);
Action<void> pm_void_a(&importer, &Importer::pm);
Action<void> ep_a(&importer, &Importer::ep);
Action<void> in_a(&importer, &Importer::reset);
Action<void> ip_a(&importer, &Importer::ip);
Action<void> sc_a(&importer, &Importer::sc);
Action<void> ro_a(&importer, &Importer::ro);
Action<std::string> lb_a(&importer, &Importer::store_text);
Action<std::string> unkown_a(&importer, &Importer::print_symbol);
Action<void> end_a(&importer, &Importer::end);

Parser<char> separator = ch_p(',') | ch_p(' ');
Parser<std::string> end = +(ch_p(';') | ch_p('\n') | eol_p());
Parser<double> parameter = float_p()[parameter_a];
Parser<bool> coord = float_p()[x_coord_a] >> separator >> float_p()[y_coord_a];
Parser<std::string> df = str_p("DF")[in_a] >> end;
Parser<std::string> in = str_p("IN")[in_a] >> end;
Parser<bool> ip = (str_p("IP") >> list_p(parameter, separator))[ip_a] >> end;
Parser<bool> sc = (str_p("SC") >> !list_p(parameter, separator))[sc_a] >> end;
Parser<bool> ro = (str_p("RO") >> !parameter)[ro_a] >> end;
Parser<bool> pu = str_p("PU")[pu_a] >> !list_p(coord, separator) >> end;
Parser<bool> pd = str_p("PD")[pd_a] >> !list_p(coord, separator) >> end;
Parser<bool> pa = str_p("PA")[pa_a] >> !list_p(coord, separator) >> end;
Parser<bool> pr = str_p("PR")[pr_a] >> !list_p(coord, separator) >> end;
Parser<bool> sp = str_p("SP") >> int_p()[sp_a] >> end;
Parser<bool> br = str_p("BR") >> list_p(parameter, separator)[br_a] >> end;
Parser<bool> bz = str_p("BZ") >> list_p(parameter, separator)[bz_a] >> end;
Parser<bool> ci = (str_p("CI") >> parameter >> !(separator >> parameter))[ci_a] >> end;
Parser<bool> aa = (str_p("AA") >> list_p(parameter, separator))[aa_a] >> end;
Parser<bool> ar = (str_p("AR") >> list_p(parameter, separator))[ar_a] >> end;
Parser<bool> ea = (str_p("EA") >> list_p(parameter, separator))[ea_a] >> end;
Parser<bool> er = (str_p("ER") >> list_p(parameter, separator))[er_a] >> end;
Parser<bool> pm = ((str_p("PM") >> digit_p()[pm_int_a]) | str_p("PM")[pm_void_a]) >> end;
Parser<std::string> ep = str_p("EP")[ep_a] >> end;

Parser<std::string> unkown_cmds = confix_p(alphaa_p() | ch_p(28), end)[unkown_a];
Parser<std::string> text_end = ch_p('\x3') | ch_p('\x4') | end;
Parser<std::string> lb = confix_p(str_p("LB"), (*anychar_p())[lb_a], text_end) >> !separator >> !end;
Parser<bool> all_cmds = pu | pd | lb | pa | pr | sp | br | bz | ci | aa | ar | ea | er | pm | ep | in | ip | sc | df | ro | unkown_cmds;

Parser<std::string> dci = confix_p(ch_p(27), end);
Parser<bool> plt = (*(all_cmds | dci))[end_a];


bool parse(std::string_view &stream, Graph *graph)
{
    importer.reset();
    importer.load_graph(graph);
    return plt(stream);
}

bool parse(std::ifstream &stream, Graph *graph)
{
    importer.reset();
    importer.load_graph(graph);
    std::stringstream sstream;
    sstream << stream.rdbuf();
    std::string str(sstream.str());
    std::string_view temp(str);
    return plt(temp);
}
} // namespace PLTParser