#include "SHXReader.hpp"
#include "base/Algorithm.hpp"


namespace SHXReader
{

void SHXShape::update_bbox()
{
    bbox.min_x = bbox.min_y = DBL_MAX;
    bbox.max_x = bbox.max_y = -DBL_MAX;
    for (const Geo::Polyline &polyline : polylines)
    {
        for (const Geo::Point &point : polyline)
        {
            bbox.max_x = std::max(bbox.max_x, point.x);
            bbox.max_y = std::max(bbox.max_y, point.y);
            bbox.min_x = std::min(bbox.min_x, point.x);
            bbox.min_y = std::min(bbox.min_y, point.y);
        }
    }
}

SHXShape SHXShape::operator+(const Geo::Point &point) const
{
    SHXShape shape;
    shape.last_point = this->last_point + point;
    shape.bbox.max_x = this->bbox.max_x + point.x;
    shape.bbox.max_y = this->bbox.max_y + point.y;
    shape.bbox.min_x = this->bbox.min_x + point.x;
    shape.bbox.min_y = this->bbox.min_y + point.y;
    for (const Geo::Polyline &polyline : this->polylines)
    {
        shape.polylines.emplace_back(polyline + point);
    }
    return shape;
}

void SHXShape::operator+=(const Geo::Point &point)
{
    this->last_point += point;
    this->bbox.max_x += point.x;
    this->bbox.max_y += point.y;
    this->bbox.min_x += point.x;
    this->bbox.min_y += point.y;
    for (Geo::Polyline &polyline : this->polylines)
    {
        polyline += point;
    }
}

SHXShape SHXShape::normalize_to_origin() const
{
    return operator+(Geo::Point(-bbox.min_x, -bbox.min_y));
}

SHXShape &SHXShape::normalized_to_origin()
{
    this->operator+=(Geo::Point(-bbox.min_x, -bbox.min_y));
    return *this;
}


int SHXFileReader::byte_to_sbyte(const int value)
{
    return (value & 127) - (value & 128 ? 128 : 0);
}

SHXFileReader::SHXFileReader(std::ifstream *stream)
    : _stream(stream)
{
    _stream->seekg(0, std::ios::end);
    _length = _stream->tellg();
    _stream->clear();
    _stream->seekg(0, std::ios::beg);
}

std::vector<uchar> SHXFileReader::read_bytes(const int length)
{
    std::vector<uchar> data(length);
    _stream->read(reinterpret_cast<char *>(data.data()), length);
    return data;
}

void SHXFileReader::skip(const int length)
{
    char *data = new char[length];
    _stream->read(data, length);
    delete []data;
}

uint8_t SHXFileReader::read_uint8()
{
    return _stream->get();
}

int8_t SHXFileReader::read_int8()
{
    return _stream->get();
}

uint16_t SHXFileReader::read_uint16()
{
    char data[2] = {0, 0};
    _stream->read(data, 2);
    return *reinterpret_cast<uint16_t *>(data);
}

int16_t SHXFileReader::read_int16()
{
    char data[2] = {0, 0};
    _stream->read(data, 2);
    return *reinterpret_cast<int16_t *>(data);
}

uint32_t SHXFileReader::read_uint32()
{
    char data[4] = {0, 0, 0, 0};
    _stream->read(data, 4);
    return *reinterpret_cast<uint32_t *>(data);
}

int32_t SHXFileReader::read_int32()
{
    char data[4] = {0, 0, 0, 0};
    _stream->read(data, 4);
    return *reinterpret_cast<int32_t *>(data);
}

float SHXFileReader::read_float32()
{
    char data[4] = {0, 0, 0, 0};
    _stream->read(data, 4);
    return *reinterpret_cast<float *>(data);
}

double SHXFileReader::read_float64()
{
    char data[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    _stream->read(data, 8);
    return *reinterpret_cast<double *>(data);
}

void SHXFileReader::set_position(const int pos)
{
    _stream->seekg(pos);
}

bool SHXFileReader::is_end() const
{
    return _stream->tellg() == _length;
}

int SHXFileReader::position() const
{
    return _stream->tellg();
}

int SHXFileReader::length() const
{
    return _length;
}


SHXHeaderParser::SHXHeaderParser(SHXFileReader &reader)
{
    std::string header = parse(reader);
    std::vector<std::string> heads;
    {
        size_t i = 0;
        for (size_t j = 1, count = header.size(); j < count; ++j)
        {
            if (header[j] == ' ')
            {
                heads.emplace_back(header.substr(i, j - i));
                i = j + 1;
            }
        }
        heads.emplace_back(header.substr(i, header.size() - i));
    }
    data.header = heads[0];
    if (heads[1].front() == 'U' || heads[1].front() == 'u')
    {
        data.type = SHXFontHeaderData::SHXFontType::UNIFONT;
    }
    else if (heads[1].front() == 'B' || heads[1].front() == 'b')
    {
        data.type = SHXFontHeaderData::SHXFontType::BIGFONT;
    }
    else
    {
        data.type = SHXFontHeaderData::SHXFontType::SHAPES;
    }
    data.version = heads[2];
}

std::string SHXHeaderParser::parse(SHXFileReader &reader)
{
    std::string result;
    int max_length = 1024;
    int header_length = 0;
    while (reader.position() < reader.length() - 2 && header_length < max_length)
    {
        const uint8_t byte1 = reader.read_uint8();
        if (byte1 == 0x0d)
        {
            // Peek next two bytes without advancing the position
            const int pos = reader.position();
            const uint8_t byte2 = reader.read_uint8();
            const uint8_t byte3 = reader.read_uint8();

            if (byte2 == 0x0a && byte3 == 0x1a)
            {
                break;
            }

            // If sequence doesn't match, reset position and add the first byte to result
            reader.set_position(pos);
            result.push_back(byte1);
        }
        else
        {
            result.push_back(byte1);
        }
        ++header_length;
    }
    while (result.back() == ' ')
    {
        result.pop_back();
    }
    while (result.front() == ' ')
    {
        result.erase(result.begin());
    }
    return result;
}


SHXShapeParser::SHXShapeParser(SHXFontData &font_data)
    : _font_data(font_data)
{}

SHXShape SHXShapeParser::char_shape(const int code, const int size)
{
    const double scale = size / _font_data.content.height;
    return parse_and_scale(code, { scale, 0, 0 });
}

SHXShape SHXShapeParser::parse_and_scale(const int code, const ScalingOptions &options)
{
    if (code == 0)
    {
        SHXShape();
    }

    SHXShape shape;
    if (_shape_cache.find(code) == _shape_cache.end())
    {
        std::map<int, std::vector<uchar>> &codes = _font_data.content.data;
        if (codes.find(code) != codes.end())
        {
            const std::vector<uchar> &data = codes[code];
            shape = parse_shape(data);
            _shape_data.insert_or_assign(code, shape);
            _shape_cache.insert_or_assign(code, shape);
        }
    }
    else
    {
        shape = _shape_cache[code];
    }

    if (shape.last_point.x == DBL_MAX && shape.last_point.y == DBL_MAX
        && shape.polylines.empty())
    {
        return shape;
    }

    if (options.factor != 0)
    {
        scale(shape, options.factor);
    }
    else if (options.height > 0)
    {
        const double width = options.width > 0 ? options.width : options.height;
        scale(shape, options.height, width);
    }
    return shape;
}

void SHXShapeParser::scale(SHXShape &shape, const double factor)
{
    if (shape.last_point.x < DBL_MAX && shape.last_point.y < DBL_MAX)
    {
        shape.last_point *= factor;
    }
    for (Geo::Polyline &polyline : shape.polylines)
    {
        for (Geo::Point &point : polyline)
        {
            point *= factor;
        }
    }
}

void SHXShapeParser::scale(SHXShape &shape, const double height, const double width)
{
    shape.update_bbox();
    const Box2D box = shape.bbox;
    const double shape_height = box.max_y - box.min_y;
    const double shape_width = box.max_x - box.min_x;
    const double height_scale = shape_height > 0 ? height / shape_height : 1;
    const double width_scale = shape_width > 0 ? width / shape_width : 1;
    if (shape.last_point.x < DBL_MAX && shape.last_point.y < DBL_MAX)
    {
        shape.last_point.x *= width_scale;
        shape.last_point.y *= height_scale;
    }
    for (Geo::Polyline &polyline : shape.polylines)
    {
        for (Geo::Point &point : polyline)
        {
            point.x *= width_scale;
            point.y *= height_scale;
        }
    }
}

SHXShape SHXShapeParser::parse_shape(const std::vector<uchar> &data)
{
    State state;
    for (int i = 0, count = data.size(); i < count; ++i)
    {
        if (uchar cb = data[i]; cb <= 0x0f)
        {
            i = special_cmd(cb, data, i, state);
        }
        else
        {
            vector_cmd(cb, state);
        }
    }
    SHXShape shape;
    shape.last_point = state.current_point.x < DBL_MAX && state.current_point.y < DBL_MAX ? state.current_point : Geo::Point(0, 0);
    shape.polylines = state.polylines;
    return shape;
}

int SHXShapeParser::special_cmd(const int cmd, const std::vector<uchar> &data, const int index, State &state)
{
    int i = index;
    switch (cmd)
    {
    case 0: // End of shape definition
        state.current_polyline.clear();
        state.pen_down = false;
        break;
    case 1: // Activate Draw mode (pen down)
        state.pen_down = true;
        if (state.current_point.x < DBL_MAX && state.current_point.y < DBL_MAX)
        {
            state.current_polyline.append(state.current_point);
        }
        else
        {
            state.current_polyline.append(Geo::Point(0, 0));
        }
        break;
    case 2: // Deactivate Draw mode (pen up)
        state.pen_down = false;
        if (state.current_polyline.size() > 1)
        {
            state.polylines.emplace_back(state.current_polyline);
        }
        state.current_polyline.clear();
        break;
    case 3: // Divide vector lengths
        state.scale /= data[++i];
        break;
    case 4: // Multiply vector lengths
        state.scale *= data[++i];
        break;
    case 5: // Push current location
        if (state.points.size() != 4)
        {
            if (state.current_point.x < DBL_MAX && state.current_point.y < DBL_MAX)
            {
                state.points.emplace_back(state.current_point);
            }
            else
            {
                state.points.emplace_back(0, 0);
            }
        }
        break;
    case 6: // Pop current location
        if (!state.points.empty())
        {
            state.current_point = state.points.back();
            state.points.pop_back();
        }
        break;
    case 7: // Draw subshape
        i = subshape_cmd(data, i, state);
        break;
    case 8: // X-Y displacement
        i = xy_displacement(data, i, state);
        break;
    case 9: // Multiple X-Y displacements
        i = multiple_xy_displacement(data, i, state);
        break;
    case 10: // Octant arc
        i = octant_arc(data, i, state);
        break;
    case 11: // Fractional arc
        i = fractional_arc(data, i, state);
        break;
    case 12: // Arc with bulge
        i = bulge_arc(data, i, state);
        break;
    case 13: // Multiple bulge arcs
        i = multiple_bulge_arcs(data, i, state);
        break;
    case 14: // Vertical text
        i = skip_code(data, ++i);
        break;
    default:
        break;
    }
    return i;
}

void SHXShapeParser::vector_cmd(const int cmd, State &state)
{
    const int len = (cmd & 0xf0) >> 4;
    const int dir = cmd & 0x0f;
    const Geo::Point vec = vector_direction(dir) * len * state.scale;
    if (state.current_point.x < DBL_MAX && state.current_point.y < DBL_MAX)
    {
        state.current_point += vec;
    }
    else
    {
        state.current_point = vec;
    }
    if (state.pen_down)
    {
        state.current_polyline.append(state.current_point);
    }
}

Geo::Point SHXShapeParser::vector_direction(const int dir)
{
    Geo::Point vec;
    switch (dir)
    {
    case 0:
        vec.x = 1;
        break;
    case 1:
        vec.x = 1;
        vec.y = 0.5;
        break;
    case 2:
        vec.x = 1;
        vec.y = 1;
        break;
    case 3:
        vec.x = 0.5;
        vec.y = 1;
        break;
    case 4:
        vec.y = 1;
        break;
    case 5:
        vec.x = -0.5;
        vec.y = 1;
        break;
    case 6:
        vec.x = -1;
        vec.y = 1;
        break;
    case 7:
        vec.x = -1;
        vec.y = 0.5;
        break;
    case 8:
        vec.x = -1;
        break;
    case 9:
        vec.x = -1;
        vec.y = -0.5;
        break;
    case 10:
        vec.x = -1;
        vec.y = -1;
        break;
    case 11:
        vec.x = -0.5;
        vec.y = -1;
        break;
    case 12:
        vec.y = -1;
        break;
    case 13:
        vec.x = 0.5;
        vec.y = -1;
        break;
    case 14:
        vec.x = 1;
        vec.y = -1;
        break;
    case 15:
        vec.x = 1;
        vec.y = -0.5;
        break;
    default:
        break;
    }
    return vec;
}

int SHXShapeParser::subshape_cmd(const std::vector<uchar> &data, const int index, State &state)
{
    int i = index, subcode = 0;
    double height = state.scale * _font_data.content.height;
    double width = height;
    Geo::Point origin(state.current_point.x < DBL_MAX && state.current_point.y < DBL_MAX ? state.current_point : Geo::Point(0, 0));

    if (state.current_polyline.size() > 1)
    {
        state.polylines.emplace_back(state.current_polyline);
        state.current_polyline.clear();
    }

    switch (_font_data.header.type)
    {
    case SHXFontHeaderData::SHXFontType::SHAPES:
        subcode = data[++i];
        break;
    case SHXFontHeaderData::SHXFontType::BIGFONT:
        subcode = data[++i];
        if (subcode == 0)
        {
            ++i;
            subcode = (data[i] << 8) | data[i + 1];
            i += 2;
            origin.x = SHXFileReader::byte_to_sbyte(data[i++]) * state.scale;
            origin.y = SHXFileReader::byte_to_sbyte(data[i++]) * state.scale;
            if (_font_data.content.extended)
            {
                width = data[i++] * state.scale;
                height = data[i] * state.scale;
            }
            else
            {
                height = data[i] * state.scale;
            }
        }
        break;
    case SHXFontHeaderData::SHXFontType::UNIFONT:
        ++i;
        subcode = (data[i] << 8) | data[i + 1];
        i += 2;
        break;
    }

    if (subcode != 0)
    {
        if (SHXShape shape(scale_subshape_at_insert_point(subcode, width, height, origin));
            (shape.last_point.x < DBL_MAX && shape.last_point.y < DBL_MAX) || !shape.polylines.empty())
        {
            state.polylines.insert(state.polylines.end(), shape.polylines.begin(), shape.polylines.end());
        }
    }
    state.current_polyline.clear();
    return i;
}

int SHXShapeParser::xy_displacement(const std::vector<uchar> &data, const int index, State &state)
{
    int i = index;
    Geo::Point vec;
    vec.x = SHXFileReader::byte_to_sbyte(data[++i]) * state.scale;
    vec.y = SHXFileReader::byte_to_sbyte(data[++i]) * state.scale;
    if (state.current_point.x < DBL_MAX && state.current_point.y < DBL_MAX)
    {
        state.current_point += vec;
    }
    else
    {
        state.current_point = vec;
    }
    if (state.pen_down)
    {
        state.current_polyline.append(state.current_point);
    }
    return i;
}

int SHXShapeParser::multiple_xy_displacement(const std::vector<uchar> &data, const int index, State &state)
{
    int i = index;
    while (true)
    {
        Geo::Point vec;
        vec.x = SHXFileReader::byte_to_sbyte(data[++i]);
        vec.y = SHXFileReader::byte_to_sbyte(data[++i]);
        if (vec.x == 0 && vec.y == 0)
        {
            break;
        }
        vec *= state.scale;
        if (state.current_point.x < DBL_MAX && state.current_point.y < DBL_MAX)
        {
            state.current_point += vec;
        }
        else
        {
            state.current_point = vec;
        }
        if (state.pen_down)
        {
            state.current_polyline.append(state.current_point);
        }
    }
    return i;
}

int SHXShapeParser::octant_arc(const std::vector<uchar> &data, const int index, State &state)
{
    int i = index;
    const double radius = data[++i] * state.scale;
    const int flag = SHXFileReader::byte_to_sbyte(data[++i]);
    const int start = (flag & 0x70) >> 4;
    const int count = flag & 0x07;
    const bool is_counterclockwise = flag > 0;
    const double start_radian = (Geo::PI / 4) * start;
    const Geo::Point center(state.current_point.x < DBL_MAX && state.current_point.y < DBL_MAX ?
        state.current_point - Geo::Point(std::cos(start_radian) * radius, std::sin(start_radian) * radius)
        : Geo::Point(-std::cos(start_radian) * radius, -std::sin(start_radian) * radius));
    if (0 < count && count < 8)
    {
        const double angle = is_counterclockwise ? Geo::PI / 4 * count : -Geo::PI / 4 * count;
        const Geo::Arc arc(state.current_point.x < DBL_MAX && state.current_point.y < DBL_MAX ?
            state.current_point : Geo::Point(0, 0), center, angle, Geo::Arc::ParameterType::StartCenterAngle, is_counterclockwise);
        if (state.pen_down)
        {
            state.current_polyline.append(arc.control_points[0]);
            state.current_polyline.append(Geo::arc_to_polyline(arc));
            state.current_polyline.append(arc.control_points[2]);
        }
        if (!arc.shape().empty())
        {
            state.current_point = arc.shape().back();
        }
    }
    else
    {
        if (state.pen_down)
        {
            const double r = Geo::distance(center, state.current_point.x < DBL_MAX &&
                state.current_point.y < DBL_MAX ? state.current_point : Geo::Point(0, 0));
            const double v = std::asin(1 / r);
            const double step = std::isnan(v) ? Geo::PI / 32 : std::min(v, Geo::PI / 64);
            double degree = start_radian;
            const double end = start_radian + Geo::PI * 2;
            std::vector<Geo::Point> points;
            while (degree < end)
            {
                points.emplace_back(r * std::cos(degree) + center.x, r * std::sin(degree) + center.y);
                degree += step;
            }
            if (points.size() >= 3)
            {
                Geo::Polygon shape(points.cbegin(), points.cend());
                if (step < Geo::PI / 32)
                {
                    Geo::down_sampling(shape, 0.02);
                }
            }
            if (points.front() != points.back())
            {
                points.emplace_back(points.front());
            }
            state.current_polyline.append(points.begin(), points.end());
        }
    }
    return i;
}

int SHXShapeParser::fractional_arc(const std::vector<uchar> &data, const int index, State &state)
{
    int i = index;
    const int start_offset = data[++i];
    const int end_offset = data[++i];
    const int hr = data[++i];
    const int lr = data[++i];
    const int r = (hr * 255 + lr) * state.scale;
    const int flag = SHXFileReader::byte_to_sbyte(data[++i]);
    const int n1 = (flag & 0x70) >> 4;
    int n2 = flag & 0x07;
    if (n2 == 0)
    {
        n2 = 8;
    }
    if (end_offset != 0)
    {
        --n2;
    }

    const double pi_4 = Geo::PI / 4;
    double span = pi_4 * n2, delta = Geo::PI / 18;
    int sign = 1;
    if (flag < 0)
    {
        delta = -delta;
        span = -span;
        sign = -1;
    }

    double start_radian = pi_4 * n1;
    double end_radian = start_radian + span;
    start_radian += ((pi_4 * start_offset) / 256) * sign;
    end_radian += ((pi_4 * end_offset) / 256) * sign;
    const Geo::Point center(state.current_point.x < DBL_MAX && state.current_point.y < DBL_MAX ?
        state.current_point - Geo::Point(r * std::cos(start_radian), r * std::sin(start_radian))
        : Geo::Point(-r * std::cos(start_radian), -r * std::sin(start_radian)));
    state.current_point = center + Geo::Point(r * std::cos(end_radian), r * std::sin(end_radian));
    if (state.pen_down)
    {
        double current_radian = start_radian;
        std::vector<Geo::Point> points;
        points.emplace_back(r * std::cos(current_radian), r * std::sin(current_radian));
        if (delta > 0)
        {
            while (current_radian + delta < end_radian)
            {
                current_radian += delta;
                points.emplace_back(r * std::cos(current_radian), r * std::sin(current_radian));
            }
        }
        else
        {
            while (current_radian + delta > end_radian)
            {
                current_radian += delta;
                points.emplace_back(r * std::cos(current_radian), r * std::sin(current_radian));
            }
        }

        points.emplace_back(center + Geo::Point(r * std::cos(end_radian), r * std::sin(end_radian)));
        state.current_polyline.append(points.begin(), points.end());
    }
    return i;
}

int SHXShapeParser::bulge_arc(const std::vector<uchar> &data, const int index, State &state)
{
    int i = index;
    Geo::Point vec;
    vec.x = SHXFileReader::byte_to_sbyte(data[++i]);
    vec.y = SHXFileReader::byte_to_sbyte(data[++i]);
    const int bulge = SHXFileReader::byte_to_sbyte(data[++i]);
    state.current_point = arc_segment(state.current_point.x < DBL_MAX && state.current_point.y < DBL_MAX ?
        state.current_point : Geo::Point(0, 0), vec, bulge, state.scale, state.pen_down, state.current_polyline);
    return i;
}

int SHXShapeParser::multiple_bulge_arcs(const std::vector<uchar> &data, const int index, State &state)
{
    int i = index;
    while (true)
    {
        Geo::Point vec;
        vec.x = SHXFileReader::byte_to_sbyte(data[++i]);
        vec.y = SHXFileReader::byte_to_sbyte(data[++i]);
        if (vec.x == 0 && vec.y == 0)
        {
            break;
        }
        const int bulge = SHXFileReader::byte_to_sbyte(data[++i]);
        state.current_point = arc_segment(state.current_point.x < DBL_MAX && state.current_point.y < DBL_MAX ?
            state.current_point : Geo::Point(0, 0), vec, bulge, state.scale, state.pen_down, state.current_polyline);
    }
    return i;
}

int SHXShapeParser::skip_code(const std::vector<uchar> &data, int index)
{
    const int cb = data[index];
    switch (cb) 
    {
    case 0x00:
        break;
    case 0x01:
        break;
    case 0x02:
        break;
    case 0x03:
    case 0x04:
        ++index;
        break;
    case 0x05:
        break;
    case 0x06:
        break;
    case 0x07:
        switch (_font_data.header.type)
        {
        case SHXFontHeaderData::SHXFontType::SHAPES:
            ++index;
            break;
        case SHXFontHeaderData::SHXFontType::BIGFONT:
            if (const int code = data[++index]; code == 0)
            {
                // Skip primitive#, basepoint-x, basepoint-y, width/height
                // primitive# is 2 bytes, basepoint-x 1, basepoint-y 1,
                // extended fonts have both width and height (2 bytes),
                // non-extended have only height (1 byte)
                index += _font_data.content.extended ? 6 : 5;
            }
            break;
          case SHXFontHeaderData::SHXFontType::UNIFONT:
            index += 2;
            break;
        }
        break;
    case 0x08:
        index += 2;
        break;
    case 0x09:
        while (true)
        {
            const double x = data[++index];
            const double y = data[++index];
            if (x == 0 && y == 0)
            {
                break;
            }
        }
        break;
    case 0x0a:
        index += 2;
        break;
    case 0x0b:
        index += 5;
        break;
    case 0x0c:
        index += 3;
        break;
    case 0x0d:
        while (true)
        {
            const double x = data[++index];
            const double y = data[++index];
            if (x == 0 && y == 0)
            {
                break;
            }
            ++index;
        }
        break;
    case 0x0e:
        break;
    default:
        break;
    }
    return index;
}

SHXShape SHXShapeParser::scale_subshape_at_insert_point(const int code, const double width, const double height, const Geo::Point &point)
{
    SHXShape shape = (_shape_cache.find(code) == _shape_cache.end()) ? SHXShape() : _shape_cache[code];
    if ((shape.last_point.x < DBL_MAX && shape.last_point.y < DBL_MAX) || !shape.polylines.empty())
    {
        if (_font_data.content.data.find(code) == _font_data.content.data.end())
        {
            return shape;
        }
        shape = parse_shape(_font_data.content.data[code]);
        _shape_cache.insert_or_assign(code, shape);
        _shape_data.insert_or_assign(code, shape);
    }

    shape.normalized_to_origin();
    scale(shape, height, width);
    shape += point;
    return shape;
}

Geo::Point SHXShapeParser::arc_segment(const Geo::Point &point, Geo::Point vec, double bulge, const double scale, const bool pen_down, Geo::Polyline &polyline)
{
    // Apply scale to vector
    vec.x *= scale;
    vec.y *= scale;
    // Clamp bulge value
    if (bulge < -127)
    {
        bulge = -127;
    }
    // Update current point position
    Geo::Point temp(point);
    if (pen_down)
    {
        if (bulge == 0)
        {
            polyline.append(temp + vec);
        }
        else
        {
            const Geo::Arc arc(temp, temp + vec, std::max(std::min(bulge / 127, 1.0), -1.0));
            Geo::Polyline points(Geo::arc_to_polyline(arc));
            polyline.append(arc.control_points[0]);
            polyline.append(points.begin(), points.end());
            polyline.append(arc.control_points[2]);
        }
    }
    temp += vec;
    return temp;
}


SHXFontContentData SHXShapeContentParser::parse(SHXFileReader &reader)
{
    SHXFontContentData result;
    reader.read_bytes(4);
    const int count = reader.read_int16();
    if (count <= 0)
    {
        return result;
    }

    std::vector<std::tuple<int, int>> items; // (code, length)
    for (int i = 0; i < count; ++i)
    {
        if (int code = reader.read_uint16(), length = reader.read_uint16(); length > 0)
        {
            // Only add valid entries
            items.emplace_back(code, length);
        }
    }

    for (const auto &[code, length] : items)
    {
        const std::vector<uchar> bytes = reader.read_bytes(length);
        if (bytes.size() == length) 
        {
            // Parse and skip the null-terminated label at the beginning of the data
            const std::vector<uchar>::const_iterator nulit = std::find(bytes.begin(), bytes.end(), 0x00);
            int startOfBytecode = 0;
            // Handle the null-terminated label header
            if (nulit != bytes.cend())
            {
                if (int index = std::distance(bytes.cbegin(), nulit) + 1; index < bytes.size())
                {
                    // Only add if we got all the bytes and there's actual bytecode data
                    result.data.insert_or_assign(code, std::vector<uchar>(bytes.begin() + index, bytes.end()));
                }
            }
        }
    }

    if (result.data.find(0) != result.data.end())
    {
        const std::vector<uint8_t> infodata = result.data[0];
        const std::string str(infodata.begin(), infodata.end());
        if (size_t index = str.find('\x00'); index != std::string::npos)
        {
            result.info = str.substr(0, index);
            if (index + 3 < infodata.size())
            {
                result.base_up = infodata[index + 1];
                result.base_down = infodata[index + 2];
                result.height = result.base_down + result.base_up;
                result.width = result.height;
                result.horizontal = infodata[index + 3] == 0;
            }
        }
    }
    return result;
}

SHXFontContentData SHXBigfontContentParser::parse(SHXFileReader &reader)
{
    SHXFontContentData result;
    // const int item_length = reader.read_int16();
    reader.read_int16();
    const int count = reader.read_int16();
    const int change_number = reader.read_int16();
    if (count <= 0)
    {
        return result;
    }
    // Skip change table
    reader.skip(change_number * 4);

    std::vector<std::tuple<int, int, unsigned int>> items; // (code, length, offset)
    for (int i = 0; i < count; ++i)
    {
        const int code = reader.read_uint16();
        const int length = reader.read_uint16();
        const unsigned int offset = reader.read_uint32();
        if (code != 0 || length != 0 || offset != 0)
        {
            items.emplace_back(code, length, offset);
        }
    }

    for (const auto &[code, length, offset] : items)
    {
        reader.set_position(offset);
        if (const std::vector<uint8_t> bytes = reader.read_bytes(length); length == bytes.size())
        {
            result.data.insert_or_assign(code, bytes);
        }
    }

    if (result.data.find(0) != result.data.end())
    {
        const std::vector<uint8_t> infodata = result.data[0];
        auto [str, pos] = utf8_array_to_str(infodata);
        if (int index = pos; index >= 0)
        {
            result.info = str;
            if (++index + 3 < infodata.size())
            {
                if (infodata.size() - index > 4)
                {
                    result.height = infodata[index++];
                    ++index;
                    result.horizontal = infodata[index++] == 0;
                    result.width = infodata[index++];
                    result.base_up = result.height;
                    result.base_down = 0;
                    result.extended = true;
                }
                else
                {
                    result.base_up = infodata[index++];
                    result.base_down = infodata[index++];
                    result.height = result.base_down + result.base_up;
                    result.width = result.height;
                    result.horizontal = infodata[index] == 0;
                }
            }
        }
    }
    return result;
}

std::tuple<std::string, int> SHXBigfontContentParser::utf8_array_to_str(const std::vector<uint8_t> &array)
{
    std::string out;
    int i = 0;
    while (i < array.size())
    {
        const int c = array[i];
        switch (c >> 4)
        {
        case 0:
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
            out.push_back(c);
            break;
        case 12:
        case 13:
            out.push_back(((c & 0x1f) << 6) | (array[i++] & 0x3f));
            break;
        case 14:
            {
                const int c2 = array[i++];
                const int c3 = array[i++];
                out.push_back(((c & 0x0f) << 12) | ((c2 & 0x3f) << 6) | ((c3 & 0x3f) << 0));
            }
            break;
        default:
            break;
        }
        if (out.back() == 0)
        {
            break;
        }
        ++i;
    }
    return std::make_tuple(out, i);
}

SHXFontContentData SHXUnifontContentParser::parse(SHXFileReader &reader)
{
    SHXFontContentData result;
    const int count = reader.read_int32();
    if (count <= 0)
    {
        return result;
    }

    const int info_length = reader.read_int16();
    const std::vector<uint8_t> infodata = reader.read_bytes(info_length);

    std::string info(infodata.begin(), infodata.end());
    if (size_t index = info.find('\x00'); index != std::string::npos)
    {
        result.info = info.substr(0, index);
        if (index + 3 < infodata.size())
        {
            result.base_up = infodata[++index];
            result.base_down = infodata[++index];
            result.height = result.base_down + result.base_up;
            result.width = result.height;
            result.horizontal = infodata[++index] == 0;
        }
    }

    for (int i = 0; i < count - 1; ++i)
    {
        const int code = reader.read_uint16();
        const int length = reader.read_uint16();
        if (length > 0)
        {
            const std::vector<uint8_t> bytes = reader.read_bytes(length);
            if (length == bytes.size())
            {
                if (const std::vector<uint8_t>::const_iterator it = std::find(bytes.cbegin(), bytes.cend(), 0x00);
                    it != bytes.cend())
                {
                    if (int index = std::distance(bytes.cbegin(), it) + 1; index < bytes.size())
                    {
                        result.data.insert_or_assign(code, std::vector<uint8_t>(bytes.begin() + index, bytes.end()));
                    }
                }
            }
        }
    }
    return result;
}

SHXContentParser *SHXContentParserFactory::parser(const SHXFontHeaderData::SHXFontType type)
{
    switch (type)
    {
    case SHXFontHeaderData::SHXFontType::SHAPES:
        return new SHXShapeContentParser();
    case SHXFontHeaderData::SHXFontType::BIGFONT:
        return new SHXBigfontContentParser();
    case SHXFontHeaderData::SHXFontType::UNIFONT:
        return new SHXUnifontContentParser();
    default:
        return nullptr;
    }
}


SHXFont::SHXFont(std::ifstream *stream)
    : _reader(stream), _shapeparser(fontdata)
{
    SHXHeaderParser header_parser(_reader);
    fontdata.header = header_parser.data;
    SHXContentParser *content_parser = SHXContentParserFactory::parser(header_parser.data.type);
    fontdata.content = content_parser->parse(_reader);
    delete content_parser;
}

bool SHXFont::has_char(const int code) const
{
    return fontdata.content.data.find(code) != fontdata.content.data.end();
}

SHXShape SHXFont::char_shape(const int code, const int size)
{
    SHXShape shape = _shapeparser.char_shape(code, size);
    if ((shape.last_point.x < DBL_MAX && shape.last_point.y < DBL_MAX) || !shape.polylines.empty())
    {
        if (fontdata.header.type == SHXFontHeaderData::SHXFontType::BIGFONT)
        {
            shape.normalized_to_origin();
        }
    }
    return shape;
}

}