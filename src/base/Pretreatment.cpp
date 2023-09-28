#include "base/Pretreatment.hpp"
#include "io/GlobalSetting.hpp"


Pretreatment::Pretreatment(Graph *graph)
    : _graph(graph)
{
    split();
    remove_outer_border();
    connect_lines(GlobalSetting::get_instance()->setting()["connect_distance"].toDouble());
    
    if (GlobalSetting::get_instance()->setting()["combine"].toBool())
    {
        combine();
    }
    finish();
};


void Pretreatment::split()
{
    for (ContainerGroup &group : _graph->container_groups())
    {
        while (!group.empty())
        {
            switch (group.back()->memo()["Type"].to_int())
            {
            case 0:
                _all_containers.emplace_back(reinterpret_cast<Container *>(group.pop_back()));
                break;
            case 1:
                _all_circles.emplace_back(reinterpret_cast<CircleContainer *>(group.pop_back()));
                break;
            case 2:
                _all_links.emplace_back(reinterpret_cast<Link *>(group.pop_back()));
                break;
            case 20:
                _all_polylines.emplace_back(reinterpret_cast<Geo::Polyline *>(group.pop_back()));
                break;
            case 21:
                _all_beziers.emplace_back(reinterpret_cast<Geo::Bezier *>(group.pop_back()));
                break;
            default:
                break;
            }
        }
    }

    _graph->clear();
    _graph->append_group();
}

void Pretreatment::connect_lines(const double value)
{
    bool flag = false;
    for (size_t i = 0, count0 = _all_polylines.size(); i < count0; ++i)
    {
        flag = false;
        for (size_t j = i + 1; j < count0; ++j)
        {
            if (_all_polylines[i]->front() == _all_polylines[j]->front() ||
                Geo::distance(_all_polylines[i]->front(), _all_polylines[j]->front()) < value)
            {
                std::reverse(_all_polylines[j]->begin(), _all_polylines[j]->end());
                _all_polylines[i]->insert(0, *_all_polylines[j]);
                delete _all_polylines[j];
                _all_polylines.erase(_all_polylines.begin() + j);
                --count0;
                --i;
                flag = true;
                break;
            }
            else if (_all_polylines[i]->front() == _all_polylines[j]->back() ||
                Geo::distance(_all_polylines[i]->front(), _all_polylines[j]->back()) < value)
            {
                _all_polylines[i]->insert(0, *_all_polylines[j]);
                delete _all_polylines[j];
                _all_polylines.erase(_all_polylines.begin() + j);
                --count0;
                --i;
                flag = true;
                break;
            }
            else if (_all_polylines[i]->back() == _all_polylines[j]->front() ||
                Geo::distance(_all_polylines[i]->back(), _all_polylines[j]->front()) < value)
            {
                _all_polylines[i]->append(*_all_polylines[j]);
                delete _all_polylines[j];
                _all_polylines.erase(_all_polylines.begin() + j);
                --count0;
                --i;
                flag = true;
                break;
            }
            else if (_all_polylines[i]->back() == _all_polylines[j]->back() ||
                Geo::distance(_all_polylines[i]->back(), _all_polylines[j]->back()) < value)
            {
                std::reverse(_all_polylines[j]->begin(), _all_polylines[j]->end());
                _all_polylines[i]->append(*_all_polylines[j]);
                delete _all_polylines[j];
                _all_polylines.erase(_all_polylines.begin() + j);
                --count0;
                --i;
                flag = true;
                break;
            }
        }
    
        if (flag)
        {
            continue;
        }
        for (size_t j = 0, count1 = _all_beziers.size(); j < count1; ++j)
        {
            if (_all_polylines[i]->front() == _all_beziers[j]->front() ||
                Geo::distance(_all_polylines[i]->front(), _all_beziers[j]->front()) < value)
            {
                std::reverse(_all_beziers[j]->begin(), _all_beziers[j]->end());
                _all_beziers[j]->update_shape();
                _all_polylines[i]->insert(0, _all_beziers[j]->shape());
                delete _all_beziers[j];
                _all_beziers.erase(_all_beziers.begin() + j);
                --i;
                break;
            }
            else if (_all_polylines[i]->front() == _all_beziers[j]->back() ||
                Geo::distance(_all_polylines[i]->front(), _all_beziers[j]->back()) < value)
            {
                _all_beziers[j]->update_shape();
                _all_polylines[i]->insert(0, _all_beziers[j]->shape());
                delete _all_beziers[j];
                _all_beziers.erase(_all_beziers.begin() + j);
                --i;
                break;
            }
            else if (_all_polylines[i]->back() == _all_beziers[j]->front() ||
                Geo::distance(_all_polylines[i]->back(), _all_beziers[j]->front()) < value)
            {
                _all_beziers[j]->update_shape();
                _all_polylines[i]->append(_all_beziers[j]->shape());
                delete _all_beziers[j];
                _all_beziers.erase(_all_beziers.begin() + j);
                --i;
                break;
            }
            else if (_all_polylines[i]->back() == _all_beziers[j]->back() ||
                Geo::distance(_all_polylines[i]->back(), _all_beziers[j]->back()) < value)
            {
                std::reverse(_all_beziers[j]->begin(), _all_beziers[j]->end());
                _all_beziers[j]->update_shape();
                _all_polylines[i]->append(_all_beziers[j]->shape());
                delete _all_beziers[j];
                _all_beziers.erase(_all_beziers.begin() + j);
                --i;
                break;
            }
        }
    }

    for (size_t i = 0, count = _all_polylines.size(); i < count; ++i)
    {
        if (_all_polylines[i]->front() == _all_polylines[i]->back() ||
            Geo::distance(_all_polylines[i]->front(), _all_polylines[i]->back()) < value)
        {
            _all_containers.emplace_back(new Container(Geo::Polygon(*_all_polylines[i])));
            delete _all_polylines[i];
            _all_polylines.erase(_all_polylines.begin() + i--);
            --count;
        }
    }

    std::sort(_all_containers.begin(), _all_containers.end(),
        [](const Container *a, const Container *b) { return a->area() > b->area(); });
    std::sort(_all_circles.begin(), _all_circles.end(),
        [](const CircleContainer *a, const CircleContainer *b) { return a->area() > b->area(); });
    std::sort(_all_polylines.begin(), _all_polylines.end(),
        [](const Geo::Polyline *a, const Geo::Polyline *b) { return a->length() < b->length(); });
}

