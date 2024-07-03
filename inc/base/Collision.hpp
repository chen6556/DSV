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

            bool append(Geometry *object);

            bool remove(Geometry *object);

            bool has(Geometry *object) const;

            bool select(const Point &pos, std::vector<Geometry *> &objects) const;

            bool select(const AABBRect &rect, std::vector<Geometry *> &objects) const;

            bool find_collision_objects(std::vector<Geometry *> &objects) const;

            bool find_collision_pairs(std::vector<std::pair<Geometry *, Geometry *>> &pairs) const;
        };
    }
}