#pragma once

#include "draw/Graph.hpp"
#include "base/Geometry.hpp"
#include <fstream>


namespace PLTParser
{

class Importer
{
private:
    Graph *_graph = nullptr;
    std::vector<Geo::Point> _points;
    std::vector<double> _parameters;
    Geo::Coord _last_coord;

    bool _relative_coord = false;

    struct Text
    {
        std::string txt;
        Geo::Point pos;

        Text(const std::string &text, const Geo::Point &position)
            : txt(text), pos(position) {};
    };
    std::list<Text> _texts;

private:
    void store_points();

    void store_arc();

public:

    void x_coord(const double value);

    void y_coord(const double value);

    void parameter(const double value);

    void pu();

    void sp(const int value);

    void ci();

    void pa();

    void pr();

    void aa();

    void ar();

    void store_text(const std::string &text);

    void end();


    void load_graph(Graph *g);

    void reset();
};


bool parse(std::string_view &stream, Graph *graph);

bool parse(std::ifstream &stream, Graph *graph);
}