#pragma once
#include <vector>
#include "base/Geometry.hpp"
#include "base/Dimension.hpp"
#include "base/Graph.hpp"


class QuadTreeNode
{
private:
    static const int min_height = 60, min_width = 80, max_depth = 8, multithreading_depth = 5, min_size = 128;
    int _depth = 1;
    Geo::AABBRectParams _rect;
    std::vector<Geo::Geometry *> _objects;
    QuadTreeNode *_nodes[4] = {nullptr, nullptr, nullptr, nullptr};

public:
    QuadTreeNode() = default;

    QuadTreeNode(const int depth);

    ~QuadTreeNode();

    void clear();

    Geo::AABBRectParams &rect();

    void find_visible_objects(const Geo::AABBRectParams &rect, std::vector<Geo::Geometry *> &visible_objects);

    void build(const Geo::AABBRectParams &rect, const std::vector<Geo::Geometry *> &objects);

    void update(Geo::Geometry *object);

    void remove(Geo::Geometry *object);

    void remove(const std::vector<Geo::Geometry *> &objects);

    void append(const Geo::AABBRectParams &rect, Geo::Geometry *object);

    bool empty() const;
};


class QuadTree
{
private:
    QuadTreeNode _root;
    std::vector<Geo::Geometry *> _objects, _visible_objects;

public:
    QuadTree() = default;

    void clear();

    void find_visible_objects(const Geo::AABBRectParams &rect, std::vector<Geo::Geometry *> &visible_objects);

    void find_visible_objects(const Geo::AABBRectParams &rect);

    const std::vector<Geo::Geometry *> &visible_objects() const;

    void build(const std::vector<Geo::Geometry *> &objects);

    void build(const Graph *graph);

    void update(Geo::Geometry *object);

    void update(const std::vector<Geo::Geometry *> &objects);

    void remove(Geo::Geometry *object);

    void remove(const std::vector<Geo::Geometry *> &objects);

    void append(Geo::Geometry *object);

    void append(const std::vector<Geo::Geometry *> &objects);
};