#pragma once
#include <set>
#include <list>
#include <vector>
#include <utility>
#include <QLineF>
#include "base/Geometry.hpp"
#include "draw/Container.hpp"


namespace Geo
{
    namespace Collision
    {
        class DirectMode
        {
        private:
            std::vector<Geo::Geometry *> _objects;

        public:
            DirectMode();

            DirectMode(const ContainerGroup &group);

            DirectMode(const std::vector<Geo::Geometry *> &objects);

            DirectMode(const std::initializer_list<Geo::Geometry *> &objects);

            void build(const ContainerGroup &group);

            void build(const std::vector<Geo::Geometry *> &objects);

            void build(const std::vector<Geo::Geometry *> &objects, const std::vector<Geo::AABBRect> &rects);

            void append(Geo::Geometry *object);

            void remove(Geo::Geometry *object);

            void update(Geo::Geometry *object);

            void update();

            bool has(Geo::Geometry *object) const;

            void clear();

            bool select(const Geo::Point &pos, std::vector<Geo::Geometry *> &objects) const;

            bool select(const Geo::AABBRect &rect, std::vector<Geo::Geometry *> &objects) const;

            bool find_collision_objects(const Geo::Geometry *object, std::vector<Geo::Geometry *> &objects, const bool norepeat = true) const;

            bool find_collision_pairs(std::vector<std::pair<Geo::Geometry *, Geo::Geometry *>> &pairs, const bool norepeat = true) const;
        };

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

            bool find_collision_objects(const Geo::Geometry *object, std::vector<Geometry *> &objects, const bool norepeat = true) const;

            bool find_collision_pairs(std::vector<std::pair<Geometry *, Geometry *>> &pairs, const bool norepeat = true) const;
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

            bool find_collision_objects(const Geo::Geometry *object, std::vector<Geometry *> &objects, const bool norepeat = true) const;

            bool find_collision_pairs(std::vector<std::pair<Geometry *, Geometry *>> &pairs, const bool norepeat = true) const;
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

            bool find_collision_objects(const Geo::Geometry *object, std::vector<Geometry *> &objects, const bool norepeat = true) const;

            bool find_collision_pairs(std::vector<std::pair<Geometry *, Geometry *>> &pairs, const bool norepeat = true) const;
        };


        Geo::Point edge_direciton(const Geo::Point &start, const Geo::Point &end, const bool to_origin); 

        void gjk_furthest_point(const Geo::Polygon &polygon, const Geo::Point &start, const Geo::Point &end, Geo::Point &result);

        void gjk_furthest_point(const Geo::AABBRect &rect, const Geo::Point &start, const Geo::Point &end, Geo::Point &result);

        void support(const Geo::Polygon &polygon0, const Geo::Polygon &polygon1, Geo::Point &start, Geo::Point &end, std::vector<Geo::Point> &points, Geo::Point &result);

        bool gjk(const Geo::Polygon &polygon0, const Geo::Polygon &polygon1);

        bool gjk(const Geo::AABBRect &rect, const Geo::Polygon &polygon);

        double epa(const Geo::Polygon &polygon0, const Geo::Polygon &polygon1, Geo::Vector &vec);

        double epa(const Geo::Polygon &polygon0, const Geo::Polygon &polygon1, const double tx, const double ty, Geo::Vector &vec);

        double epa(const Geo::Polygon &polygon0, const Geo::Polygon &polygon1, Geo::Point &start, Geo::Point &end);


        template <typename T = GridMap>
        class CollisionDetector
        {
        private:
            T _detector;

            static bool pair_in_pairs(const std::vector<std::pair<Geo::Geometry *, Geo::Geometry *>> &pairs, const Geo::Geometry *object0, const Geo::Geometry *object1, const bool ordered = false)
            {
                if (ordered)
                {
                    for (const std::pair<Geo::Geometry *, Geo::Geometry *> &pair : pairs)
                    {
                        if (pair.first == object0 && pair.second == object1)
                        {
                            return true;
                        }
                    }
                }
                else
                {
                    for (const std::pair<Geo::Geometry *, Geo::Geometry *> &pair : pairs)
                    {
                        if ((pair.first == object0 && pair.second == object1) || 
                            (pair.first == object1 && pair.second == object0))
                        {
                            return true;
                        }
                    }
                }
                return false;
            }

