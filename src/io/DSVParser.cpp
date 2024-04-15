#include "io/DSVParser.hpp"
#include "io/Parser/ParserGen2.hpp"
#include <QString>
#include <sstream>


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
            dynamic_cast<Combination *>(_graph->container_groups().back().back())->append(
                new Container(Geo::Polygon(_points.cbegin(), _points.cend())));
        }
        else
        {
            _graph->container_groups().back().append(
                new Container(Geo::Polygon(_points.cbegin(), _points.cend())));
        }
    }
    else
    {
        if (_is_combination)
        {
            dynamic_cast<Combination *>(_graph->container_groups().back().back())->append(
                new Container(QString::fromStdString(_text), Geo::Polygon(_points.cbegin(), _points.cend())));
        }
        else
        {
            _graph->container_groups().back().append(new Container(
                QString::fromStdString(_text), Geo::Polygon(_points.cbegin(), _points.cend())));
        }
        _text.clear();
    }
    _points.clear();
}

void Importer::store_circle()
{
    if (_is_combination)
    {
        dynamic_cast<Combination *>(_graph->container_groups().back().back())->append(new CircleContainer(
            QString::fromStdString(_text), _points.back().x, _points.back().y, _parameters.back()));
    }
    else
    {
        _graph->container_groups().back().append(new CircleContainer(QString::fromStdString(_text), 
            _points.back().x, _points.back().y, _parameters.back()));
    }
    _text.clear();
    _points.clear();
    _parameters.pop_back();
}

void Importer::store_polyline()
{
    if (_is_combination)
    {
        dynamic_cast<Combination *>(_graph->container_groups().back().back())->append(
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
        dynamic_cast<Combination *>(_graph->container_groups().back().back())->append(
            new Geo::Bezier(_points.cbegin(), _points.cend(), _parameters.back()));
        dynamic_cast<Geo::Bezier *>(dynamic_cast<Combination *>(
            _graph->container_groups().back().back())->back())->update_shape();
    }
    else
    {
        _graph->container_groups().back().append(
            new Geo::Bezier(_points.cbegin(), _points.cend(), _parameters.back()));
        dynamic_cast<Geo::Bezier *>(_graph->container_groups().back().back())->update_shape();
    }
    _points.clear();
    _parameters.pop_back();
}

void Importer::store_text()
{
    if (_is_combination)
    {
        dynamic_cast<Combination *>(_graph->container_groups().back().back())->append(
            new Text(_points.back().x, _points.back().y, 12, QString::fromStdString(_text)));
    }
    else
    {
        _graph->container_groups().back().append(new Text(_points.back().x, 
            _points.back().y, 12, QString::fromStdString(_text)));
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
    dynamic_cast<Combination *>(_graph->container_groups().back().back())->update_border();
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



Importer importer;

Action<double> x_coord_a(&importer, &Importer::x_coord);
Action<double> y_coord_a(&importer, &Importer::y_coord);
Action<double> parameter_a(&importer, &Importer::parameter);
Action<void> polygon_a(&importer, &Importer::store_polygon);
Action<void> circle_a(&importer, &Importer::store_circle);
Action<void> polyline_a(&importer, &Importer::store_polyline);
Action<void> bezier_a(&importer, &Importer::store_bezier);
Action<void> text_a(&importer, &Importer::store_text);
Action<std::string> str_a(&importer, &Importer::store_text);
Action<void> begin_combination_a(&importer, &Importer::begin_combination);
Action<void> end_combination_a(&importer, &Importer::end_combination);
Action<void> group_a(&importer, &Importer::store_group);


Parser<char> separator = ch_p(',');
Parser<double> parameter = float_p()[parameter_a];
Parser<std::string> end = str_p("END") >> !eol_p();
Parser<bool> coord = float_p()[x_coord_a] >> separator >> float_p()[y_coord_a];
Parser<std::string> str = confix_p(ch_p('<'), (*anychar_p())[str_a], ch_p('>'));
Parser<bool> polygon = str_p("POLYGON") >> !str >> !eol_p() >> +(coord >> !separator) >> !eol_p() >> end[polygon_a];
Parser<bool> circle = str_p("CIRCLE") >> !str >> !eol_p() >> coord >> separator >> parameter >> !eol_p() >> end[circle_a];
Parser<bool> polyline = str_p("POLYLINE") >> !eol_p() >> +(coord >> !separator) >> !eol_p() >> end[polyline_a];
Parser<bool> bezier = str_p("BEZIER") >> !eol_p() >> parameter >> +(separator >> coord) >> !eol_p() >> end[bezier_a];
Parser<bool> text = str_p("TEXT") >> str >> !eol_p() >> coord >> !eol_p() >> end[text_a];
Parser<bool> combination = str_p("COMBINATION")[begin_combination_a] >> !eol_p() >>
    +(polygon | polyline | circle | text | bezier ) >> end[end_combination_a];
Parser<bool> group = str_p("GROUP") >> str[group_a] >> !eol_p() >>
    *(polygon | polyline | circle | text | combination | bezier ) >> str_p("END") >> !eol_p();
Parser<bool> dsv = +group;



bool DSVParser::parse(std::string_view &stream, Graph *graph)
{
    importer.reset();
    importer.load_graph(graph);
    return dsv(stream);
}

bool DSVParser::parse(std::ifstream &stream, Graph *graph)
{
    importer.reset();
    importer.load_graph(graph);
    std::stringstream sstream;
    sstream << stream.rdbuf();
    std::string str(sstream.str());
    std::string_view temp(str);
    return dsv(temp);
}

}