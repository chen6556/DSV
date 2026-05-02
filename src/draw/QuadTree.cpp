#include <future>
#include <unordered_set>
#include "QuadTree.hpp"
#include "base/Algorithm.hpp"


QuadTreeNode::QuadTreeNode(const int depth) : _depth(depth)
{
}

QuadTreeNode::~QuadTreeNode()
{
    for (int i = 0; i < 4; ++i)
    {
        delete _nodes[i];
    }
}

void QuadTreeNode::clear()
{
    for (int i = 0; i < 4; ++i)
    {
        delete _nodes[i];
        _nodes[i] = nullptr;
    }
    _objects.clear();
    _rect.left = _rect.top = _rect.right = _rect.bottom = 0;
}

Geo::AABBRectParams &QuadTreeNode::rect()
{
    return _rect;
}

void QuadTreeNode::find_visible_objects(const Geo::AABBRectParams &rect, std::vector<Geo::Geometry *> &visible_objects)
{
    if (!Geo::is_intersected(_rect, rect))
    {
        return;
    }

    if (_nodes[0] != nullptr && _nodes[1] != nullptr && _nodes[2] != nullptr && _nodes[3] != nullptr)
    {
        if (std::vector<Geo::Geometry *> temp_objects[4]; _depth % 2 == 0) //> multithreading_depth)
        {
            std::future<void> threads[4];
            for (int i = 0; i < 4; ++i)
            {
                threads[i] = std::async(std::launch::async,
                                        [&temp_objects, i, &rect, this]() { _nodes[i]->find_visible_objects(rect, temp_objects[i]); });
            }
            for (int i = 0; i < 4; ++i)
            {
                threads[i].wait();
                visible_objects.insert(visible_objects.end(), temp_objects[i].begin(), temp_objects[i].end());
            }
        }
        else
        {
            for (int i = 0; i < 4; ++i)
            {
                _nodes[i]->find_visible_objects(rect, temp_objects[i]);
                visible_objects.insert(visible_objects.end(), temp_objects[i].begin(), temp_objects[i].end());
            }
        }
    }
    else
    {
        if (rect.bottom <= _rect.bottom && rect.left <= _rect.left && rect.right >= _rect.right && rect.top >= _rect.top)
        {
            visible_objects.assign(_objects.begin(), _objects.end());
        }
        else
        {
            for (Geo::Geometry *object : _objects)
            {
                if (Geo::is_intersected(rect, object->aabbrect_params()))
                {
                    visible_objects.push_back(object);
                }
            }
        }
    }
}

void QuadTreeNode::build(const Geo::AABBRectParams &rect, const std::vector<Geo::Geometry *> &objects)
{
    _rect = rect;
    _objects.clear();
    for (int i = 0; i < 4; ++i)
    {
        delete _nodes[i];
        _nodes[i] = nullptr;
    }
    if (objects.empty())
    {
        return;
    }

    if (const double width = rect.right - rect.left, height = rect.top - rect.bottom;
        _depth < max_depth && objects.size() > min_size && (width > min_width || height > min_height))
    {
        Geo::AABBRectParams rects[4];
        rects[0].left = rect.left, rects[0].top = rect.top, rects[0].right = (rect.left + rect.right) / 2,
        rects[0].bottom = (rect.top + rect.bottom) / 2;
        rects[1].left = (rect.left + rect.right) / 2, rects[1].top = rect.top, rects[1].right = rect.right,
        rects[1].bottom = (rect.top + rect.bottom) / 2;
        rects[3].left = rect.left, rects[3].top = (rect.top + rect.bottom) / 2, rects[3].right = (rect.left + rect.right) / 2,
        rects[3].bottom = rect.bottom;
        rects[2].left = (rect.left + rect.right) / 2, rects[2].top = (rect.top + rect.bottom) / 2, rects[2].right = rect.right,
        rects[2].bottom = rect.bottom;

        std::vector<Geo::Geometry *> children[4];
        for (Geo::Geometry *object : objects)
        {
            const Geo::AABBRectParams params = object->aabbrect_params();
            for (int i = 0; i < 4; ++i)
            {
                if (Geo::is_intersected(rects[i], params))
                {
                    children[i].push_back(object);
                }
            }
        }
        if (_depth % 2 == 1) // > multithreading_depth)
        {
            std::future<void> threads[4];
            for (int i = 0; i < 4; ++i)
            {
                _nodes[i] = new QuadTreeNode(_depth + 1);
                threads[i] = std::async(std::launch::async, [this, i, &rects, &children]() { _nodes[i]->build(rects[i], children[i]); });
            }
            for (int i = 0; i < 4; ++i)
            {
                threads[i].wait();
            }
        }
        else
        {
            for (int i = 0; i < 4; ++i)
            {
                _nodes[i] = new QuadTreeNode(_depth + 1);
                _nodes[i]->build(rects[i], children[i]);
            }
        }
    }
    else
    {
        _objects.assign(objects.begin(), objects.end());
    }
}

