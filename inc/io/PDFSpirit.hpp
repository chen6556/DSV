#pragma once
#include "Spirit.hpp"
#include "draw/Graph.hpp"


class PDFSpirit : public Spirit<PDFSpirit>
{
private:
    bool _active = false;
    std::vector<double> _values;
    std::vector<Geo::Point> _points;
    Graph *_graph = nullptr;

    enum Tool {None, Line, Curve};
    Tool _last_tool = Tool::None, _cur_tool = Tool::None;
    Geo::Coord _start_point;

    void start(const std::string &value);

    void store_value(const std::string &value);

    void skip(const std::string &value);

    void line(const std::string &value);

    void curve(const std::string &value);
    

    void store(const std::string &value);

    void on(const std::string &value);

    void off(const std::string &value);


public:
    PDFSpirit();

    void load_graph(Graph *g);
};