#include <sstream>
#include <string>
#include <QDebug>

#include "base/Geometry.hpp"
#include "base/Algorithm.hpp"
#include "draw/Container.hpp"
#include "io/RS274DParser.hpp"
#include "io/Parser/ParserGen2.hpp"
#include "io/GlobalSetting.hpp"


namespace RS274DParser
{

double Importer::unit_scale(int value)
{
    return (double)value * (UNIT_SACLES[static_cast<int>(this->unit)]) / 10;
}

void Importer::set_x_coord(const int value)
{
    _last_coord.x = unit_scale(value);
}

void Importer::set_y_coord(const int value)
{
    _last_coord.y = unit_scale(value);
    if (_points.empty() || _points.back() != _last_coord)
    {
        _points.emplace_back(_last_coord);
    }
}

void Importer::store_points()
{
    if (_points.size() <= 1)
    {
        _points.clear();
        return;
    }

    for (size_t i = _points.size() - 1; i > 0; --i)
    {
        if (_points[i] == _points[i - 1])
        {
            _points.erase(_points.begin() + i);
        }
    }

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

void Importer::draw_circle()
{
    _last_circle_container = new Container<Geo::Circle>(Geo::Circle(_last_coord, _circle_radius));
    _last_container = nullptr;
    _graph->container_groups().back().append(_last_circle_container);
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
    _points.emplace_back(_last_coord);
    _command = Command::D1;
}

void Importer::pen_up()
{
    this->_is_pen_down = false;
    store_points();
    _command = Command::D2;
}

void Importer::knife_down(const std::string &value)
{
    this->_is_knife_down = true;
    _points.emplace_back(_last_coord);
    if (value == "M14")
    {
        _command = Command::M14;
    }
    else
    {
        _command = Command::M19;
    }
}

void Importer::knife_up()
{
    if (_command != Command::M19)
    {
        this->_is_knife_down = false;
        store_points();
    }
    _command = Command::M15;
}

void Importer::set_circle_radius(const std::string &text)
{
    if (text == "M43")
    {
        _circle_radius = 10;
    }
    else if (text == "M44")
    {
        _circle_radius = 5;
    }
    else if (text == "M45" || text == "M72")
    {
        _circle_radius = 15;
    }
    else if (text == "M73")
    {
        _circle_radius = 20;
    }
    else
    {
        _circle_radius = 10;
    }
}

void Importer::load_graph(Graph *g)
{
    _graph = g;
    _graph->append_group();
}

void Importer::reset()
{
    _points.clear();
    _last_coord.clear();
    _circle_radius = 10;
    _index = 0;
    _is_pen_down = _is_knife_down = false;
    _curve_type = CurveType::Linear;
    unit = Unit::mm;
    _last_circle_container = nullptr;
    _last_container = nullptr;
}

void Importer::read_text()
{
    if (!_points.empty() && _command == Command::M15)
    {
        _points.pop_back();
    }
    _command = Command::M31;
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
            GlobalSetting::get_instance()->setting["text_size"].toInt(), QString::fromUtf8(text)));
    }
}

void Importer::store_table_text(const std::string &text)
{
    if (!_points.empty())
    {
        _points.pop_back();
    }
    Containerized *containerized = nullptr;
    for (Geo::Geometry *container : _graph->container_groups().back())
    {
        containerized = dynamic_cast<Containerized *>(container);
        if (containerized == nullptr)
        {
            continue;
        }
        switch (container->type())
        {
        case Geo::Type::POLYGON:
            if (Geo::is_inside(_last_coord, *dynamic_cast<Geo::Polygon *>(container)))
            {
                if (containerized->text().isEmpty())
                {
                    containerized->set_text(QString::fromUtf8(text));
                }
                else
                {
                    containerized->set_text(containerized->text() + '\n' + QString::fromUtf8(text));
                }
                return;
            }
            break;
        case Geo::Type::CIRCLE:
            if (Geo::is_inside(_last_coord, *dynamic_cast<Geo::Circle *>(container)))
            {
                if (containerized->text().isEmpty())
                {
                    containerized->set_text(QString::fromUtf8(text));
                }
                else
                {
                    containerized->set_text(containerized->text() + '\n' + QString::fromUtf8(text));
                }
                return;
            }
            break;
        default:
            break;
        }
    }
}