void QuadTreeNode::update(const Geo::AABBRectParams &rect, Geo::Geometry *object)
{
    if (_nodes[0] != nullptr || _nodes[1] != nullptr || _nodes[2] != nullptr || _nodes[3] != nullptr)
    {
        if (Geo::is_intersected(_rect, rect))
        {
            if (_depth %  2 == 1) // > multithreading_depth)
            {
                std::future<void> threads[4];
                for (int i = 0; i < 4; ++i)
                {
                    threads[i] = std::async(std::launch::async, [this, i, &rect, object]() { _nodes[i]->update(rect, object); });
                }
                for (int i = 0; i < 4; ++i)
                {
                    threads[i].wait();
                }
            }
            else
            {
                for (int i = 0; i < 4; ++i)
                {
                    _nodes[i]->update(rect, object);
                }
            }
            if (_nodes[0]->empty() && _nodes[1]->empty() && _nodes[2]->empty() && _nodes[3]->empty())
            {
                for (int i = 0; i < 4; ++i)
                {
                    delete _nodes[i];
                    _nodes[i] = nullptr;
                }
            }
        }
        else
        {
            remove(object);
        }
    }
    else
    {
        if (Geo::is_intersected(_rect, rect))
        {
            if (std::find(_objects.begin(), _objects.end(), object) == _objects.end())
            {
                _objects.push_back(object);
                if (_objects.size() > min_size && (_rect.top - _rect.bottom > min_height || _rect.right - _rect.left > min_width))
                {
                    std::vector<Geo::Geometry *> objects(_objects);
                    build(_rect, objects);
                }
            }
        }
        else
        {
            _objects.erase(std::remove(_objects.begin(), _objects.end(), object), _objects.end());
        }
    }
}

void QuadTreeNode::remove(Geo::Geometry *object)
{
    if (_nodes[0] != nullptr || _nodes[1] != nullptr || _nodes[2] != nullptr || _nodes[3] != nullptr)
    {
        for (QuadTreeNode *node : _nodes)
        {
            node->remove(object);
        }
        if (_nodes[0]->empty() && _nodes[1]->empty() && _nodes[2]->empty() && _nodes[3]->empty())
        {
            for (int i = 0; i < 4; ++i)
            {
                delete _nodes[i];
                _nodes[i] = nullptr;
            }
        }
    }
    else
    {
        _objects.erase(std::remove(_objects.begin(), _objects.end(), object), _objects.end());
    }
}

void QuadTreeNode::remove(const std::vector<Geo::Geometry *> &objects)
{
    if (_nodes[0] != nullptr || _nodes[1] != nullptr || _nodes[2] != nullptr || _nodes[3] != nullptr)
    {
        for (int i = 0; i < 4; ++i)
        {
            _nodes[i]->remove(objects);
        }
        if (_nodes[0]->empty() && _nodes[1]->empty() && _nodes[2]->empty() && _nodes[3]->empty())
        {
            for (int i = 0; i < 4; ++i)
            {
                delete _nodes[i];
                _nodes[i] = nullptr;
            }
        }
    }
    else
    {
        for (Geo::Geometry *object : objects)
        {
            _objects.erase(std::remove(_objects.begin(), _objects.end(), object), _objects.end());
        }
    }
}

void QuadTreeNode::append(const Geo::AABBRectParams &rect, Geo::Geometry *object)
{
    if (!Geo::is_intersected(_rect, rect))
    {
        return;
    }
    if (_nodes[0] != nullptr || _nodes[1] != nullptr || _nodes[2] != nullptr || _nodes[3] != nullptr)
    {
        if (_depth % 2 == 1) // > multithreading_depth)
        {
            std::future<void> threads[4];
            for (int i = 0; i < 4; ++i)
            {
                threads[i] = std::async(std::launch::async, [this, i, rect, object]() { _nodes[i]->append(rect, object); });
            }
            for (int i = 0; i < 4; ++i)
            {
                threads[i].wait();
            }
        }
        else
        {
            for (QuadTreeNode *node : _nodes)
            {
                node->append(rect, object);
            }
        }
    }
    else if (_depth >= max_depth || _objects.size() < min_size)
    {
        _objects.push_back(object);
    }
    else
    {
        std::vector<Geo::Geometry *> objects(_objects);
        objects.push_back(object);
        build(_rect, objects);
    }
}

