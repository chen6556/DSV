#pragma once
#include <vector>
#include "base/Geometry.hpp"
#include "base/Dimension.hpp"
#include "base/Graph.hpp"


class QuadTreeNode
{
private:
    static const int min_height = 60, min_width = 80, max_depth = 5, min_size = 64;
    int _depth = 1;
    Geo::AABBRect _rect;
    std::vector<Geo::DObject *> _objects;
    QuadTreeNode *_nodes[4] = {nullptr, nullptr, nullptr, nullptr};

public:
    QuadTreeNode() = default;

    QuadTreeNode(const int depth);

    ~QuadTreeNode();

    void clear();

    Geo::AABBRect &rect();

    void find_visible_objects(const Geo::AABBRect &rect, std::vector<Geo::DObject *> &visible_objects);

    void build(const Geo::AABBRect &rect, const std::vector<Geo::DObject *> &objects);

    void update(const Geo::AABBRect &rect, Geo::DObject *object);

    void remove(Geo::DObject *object);

    void remove(const std::vector<Geo::DObject *> &objects);

    void append(const Geo::AABBRect &rect, Geo::DObject *object);

    bool empty() const;
};


class QuadTree
{
private:
    QuadTreeNode _root;
    std::vector<Geo::DObject *> _objects, _visible_objects;

public:
    void clear();

    void find_visible_objects(const Geo::AABBRect &rect, std::vector<Geo::DObject *> &visible_objects);

    void find_visible_objects(const Geo::AABBRect &rect);

    const std::vector<Geo::DObject *> &visible_objects() const;

    void build(const std::vector<Geo::DObject *> &objects);

    void build(const Graph *graph);

    void update(Geo::DObject *object);

    void update(const std::vector<Geo::DObject *> &objects);

    void remove(Geo::DObject *object);

    void remove(const std::vector<Geo::DObject *> &objects);

    void append(Geo::DObject *object);

    void append(const std::vector<Geo::DObject *> &objects);
};