void Pretreatment::remove_outer_border()
{
    // 找出外接正交矩形
    double rect_left = FLT_MAX, rect_right = -FLT_MAX, rect_top = -FLT_MAX, rect_bottom = FLT_MAX;
    Geo::Rectangle outer_rect;
    for (Container *container : _all_containers)
    {
        outer_rect = container->bounding_rect();
        rect_left = std::min(outer_rect.left(), rect_left);
        rect_right = std::max(outer_rect.right(), rect_right);
        rect_top = std::max(outer_rect.top(), rect_top);
        rect_bottom = std::min(outer_rect.bottom(), rect_bottom);
    }
    for (Geo::Polyline *polyline : _all_polylines)
    {
        outer_rect = polyline->bounding_rect();
        rect_left = std::min(outer_rect.left(), rect_left);
        rect_right = std::max(outer_rect.right(), rect_right);
        rect_top = std::max(outer_rect.top(), rect_top);
        rect_bottom = std::min(outer_rect.bottom(), rect_bottom);
    }
    outer_rect.reshape(rect_left, rect_top, rect_right, rect_bottom);
    
    // 找出四个边角点
    Geo::Point rect_top_left(rect_left, rect_top), rect_top_right(rect_right, rect_top),
        rect_bottom_right(rect_right, rect_bottom), rect_bottom_left(rect_left, rect_bottom);
    Geo::Point top_left(rect_top_left), top_right(rect_top_right), bottom_right(rect_bottom_right), bottom_left(rect_bottom_left);
    double top_left_distance = DBL_MAX, top_right_distance = DBL_MAX,
        bottom_right_distance = DBL_MAX, bottom_left_distance = DBL_MAX;
    for (Container *container : _all_containers)
    {
        for (const Geo::Point &point : container->shape())
        {
            if (Geo::distance(rect_top_left, point) < top_left_distance)
            {
                top_left_distance = Geo::distance(rect_top_left, point);
                top_left = point;
            }
            if (Geo::distance(rect_top_right, point) < top_right_distance)
            {
                top_right_distance = Geo::distance(rect_top_right, point);
                top_right = point;
            }
            if (Geo::distance(rect_bottom_right, point) < bottom_right_distance)
            {
                bottom_right_distance = Geo::distance(rect_bottom_right, point);
                bottom_right = point;
            }
            if (Geo::distance(rect_bottom_left, point) < bottom_left_distance)
            {
                bottom_left_distance = Geo::distance(rect_bottom_left, point);
                bottom_left = point;
            }
        }
    }
    for (Geo::Polyline *polyline : _all_polylines)
    {
        for (const Geo::Point &point : *polyline)
        {
            if (Geo::distance(rect_top_left, point) < top_left_distance)
            {
                top_left_distance = Geo::distance(rect_top_left, point);
                top_left = point;
            }
            if (Geo::distance(rect_top_right, point) < top_right_distance)
            {
                top_right_distance = Geo::distance(rect_top_right, point);
                top_right = point;
            }
            if (Geo::distance(rect_bottom_right, point) < bottom_right_distance)
            {
                bottom_right_distance = Geo::distance(rect_bottom_right, point);
                bottom_right = point;
            }
            if (Geo::distance(rect_bottom_left, point) < bottom_left_distance)
            {
                bottom_left_distance = Geo::distance(rect_bottom_left, point);
                bottom_left = point;
            }
        }
    }

    // 找出边框矩形
    double left = std::max(bottom_left.coord().x, top_left.coord().x),
        right = std::min(bottom_right.coord().x, top_right.coord().x),
        top = std::max(top_left.coord().y, top_right.coord().y),
        bottom = std::min(bottom_left.coord().y, bottom_right.coord().y);
    
    // 去除标题栏宽度
    std::vector<bool> deleted_list(_all_polylines.size(), false);
    for (size_t i = 0, count = _all_polylines.size(); i < count; ++i)
    {
        Geo::Rectangle rect(_all_polylines[i]->bounding_rect());
        if (_all_polylines[i]->size() == 4)
        {
            if (rect.heigh() > (top - bottom) * 0.95 && rect.width() < 16)
            {
                if (rect.center().coord().x > outer_rect.center().coord().x)
                {
                    right = std::min(right, rect.left());
                }
                else
                {
                    left = std::max(left, rect.right());
                }
                deleted_list[i] = true;
            }
        }
        else if (rect.width() * rect.heigh() < 120 && (rect.center().coord().y > top - 1 ||
            rect.center().coord().y < bottom + 1 || rect.center().coord().x < left + 1 ||
            rect.center().coord().x > right - 1))
        {
            deleted_list[i] = true;
        }
        
    }

    // 找出四个边角点
    Geo::Rectangle inner_rect(rect_left + 1, rect_top - 1, rect_right - 1, rect_bottom + 1);
    bool has_edge = false, is_bottom_left = false, is_top_right = false;
    for (Geo::Polyline *polyline : _all_polylines)
    {
        for (const Geo::Point &point : *polyline)
        {
            if (point.coord().x == left && point.coord().y == bottom)
            {
                is_bottom_left = true;
            }
            if (point.coord().x == right && point.coord().y == top)
            {
                is_top_right = true;
            }
            if (is_bottom_left || is_top_right)
            {
                break;
            }
        }
        if (is_bottom_left || is_top_right)
        {
            for (size_t i = 0, count = _all_polylines.size(); i < count; ++i)
            {
                if (deleted_list[i])
                {
                    continue;
                }

                Geo::Rectangle rect(_all_polylines[i]->bounding_rect());
                if (rect.right() < left || rect.left() > right || rect.bottom() > top || rect.top() < bottom)
                {
                    deleted_list[i] = true;
                }
                else if (rect.width() < 16 && rect.heigh() < 16 && (rect.center().coord().x < left ||
                    rect.center().coord().x > right || rect.center().coord().y < bottom || rect.center().coord().y > top))
                {
                    deleted_list[i] = true;
                }
            }
            break;
        }
    }

    // 删除边缘矩形
    for (size_t i = 0, count = _all_polylines.size(); i < count; ++i)
    {
        if (deleted_list[i])
        {
            continue;
        }

        if (_all_polylines[i]->size() == 2 && ((_all_polylines[i]->front().coord().y == top && _all_polylines[i]->back().coord().y == top)
            || (_all_polylines[i]->front().coord().y == bottom && _all_polylines[i]->back().coord().y == bottom)
            || (_all_polylines[i]->front().coord().x == left && _all_polylines[i]->back().coord().x == left)
            || (_all_polylines[i]->front().coord().x == right && _all_polylines[i]->back().coord().x == right)))
        {
            deleted_list[i] = true;
        }
        else if (std::all_of(_all_polylines[i]->begin(), _all_polylines[i]->end(),
            [&](const Geo::Point &point) { return !Geo::is_inside(point, inner_rect); }))
        {
            deleted_list[i] = true;
        }
    }

    std::list<size_t> recover_list;
    bool left_flag, right_flag;
    const double length = GlobalSetting::get_instance()->setting()["connect_distance"].toDouble();
    for (size_t i = 0, count = _all_polylines.size(); i < count; ++i)
    {
        if (deleted_list[i])
        {
            if (_all_polylines[i]->size() == 2 && (_all_polylines[i]->front().coord().y == _all_polylines[i]->back().coord().y ||
                _all_polylines[i]->front().coord().x == _all_polylines[i]->back().coord().x))
            {
                continue;
            }

            left_flag = right_flag = false;
            for (size_t j = 0; j < count; ++j)
            {
                if (deleted_list[j])
                {
                    continue;
                }

                if (Geo::distance(_all_polylines[i]->front(), _all_polylines[j]->front()) < length ||
                    Geo::distance(_all_polylines[i]->front(), _all_polylines[j]->back()) < length)
                {
                    left_flag = true;
                }
                if (Geo::distance(_all_polylines[i]->back(), _all_polylines[j]->front()) < length ||
                    Geo::distance(_all_polylines[i]->back(), _all_polylines[j]->back()) < length)
                {
                    right_flag = true;
                }
                if (left_flag || right_flag)
                {
                    recover_list.push_back(i);
                    break;
                }
            }
        }
        else
        {
            Geo::Rectangle rect(_all_polylines[i]->bounding_rect());
            if (rect.heigh() / (top - bottom) > 0.95 || rect.width() / (right - left) > 0.95) // 位于四角的边框
            {
                deleted_list[i] = true;
            }
        }
    }
    for (const size_t i : recover_list)
    {
        deleted_list[i] = false;
    }

    // 二次筛选
    if (is_bottom_left || is_top_right)
    {
        rect_left = FLT_MAX, rect_right = -FLT_MAX, rect_top = -FLT_MAX, rect_bottom = FLT_MAX;
        for (size_t i = 0, count = _all_polylines.size(); i < count; ++i)
        {
            outer_rect = _all_polylines[i]->bounding_rect();
            rect_left = std::min(outer_rect.left(), rect_left);
            rect_right = std::max(outer_rect.right(), rect_right);
            rect_bottom = std::min(outer_rect.bottom(), rect_bottom);
            rect_top = std::max(outer_rect.top(), rect_top);
        }
        outer_rect.reshape(rect_left, rect_top, rect_right, rect_bottom);
        bottom_left.coord().x = rect_left, bottom_left.coord().y = rect_bottom;
        top_left.coord().x = rect_left, top_left.coord().y = rect_top;
        top_right.coord().x = rect_right, top_right.coord().y = rect_top;
        bottom_right.coord().x = rect_right, bottom_right.coord().y = rect_bottom;
        inner_rect.reshape(rect_left + 1, rect_top - 1, rect_right - 1, rect_bottom + 1);
        top = rect_top - 1, bottom = rect_bottom + 1, left = rect_left + 1, right = rect_right - 1;

        for (size_t i = 0, count = _all_polylines.size(); i < count; ++i)
        {
            if (deleted_list[i])
            {
                continue;
            }

            if (_all_polylines[i]->size() == 4 && _all_polylines[i]->bounding_rect() == outer_rect)
            {
                if ((*_all_polylines[i])[0] == bottom_left && (*_all_polylines[i])[1] == top_left &&
                    (*_all_polylines[i])[2] == top_right && (*_all_polylines[i])[3] == bottom_right)
                {
                    deleted_list[i] = true;
                }
                else if ((*_all_polylines[i])[1] == bottom_left && (*_all_polylines[i])[2] == top_left &&
                    (*_all_polylines[i])[3] == top_right && (*_all_polylines[i])[0] == bottom_right)
                {
                    deleted_list[i] = true;
                }
                else if ((*_all_polylines[i])[2] == bottom_left && (*_all_polylines[i])[3] == top_left &&
                    (*_all_polylines[i])[0] == top_right && (*_all_polylines[i])[1] == bottom_right)
                {
                    deleted_list[i] = true;
                }
                else if ((*_all_polylines[i])[3] == bottom_left && (*_all_polylines[i])[0] == top_left &&
                    (*_all_polylines[i])[1] == top_right && (*_all_polylines[i])[2] == bottom_right)
                {
                    deleted_list[i] = true;
                }
                continue;
            }
        
            Geo::Rectangle rect(_all_polylines[i]->bounding_rect());
            if (rect.heigh() / (top - bottom) > 0.95 || rect.width() / (right - left) > 0.95)
            {
                deleted_list[i] = true;
            }
        }
    }

    if (std::count(deleted_list.begin(), deleted_list.end(), true) < deleted_list.size())
    {
        for (size_t i = _all_polylines.size() - 1; i > 0; --i)
        {
            if (deleted_list[i])
            {
                delete _all_polylines[i];
                _all_polylines.erase(_all_polylines.begin() + i);
            }
        }
        if (deleted_list.front())
        {
            delete _all_polylines.front();
            _all_polylines.erase(_all_polylines.begin());
        }
    }
}

