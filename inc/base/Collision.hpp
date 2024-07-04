#pragma once
#include <vector>
#include <utility>
#include "base/Geometry.hpp"
#include "draw/Container.hpp"


namespace Geo
{
    namespace Collision
    {
        class GridNode
        {
        protected:
            AABBRect _rect;
            std::vector<Geometry *> _objects;

        public:
            GridNode();

            GridNode(const AABBRect &rect);

            GridNode(const double left, const double top, const double right, const double bottom);

            void set_rect(const AABBRect &rect);

            const AABBRect &rect() const;

            bool append(Geometry *object);

            bool remove(Geometry *object);

            bool has(Geometry *object) const;

            void clear();

            bool select(const Point &pos, std::vector<Geometry *> &objects) const;

            bool select(const AABBRect &rect, std::vector<Geometry *> &objects) const;

            bool find_collision_objects(const Geo::Geometry *object, std::vector<Geometry *> &objects) const;

            bool find_collision_pairs(std::vector<std::pair<Geometry *, Geometry *>> &pairs) const;
        };
    
        class GridMap
        {
        private:
            std::vector<GridNode> _grids;
            std::vector<Geo::Geometry *> _objects;
            std::vector<Geo::AABBRect> _rects;
            double _left, _top, _right, _bottom;

        public:
            GridMap();

            GridMap(const ContainerGroup &group);

            GridMap(const std::vector<Geo::Geometry *> &objects);

            GridMap(const std::initializer_list<Geo::Geometry *> &objects);

            void build(const ContainerGroup &group);

            void build(const std::vector<Geo::Geometry *> &objects);

            void build(const std::vector<Geo::Geometry *> &objects, const std::vector<Geo::AABBRect> &rects);

            void append(Geo::Geometry *object);

            void remove(Geo::Geometry *object);

            void update(Geo::Geometry *object);

            void update();

            bool has(Geometry *object) const;

            void clear();

            bool select(const Point &pos, std::vector<Geometry *> &objects) const;

            bool select(const AABBRect &rect, std::vector<Geometry *> &objects) const;

            bool find_collision_objects(const Geo::Geometry *object, std::vector<Geometry *> &objects) const;

            bool find_collision_pairs(std::vector<std::pair<Geometry *, Geometry *>> &pairs) const;
        };

        class QuadTreeNode : public GridNode
        {
        private:
            QuadTreeNode *_nodes[4] = {nullptr};

            void split();

            bool is_tail_node() const;

            bool merge();
        
        public:
            QuadTreeNode();

            QuadTreeNode(const AABBRect &rect);

            QuadTreeNode(const double left, const double top, const double right, const double bottom);

            ~QuadTreeNode();

            bool append(Geometry *object);

            void append_node(const size_t index, QuadTreeNode *node);

            bool remove(Geometry *object);

            bool has(Geometry *object) const;

            void clear();

            bool select(const Geo::Point &pos, std::vector<Geometry *> &objects) const;

            bool select(const Geo::AABBRect &rect, std::vector<Geometry *> &objects) const;

            bool find_collision_objects(const Geo::Geometry *object, std::vector<Geometry *> &objects) const;

            bool find_collision_pairs(std::vector<std::pair<Geometry *, Geometry *>> &pairs) const;
        };
    
        class QuadTree
        {
        private:
            QuadTreeNode *_root = nullptr;
            std::vector<Geo::Geometry *> _objects;
            std::vector<Geo::AABBRect> _rects;
            double _left, _top, _right, _bottom;

        public:
            QuadTree();

            QuadTree(const ContainerGroup &group);

            QuadTree(const std::vector<Geo::Geometry *> &objects);

            QuadTree(const std::initializer_list<Geo::Geometry *> &objects);

            ~QuadTree();

            void build(const ContainerGroup &group);

            void build(const std::vector<Geo::Geometry *> &objects);

            void build(const std::vector<Geo::Geometry *> &objects, const std::vector<Geo::AABBRect> &rects);

            void append(Geo::Geometry *object);

            void remove(Geo::Geometry *object);

            void update(Geo::Geometry *object);

            void update();

            bool has(Geometry *object) const;

            void clear();

            bool select(const Point &pos, std::vector<Geometry *> &objects) const;

            bool select(const AABBRect &rect, std::vector<Geometry *> &objects) const;

            bool find_collision_objects(const Geo::Geometry *object, std::vector<Geometry *> &objects) const;

            bool find_collision_pairs(std::vector<std::pair<Geometry *, Geometry *>> &pairs) const;
        };
    }
}