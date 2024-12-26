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

enum class Command
{
    None,		// Nothing
    Q,			// Invalid

    // G Command
    G70,		// Unit inch
    G71,		// Unit mm

    // D Command
    D1,			// Pen down
    D2,			// Pen up

    // M Command
    M01,		// 
    M14,		// Knife down
    M15,		// Knife up
    M19,		// Notch
    M31,        // Text
    M43,		// Drill2
    M44,		// Drill1
    M45,		// Drill3
    M72,		// Drill3
    M73			// Drill4
};

class Importer
{
private:
    Graph *_graph = nullptr;
    std::vector<Geo::Point> _points;
    Geo::Point _last_coord;
    CircleContainer *_last_circle_container;
    Container *_last_container;

    int _circle_radius = 10;
    size_t _index = 0;

    bool _is_pen_down = false;
    bool _is_knife_down = false;
    Command _command = Command::None;

    CurveType _curve_type = CurveType::Linear;

    Unit unit = Unit::mm;

public:
    void set_x_coord(const int value);
    void set_y_coord(const int value);
    void store_points();

    void draw_circle();
    void set_circle_radius(const std::string &text);

    void set_unit_mm();
    void set_unit_hectomil();

    void pen_down();
    void pen_up();

    void knife_down(const std::string &value);
    void knife_up();

    void load_graph(Graph *g);
    void reset();

    void read_text();
    void store_text(const std::string& text);

    void print_symbol(const std::string& str);

    void end();
private:
    inline double unit_scale(int);
};

bool parse(std::string_view &stream, Graph *graph);

bool parse(std::ifstream &stream, Graph *graph);
}; // namespace RS274DParser