#include "io/PLTParser.hpp"
#include "io/Parser.hpp"
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
        _points.back().coord().y = _last_coord.y + value;
    }
    else
    {
        _points.back().coord().y = value;
    }
    if (_points.back().coord() == _last_coord)
    {
        _points.pop_back();
    }
    else
    {
        _last_coord = _points.back().coord();
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
        _points.front().coord().x, _points.front().coord().y, _parameters.back()));
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
    _texts.emplace_back(Text(text, _points.back()));
    _points.clear();
}

void Importer::end()
{
    for (Text &text : _texts)
    {
        for (Geo::Geometry *geo : _graph->container_group())
        {
            if (geo->memo()["Type"].to_int() == 0 && Geo::is_inside(text.pos, dynamic_cast<Container *>(geo)->shape(), true))
            {
                if (dynamic_cast<Container *>(geo)->text().isEmpty())
                {
                    dynamic_cast<Container *>(geo)->set_text(QString::fromStdString(text.txt));
                }
                else
                {
                    dynamic_cast<Container *>(geo)->set_text(dynamic_cast<Container *>(geo)->text() + '\n' + QString::fromStdString(text.txt));
                }
                break;
            }
            else if (geo->memo()["Type"].to_int() == 1 && Geo::is_inside(text.pos, dynamic_cast<CircleContainer *>(geo)->shape(), true))
            {
                if (dynamic_cast<CircleContainer *>(geo)->text().isEmpty())
                {
                    dynamic_cast<CircleContainer *>(geo)->set_text(QString::fromStdString(text.txt));
                }
                else
                {
                    dynamic_cast<CircleContainer *>(geo)->set_text(dynamic_cast<CircleContainer *>(geo)->text() + '\n' + QString::fromStdString(text.txt));
                }
                break;
            }
        }
    }
    _texts.clear();
}


static Importer importer;

static Action<double> x_coord_a(&importer, &Importer::x_coord);
static Action<double> y_coord_a(&importer, &Importer::y_coord);
static Action<double> parameter_a(&importer, &Importer::parameter);
static Action<void> pu_a(&importer, &Importer::pu);
static Action<void> pa_a(&importer, &Importer::pa);
static Action<void> pr_a(&importer, &Importer::pr);
static Action<int> sp_a(&importer, &Importer::sp);
static Action<void> ci_a(&importer, &Importer::ci);
static Action<void> aa_a(&importer, &Importer::aa);
static Action<void> ar_a(&importer, &Importer::ar);
static Action<void> in_a(&importer, &Importer::reset);
static Action<std::string> lb_a(&importer, &Importer::store_text);
static Action<void> end_a(&importer, &Importer::end);

static Parser<char> separator = ch_p(',') | ch_p(' ');
static Parser<std::vector<char>> end = +(ch_p(';') | ch_p('\n') | eol_p());
static Parser<double> parameter = float_p()[parameter_a];
static Parser<std::vector<double>> coord = float_p()[x_coord_a] >> separator >> float_p()[y_coord_a];
static auto in = str_p("IN")[in_a] >> end;
static auto pu = str_p("PU")[pu_a] >> !list(coord, separator) >> end;
static auto pd = str_p("PD") >> !list(coord, separator) >> end;
static auto pa = str_p("PA")[pa_a] >> !list(coord, separator) >> end;
static auto pr = str_p("PR")[pr_a] >> !list(coord, separator) >> end;
static auto sp = str_p("SP") >> int_p()[sp_a] >> end;
static auto ci = (str_p("CI") >> parameter >> !parameter)[ci_a] >> end;
static auto aa = (str_p("AA") >> coord >> separator >> parameter >> !parameter)[aa_a] >> end;
static auto ar = (str_p("AR") >> coord >> separator >> parameter >> !parameter)[ar_a] >> end;

static Parser<std::vector<char>> unkown_cmds = confix_p(alphaa_p() | ch_p(28), *anychar_p(), end);
static Parser<std::vector<char>> lb = confix_p(str_p("LB"), (*anychar_p())[lb_a], end);
static auto all_cmds = in | pu | pd | pa | pr | sp | ci | aa | ar | lb | unkown_cmds;

static Parser<std::vector<char>> dci = confix_p(ch_p(27), *anychar_p(), end);
static auto plt = (*(all_cmds | dci))[end_a];



bool parse(std::string_view &stream, Graph *graph)
{
    importer.reset();
    importer.load_graph(graph);
    return plt(stream).has_value();
}

bool parse(std::ifstream &stream, Graph *graph)
{
    importer.reset();
    importer.load_graph(graph);
    std::stringstream sstream;
    sstream << stream.rdbuf();
    std::string str(sstream.str());
    std::string_view temp(str);
    return plt(temp).has_value();
}

}