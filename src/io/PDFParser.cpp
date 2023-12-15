#include "io/PDFParser.hpp"
#include "io/Parser.hpp"
#include <sstream>
#include <QStringList>


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

void Importer::store_key(const std::string &value)
{
    _keys.emplace_back(value);
}

void Importer::CS()
{
    if (_keys.back() == "DeviceGray")
    {
        _stroking_color_space = ColorSpace::Gray;
    }
    else if (_keys.back() == "DeviceCMYK")
    {
        _stroking_color_space = ColorSpace::CMYK;
    }
    else if (_keys.back() == "DeviceRGB")
    {
        _stroking_color_space = ColorSpace::RGB;
    }
    else if (_color_map_index.find(_keys.back()) != _color_map_index.end())
    {
        _cur_color_map_index = _color_map_index[_keys.back()];
    }

    _keys.clear();
    _values.clear();
}

void Importer::cs()
{
    if (_keys.back() == "DeviceGray")
    {
        _nonstroking_color_space = ColorSpace::Gray;
    }
    else if (_keys.back() == "DeviceCMYK")
    {
        _nonstroking_color_space = ColorSpace::CMYK;
    }
    else if (_keys.back() == "DeviceRGB")
    {
        _nonstroking_color_space = ColorSpace::RGB;
    }
    else if (_color_map_index.find(_keys.back()) != _color_map_index.end())
    {
        _cur_color_map_index = _color_map_index[_keys.back()];
    }

    _keys.clear();
    _values.clear();
}

void Importer::SCN()
{
    if (_cur_color_map_index > -1)
    {
        std::fill_n(_stroking_color, 4, 0);
        _stroking_color_space = _color_map[_cur_color_map_index]._color_space;
        int i = 0;
        for (const double value : _color_map[_cur_color_map_index](_values))
        {
            _stroking_color[i++] = value;
        }
        _cur_color_map_index = -1;
        _values.clear();
    }
}

void Importer::G()
{
    _stroking_color[0] = _values.back();
	_stroking_color[1] = _stroking_color[2] = _stroking_color[3] = 0;

	_values.clear();
}

void Importer::g()
{
    _nonstroking_color[0] = _values.back();
	_nonstroking_color[1] = _nonstroking_color[2] = _nonstroking_color[3] = 0;

	_values.clear();
}

void Importer::RG()
{
    if (_values.size() > 3)
    {
        _values.erase(_values.begin(), _values.begin() + _values.size() - 3);
    }

    _stroking_color[0] = _values[0];
    _stroking_color[1] = _values[1];
    _stroking_color[2] = _values[2];
    _stroking_color[3] = 0;

    _values.clear();
}

void Importer::rg()
{
    if (_values.size() > 3)
    {
        _values.erase(_values.begin(), _values.begin() + _values.size() - 3);
    }

    _nonstroking_color[0] = _values[0];
    _nonstroking_color[1] = _values[1];
    _nonstroking_color[2] = _values[2];
    _nonstroking_color[3] = 0;

    _values.clear();
}

void Importer::K()
{
    if (_values.size() > 4)
    {
        _values.erase(_values.begin(), _values.begin() + _values.size() - 4);
    }

    _stroking_color[0] = _values[0];
    _stroking_color[1] = _values[1];
    _stroking_color[2] = _values[2];
    _stroking_color[3] = _values[3];

    _values.clear();
}