            static bool in_pair_first(const Geo::Geometry *object, const std::vector<std::pair<Geo::Geometry *, Geo::Geometry *>> &pairs)
            {
                for (const std::pair<Geo::Geometry *, Geo::Geometry *> &pair : pairs)
                {
                    if (pair.first == object)
                    {
                        return true;
                    }
                }
                return false;
            }

            static bool in_pair_second(const Geo::Geometry *object, const std::vector<std::pair<Geo::Geometry *, Geo::Geometry *>> &pairs)
            {
                for (const std::pair<Geo::Geometry *, Geo::Geometry *> &pair : pairs)
                {
                    if (pair.second == object)
                    {
                        return true;
                    }
                }
                return false;
            }

            static bool in_pairs(const Geo::Geometry *object, const std::vector<std::pair<Geo::Geometry *, Geo::Geometry *>> &pairs)
            {
                for (const std::pair<Geo::Geometry *, Geo::Geometry *> &pair : pairs)
                {
                    if (pair.first == object || pair.second == object)
                    {
                        return true;
                    }
                }
                return false;
            }

        public:
            CollisionDetector() {}

            CollisionDetector(const ContainerGroup &group)
                : _detector(group) {}

            CollisionDetector(const std::vector<Geo::Geometry *> &objects)
                : _detector(objects) {}

            CollisionDetector(const std::initializer_list<Geo::Geometry *> &objects)
                : _detector(objects) {}

            void build(const ContainerGroup &group)
            {
                return _detector.build(group);
            }

            void build(const std::vector<Geo::Geometry *> &objects)
            {
                return _detector.build(objects);
            }

            void build(const std::vector<Geo::Geometry *> &objects, const std::vector<Geo::AABBRect> &rects)
            {
                return _detector.build(objects, rects);
            }

            void append(Geo::Geometry *object)
            {
                return _detector.append(object);
            }

            void remove(Geo::Geometry *object)
            {
                return _detector.remove(object);
            }

            void update(Geo::Geometry *object)
            {
                return _detector.update(object);
            }

            void update()
            {
                return _detector.update();
            }

            bool has(Geometry *object) const
            {
                return _detector.has(object);
            }

            void clear()
            {
                return _detector.clear();
            }

            bool select(const Point &pos, std::vector<Geometry *> &objects) const
            {
                return _detector.select(pos, objects);
            }

            bool select(const AABBRect &rect, std::vector<Geometry *> &objects) const
            {
                return _detector.select(rect, objects);
            }

            bool find_collision_objects(const Geo::Geometry *object, std::vector<Geometry *> &objects, const bool norepeat = true) const
            {
                return _detector.find_collision_objects(object, objects, norepeat);
            }

            bool find_collision_pairs(std::vector<std::pair<Geometry *, Geometry *>> &pairs, const bool norepeat = true) const
            {
                return _detector.find_collision_pairs(pairs, norepeat);
            }

            void collision_translate(Geo::Geometry *object, const double tx, const double ty)
            {
                std::vector<Geo::Geometry *> crushed_objects({object});
                std::vector<std::pair<Geo::Geometry *, Geo::Geometry *>> moved_object_pairs;
                size_t index = 0;
                Geo::Point vec;
                while (!crushed_objects.empty())
                {
                    object = crushed_objects.back();
                    crushed_objects.pop_back();
                    index = crushed_objects.size();
                    if (_detector.find_collision_objects(object, crushed_objects, false))
                    {
                        for (size_t i = index, count = crushed_objects.size(); i < count; ++i)
                        {
                            if (pair_in_pairs(moved_object_pairs, object, crushed_objects[i], true))
                            {
                                crushed_objects.erase(crushed_objects.begin() + i--);
                                --count;
                            }
                            else
                            {
                                moved_object_pairs.emplace_back(object, crushed_objects[i]);
                                if (object->type() == Geo::Type::CONTAINER && crushed_objects[i]->type() == Geo::Type::CONTAINER &&
                                    Collision::epa(*static_cast<Geo::Polygon *>(object), *static_cast<Geo::Polygon *>(crushed_objects[i]), tx, ty, vec) > 0)
                                {
                                    if (vec.x * tx + vec.y * ty >= 0)
                                    {
                                        crushed_objects[i]->translate(vec.x, vec.y);
                                        _detector.update(crushed_objects[i]);
                                    }
                                    else
                                    {
                                        crushed_objects.erase(crushed_objects.begin() + i--);
                                        --count;
                                    }
                                    vec.clear();
                                }
                            }
                        }
                    }
                }
            }
        };
    }
}