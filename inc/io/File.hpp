#pragma once
#include "draw/Graph.hpp"


class File
{
private:
    File(){};

    static void write_json(const QString &path, Graph *graph);

    static void write_plt(const std::string &path, Graph *graph);

public:
    enum FileType {JSON, PLT};

    static void read(const QString &path, Graph *graph);

    static void write(const QString &path, Graph *graph, const FileType type);
};