#pragma once

#include "draw/Graph.hpp"


class File
{
private:
    File(){};

    static void write_dsv(const std::string &path, const Graph *graph);

    static void write_plt(const std::string &path, const Graph *graph);

public:
    enum FileType {DSV, PLT};

    static void write(const QString &path, const Graph *graph, const FileType type);
};