void Importer::k()
{
    if (_values.size() > 4)
    {
        _values.erase(_values.begin(), _values.begin() + _values.size() - 4);
    }

    _nonstroking_color[0] = _values[0];
    _nonstroking_color[1] = _values[1];
    _nonstroking_color[2] = _values[2];
    _nonstroking_color[3] = _values[3];

    _values.clear();
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

void Importer::store_object(const int value)
{
    _cur_object = value;
    _objects.emplace_back(value);
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


void Importer::store_text(const std::string &value)
{
    _text = value;
}

void Importer::store_encoding(const std::string &value)
{
    if (_text.empty())
    {
        _text = value;
    }
    else
    {
        _encoding_map[_text] = value;
        _text.clear();
    }
}

void Importer::BT()
{
    _text.clear();
}

void Importer::ET()
{
    _texts.back().txt = _text;
	_text.clear();
}

void Importer::Tm()
{
    if (_values.size() > 6)
    {
        _values.erase(_values.begin(), _values.begin() + _values.size() - 6);
    }
	_texts.emplace_back(Text(_text, _points.back()));

	const double a = _trans_mat[0] * _values[0] + _trans_mat[1] * _values[1];
    const double b = _trans_mat[0] * _values[2] + _trans_mat[1] * _values[3];
    const double c = _trans_mat[0] * _values[4] + _trans_mat[1] * _values[5] + _trans_mat[2];
    const double d = _trans_mat[3] * _values[0] + _trans_mat[4] * _values[1];
    const double e = _trans_mat[3] * _values[2] + _trans_mat[4] * _values[3];
    const double f = _trans_mat[3] * _values[4] + _trans_mat[4] * _values[5] + _trans_mat[5];
	_texts.back().pos.transform(a, b, c, d, e, f);
}

void Importer::end()
{
    std::vector<ushort> values;
    for (Text &text : _texts)
    {
        for (unsigned int i = 0, count = text.txt.length(); i < count; i+= 4)
        {
            values.emplace_back(std::stoi(_encoding_map[text.txt.substr(i, 4)], nullptr, 16));
        }
        text.txt = QString::fromUtf16(&values.front(), values.size()).toStdString();
        values.clear();

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
    _text.clear();
    _encoding_map.clear();
}


void Importer::get_color_map_index(const std::string_view &stream)
{
    const size_t pos = stream.find("/ColorSpace");
    if (pos != std::string::npos)
    {
        QString items = QString::fromStdString(std::string(stream.substr(stream.find("<<", pos) + 2, stream.find(">>", pos) - stream.find("<<", pos) - 2)));
        for (QString &item : items.split('R'))
        {
            QStringList values = item.split(' ');
            if (values.size() < 3)
            {
                break;
            }
            if (values.front().length() < 3)
            {
                values.pop_front();
            }
            _color_map_index[values.front().toStdString().substr(1)] = values[1].toInt();
        }
    }
}

void Importer::get_color_map(const std::string_view &stream)
{
    size_t pos0, pos1, pos = stream.find(" obj");
    size_t mapPos = stream.find("FunctionType") - 2;
    if (mapPos == std::string::npos - 2)
    {
        return;
    }
    while (stream[mapPos] != '<' && stream[mapPos - 1] != '<')
    {
        --mapPos;
    }
    int index, i;
    while (pos != std::string::npos)
    {
        pos1 = pos;
        while (stream[--pos1] != ' ');
        pos0 = pos1 - 1;
        while (stream[pos0] >= '0' && stream[pos0] <= '9')
        {
            --pos0;
        }
        ++pos0;

        pos = stream.find("endobj", pos);
        if (mapPos < pos)
        {
            index = std::stoi(std::string(stream.substr(pos0, pos1 - pos0)));
            _color_map[index] = DeviceColor();
            QString items = QString::fromStdString(std::string(stream.substr(mapPos, stream.find(">>", mapPos) - mapPos - 1)));
            for (QString &item : items.split('/'))
            {
                if (item.startsWith("FunctionType"))
                {
                    item.remove(0, item.indexOf('e') + 1);
                    switch (item.toInt())
                    {
                    case 0:
                        _color_map[index]._func = std::make_shared<FunctionType0>(FunctionType0());
                        break;
                    case 2:
                        _color_map[index]._func = std::make_shared<FunctionType2>(FunctionType2());
                        break;
                    case 3:
                        _color_map[index]._func = std::make_shared<FunctionType3>(FunctionType3());
                        break;
                    default:
                        break;
                    }
                    _color_map[index]._func->_type = item.toInt();
                    break;
                }
            }

            for (QString &item : items.split('/'))
            {
                if (item.startsWith("Domain"))
                {
                    item.remove(0, item.indexOf('[') + 1);
                    item.remove("]");
                    for (QString &value : item.split(' '))
                    {
                        if (value.length() > 0)
                        {
                            _color_map[index]._func->_domain.push_back(value.toDouble());
                        }
                    }
                }
                else if (item.startsWith("Range"))
                {
                    item.remove(0, item.indexOf('[') + 1);
                    item.remove("]");
                    i = 0;
                    for (QString &value : item.split(' '))
                    {
                        if (value.length() > 0)
                        {
                            _color_map[index]._func->_range.push_back(value.toDouble());
                            ++i;
                        }
                    }
                    switch (i)
                    {
                    case 2:
                        _color_map[index]._color_space = ColorSpace::Gray;
                        break;
                    case 8:
                        _color_map[index]._color_space = ColorSpace::CMYK;
                        break;
                    default:
                        break;
                    }
                }

                else if (item.startsWith("Size"))
                {
                    item.remove(0, item.indexOf('[') + 1);
                    item.remove("]");
                    for (QString &value : item.split(' '))
                    {
                        if (value.length() > 0)
                        {
                            std::dynamic_pointer_cast<FunctionType0>(_color_map[index]._func)->_size.push_back(value.toInt());
                        }
                    }
                }
                else if (item.startsWith("Encode"))
                {
                    item.remove(0, item.indexOf('[') + 1);
                    item.remove("]");
                    if (_color_map[index]._func->_type == 0)
                    {
                        for (QString &value : item.split(' '))
                        {
                            if (value.length() > 0)
                            {
                                std::dynamic_pointer_cast<FunctionType0>(_color_map[index]._func)->_encoded.push_back(value.toDouble());
                            }
                        }
                    }
                    else
                    {
                        for (QString &value : item.split(' '))
                        {
                            if (value.length() > 0)
                            {
                                std::dynamic_pointer_cast<FunctionType3>(_color_map[index]._func)->_encode.push_back(value.toDouble());
                            }
                        }
                    }
                }
                else if (item.startsWith("Dncode"))
                {
                    item.remove(0, item.indexOf('[') + 1);
                    item.remove("]");
                    for (QString &value : item.split(' '))
                    {
                        if (value.length() > 0)
                        {
                            std::dynamic_pointer_cast<FunctionType0>(_color_map[index]._func)->_decoded.push_back(value.toDouble());
                        }
                    }
                }
                else if (item.startsWith("BitsPerSample"))
                {
                    item.remove(0, item.indexOf('l') + 2);
                    std::dynamic_pointer_cast<FunctionType0>(_color_map[index]._func)->_bits_per_sample = item.toInt();
                }
                
                else if (item.startsWith("C0"))
                {
                    item.remove(0, item.indexOf('[') + 1);
                    item.remove("]");			
                    for (QString &value : item.split(' '))
                    {
                        if (value.length() > 0)
                        {
                            std::dynamic_pointer_cast<FunctionType2>(_color_map[index]._func)->_C0.push_back(value.toDouble());
                        }
                    }
                }
                else if (item.startsWith("C1"))
                {
                    item.remove(0, item.indexOf('[') + 1);
                    item.remove("]");
                    for (QString &value : item.split(' '))
                    {
                        if (value.length() > 0)
                        {
                            std::dynamic_pointer_cast<FunctionType2>(_color_map[index]._func)->_C1.push_back(value.toDouble());
                        }
                    }
                }
                else if (item.startsWith("N"))
                {
                    item.remove(0, item.indexOf('N') + 1);
                    std::dynamic_pointer_cast<FunctionType2>(_color_map[index]._func)->_N = item.toDouble();
                }

                else if (item.startsWith("Bounds"))
                {
                    item.remove(0, item.indexOf('[') + 1);
                    item.remove("]");
                    for (QString &value : item.split(' '))
                    {
                        if (value.length() > 0)
                        {
                            std::dynamic_pointer_cast<FunctionType3>(_color_map[index]._func)->_bounds.push_back(value.toDouble());
                        }
                    }
                }
                else if (item.startsWith("Functions"))
                {
                    item.remove(0, item.indexOf('[') + 1);
                    item.remove("]");
                    for (QString &value : item.split(' '))
                    {
                        if (value.length() > 0 && value.toInt() > 0)
                        {
                            _color_map[index]._function_index.push_back(value.toInt());
                        }
                    }
                }
            }

            if (_color_map[index]._func->_type == 0)
            {
                const size_t streamPos = stream.find("stream", mapPos) + 7, endstreamPos = stream.find("endstream", mapPos);
                std::dynamic_pointer_cast<FunctionType0>(_color_map[index]._func)->read_samples(std::string(stream.substr(streamPos, endstreamPos - streamPos)));
            }
        
            mapPos = stream.find("FunctionType", pos) - 2;
            if (mapPos == std::string::npos - 2)
            {
                break;
            }
            while (stream[mapPos] != '<' && stream[mapPos - 1] != '<')
            {
                --mapPos;
            }
        }

        pos = stream.find(" obj", pos);
    }

    for (std::pair<const int, DeviceColor> &pair : _color_map)
    {
        if (pair.second._func->_type == 3)
        {
            for (const int index : pair.second._function_index)
            {
                std::dynamic_pointer_cast<FunctionType3>(pair.second._func)->_functions.push_back(_color_map[index]._func);
            }
        }				
    }
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

    _cur_object = 0;
    _objects.clear();

    _encoding_map.clear();
    _text.clear();
    _texts.clear();

    std::fill_n(_stroking_color, 4, 0);
    _stroking_color_space = ColorSpace::RGB;
    std::fill_n(_nonstroking_color, 4, 0);
    _nonstroking_color_space = ColorSpace::RGB;
    _cur_color_map_index = -1;
    _color_map.clear();
    _color_map_index.clear();
    _keys.clear();
}    


static Importer importer;


static Action<double> parameter_a(&importer, &Importer::store_value);
static Action<int> object_a(&importer, &Importer::store_object);
static Action<void> cm_a(&importer, &Importer::change_trans_mat);
static Action<void> CS_a(&importer, &Importer::CS);
static Action<void> cs_a(&importer, &Importer::cs);
static Action<void> SCN_a(&importer, &Importer::SCN);
static Action<void> G_a(&importer, &Importer::G);
static Action<void> g_a(&importer, &Importer::g);
static Action<void> RG_a(&importer, &Importer::RG);
static Action<void> rg_a(&importer, &Importer::rg);
static Action<void> K_a(&importer, &Importer::K);
static Action<void> k_a(&importer, &Importer::k);
static Action<void> Q_a(&importer, &Importer::pop_trans_mat);
static Action<void> q_a(&importer, &Importer::store_trans_mat);
static Action<void> m_a(&importer, &Importer::start);
static Action<void> l_a(&importer, &Importer::line);
static Action<void> c_a(&importer, &Importer::curve);
static Action<void> S_a(&importer, &Importer::store);
static Action<void> s_a(&importer, &Importer::close_and_store_shape);
static Action<void> re_a(&importer, &Importer::rect);
static Action<void> h_a(&importer, &Importer::close_shape);
static Action<void> W8_a(&importer, &Importer::close_and_store_shape);
static Action<std::string> text_a(&importer, &Importer::store_text);
static Action<std::string> encoding_a(&importer, &Importer::store_encoding);
static Action<void> end_a(&importer, &Importer::end);
static Action<void> BT_a(&importer, &Importer::BT);
static Action<void> ET_a(&importer, &Importer::ET);
static Action<void> Tm_a(&importer, &Importer::Tm);
static Action<std::string> key_a(&importer, &Importer::store_key);

static Parser<char> end = eol_p() | ch_p(' ');
static Parser<char> space = ch_p(' ');
static Parser<char> CS = str_p("CS")[CS_a] >> end;
static Parser<char> cs = str_p("cs")[cs_a] >> end;
static Parser<char> SCN = str_p("SCN")[SCN_a] >> end;
static Parser<char> G = ch_p('G')[G_a] >> end;
static Parser<char> g = ch_p('g')[g_a] >> end;
static Parser<char> RG = str_p("RG")[RG_a] >> end;
static Parser<char> rg = str_p("rg")[rg_a] >> end;
static Parser<char> K = ch_p('K')[K_a] >> end;
static Parser<char> k = ch_p('k')[k_a] >> end;
static Parser<char> cm = str_p("cm")[cm_a] >> end;
static Parser<char> Q = ch_p('Q')[Q_a] >> end;
static Parser<char> q = ch_p('q')[q_a] >> end;
static Parser<char> m = ch_p('m')[m_a] >> end;
static Parser<char> l = ch_p('l')[l_a] >> end;
static Parser<char> c = ch_p('c')[c_a] >> end;
static Parser<char> v = ch_p('v') >> end;
static Parser<char> S = ch_p('S')[S_a] >> end;
static Parser<char> s = ch_p('s')[s_a] >> end;
static Parser<char> re = str_p("re")[re_a] >> end;
static Parser<char> h = ch_p('h')[h_a] >> end;
static Parser<char> W8 = str_p("W*")[W8_a] >> end;
static Parser<char> B = ch_p('B') >> end;
static Parser<char> f = ch_p('f') >> end;

static Parser<char> BT = str_p("BT")[BT_a] >> end;
static Parser<char> ET = str_p("ET")[ET_a] >> end;
static Parser<char> Tm = str_p("Tm")[Tm_a] >> end;
static Parser<char> TL = str_p("TL") >> end;

static auto order = cm | q | Q | m | l | v | CS | cs | c | G | RG | rg | K | k | re | h |
            BT | ET | Tm | SCN | W8 | TL |
            ((ch_p('w') | ch_p('J') | ch_p('j') | ch_p('M') | ch_p('d') | str_p("ri") | ch_p('i') |
            str_p("gs") | ch_p('y') | str_p("f*") | ch_p('F') | str_p("B*") | str_p("b*") | ch_p('b') |
            ch_p('n') | ch_p('W') | str_p("Tc") | str_p("Tw") | str_p("Tz") | str_p("Tf") | str_p("Tr") |
            str_p("Ts") | str_p("Td") | str_p("TD") | str_p("T*") | str_p("Tj") | str_p("TJ") | ch_p('\'') |
            ch_p('\"') | str_p("d0") | str_p("d1") | str_p("SC") | str_p("scn") | str_p("sc") | str_p("sh") |
            str_p("BI") | str_p("ID") | str_p("EI") | str_p("Do") | str_p("MP") | str_p("DP") |
            str_p("BMC") | str_p("BDC") | str_p("EMC") | str_p("BX") | str_p("EX")) >> end) |
            S | s | B | f | g ;

static Parser<double> parameter = float_p()[parameter_a];
static Parser<std::vector<char>> key = confix_p(ch_p('/'), (*alnum_p())[key_a], end);
static Parser<std::vector<char>> text = confix_p(ch_p('<'), (*alnum_p())[text_a], ch_p('>'));
static Parser<std::vector<char>> annotation = pair(ch_p('['), eol_p());
static auto command = *(parameter | key | text | space) >> order;
static auto array = pair(ch_p('['), ch_p(']')) >> !eol_p();
static auto dict = pair(str_p("<<"), str_p(">>")) >> !eol_p();
static Parser<std::vector<char>> code = confix_p(ch_p('<'), (+alnum_p())[encoding_a], ch_p('>'));
static auto font_info = str_p("/CIDInit /ProcSet findresource begin") >> (+anychar_p() - (str_p("beginbfchar") >> eol_p()))
    	                >> str_p("beginbfchar") >> end >> +(code >> end) >> !eol_p() >> str_p("endbfchar")
                        >> end >> (+anychar_p() - (str_p("end") >> eol_p())) >> +(str_p("end") >> eol_p());
static Parser<std::vector<char>> xml = pair(str_p("<?xpacket"), str_p("<?xpacket end=\"w\"?>")) >> eol_p();
static Parser<std::vector<char>> others = (+anychar_p() - str_p("endstream"));

static Parser<char> stream_start = str_p("stream") >> eol_p();
static Parser<char>	stream_end = str_p("endstream") >> eol_p();
static auto stream = stream_start >> +(command | dict | float_p() | space | eol_p() | font_info | xml | others) >> stream_end;
static auto object = (int_p()[object_a] >> space >> int_p() >> space >> str_p("obj") >> eol_p() >>
					*(int_p() | dict | stream | array) >> str_p("endobj") >> eol_p());

static auto head = str_p("%PDF-") >> float_p() >> (+anychar_p() - ch_p('%')) >> ch_p('%') >> (*anychar_p() - eol_p()) >> eol_p();
static Parser<std::vector<char>> tail = (str_p("xref") >> *anychar_p())[end_a];
static auto pdf = head >> *(annotation | object | eol_p()) >> tail;



bool parse(std::string_view &stream, Graph *graph)
{
    importer.reset();
    importer.load_graph(graph);
    importer.get_color_map_index(stream);
    importer.get_color_map(stream);
    return pdf(stream).has_value();
}

bool parse(std::ifstream &stream, Graph *graph)
{
    importer.reset();
    importer.load_graph(graph);
    std::stringstream sstream;
    sstream << stream.rdbuf();
    std::string str(sstream.str());
    std::string_view temp(str);
    importer.get_color_map_index(temp);
    importer.get_color_map(temp);
    return pdf(temp).has_value();
}

}