void Importer::print_symbol(const std::string &str)
{
    qDebug() << str;
}

void Importer::end()
{
    this->_is_pen_down = false;
    this->_is_knife_down = false;
    store_points();
}

Importer importer;


// 分隔符设置 '*'
Parser<char> blank = ch_p(' ') | ch_p('\t') | ch_p('\v');
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
Parser<std::string> set_mil_unit = (str_p("G72") | str_p("G70"))[set_mil_unit_a];

Parser<std::string> set_unit = (set_mm_unit | set_mil_unit) >> separator;

// 下刀提刀，下笔提笔
Action<std::string> knife_down_a(&importer, &Importer::knife_down);
Action<void> knife_up_a(&importer, &Importer::knife_up);
Action<void> pen_down_a(&importer, &Importer::pen_down);
Action<void> pen_up_a(&importer, &Importer::pen_up);

Parser<std::string> knife_down = (str_p("M14") | str_p("M19"))[knife_down_a];
Parser<std::string> knife_up = str_p("M15")[knife_up_a];
Parser<std::string> pen_down = (str_p("D1") | str_p("D01"))[pen_down_a];
Parser<std::string> pen_up = (str_p("D2") | str_p("D02"))[pen_up_a];

Parser<std::string> pen_move = (knife_down | knife_up | pen_down | pen_up);

// 插值方式(线型?) 目前只有线性
Parser<std::string> linear = str_p("G01");

Parser<std::string> interp = (linear) >> separator;

// 圆
Action<std::string> circle_radius_a(&importer, &Importer::set_circle_radius);
Action<void> circle_a(&importer, &Importer::draw_circle);

Parser<std::string> circle_radius = (str_p("M43") | str_p("M44") | str_p("M45") | str_p("M72") | str_p("M73"))[circle_radius_a];
Parser<bool> circle = (circle_radius >> (separator | coord))[circle_a];

// 文字处理
Action<void> read_text_a(&importer, &Importer::read_text);
Action<std::string> text_a(&importer, &Importer::store_text);
Parser<bool> text = confix_p(str_p("M31*")[read_text_a] >> !coord, (*anychar_p())[text_a], separator);
Parser<std::string> skip_text = confix_p(str_p("M20*"), separator);

// 步骤
Parser<bool> steps = ch_p('N') >> int_p() >> separator;
// 文件终止
Action<void> end_a(&importer, &Importer::end);
Parser<std::string> end = str_p("M0")[end_a] >> separator;
// 未知命令
Action<std::string> a_unkown(&importer, &Importer::print_symbol);

Parser<std::string> unkown_cmds = confix_p(alphaa_p(), *anychar_p(), separator)[a_unkown];

Parser<bool> cmd = *(eol_p() | coord | set_unit | pen_move | interp | circle | steps | text | skip_text | blank | skip_cmd | separator | end | unkown_cmds);

Action<std::string> table_text_a(&importer, &Importer::store_table_text);

Parser<std::string> table_start = str_p("N,0001") >> eol_p();
Parser<std::string> rest_of_line = *(anychar_p() - eol_p());
Parser<std::string> unknown_gap_line = rest_of_line >> eol_p();
Parser<std::string> unknown_gap = *(unknown_gap_line - table_start);
Parser<std::string> table_line = rest_of_line >> eol_p();
Parser<bool> position_line = str_p("P,") >> int_p()[x_coord_a] >> ch_p(',') >> int_p()[y_coord_a] >> rest_of_line >> eol_p();
Parser<bool> text_line = str_p("D,") >> int_p() >> ch_p(',') >> rest_of_line[table_text_a] >> eol_p();
Parser<std::string> table_end = str_p("L0*") >> !eol_p();
Parser<bool> table = confix_p(table_start, *(text_line | position_line | table_line), table_end);

Parser<bool> rs274 = cmd >> *(anychar_p() - table_start) >> !table;


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