void Pretreatment::combine()
{
    bool flag;
    for (size_t i = 0, count = _all_containers.size(); i < count; ++i)
    {
        flag = false;
        for (size_t j = i + 1; j < count; ++j)
        {
            if (std::all_of(_all_containers[j]->cbegin(), _all_containers[j]->cend(),
                [&](const Geo::Point &point) { return Geo::is_inside(point, _all_containers[i]->shape(), true); }))
            {
                if (!flag)
                {
                    flag = true;
                    _all_combinations.emplace_back(new Combination());
                    _all_combinations.back()->append(_all_containers[i]);
                }
                _all_combinations.back()->append(_all_containers[j]);
                _all_containers.erase(_all_containers.begin() + j--);
                --count;
            }
        }

        for (size_t j = 0, num = _all_circles.size(); j < num; ++j)
        {
            if (Geo::is_inside(_all_circles[j]->center(), _all_containers[i]->shape()) &&
                Geo::distance(_all_circles[j]->center(), _all_containers[i]->shape()) <= _all_circles[j]->radius())
            {
                if (!flag)
                {
                    flag = true;
                    _all_combinations.emplace_back(new Combination());
                    _all_combinations.back()->append(_all_containers[i]);
                }
                _all_combinations.back()->append(_all_circles[j]);
                _all_circles.erase(_all_circles.begin() + j--);
                --num;
            }
        }

        if (flag)
        {
            _all_containers.erase(_all_containers.begin() + i--);
            --count;
        }
    }

    for (size_t i = 0, count = _all_circles.size(); i < count; ++i)
    {
        flag = false;
        for (size_t j = i + 1; j < count; ++j)
        {
            if (_all_circles[j]->radius() < _all_circles[i]->radius() &&
                Geo::distance(_all_circles[i]->center(), _all_circles[j]->center()) <= _all_circles[j]->radius())
            {
                if (!flag)
                {
                    flag = true;
                    _all_combinations.emplace_back(new Combination());
                    _all_combinations.back()->append(_all_circles[i]);
                }
                _all_combinations.back()->append(_all_circles[j]);
                _all_circles.erase(_all_circles.begin() + j--);
                --count;
            }
        }

        for (size_t j = 0, num = _all_containers.size(); j < num; ++j)
        {
            if (std::all_of(_all_containers[j]->begin(), _all_containers[j]->end(),
                [&](const Geo::Point &point) { return Geo::is_inside(point, _all_circles[i]->shape(), true); }))
            {
                if (!flag)
                {
                    flag = true;
                    _all_combinations.emplace_back(new Combination());
                    _all_combinations.back()->append(_all_circles[i]);
                }
                _all_combinations.back()->append(_all_containers[j]);
                _all_containers.erase(_all_containers.begin() + j--);
                --num;
            }
        }

        if (flag)
        {
            _all_circles.erase(_all_circles.begin() + i--);
            --count;
        }
    }

    for (size_t i = 0, count = _all_combinations.size(); i < count; ++i)
    {
        for (size_t j = 0, num = _all_polylines.size(); j < num; ++j)
        {
            if (dynamic_cast<Container *>(_all_combinations[i]->front()) != nullptr &&
                std::all_of(_all_polylines[j]->begin(), _all_polylines[j]->end(),
                    [&](const Geo::Point &point) { return Geo::is_inside(point, reinterpret_cast<Container *>(_all_combinations[i]->front())->shape(), true); }))
            {
                _all_combinations[i]->append(_all_polylines[j]);
                _all_polylines.erase(_all_polylines.begin() + j--);
                --num;
            }
            else if (dynamic_cast<CircleContainer *>(_all_combinations[i]->front()) != nullptr &&
                std::all_of(_all_polylines[j]->begin(), _all_polylines[j]->end(),
                    [&](const Geo::Point &point) { return Geo::is_inside(point, dynamic_cast<CircleContainer *>(_all_combinations[i]->front())->shape(), true); }))
            {
                _all_combinations[i]->insert(0, _all_polylines[j]);
                _all_polylines.erase(_all_polylines.begin() + j--);
                --num;
            }
        }

        for (size_t j = 0, num = _all_beziers.size(); j < num; ++j)
        {
            if (dynamic_cast<Container *>(_all_combinations[i]->front()) != nullptr &&
                std::all_of(_all_beziers[j]->begin(), _all_beziers[j]->end(),
                    [&](const Geo::Point &point) { return Geo::is_inside(point, reinterpret_cast<Container *>(_all_combinations[i]->front())->shape(), true); }))
            {
                _all_combinations[i]->append(_all_beziers[j]);
                _all_beziers.erase(_all_beziers.begin() + j--);
                --num;
            }
            else if (dynamic_cast<CircleContainer *>(_all_combinations[i]->front()) != nullptr &&
                std::all_of(_all_beziers[j]->begin(), _all_beziers[j]->end(),
                    [&](const Geo::Point &point) { return Geo::is_inside(point, dynamic_cast<CircleContainer *>(_all_combinations[i]->front())->shape(), true); }))
            {
                _all_combinations[i]->insert(0, _all_beziers[j]);
                _all_beziers.erase(_all_beziers.begin() + j--);
                --num;
            }
        }
    }

    for (Combination *combination : _all_combinations)
    {
        for (Geo::Geometry *geo : *combination)
        {
            geo->memo()["is_selected"] = true;
        }
    }
}

