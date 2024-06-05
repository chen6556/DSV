#include "io/PLTParser.hpp"
#include "io/Parser/ParserGen2.hpp"
#include "io/GlobalSetting.hpp"
#include <sstream>


namespace PLTParser
{

void Importer::load_graph(Graph *g)
{
    _graph = g;
    _graph->append_group();
}

void Importer::reset()
{
    _points.clear();
    _parameters.clear();
    _last_coord.x = _last_coord.y = 0;
    _relative_coord = false;
    _texts.clear();
}


void Importer::store_points()
{
    if (_points.size() <= 1)
    {
        _points.clear();
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

void Importer::store_arc()
{
    _points.front() += _points[1];
    double radius, step;

    if (_parameters.size() >= 2)
    {
        radius = _parameters[_parameters.size() - 2];
        step = _parameters.back() >= 15 ? _parameters.back() : _parameters.back() * 180 / Geo::PI;
    }
    else
    {
        step = 1;
    }

    if (radius < 0)
    {
        step = -step;
    }

    Geo::Polyline *polyline = new Geo::Polyline();
    const Geo::Point center(_points.front());
    Geo::Vector dir = _points[1] - _points.front();

    double angle = step;
    if (radius >= 0)
    {
        while (angle <= radius)
        {
            polyline->append(center + dir);
            dir.rotate(0, 0, step * Geo::PI / 180.0);
            angle += step;
        }
    }
    else
    {
        while (angle >= radius)
        {
            polyline->append(center + dir);
            dir.rotate(0, 0, step * Geo::PI / 180.0);
            angle += step;
        }
    }
    polyline->append(center + dir);
    
    _graph->container_groups().back().append(polyline);
    _points.clear();
    _parameters.clear();
}


void Importer::pu()
{
    store_points();
}

void Importer::sp(const int value)
{
    store_points();
}

void Importer::x_coord(const double value)
{
    if (_relative_coord)
    {
        _points.emplace_back(Geo::Point(_last_coord.x + value, 0));
    }
    else
    {
        _points.emplace_back(Geo::Point(value, 0));
    }
}

void Importer::y_coord(const double value)
{
    if (_relative_coord)
    {
        _points.back().y = _last_coord.y + value;
    }
    else
    {
        _points.back().y = value;
    }
    if (_points.back() == _last_coord)
    {
        _points.pop_back();
    }
    else
    {
        _last_coord = _points.back();
    }
}

void Importer::parameter(const double value)
{
    _parameters.emplace_back(value);
}


void Importer::ci()
{
    if (_parameters.size() > 1)
    {
        _parameters.pop_back();
    }
    _graph->container_groups().back().append(new CircleContainer(QString(), 
        _points.front().x, _points.front().y, _parameters.back()));
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
    store_points();
}

void Importer::ar()
{
    store_points();
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

void Importer::end()
{
    store_points();
    const int text_size = GlobalSetting::get_instance()->setting()["text_size"].toInt();
    for (Txt &text : _texts)
    {
        for (Geo::Geometry *geo : _graph->container_group())
        {
            if (geo->type() == Geo::Type::CONTAINER && Geo::is_inside(text.pos, dynamic_cast<Container *>(geo)->shape(), true))
            {
                if (dynamic_cast<Container *>(geo)->text().isEmpty())
                {
                    dynamic_cast<Container *>(geo)->set_text(QString::fromUtf8(text.txt));
                }
                else
                {
                    dynamic_cast<Container *>(geo)->set_text(dynamic_cast<Container *>(geo)->text() + '\n' + QString::fromUtf8(text.txt));
                }
                text.marked = true;
                break;
            }
            else if (geo->type() == Geo::Type::CIRCLECONTAINER && Geo::is_inside(text.pos, dynamic_cast<CircleContainer *>(geo)->shape(), true))
            {
                if (dynamic_cast<CircleContainer *>(geo)->text().isEmpty())
                {
                    dynamic_cast<CircleContainer *>(geo)->set_text(QString::fromUtf8(text.txt));
                }
                else
                {
                    dynamic_cast<CircleContainer *>(geo)->set_text(dynamic_cast<CircleContainer *>(geo)->text() + '\n' + QString::fromUtf8(text.txt));
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
Action<void> pa_a(&importer, &Importer::pa);
Action<void> pr_a(&importer, &Importer::pr);
Action<int> sp_a(&importer, &Importer::sp);
Action<void> ci_a(&importer, &Importer::ci);
Action<void> aa_a(&importer, &Importer::aa);
Action<void> ar_a(&importer, &Importer::ar);
Action<void> in_a(&importer, &Importer::reset);
Action<std::string> lb_a(&importer, &Importer::store_text);
Action<void> end_a(&importer, &Importer::end);

Parser<char> separator = ch_p(',') | ch_p(' ');
Parser<std::string> end = +(ch_p(';') | ch_p('\n') | eol_p());
Parser<double> parameter = float_p()[parameter_a];
Parser<bool> coord = float_p()[x_coord_a] >> separator >> float_p()[y_coord_a];
Parser<std::string> in = str_p("IN")[in_a] >> end;
Parser<bool> pu = str_p("PU")[pu_a] >> !list(coord, separator) >> end;
Parser<bool> pd = str_p("PD") >> !list(coord, separator) >> end;
Parser<bool> pa = str_p("PA")[pa_a] >> !list(coord, separator) >> end;
Parser<bool> pr = str_p("PR")[pr_a] >> !list(coord, separator) >> end;
Parser<bool> sp = str_p("SP") >> int_p()[sp_a] >> end;
Parser<bool> ci = (str_p("CI") >> parameter >> !parameter)[ci_a] >> end;
Parser<bool> aa = (str_p("AA") >> coord >> separator >> parameter >> !parameter)[aa_a] >> end;
Parser<bool> ar = (str_p("AR") >> coord >> separator >> parameter >> !parameter)[ar_a] >> end;

Parser<std::string> unkown_cmds = confix_p(alphaa_p() | ch_p(28), end);
Parser<std::string> lb = confix_p(str_p("LB"), (*anychar_p())[lb_a], end);
Parser<bool> all_cmds = in | pu | pd | pa | pr | sp | ci | aa | ar | lb | unkown_cmds;

Parser<std::string> dci = confix_p(ch_p(27), end);
Parser<bool> plt = (*(all_cmds | dci))[end_a];



bool PLTParser::parse(std::string_view &stream, Graph *graph)
{
    importer.reset();
    importer.load_graph(graph);
    return plt(stream);
}

bool PLTParser::parse(std::ifstream &stream, Graph *graph)
{
    importer.reset();
    importer.load_graph(graph);
    std::stringstream sstream;
    sstream << stream.rdbuf();
    std::string str(sstream.str());
    std::string_view temp(str);
    return plt(temp);
}

}