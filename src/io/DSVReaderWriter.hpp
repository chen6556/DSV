#pragma once
#include <string>
#include <fstream>
#include <vector>
#include <unordered_map>
#include "base/Graph.hpp"
#include "base/Dimension.hpp"


class DSVReaderWriter
{
private:
    static constexpr int code_type = 0, code_hanlde = 1, code_name = 2, code_layer = 3, code_pathx = 10, code_pathy = 11,
                         code_controlx = 12, code_controly = 13, code_labelx = 14, code_labley = 15, code_xlength = 20, code_ylength = 21,
                         code_rotateangle = 30, code_startangle = 31, code_endangle = 32, code_intvalue = 40, code_floatvalue = 41,
                         code_strvalue = 42, code_minor_arc = 43, code_pointer = 80;

    Graph *_graph = nullptr;
    int _global_handle = 0;
    std::unordered_map<int, Geo::Geometry *> _handle_to_object;
    std::unordered_map<Geo::Geometry *, int> _object_to_handle;
    std::unordered_map<int, std::vector<int>> _parent_to_children;
    std::unordered_map<int, int> _child_to_parent;
    std::unordered_map<std::string, size_t> _group_name_to_index;
    std::string _current_layer;

    struct Pair
    {
        int code = 0;
        int value = 0;
        double real = 0;
        std::string str;
    } _pair;
    std::vector<std::vector<Pair>> _data;
    struct Info
    {
        int hanlde = 0;
        std::string name;
        std::string layer;
        std::string type;
    } _info;

public:
    DSVReaderWriter(Graph *graph);

    void read(std::ifstream &stream);

    void write(std::ofstream &stream);

private:
    static void check_group_name(Graph *graph);

    void record_handle(Graph *graph);

    void write(std::ofstream &stream, Geo::Point *point);

    void write(std::ofstream &stream, Geo::Polyline *polyline);

    void write(std::ofstream &stream, Geo::Polygon *polygon);

    void write(std::ofstream &stream, Geo::Circle *circle);

    void write(std::ofstream &stream, Geo::Arc *arc);

    void write(std::ofstream &stream, Geo::Ellipse *ellipse);

    void write(std::ofstream &stream, Geo::BSpline *bspline);

    void write(std::ofstream &stream, Geo::CubicBezier *bezier);

    void write(std::ofstream &stream, Text *text);

    void write(std::ofstream &stream, Combination *combination);

    void write(std::ofstream &stream, Dim::DimAligned *dim);

    void write(std::ofstream &stream, Dim::DimAngle *dim);

    void write(std::ofstream &stream, Dim::DimArc *dim);

    void write(std::ofstream &stream, Dim::DimDiameter *dim);

    void write(std::ofstream &stream, Dim::DimLinear *dim);

    void write(std::ofstream &stream, Dim::DimRadius *dim);

    void write(std::ofstream &stream, Dim::DimOrdinate *dim);

    bool read_code(std::ifstream &stream);

    bool read_value(std::ifstream &stream);

    bool read_pair(std::ifstream &stream);

    bool read_data(const std::vector<Pair> &data);

    bool check_data(const std::vector<Pair> &data);

    void check_group(const std::string &name);

    bool read_point(const std::vector<Pair> &data);

    bool read_line(const std::vector<Pair> &data);

    bool read_polyline(const std::vector<Pair> &data);

    bool read_polygon(const std::vector<Pair> &data);

    bool read_circle(const std::vector<Pair> &data);

    bool read_arc(const std::vector<Pair> &data);

    bool read_ellipse(const std::vector<Pair> &data);

    bool read_bspline(const std::vector<Pair> &data);

    bool read_cubicbezier(const std::vector<Pair> &data);

    bool read_text(const std::vector<Pair> &data);

    bool read_combination(const std::vector<Pair> &data);

    bool read_aligned_dim(const std::vector<Pair> &data);

    bool read_angle_dim(const std::vector<Pair> &data);

    bool read_arc_dim(const std::vector<Pair> &data);

    bool read_diameter_dim(const std::vector<Pair> &data);

    bool read_linear_dim(const std::vector<Pair> &data);

    bool read_radius_dim(const std::vector<Pair> &data);

    bool read_ordinate_dim(const std::vector<Pair> &data);
};