bool QuadTreeNode::empty() const
{
    if (_nodes[0] == nullptr || _nodes[1] == nullptr || _nodes[2] == nullptr || _nodes[3] == nullptr)
    {
        return _objects.empty();
    }
    else
    {
        return false;
    }
}


void QuadTree::clear()
{
    _root.clear();
    _objects.clear();
}

void QuadTree::find_visible_objects(const Geo::AABBRectParams &rect, std::vector<Geo::Geometry *> &visible_objects)
{
    _root.find_visible_objects(rect, visible_objects);
    std::unordered_set<Geo::Geometry *> temp(visible_objects.begin(), visible_objects.end());
    visible_objects.assign(temp.begin(), temp.end());
    std::sort(visible_objects.begin(), visible_objects.end());
}

void QuadTree::find_visible_objects(const Geo::AABBRectParams &rect)
{
    _visible_objects.clear();
    find_visible_objects(rect, _visible_objects);
}

const std::vector<Geo::Geometry *> &QuadTree::visible_objects() const
{
    return _visible_objects;
}

void QuadTree::build(const std::vector<Geo::Geometry *> &objects)
{
    if (objects.empty())
    {
        Geo::AABBRectParams rect;
        rect.left = rect.bottom = -100;
        rect.top = 600;
        rect.right = 1000;
        return _root.build(rect, objects);
    }
    Geo::AABBRectParams rect = objects.front()->aabbrect_params();
    for (Geo::Geometry *object : objects)
    {
        Geo::AABBRectParams temp = object->aabbrect_params();
        if (temp.left < rect.left)
        {
            rect.left = temp.left;
        }
        if (temp.right > rect.right)
        {
            rect.right = temp.right;
        }
        if (temp.top > rect.top)
        {
            rect.top = temp.top;
        }
        if (temp.bottom < rect.bottom)
        {
            rect.bottom = temp.bottom;
        }
    }
    const double width = rect.right - rect.left, height = rect.top - rect.bottom;
    rect.left -= std::max(width / 20, 100.0);
    rect.right += std::max(width / 20, 100.0);
    rect.top += std::max(height / 20, 100.0);
    rect.bottom -= std::max(height / 20, 100.0);
    _root.build(rect, objects);
    _objects.assign(objects.begin(), objects.end());
}

void QuadTree::build(const Graph *graph)
{
    std::vector<Geo::Geometry *> objects;
    for (const ContainerGroup &group : graph->container_groups())
    {
        for (Geo::Geometry *geo : group)
        {
            geo->is_selected = false;
        }
        if (group.visible())
        {
            objects.insert(objects.end(), group.begin(), group.end());
        }
    }
    return build(objects);
}

void QuadTree::update(Geo::Geometry *object)
{
    if (Geo::AABBRectParams rect = object->aabbrect_params(); rect.left >= _root.rect().left && rect.right <= _root.rect().right &&
                                                              rect.bottom >= _root.rect().bottom && rect.top <= _root.rect().top)
    {
        _root.update(rect, object);
    }
    else
    {
        if (rect.left > _root.rect().left)
        {
            rect.left = _root.rect().left;
        }
        if (rect.top < _root.rect().top)
        {
            rect.top = _root.rect().top;
        }
        if (rect.right < _root.rect().right)
        {
            rect.right = _root.rect().right;
        }
        if (rect.bottom > _root.rect().bottom)
        {
            rect.bottom = _root.rect().bottom;
        }
        const double width = rect.right - rect.left, height = rect.top - rect.bottom;
        rect.left -= std::max(width / 20, 100.0);
        rect.right += std::max(width / 20, 100.0);
        rect.top += std::max(height / 20, 100.0);
        rect.bottom -= std::max(height / 20, 100.0);
        _root.build(rect, _objects);
    }
}

