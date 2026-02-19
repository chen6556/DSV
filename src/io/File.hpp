#pragma once

#include "base/Graph.hpp"


class File
{
private:
    File() {};

    static void write_plt(const std::string &path, const Graph *graph);

public:
    enum class FileType
    {
        PLT
    };

    static void write(const QString &path, const Graph *graph, const FileType type);
};