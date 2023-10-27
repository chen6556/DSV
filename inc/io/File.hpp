#pragma once
#include "draw/Graph.hpp"


class File
{
private:
    File(){};

public:
    enum FileType {JSON, PLT};

    static void read(const QString &path, Graph *graph);

    static void write_json(const QString &path, Graph *graph);

    static void wirte_plt(const std::string &path, Graph *graph);

    static void write(const QString &path, Graph *graph, const FileType type = FileType::JSON);
};