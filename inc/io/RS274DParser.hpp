#pragma once

#include <fstream>
#include <vector>

#include "base/Geometry.hpp"
#include "draw/Container.hpp"
#include "draw/Graph.hpp"


namespace RS274DParser
{

static constexpr double UNIT_SACLES[2] = {1, 2.54};

enum class Unit
{
    mm = 0,
    hectomil = 1,
    // mil实际上是100mil 等同于hectomil
    mil = 1
};

enum class CurveType
{
    Linear = 0,
};

class Importer
{
private:
    Graph *_graph = nullptr;
    std::vector<Geo::Point> _points;
    Geo::Point _last_coord;
    CircleContainer *_last_circle_container;
    Container *_last_container;

    bool _is_pen_down = false;
    bool _is_knife_down = false;

    CurveType _curve_type = CurveType::Linear;

    Unit unit = Unit::mm;

public:
    void set_x_coord(const int value);
    void set_y_coord(const int value);
    void store_points();

    void draw_circle(const int radius);
    void draw_cricle_10();
    void draw_cricle_20();
    void draw_cricle_30();
    void draw_cricle_40();

    void set_unit_mm();
    void set_unit_hectomil();

    void pen_down();
    void pen_up();

    void knife_down();
    void knife_up();

    void load_graph(Graph *g);
    void reset();

    void store_text(const std::string& text);

    void print_symbol(const std::string& str);

    void end();
private:
    inline double unit_scale(int);
};

bool parse(std::string_view &stream, Graph *graph);

bool parse(std::ifstream &stream, Graph *graph);
}; // namespace RS274DParser