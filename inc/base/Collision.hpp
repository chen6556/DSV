#pragma once
#include <vector>
#include <utility>
#include "base/Geometry.hpp"



namespace Geo
{
    namespace Collision
    {
        class GridNode
        {
        private:
            AABBRect _rect;
            std::vector<Geometry *> _objects;

        public:
            GridNode();

            GridNode(const AABBRect &rect);

            void set_rect(const AABBRect &rect);

            const AABBRect &rect() const;

            bool append(Geometry *object);

            bool remove(Geometry *object);

            bool has(Geometry *object) const;

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

            void build(const std::vector<Geo::Geometry *> &objects);

            void build(const std::vector<Geo::Geometry *> &objects, const std::vector<Geo::AABBRect> &rects);

            void append(Geo::Geometry *object);

            void remove(Geo::Geometry *object);

            void update(Geo::Geometry *object);

            bool has(Geometry *object) const;

            bool select(const Point &pos, std::vector<Geometry *> &objects) const;

            bool select(const AABBRect &rect, std::vector<Geometry *> &objects) const;

            bool find_collision_objects(const Geo::Geometry *object, std::vector<Geometry *> &objects) const;

            bool find_collision_pairs(std::vector<std::pair<Geometry *, Geometry *>> &pairs) const;
        };
    }
}