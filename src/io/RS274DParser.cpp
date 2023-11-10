#include <QDebug>
#include "io/Parser.hpp"
#include "io/RS274DParser.hpp"
#include <sstream>

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

void Importer::load_graph(Graph *g)
{
    _graph = g;
    _graph->append_group();
}
void Importer::reset()
{
}

void Importer::unkown_handle(const std::string &cmd)
{
    qDebug() << "RS274D Unkown Cmd: " << cmd << "\n";
}

static Importer importer;


static Action<int> h_a();
static Action<int> n_a();

// 分隔符设置 '*'
static Parser<char> blank = ch_p(' ') | ch_p('\t') | ch_p('\v');
static Parser<char> separator = ch_p('*');
// 坐标设置
static Action<int> x_coord_a(&importer, &Importer::set_x_coord);
static Action<int> y_coord_a(&importer, &Importer::set_y_coord);
static auto coord = ch_p('X') >> int_p()[x_coord_a] >>
                    ch_p('Y') >> int_p()[y_coord_a] >> separator;
// 单位设置 mil实际为100mil，换算为2.54mm
static Action<void> set_mm_unit_a(&importer, &Importer::set_unit_mm);
static Action<void> set_mil_unit_a(&importer, &Importer::set_unit_hectomil);

static Parser<std::string> set_mm_unit = str_p("G71")[set_mm_unit_a];
static Parser<std::string> set_mil_unit = str_p("G72")[set_mil_unit_a];

static Parser<char> set_unit = (set_mm_unit | set_mil_unit) >> separator;

// 粗细
static Parser<std::string> hole_1 = str_p("H1");

static Parser<char> pen_size = (hole_1) >> separator;

// 下刀提刀，下笔提笔
static Action<void> knife_down_a(&importer, &Importer::knife_down);
static Action<void> knife_up_a(&importer, &Importer::knife_up);
static Action<void> pen_down_a(&importer, &Importer::pen_down);
static Action<void> pen_up_a(&importer, &Importer::pen_up);

static Parser<std::string> knife_down = str_p("M14")[knife_down_a];
static Parser<std::string> knife_up = str_p("M15")[knife_up_a];
static Parser<std::string> pen_down = (str_p("D1") | str_p("D01"))[pen_down_a];
static Parser<std::string> pen_up = (str_p("D2") | str_p("D02"))[pen_up_a];

static Parser<char> pen_move = (knife_down | knife_up | pen_down | pen_up) >> separator;

// 插值方式(线型?) 目前只有线性
static Parser<std::string> linear = str_p("G01");

static Parser<char> interp = (linear) >> separator;

// 圆
static Parser<std::string> circle20 = str_p("M43");

static Parser<char> circle = (circle20) >> separator;

// 步骤
static Parser<int> steps = ch_p('N') >> int_p() >> separator;

// 未知命令
static Action<std::string> unkown_a(&importer, &Importer::unkown_handle);
static Parser<std::vector<char>> unkown_cmds = confix_p(alphaa_p(), *anychar_p(), separator)[unkown_a];

static auto cmd = *(eol_p() | coord | set_unit | pen_size | pen_move |
                    interp | circle | steps | blank | unkown_cmds);
// static auto cmd = *(coord | set_unit | pen_size | pen_move | interp | circle | steps | blank | eol_p() | unkown_cmds);


bool parse(std::string_view &stream, Graph *graph)
{
    importer.reset();
    importer.load_graph(graph);
    return cmd(stream).has_value();
}

bool parse(std::ifstream &stream, Graph *graph)
{
    importer.reset();
    importer.load_graph(graph);
    std::stringstream sstream;
    sstream << stream.rdbuf();
    std::string str(sstream.str());
    std::string_view temp(str);
    return cmd(temp).has_value();
}
} // namespace RS274DParser