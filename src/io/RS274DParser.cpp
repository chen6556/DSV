#include "io/RS274DParser.hpp"
#include "base/Geometry.hpp"
#include "draw/Container.hpp"
#include "io/Action.hpp"
#include "io/Parser.hpp"
#include "io/GlobalSetting.hpp"
#include <sstream>
#include <string>
#include <QDebug>

namespace RS274DParser
{

double Importer::unit_scale(int value)
{
    return (double)value * (UNIT_SACLES[static_cast<int>(this->unit)]);
}

void Importer::set_x_coord(const int value)
{
    _last_coord.x = unit_scale(value);
}

void Importer::set_y_coord(const int value)
{
    _last_coord.y = unit_scale(value);
    _points.emplace_back(Geo::Point(_last_coord));
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
        _last_container = new Container(Geo::Polygon(_points.cbegin(), _points.cend()));
        _last_circle_container = nullptr;
        _graph->container_groups().back().append(_last_container);
    }
    else
    {
        _graph->container_groups().back().append(new Geo::Polyline(_points.cbegin(), _points.cend()));
    }
    _points.clear();
}

void Importer::draw_circle(const int radius)
{
    _last_circle_container = new CircleContainer(Geo::Circle(Geo::Point(_last_coord), radius));
    _last_container = nullptr;
    _graph->container_groups().back().append(_last_circle_container);
}

void Importer::set_unit_mm()
{
    this->unit = Unit::mm;
}

void Importer::set_unit_hectomil()
{
    this->unit = Unit::hectomil;
}

void Importer::pen_down()
{
    this->_is_pen_down = true;
    _points.emplace_back(Geo::Point(_last_coord));
}

void Importer::pen_up()
{
    this->_is_pen_down = false;
    store_points();
}

void Importer::knife_down()
{
    this->_is_knife_down = true;
    _points.emplace_back(Geo::Point(_last_coord));
}

void Importer::knife_up()
{
    this->_is_knife_down = false;
    store_points();
}

void Importer::draw_cricle_10()
{
    draw_circle(10);
}

void Importer::draw_cricle_20()
{
    draw_circle(20);
}

void Importer::draw_cricle_30()
{
    draw_circle(30);
}

void Importer::draw_cricle_40()
{
    draw_circle(40);
}

void Importer::load_graph(Graph *g)
{
    _graph = g;
    _graph->append_group();
}

void Importer::reset()
{
}

void Importer::store_text(const std::string &text)
{
    if (_last_container != nullptr)
    {
        _last_container->set_text(_last_container->text() + '\n' + QString::fromUtf8(text));
    }
    else if (_last_circle_container != nullptr)
    {
        _last_circle_container->set_text(_last_circle_container->text() + '\n' + QString::fromUtf8(text));
    }
    else
    {
        _graph->container_groups().back().append(new Text(_last_coord.x, _last_coord.y,
            GlobalSetting::get_instance()->setting()["text_size"].toInt(), QString::fromUtf8(text)));
    }
}

void Importer::print_symbol(const std::string &str)
{
    qDebug() << str;
}

Importer importer;


// 分隔符设置 '*'
Parser<bool> blank = ch_p(' ') | ch_p('\t') | ch_p('\v');
Parser<char> separator = ch_p('*');

Parser<bool> skip_cmd = ((ch_p('H') >> int_p()) | ch_p('Q')) >> !separator;

// 坐标设置
Action<int> x_coord_a(&importer, &Importer::set_x_coord);
Action<int> y_coord_a(&importer, &Importer::set_y_coord);
Parser<bool> coord = ch_p('X') >> int_p()[x_coord_a] >>
                    ch_p('Y') >> int_p()[y_coord_a] >> !separator;
// 单位设置 mil实际为100mil，换算为2.54mm
Action<void> set_mm_unit_a(&importer, &Importer::set_unit_mm);
Action<void> set_mil_unit_a(&importer, &Importer::set_unit_hectomil);

Parser<std::string> set_mm_unit = str_p("G71")[set_mm_unit_a];
Parser<bool> set_mil_unit = (str_p("G72") | str_p("G70"))[set_mil_unit_a];