void Pretreatment::finish()
{
    for (Link *link : _all_links)
    {
        _graph->back().append(link);
    }
    for (Container *container : _all_containers)
    {
        _graph->back().append(container);
    }
    for (CircleContainer *circle : _all_circles)
    {
        _graph->back().append(circle);
    }
    for (Geo::Polyline *polyline : _all_polylines)
    {
        _graph->back().append(polyline);
    }
    for (Geo::Bezier *bezier : _all_beziers)
    {
        _graph->back().append(bezier);
    }
    for (Combination *combination : _all_combinations)
    {
        combination->update_border();
        _graph->back().append(combination);
    }
    _all_links.clear();
    _all_containers.clear();
    _all_circles.clear();
    _all_polylines.clear();
    _all_beziers.clear();
    _all_combinations.clear();
}

/* const bool Pretreatment::is_closed(const Geo::Polyline &polyline)
{
    const Geo::Point center(polyline.bounding_rect().center());
    size_t x_count = 0, y_count = 0;

    Geo::Point point, next_point;
    for (size_t i = 0, count = polyline.size() - 1; i < count; ++i)
    {
        point = polyline[i];
        next_point = polyline[i + 1];

        if ((point.coord().y >= center.coord().y && next_point.coord().y < center.coord().y) ||
            (point.coord().y <= center.coord().y && next_point.coord().y > center.coord().y))
        {
            ++x_count;
        }
        if ((point.coord().x >= center.coord().x && next_point.coord().x < center.coord().x) ||
            (point.coord().x <= center.coord().x && next_point.coord().x > center.coord().x))
        {
            ++y_count;
        }
    }

    if (x_count > 0 && y_count > 0 && x_count % 2 == 0 && y_count % 2 == 0)
    {
        return true;
    }
    else
    {
        return polyline.size() > 3 && Geo::distance(polyline.front(), polyline.back()) <
                GlobalSetting::get_instance()->setting()["connect_distance"].toDouble();
    }
} */