#pragma once

#include "draw/Graph.hpp"
#include "base/Geometry.hpp"
#include <fstream>


namespace DSVParser
{

class Importer
{
private:
    Graph *_graph = nullptr;
    std::vector<Geo::Point> _points;
    std::vector<double> _parameters;
    std::string _text;
    double _temp = 0;
    bool _is_combination = false;

public:

    void x_coord(const double value);

    void y_coord(const double value);

    void parameter(const double value);

    void store_polygon();

    void store_circle();

    void store_polyline();

    void store_bezier();

    void store_text();

    void begin_combination();

    void end_combination();

    void store_text(const std::string &text);

    void store_group();


    void load_graph(Graph *g);

    void reset();
};


bool parse(std::string_view &stream, Graph *graph);

bool parse(std::ifstream &stream, Graph *graph);
}