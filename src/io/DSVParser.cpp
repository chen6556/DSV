#include <sstream>
#include <QString>

#include "io/DSVParser.hpp"
#include <Parser/ParserGen2.hpp>


namespace DSVParser
{
void Importer::x_coord(const double value)
{
    _temp = value;
}

void Importer::y_coord(const double value)
{
    _points.emplace_back(_temp, value);
}

void Importer::parameter(const double value)
{
    _parameters.push_back(value);
}

void Importer::store_polygon()
{
    if (_text.empty())
    {
        if (_is_combination)
        {
            static_cast<Combination *>(_graph->container_groups().back().back())->append(
                new Geo::Polygon(_points.cbegin(), _points.cend()));
        }
        else
        {
            _graph->container_groups().back().append(new Geo::Polygon(_points.cbegin(), _points.cend()));
        }
    }
    else
    {
        if (_is_combination)
        {
            static_cast<Combination *>(_graph->container_groups().back().back())->append(
                new Geo::Polygon(_points.cbegin(), _points.cend()));
        }
        else
        {
            _graph->container_groups().back().append(new Geo::Polygon(_points.cbegin(), _points.cend()));
        }
        _text.clear();
    }
    _points.clear();
}

void Importer::store_circle()
{
    if (_is_combination)
    {
        static_cast<Combination *>(_graph->container_groups().back().back())->append(
            new Geo::Circle(_points.back().x, _points.back().y, _parameters.back()));
    }
    else
    {
        _graph->container_groups().back().append(new Geo::Circle(_points.back().x, _points.back().y, _parameters.back()));
    }
    _text.clear();
    _points.clear();
    _parameters.pop_back();
}

void Importer::store_ellipse()
{
    if (_is_combination)
    {
        static_cast<Combination *>(_graph->container_groups().back().back())->append(
            new Geo::Ellipse(_points[0], _points[1], _points[2], _points[3]));
    }
    else
    {
        _graph->container_groups().back().append(
            new Geo::Ellipse(_points[0], _points[1], _points[2], _points[3]));
    }
    _text.clear();
    _points.clear();
}

void Importer::store_polyline()
{
    if (_is_combination)
    {
        static_cast<Combination *>(_graph->container_groups().back().back())->append(
            new Geo::Polyline(_points.cbegin(), _points.cend()));
    }
    else
    {
        _graph->container_groups().back().append(
            new Geo::Polyline(_points.cbegin(), _points.cend()));
    }
    _points.clear();
}

void Importer::store_bezier()
{
    if (_is_combination)
    {
        static_cast<Combination *>(_graph->container_groups().back().back())->append(
            new Geo::Bezier(_points.cbegin(), _points.cend(), _parameters.back(), false));
        static_cast<Geo::Bezier *>(static_cast<Combination *>(_graph->container_groups().back().back())
            ->back())->update_shape();
    }
    else
    {
        _graph->container_groups().back().append(
            new Geo::Bezier(_points.cbegin(), _points.cend(), _parameters.back(), false));
        static_cast<Geo::Bezier *>(_graph->container_groups().back().back())->update_shape();
    }
    _points.clear();
    _parameters.pop_back();
}

void Importer::store_bspline()
{
    if (_parameters.size() > 2)
    {
        _parameters.erase(_parameters.begin(), _parameters.begin() + (_parameters.size() - 2));
    }
    if (_is_combination)
    {
        if (_parameters.front() == 2)
        {
            static_cast<Combination *>(_graph->container_groups().back().back())->append(
                new Geo::QuadBSpline(_points.cbegin(), _points.cend(), true));
        }
        else
        {
            static_cast<Combination *>(_graph->container_groups().back().back())->append(
                new Geo::CubicBSpline(_points.cbegin(), _points.cend(), true));
        }
    }
    else
    {
        if (_parameters.front() == 2)
        {
            _graph->container_groups().back().append(
                new Geo::QuadBSpline(_points.cbegin(), _points.cend(), true));
        }
        else
        {
            _graph->container_groups().back().append(
                new Geo::CubicBSpline(_points.cbegin(), _points.cend(), true));
        }
    }
    _points.clear();
    _parameters.pop_back();
}

void Importer::store_text()
{
    if (_is_combination)
    {
        static_cast<Combination *>(_graph->container_groups().back().back())->append(
            new Text(_points.back().x, _points.back().y, 12, QString::fromStdString(_text)));
    }
    else
    {
        _graph->container_groups().back().append(
            new Text(_points.back().x, _points.back().y, 12, QString::fromStdString(_text)));
    }
    _points.clear();
}

void Importer::store_arc()
{
    if (_is_combination)
    {
        static_cast<Combination *>(_graph->container_groups().back().back())->append(
            new Geo::Arc(_points[0], _points[1], _points[2]));
    }
    else
    {
        _graph->container_groups().back().append(new Geo::Arc(_points[0], _points[1], _points[2]));
    }
    _points.clear();
}

void Importer::begin_combination()
{
    _graph->container_groups().back().append(new Combination());
    _is_combination = true;
}

void Importer::end_combination()
{
    static_cast<Combination *>(_graph->container_groups().back().back())->update_border();
    _is_combination = false;
}

void Importer::store_text(const std::string &text)
{
    _text = text;
}

void Importer::store_group()
{
    _graph->append_group();
    if (!_text.empty())
    {
        _graph->container_groups().back().name = QString::fromStdString(_text);
        _text.clear();
    }
}


void Importer::load_graph(Graph *g)
{
    _graph = g;
}

void Importer::reset()
{
    _is_combination = false;
    _parameters.clear();
    _text.clear();
}


static Importer importer;

static Action<double> x_coord_a(&importer, &Importer::x_coord);
static Action<double> y_coord_a(&importer, &Importer::y_coord);
static Action<double> parameter_a(&importer, &Importer::parameter);
static Action<void> polygon_a(&importer, &Importer::store_polygon);
static Action<void> circle_a(&importer, &Importer::store_circle);
static Action<void> ellipse_a(&importer, &Importer::store_ellipse);
static Action<void> polyline_a(&importer, &Importer::store_polyline);
static Action<void> bezier_a(&importer, &Importer::store_bezier);
static Action<void> bspline_a(&importer, &Importer::store_bspline);
static Action<void> text_a(&importer, &Importer::store_text);
static Action<std::string> str_a(&importer, &Importer::store_text);
static Action<void> arc_a(&importer, &Importer::store_arc);
static Action<void> begin_combination_a(&importer, &Importer::begin_combination);
static Action<void> end_combination_a(&importer, &Importer::end_combination);
static Action<void> group_a(&importer, &Importer::store_group);


static Parser<char> separator = ch_p(',');
static Parser<double> parameter = float_p()[parameter_a];
static Parser<std::string> end = str_p("END") >> !eol_p();
static Parser<bool> coord = float_p()[x_coord_a] >> separator >> float_p()[y_coord_a];
static Parser<std::string> str = confix_p(ch_p('<'), (*anychar_p())[str_a], ch_p('>'));
static Parser<bool> polygon = str_p("POLYGON") >> !str >> !eol_p() >> list_p(coord, separator) >> !eol_p() >> end[polygon_a];
static Parser<bool> circle = str_p("CIRCLE") >> !str >> !eol_p() >> coord >> separator >> parameter >> !eol_p() >> end[circle_a];
static Parser<bool> ellipse = str_p("ELLIPSE") >> !str >> !eol_p() >> list_p(coord, separator) >> !eol_p() >> end[ellipse_a];
static Parser<bool> polyline = str_p("POLYLINE") >> !eol_p() >> list_p(coord, separator) >> !eol_p() >> end[polyline_a];
static Parser<bool> bezier = str_p("BEZIER") >> !eol_p() >> parameter >> separator >> list_p(coord, separator) >> !eol_p() >> end[bezier_a];
static Parser<bool> bspline = str_p("BSPLINE") >> !eol_p() >> parameter >> separator >> list_p(coord, separator) >> !eol_p() >> end[bspline_a];
static Parser<bool> text = str_p("TEXT") >> str >> !eol_p() >> coord >> !eol_p() >> end[text_a];
static Parser<bool> arc = str_p("ARC") >> !eol_p() >> coord >> separator >> coord >> separator >> coord >> !eol_p() >> end[arc_a];
static Parser<bool> combination = str_p("COMBINATION")[begin_combination_a] >> !eol_p() >>
                           +(polygon | polyline | circle | ellipse | text | bezier) >> end[end_combination_a];
static Parser<bool> group = str_p("GROUP") >> str[group_a] >> !eol_p() >>
                    *(polygon | polyline | circle | ellipse | text | arc | combination | bezier | bspline) >> str_p("END") >> !eol_p();
static Parser<bool> dsv = +group;


bool parse(std::string_view &stream, Graph *graph)
{
    importer.reset();
    importer.load_graph(graph);
    return dsv(stream);
}

bool parse(std::ifstream &stream, Graph *graph)
{
    importer.reset();
    importer.load_graph(graph);
    std::stringstream sstream;
    sstream << stream.rdbuf();
    std::string str(sstream.str());
    std::string_view temp(str);
    return dsv(temp);
}
} // namespace DSVParser