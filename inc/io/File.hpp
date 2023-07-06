#pragma once
#include "draw/Graph.hpp"


class File
{
private:
    File(){};

public:
    static void read(const QString &path, Graph *graph);

    static void write(const QString &path, Graph *graph);
};