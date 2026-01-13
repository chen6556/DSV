#pragma once
#include <map>
#include <tuple>
#include <string>
#include <fstream>
#include "base/Geometry.hpp"

namespace SHXReader
{

struct Box2D
{
    double min_x = 0;
    double min_y = 0;
    double max_x = 0;
    double max_y = 0;
};

class SHXShape
{
public:
    Geo::Point last_point = Geo::Point(DBL_MAX, DBL_MAX);
    std::vector<Geo::Polyline> polylines;
    Box2D bbox;

    SHXShape() = default;

    void update_bbox();

    SHXShape operator+(const Geo::Point &podouble) const;

    void operator+=(const Geo::Point &podouble);

    SHXShape normalize_to_origin() const;

    SHXShape &normalized_to_origin();
};


struct SHXFontContentData
{
    bool horizontal = true;
    bool extended = false;
    double base_up = 8;
    double base_down = 2;
    double height = 10;
    double width = 10;
    std::string info;
    std::map<int, std::vector<uint8_t>> data;
};

struct SHXFontHeaderData
{
    // The type of font (shapes, bigfont, or unifont)
    enum class SHXFontType {SHAPES, BIGFONT, UNIFONT} type = SHXFontType::BIGFONT;
    // Header information from the font file
    std::string header;
    // Version information of the font file
    std::string version;
};

struct SHXFontData
{
    SHXFontHeaderData header;
    SHXFontContentData content;
};


class SHXFileReader
{
private:
    std::ifstream *_stream = nullptr;
    int _length = 0;

public:
    static int byte_to_sbyte(const int value);

    SHXFileReader(std::ifstream *stream);

    std::vector<uchar> read_bytes(const int length = 1);

    void skip(const int length);

    uint8_t read_uint8();

    int8_t read_int8();

    uint16_t read_uint16();

    int16_t read_int16();

    uint32_t read_uint32();

    int32_t read_int32();

    float read_float32();

    double read_float64();

    void set_position(const int pos);

    bool is_end() const;

    int position() const;

    int length() const;
};


class SHXHeaderParser
{
public:
    SHXFontHeaderData data;

    SHXHeaderParser(SHXFileReader &reader);

    std::string parse(SHXFileReader &reader);
};


struct ScalingOptions
{
    // Scale by a uniform factor
    double factor = 0;
    // Scale by specific height and width
    double height = 0;
    double width = 0;
};

class SHXShapeParser
{
private:
    SHXFontData &_font_data;
    std::map<int, SHXShape> _shape_cache, _shape_data;

public:
    struct State
    {
        bool pen_down = false;
        double scale = 1;
        Geo::Point current_point = Geo::Point(DBL_MAX, DBL_MAX);
        std::vector<Geo::Polyline> polylines;
        Geo::Polyline current_polyline;
        std::vector<Geo::Point> points;
    };

public:
    SHXShapeParser(SHXFontData &font_data);

    SHXShape char_shape(const int code, const int size);

private:
    SHXShape parse_and_scale(const int code, const ScalingOptions &options);

    static void scale(SHXShape &shape, const double factor);

    static void scale(SHXShape &shape, const double height, const double width);

    SHXShape parse_shape(const std::vector<uchar> &data);

    int special_cmd(const int cmd, const std::vector<uchar> &data, const int index, State &state);

    static void vector_cmd(const int cmd, State &state);

    static Geo::Point vector_direction(const int dir);

    int subshape_cmd(const std::vector<uchar> &data, const int index, State &state);

    static int xy_displacement(const std::vector<uchar> &data, const int index, State &state);

    static int multiple_xy_displacement(const std::vector<uchar> &data, const int index, State &state);

    static int octant_arc(const std::vector<uchar> &data, const int index, State &state);

    static int fractional_arc(const std::vector<uchar> &data, const int index, State &state);

    static int bulge_arc(const std::vector<uchar> &data, const int index, State &state);

    static int multiple_bulge_arcs(const std::vector<uchar> &data, const int index, State &state);

    int skip_code(const std::vector<uchar> &data, int index);

    SHXShape scale_subshape_at_insert_point(const int code, const double width, const double height, const Geo::Point &point);

    static Geo::Point arc_segment(const Geo::Point &point, Geo::Point vec, double bulge, const double scale, const bool pen_down, Geo::Polyline &polyline);
};


class SHXContentParser
{
public:
    virtual SHXFontContentData parse(SHXFileReader &reader) = 0;

    virtual ~SHXContentParser() = default;
};

class SHXShapeContentParser : public SHXContentParser
{
public:
    SHXFontContentData parse(SHXFileReader &reader) override;
};

class SHXBigfontContentParser : public SHXContentParser
{
public:
    SHXFontContentData parse(SHXFileReader &reader) override;

private:
    std::tuple<std::string, int> utf8_array_to_str(const std::vector<uint8_t> &array);
};

class SHXUnifontContentParser : public SHXContentParser
{
public:
    SHXFontContentData parse(SHXFileReader &reader) override;
};

class SHXContentParserFactory
{
public:
    static SHXContentParser *parser(const SHXFontHeaderData::SHXFontType type);
};


class SHXFont
{
public:
    SHXFontData fontdata;
private:
    SHXShapeParser _shapeparser;
    SHXFileReader _reader;

public:
    SHXFont(std::ifstream *stream);

    bool has_char(const int code) const;

    SHXShape char_shape(const int code, const int size);
};

}