Parser<bool> set_unit = (set_mm_unit | set_mil_unit) >> separator;

// 下刀提刀，下笔提笔
Action<void> knife_down_a(&importer, &Importer::knife_down);
Action<void> knife_up_a(&importer, &Importer::knife_up);
Action<void> pen_down_a(&importer, &Importer::pen_down);
Action<void> pen_up_a(&importer, &Importer::pen_up);

Parser<std::string> knife_down = str_p("M14")[knife_down_a];
Parser<bool> knife_up = (str_p("M15") | str_p("M19"))[knife_up_a];
Parser<bool> pen_down = (str_p("D1") | str_p("D01"))[pen_down_a];
Parser<bool> pen_up = (str_p("D2") | str_p("D02"))[pen_up_a];

Parser<bool> pen_move = (knife_down | knife_up | pen_down | pen_up);

// 插值方式(线型?) 目前只有线性
Parser<std::string> linear = str_p("G01");

Parser<bool> interp = (linear) >> separator;

// 圆
Action<void> circle20_a(&importer, &Importer::draw_cricle_20);
Action<void> circle10_a(&importer, &Importer::draw_cricle_10);
Action<void> circle30_a(&importer, &Importer::draw_cricle_30);
Action<void> circle40_a(&importer, &Importer::draw_cricle_40);

// Parser<std::string> circle00 = str_p("M0")[circle00_a];
Parser<std::string> circle20 = str_p("M43")[circle20_a];
Parser<std::string> circle10 = str_p("M44")[circle10_a];
Parser<bool> circle30 = (str_p("M45") | str_p("M72"))[circle30_a];
Parser<std::string> circle40 = str_p("M73")[circle40_a];

Parser<bool> circle = (circle10 | circle20 | circle30 | circle40) >> separator;

// 文字处理
Action<std::string> a_text(&importer, &Importer::store_text);
Parser<bool> text = confix_p(str_p("M31*"), (*anychar_p())[a_text], separator);
Parser<bool> skip_text = confix_p(str_p("M20*"), *anychar_p(), separator);

// 步骤
Parser<bool> steps = ch_p('N') >> int_p() >> separator;
// 文件终止
Parser<bool> end = str_p("M0") >> separator;
// 未知命令
Action<std::string> a_unkown(&importer, &Importer::print_symbol);

Parser<bool> unkown_cmds = confix_p(alphaa_p(), (*anychar_p())[a_unkown], separator) - end;

Parser<bool> cmd = *(eol_p() | coord | set_unit | pen_move | interp | circle | steps | text | skip_text | blank | skip_cmd | separator | unkown_cmds);

Parser<bool> table_start = str_p("N,0001") >> eol_p();
Parser<bool> rest_of_line = *(anychar_p() - eol_p());
Parser<bool> unknown_gap_line = rest_of_line >> eol_p();
Parser<bool> unknown_gap = *(unknown_gap_line - table_start);
Parser<bool> table_line = rest_of_line >> eol_p();
Parser<bool> position_line = str_p("P,") >> int_p() >> ch_p(',') >> int_p() >> rest_of_line >> eol_p();
Parser<bool> text_line = str_p("D,") >> int_p() >> ch_p(',') >> rest_of_line >> eol_p();
Parser<bool> table_end = str_p("L0*") >> !eol_p();
Parser<bool> table = confix_p(table_start, *(text_line | position_line | table_line), table_end);

Parser<bool> rs274 = cmd >> !end >> *(anychar_p() - table_start) >> !table;


bool parse(std::string_view &stream, Graph *graph)
{
    importer.reset();
    importer.load_graph(graph);
    return rs274(stream);
}

bool parse(std::ifstream &stream, Graph *graph)
{
    importer.reset();
    importer.load_graph(graph);
    std::stringstream sstream;
    sstream << stream.rdbuf();
    std::string str(sstream.str());
    std::string_view temp(str);
    return rs274(temp);
}
} // namespace RS274DParser