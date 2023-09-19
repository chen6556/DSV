#include "io/PDFParser.hpp"
#include "io/Parser.hpp"
#include <sstream>


namespace PDFParser
{

void Importer::start()
{
    if (_values.size() > 2)
    {
        _values.erase(_values.begin(), _values.begin() + _values.size() - 2);
    }
    _points.clear();
    _start_point.x = _values.front();
    _start_point.y = _values.back();
    _points.emplace_back(Geo::Point(_start_point));
}

void Importer::store_value(const double value)
{
    _values.emplace_back(value);
}


void Importer::change_trans_mat()
{
    if (_values.size() > 6)
    {
        _values.erase(_values.begin(), _values.begin() + _values.size() - 6);
    }

    _trans_mat[0] = _values[0];
    _trans_mat[1] = _values[2];
    _trans_mat[2] = _values[4];
    _trans_mat[3] = _values[1];
    _trans_mat[4] = _values[3];
    _trans_mat[5] = _values[5];

    _values.clear();
}

void Importer::store_trans_mat()
{
    _trans_mats.push(_trans_mat);
}

void Importer::pop_trans_mat()
{
    _trans_mat = _trans_mats.top();
    _trans_mats.pop();
}


void Importer::line()
{
    _last_tool = _cur_tool;
    _cur_tool = Tool::Line;

    _points.emplace_back(Geo::Point(_values[_values.size() - 2], _values.back()));
    _values.erase(_values.begin(), _values.begin() + _values.size() - 2);
}

void Importer::curve()
{
    _last_tool = _cur_tool;
    _cur_tool = Tool::Curve;
    
    std::vector<Geo::Point> points;
    for (int i = 0; i < 7; i += 2)
    {
        points.emplace_back(Geo::Point(_values[i], _values[i + 1]));
    }
    Geo::Bezier bezier(points.cbegin(), points.cend(), 3);
    _values.erase(_values.begin(), _values.begin() + _values.size() - 2);
    for (const Geo::Point &point : bezier.shape())
    {
        _points.emplace_back(point);
    }
}

void Importer::rect()
{
    _points.clear();
    _points.emplace_back(Geo::Point(_values[_values.size() - 4], _values[_values.size() - 3]));
    _points.emplace_back(Geo::Point(_values[_values.size() - 4] + _values[_values.size() - 2], _values[_values.size() - 3]));
    _points.emplace_back(Geo::Point(_values[_values.size() - 4] + _values[_values.size() - 2], _values[_values.size() - 3] + _values.back()));
    _points.emplace_back(Geo::Point(_values[_values.size() - 4], _values[_values.size() - 3] + _values.back()));
    _points.emplace_back(Geo::Point(_values[_values.size() - 4], _values[_values.size() - 3]));

    _values.clear();
    _values.push_back(_points.back().coord().x);
    _values.push_back(_points.back().coord().y);
    _start_point.x = _values.front();
    _start_point.y = _values.back();
}

void Importer::close_shape()
{
    _points.emplace_back(_points.front());
}

void Importer::close_and_store_shape()
{
    _graph->back().append(new Container(Geo::Polygon(_points.cbegin(), _points.cend())));
    _graph->back().back()->transform(_trans_mat[0], _trans_mat[1],
        _trans_mat[2], _trans_mat[3], _trans_mat[4], _trans_mat[5]);

    _start_point.x = _start_point.y = 0;
    _values.clear();
    _points.clear();
}


void Importer::store()
{
    if ((_start_point.x == _values[_values.size() - 2] && _start_point.y == _values.back())
        || _points.front() == _points.back())
    {
        _graph->back().append(new Container(Geo::Polygon(_points.cbegin(), _points.cend())));
    }
    else
    {
        _graph->back().append(new Geo::Polyline(_points.cbegin(), _points.cend()));
    }
    _graph->back().back()->transform(_trans_mat[0], _trans_mat[1],
        _trans_mat[2], _trans_mat[3], _trans_mat[4], _trans_mat[5]);

    _start_point.x = _start_point.y = 0;
    _values.clear();
    _points.clear();
}

void Importer::load_graph(Graph *g)
{
    _graph = g;
    if (_graph->container_groups().empty() || !_graph->back().empty())
    {
        _graph->append_group();
    }
}

void Importer::reset()
{
    _values.clear();
    _points.clear();
    _last_tool = _cur_tool = Tool::None;
    _start_point.x = _start_point.y = 0;
    _trans_mat = {1, 0, 0, 0, 1, 0};
    while (!_trans_mats.empty())
    {
        _trans_mats.pop();
    }
}    

void Importer::print(const std::string &value)
{
    int a = 0;
}


static Importer importer;


static Action<double> parameter_a(&importer, &Importer::store_value);
static Action<void> cm_a(&importer, &Importer::change_trans_mat);
static Action<void> Q_a(&importer, &Importer::pop_trans_mat);
static Action<void> q_a(&importer, &Importer::store_trans_mat);
static Action<void> m_a(&importer, &Importer::start);
static Action<void> l_a(&importer, &Importer::line);
static Action<void> c_a(&importer, &Importer::curve);
static Action<void> S_a(&importer, &Importer::store);
static Action<void> re_a(&importer, &Importer::rect);
static Action<void> h_a(&importer, &Importer::close_shape);
static Action<void> W8_a(&importer, &Importer::close_and_store_shape);

static Action<std::string> print_a(&importer, &Importer::print);

static Parser<char> end = eol_p() | ch_p(' ');
static Parser<char> space = ch_p(' ');
static Parser<char> CS = str_p("CS") >> end;
static Parser<char> cs = str_p("cs") >> end;
static Parser<char> SCN = str_p("SCN") >> end;
static Parser<char> G = ch_p('G') >> end;
static Parser<char> g = ch_p('g') >> end;
static Parser<char> RG = str_p("RG") >> end;
static Parser<char> rg = str_p("rg") >> end;
static Parser<char> K = ch_p('K') >> end;
static Parser<char> k = ch_p('k') >> end;
static Parser<char> cm = str_p("cm")[cm_a] >> end;
static Parser<char> Q = ch_p('Q')[Q_a] >> end;
static Parser<char> q = ch_p('q')[q_a] >> end;
static Parser<char> m = ch_p('m')[m_a] >> end;
static Parser<char> l = ch_p('l')[l_a] >> end;
static Parser<char> c = ch_p('c')[c_a] >> end;
static Parser<char> v = ch_p('v') >> end;
static Parser<char> S = ch_p('S')[S_a] >> end;
static Parser<char> s = ch_p('s') >> end;
static Parser<char> re = str_p("re")[re_a] >> end;
static Parser<char> h = ch_p('h')[h_a] >> end;
static Parser<char> W8 = str_p("W*")[W8_a] >> end;
static Parser<char> B = ch_p('B') >> end;
static Parser<char> f = ch_p('f') >> end;

static Parser<char> BT = str_p("BT") >> end;
static Parser<char> ET = str_p("ET") >> end;
static Parser<char> Tm = str_p("Tm") >> end;
static Parser<char> TL = str_p("TL") >> end;

static auto order = cm | q | Q | m | l | c | v | CS | cs | G | g | RG | rg | K | k | re | h |
            BT | ET | Tm | SCN | W8 | TL |
            ((ch_p('w') | ch_p('J') | ch_p('j') | ch_p('M') | ch_p('d') | str_p("ri") | ch_p('i') |
            str_p("gs") | ch_p('y') | str_p("f*") | ch_p('F') | str_p("B*") | str_p("b*") | ch_p('b') |
            ch_p('n') | ch_p('W') | str_p("Tc") | str_p("Tw") | str_p("Tz") | str_p("Tf") | str_p("Tr") |
            str_p("Ts") | str_p("Td") | str_p("TD") | str_p("T*") | str_p("Tj") | str_p("TJ") | ch_p('\'') |
            ch_p('\"') | str_p("d0") | str_p("d1") | str_p("SC") | str_p("scn") | str_p("sc") | str_p("sh") |
            str_p("BI") | str_p("ID") | str_p("EI") | str_p("Do") | str_p("MP") | str_p("DP") |
            str_p("BMC") | str_p("BDC") | str_p("EMC") | str_p("BX") | str_p("EX")) >> end) |
            S | s | B | f;

static Parser<double> parameter = float_p()[parameter_a];
static Parser<std::vector<char>> key = confix_p(ch_p('/'), *anychar_p(), eol_p());
static Parser<std::vector<char>> text = confix_p(ch_p('<'), *anychar_p(), ch_p('>'));
static Parser<std::vector<char>> annotation = pair(ch_p('['), eol_p());
static auto command = *(parameter | key | text | space) >> order;
static auto array = pair(ch_p('['), ch_p(']')) >> !eol_p();
static auto dict = pair(str_p("<<"), str_p(">>"))[print_a] >> !eol_p();
static Parser<std::vector<double>> code = confix_p(ch_p('<'), repeat(4, parameter), ch_p('>'));
static auto font_info = str_p("/CIDInit /ProcSet findresource begin") >> (+anychar_p() - (str_p("beginbfchar") >> eol_p()))
    	                >> str_p("beginbfchar") >> end >> +(code >> eol_p()) >> !eol_p() >> str_p("endbfchar")
                        >> end >> (+anychar_p() - (str_p("end") >> eol_p())) >> +(str_p("end") >> eol_p());
static Parser<std::vector<char>> xml = pair(str_p("<?xpacket"), str_p("<?xpacket end=\"w\"?>")) >> eol_p();
static Parser<std::vector<char>> others = (+anychar_p() - str_p("endstream"));

static Parser<char> stream_start = str_p("stream") >> eol_p();
static Parser<char>	stream_end = str_p("endstream") >> eol_p();
static auto stream = stream_start >> +(command | dict | float_p() | space | eol_p() | font_info | xml | others) >> stream_end;
static auto object = (int_p() >> space >> int_p() >> space >> str_p("obj") >> eol_p() >>
					*(int_p() | dict | stream | array) >> str_p("endobj") >> eol_p());

static auto head = str_p("%PDF-") >> float_p() >> (+anychar_p() - ch_p('%')) >> ch_p('%') >> (*anychar_p() - eol_p()) >> eol_p();
static Parser<std::vector<char>> tail = (str_p("xref") >> *anychar_p());
static auto pdf = head >> *(annotation | object | eol_p()) >> tail;



bool PDFParser::parse(std::string_view &stream, Graph *graph)
{
    importer.reset();
    importer.load_graph(graph);
    return pdf(stream).has_value();
}

bool PDFParser::parse(std::ifstream &stream, Graph *graph)
{
    importer.reset();
    importer.load_graph(graph);
    std::stringstream sstream;
    sstream << stream.rdbuf();
    std::string str(sstream.str());
    std::string_view temp(str);
    return pdf(temp).has_value();
}

}