void QuadTree::update(const std::vector<Geo::Geometry *> &objects)
{
    if (objects.empty())
    {
        return;
    }
    Geo::AABBRectParams rect = objects.front()->aabbrect_params();
    std::vector<Geo::AABBRectParams> rects({rect});
    for (Geo::Geometry *object : objects)
    {
        const Geo::AABBRectParams temp = object->aabbrect_params();
        rects.emplace_back(temp);
        if (temp.left < rect.left)
        {
            rect.left = temp.left;
        }
        if (temp.right > rect.right)
        {
            rect.right = temp.right;
        }
        if (temp.top > rect.top)
        {
            rect.top = temp.top;
        }
        if (temp.bottom < rect.bottom)
        {
            rect.bottom = temp.bottom;
        }
    }
    if (rect.left >= _root.rect().left && rect.right <= _root.rect().right && rect.bottom >= _root.rect().bottom &&
        rect.top <= _root.rect().top)
    {
        for (size_t i = 0, count = objects.size(); i < count; ++i)
        {
            _root.update(rects[i], objects[i]);
        }
    }
    else
    {
        if (rect.left > _root.rect().left)
        {
            rect.left = _root.rect().left;
        }
        if (rect.top < _root.rect().top)
        {
            rect.top = _root.rect().top;
        }
        if (rect.right < _root.rect().right)
        {
            rect.right = _root.rect().right;
        }
        if (rect.bottom > _root.rect().bottom)
        {
            rect.bottom = _root.rect().bottom;
        }
        const double width = rect.right - rect.left, height = rect.top - rect.bottom;
        rect.left -= std::max(width / 20, 100.0);
        rect.right += std::max(width / 20, 100.0);
        rect.top += std::max(height / 20, 100.0);
        rect.bottom -= std::max(height / 20, 100.0);
        _root.build(rect, objects);
    }
}

void QuadTree::remove(Geo::Geometry *object)
{
    _root.remove(object);
    _objects.erase(std::remove(_objects.begin(), _objects.end(), object), _objects.end());
}

void QuadTree::remove(const std::vector<Geo::Geometry *> &objects)
{
    if (objects.empty())
    {
        return;
    }
    _root.remove(objects);
    for (Geo::Geometry *object : objects)
    {
        _objects.erase(std::remove(_objects.begin(), _objects.end(), object), _objects.end());
    }
}

void QuadTree::append(Geo::Geometry *object)
{
    _objects.push_back(object);
    if (Geo::AABBRectParams rect = object->aabbrect_params(); rect.left >= _root.rect().left && rect.right <= _root.rect().right &&
                                                              rect.bottom >= _root.rect().bottom && rect.top <= _root.rect().top)
    {
        _root.append(rect, object);
    }
    else
    {
        if (rect.left > _root.rect().left)
        {
            rect.left = _root.rect().left;
        }
        if (rect.top < _root.rect().top)
        {
            rect.top = _root.rect().top;
        }
        if (rect.right < _root.rect().right)
        {
            rect.right = _root.rect().right;
        }
        if (rect.bottom > _root.rect().bottom)
        {
            rect.bottom = _root.rect().bottom;
        }
        const double width = rect.right - rect.left, height = rect.top - rect.bottom;
        rect.left -= std::max(width / 20, 100.0);
        rect.right += std::max(width / 20, 100.0);
        rect.top += std::max(height / 20, 100.0);
        rect.bottom -= std::max(height / 20, 100.0);
        _root.build(rect, _objects);
    }
}

void QuadTree::append(const std::vector<Geo::Geometry *> &objects)
{
    if (objects.empty())
    {
        return;
    }
    _objects.insert(_objects.end(), objects.begin(), objects.end());
    Geo::AABBRectParams rect = objects.front()->aabbrect_params();
    std::vector<Geo::AABBRectParams> rects;
    for (Geo::Geometry *object : objects)
    {
        Geo::AABBRectParams temp = object->aabbrect_params();
        rects.emplace_back(temp);
        if (temp.left < rect.left)
        {
            rect.left = temp.left;
        }
        if (temp.right > rect.right)
        {
            rect.right = temp.right;
        }
        if (temp.top > rect.top)
        {
            rect.top = temp.top;
        }
        if (temp.bottom < rect.bottom)
        {
            rect.bottom = temp.bottom;
        }
    }
    if (rect.left >= _root.rect().left && rect.right <= _root.rect().right && rect.bottom >= _root.rect().bottom &&
        rect.top <= _root.rect().top)
    {
        for (size_t i = 0, count = objects.size(); i < count; ++i)
        {
            _root.append(rects[i], objects[i]);
        }
    }
    else
    {
        if (rect.left > _root.rect().left)
        {
            rect.left = _root.rect().left;
        }
        if (rect.top < _root.rect().top)
        {
            rect.top = _root.rect().top;
        }
        if (rect.right < _root.rect().right)
        {
            rect.right = _root.rect().right;
        }
        if (rect.bottom > _root.rect().bottom)
        {
            rect.bottom = _root.rect().bottom;
        }
        const double width = rect.right - rect.left, height = rect.top - rect.bottom;
        rect.left -= std::max(width / 20, 100.0);
        rect.right += std::max(width / 20, 100.0);
        rect.top += std::max(height / 20, 100.0);
        rect.bottom -= std::max(height / 20, 100.0);
        _root.build(rect, _objects);
    }
}
