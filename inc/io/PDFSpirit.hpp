#pragma once
#include "Spirit.hpp"
#include "draw/Graph.hpp"
#include <array>
#include <stack>


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

    std::array<double, 6> _trans_mat = {1, 0, 0, 0, 1, 0};
    std::stack<std::array<double, 6>> _trans_mats;

    void start(const std::string &value);

    void store_value(const std::string &value);


    void change_trans_mat(const std::string &value);

    void store_trans_mat(const std::string &value);

    void pop_trans_mat(const std::string &value);


    void line(const std::string &value);

    void curve(const std::string &value);

    void rect(const std::string &value);

    void close_shape(const std::string &value);
    

    void store(const std::string &value);

    void on(const std::string &value);

    void off(const std::string &value);


public:
    PDFSpirit();

    void load_graph(Graph *g);
};