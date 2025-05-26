#include <limits>

#include "io/DXFReaderWriter.hpp"
#include "io/GlobalSetting.hpp"
#include "base/Algorithm.hpp"


Bulge::Bulge(const Geo::Point &s, const Geo::Point &e, const double t)
    :start(s), end(e), tangent(t) {}  

Bulge::Bulge(const double x0, const double y0, const double x1, const double y1, const double t)
    :start(x0, y0), end(x1, y1), tangent(t) {}

double Bulge::radius() const
{
	return (Geo::distance(start, end) * (1 + std::pow(tangent, 2))) / (4 * std::abs(tangent));
}

Geo::Point Bulge::relative_center() const
{
    const Geo::Point line = end - start;
	const Geo::Point half_line = line / 2;
	const double line_to_center_angle = Geo::PI / 2 - std::atan(tangent) * 2;
	if (std::abs(line_to_center_angle) < Geo::EPSILON)
    {
		return half_line;
	}
	else
    {
		return half_line + Geo::Point(-half_line.y, half_line.x) * std::tan(line_to_center_angle);
	}
}

Geo::Point Bulge::center() const
{
    return start + relative_center();
}

double Bulge::start_angle() const
{
    return Geo::angle(center(), start);
}

double Bulge::end_angle() const
{
    return Geo::angle(center(), end);
}

bool Bulge::is_cw() const
{
    return tangent < 0;
}

bool Bulge::is_line() const
{
    return std::abs(tangent) < std::numeric_limits<double>::epsilon();
}


DXFReaderWriter::DXFReaderWriter(Graph *graph)
    : _graph(graph)
{}

DXFReaderWriter::~DXFReaderWriter()
{
    check_block();
    clear_empty_group();
}

void DXFReaderWriter::addHeader(const DRW_Header *data)
{}

void DXFReaderWriter::addLType(const DRW_LType &data)
{
    _handle_pairs.insert_or_assign(data.handle, data.parentHandle);
}

void DXFReaderWriter::addLayer(const DRW_Layer &data)
{
    _handle_pairs.insert_or_assign(data.handle, data.parentHandle);
    const QString name = QString::fromUtf8(data.name.c_str());
    if (name != "0" && _graph->has_group(name))
    {
        return;
    }

    _graph->append_group();
    ContainerGroup &group = _graph->container_groups().back();
    group.name = QString::fromUtf8(data.name.c_str());
    if (!data.plotF || (data.flags & 0x01))
    {
        group.hide();
    }
}

void DXFReaderWriter::addDimStyle(const DRW_Dimstyle &data)
{
    _handle_pairs.insert_or_assign(data.handle, data.parentHandle);
}

void DXFReaderWriter::addVport(const DRW_Vport &data)
{
    _handle_pairs.insert_or_assign(data.handle, data.parentHandle);
}

void DXFReaderWriter::addTextStyle(const DRW_Textstyle &data)
{
    _handle_pairs.insert_or_assign(data.handle, data.parentHandle);
}

void DXFReaderWriter::addAppId(const DRW_AppId &data)
{
    _handle_pairs.insert_or_assign(data.handle, data.parentHandle);
}

void DXFReaderWriter::addBlock(const DRW_Block &data)
{
    _handle_pairs.insert_or_assign(data.handle, data.parentHandle);
    _to_graph = false;
    _ignore_entity = (false || !data.visible || data.name.empty() || data.name.front() == '*');
    const QString name = QString::fromUtf8(data.name.c_str());
    const QString mid = name.mid(1,11);
    if (!_ignore_entity && mid.toLower() != "paper_space" && mid.toLower() != "model_space")
    {
        QString layer_name = QString::fromStdString(data.layer);
        _combination = new Combination();
        _block_map1.insert_or_assign(_combination, data.handle);
        _block_names.insert_or_assign(_combination, name.toStdString());
        for (ContainerGroup &group : _graph->container_groups())
        {
            if (group.name == layer_name)
            {
                return group.append(_combination);
            }
        }
        _graph->append_group();
        _graph->container_groups().back().name = layer_name;
        _graph->container_groups().back().append(_combination);
    }
    else
    {
        _ignore_entity = true;
    }
}

void DXFReaderWriter::setBlock(const int handle)
{}

void DXFReaderWriter::endBlock()
{
    if (_version !=1009 && _combination != nullptr && _combination->name.startsWith('*'))
    {
        _block_map1.erase(_combination);
        _block_names.erase(_combination);
        _graph->remove_object(_combination);
    }
    else if (_combination != nullptr && _combination->empty())
    {
        _block_map1.erase(_combination);
        _block_names.erase(_combination);
        _graph->remove_object(_combination);
    }
    _combination = nullptr;
    _to_graph = true;
    _ignore_entity = false;
}

void DXFReaderWriter::addPoint(const DRW_Point &data)
{
    _handle_pairs.insert_or_assign(data.handle, data.parentHandle);
}

void DXFReaderWriter::addLine(const DRW_Line &data)
{
    _handle_pairs.insert_or_assign(data.handle, data.parentHandle);
    if (_ignore_entity)
    {
        return;
    }
    if (_to_graph)
    {
        for (ContainerGroup &group : _graph->container_groups())
        {
            if (group.name.toStdString() == data.layer)
            {
                group.append(new Geo::Polyline({{data.basePoint.x, data.basePoint.y},
                    {data.secPoint.x, data.secPoint.y}}));
                _object_map[group.back()] = data.handle;
                return;
            }
        }
        _graph->append_group();
        _graph->container_groups().back().name = QString::fromStdString(data.layer);
        _graph->container_groups().back().append(new Geo::Polyline({
            {data.basePoint.x, data.basePoint.y}, {data.secPoint.x, data.secPoint.y}}));
        _object_map[_graph->container_groups().back().back()] = data.handle;
    }
    else
    {
        _combination->append(new Geo::Polyline({{data.basePoint.x, data.basePoint.y},
            {data.secPoint.x, data.secPoint.y}}));
        _object_map[_combination->back()] = data.handle;
    }
}

void DXFReaderWriter::addRay(const DRW_Ray &data)
{
    _handle_pairs.insert_or_assign(data.handle, data.parentHandle);
    if (_ignore_entity)
    {
        return;
    }
    if (_to_graph)
    {
        for (ContainerGroup &group : _graph->container_groups())
        {
            if (group.name.toStdString() == data.layer)
            {
                group.append(new Geo::Polyline({{data.basePoint.x, data.basePoint.y},
                    {data.basePoint.x + data.secPoint.x, data.basePoint.y + data.secPoint.y}}));
                _object_map[group.back()] = data.handle;
                return;
            }
        }
        _graph->append_group();
        _graph->container_groups().back().name = QString::fromStdString(data.layer);
        _graph->container_groups().back().append(new Geo::Polyline({{data.basePoint.x, data.basePoint.y},
            {data.basePoint.x + data.secPoint.x, data.basePoint.y + data.secPoint.y}}));
        _object_map[_graph->container_groups().back().back()] = data.handle;
    }
    else
    {
        _combination->append(new Geo::Polyline({{data.basePoint.x, data.basePoint.y},
            {data.secPoint.x, data.secPoint.y}}));
        _object_map[_combination] = data.handle;
    }
}

void DXFReaderWriter::addXline(const DRW_Xline &data)
{
    _handle_pairs.insert_or_assign(data.handle, data.parentHandle);
    if (_ignore_entity)
    {
        return;
    }
    if (_to_graph)
    {
        for (ContainerGroup &group : _graph->container_groups())
        {
            if (group.name.toStdString() == data.layer)
            {
                group.append(new Geo::Polyline({{data.basePoint.x, data.basePoint.y},
                    {data.basePoint.x + data.secPoint.x, data.basePoint.y + data.secPoint.y}}));
                _object_map[group.back()] = data.handle;
                return;
            }
        }
        _graph->append_group();
        _graph->container_groups().back().name = QString::fromStdString(data.layer);
        _graph->container_groups().back().append(new Geo::Polyline({{data.basePoint.x, data.basePoint.y},
            {data.basePoint.x + data.secPoint.x, data.basePoint.y + data.secPoint.y}}));
        _object_map[_graph->container_groups().back().back()] = data.handle;
    }
    else
    {
        _combination->append(new Geo::Polyline({{data.basePoint.x, data.basePoint.y},
            {data.basePoint.x + data.secPoint.x, data.basePoint.y + data.secPoint.y}}));
        _object_map[_combination] = data.handle;
    }
}

void DXFReaderWriter::addArc(const DRW_Arc &data)
{
    _handle_pairs.insert_or_assign(data.handle, data.parentHandle);
    if (_ignore_entity)
    {
        return;
    }
    if (_to_graph)
    {
        for (ContainerGroup &group : _graph->container_groups())
        {
            if (group.name.toStdString() == data.layer)
            {
                Geo::Polyline *polyline = new Geo::Polyline(Geo::arc_to_polyline(Geo::Point(data.basePoint.x, data.basePoint.y), 
                    data.radious, data.staangle, data.endangle, !data.isccw, Geo::Circle::default_down_sampling_value));
                group.append(polyline);
                _object_map[polyline] = data.handle;
                return;
            }
        }
        _graph->append_group();
        _graph->container_groups().back().name = QString::fromStdString(data.layer);
        Geo::Polyline *polyline = new Geo::Polyline(Geo::arc_to_polyline(Geo::Point(data.basePoint.x,
            data.basePoint.y), data.radious, data.staangle, data.endangle, !data.isccw, Geo::Circle::default_down_sampling_value));
        _graph->container_groups().back().append(polyline);
        _object_map[polyline] = data.handle;
    }
    else
    {
        Geo::Polyline *polyline = new Geo::Polyline(Geo::arc_to_polyline(Geo::Point(data.basePoint.x, data.basePoint.y),
            data.radious, data.staangle, data.endangle, !data.isccw, Geo::Circle::default_down_sampling_value));
        _combination->append(polyline);
        _object_map[polyline] = data.handle;
    }
}

void DXFReaderWriter::addCircle(const DRW_Circle &data)
{
    _handle_pairs.insert_or_assign(data.handle, data.parentHandle);
    if (_ignore_entity)
    {
        return;
    }
    if (_to_graph)
    {
        for (ContainerGroup &group : _graph->container_groups())
        {
            if (group.name.toStdString() == data.layer)
            {
                group.append(new Container<Geo::Circle>(QString(), data.basePoint.x, data.basePoint.y, data.radious));
                _object_map[group.back()] = data.handle;
                return;
            }
        }
        _graph->append_group();
        _graph->container_groups().back().name = QString::fromStdString(data.layer);
        _graph->container_groups().back().append(new Container<Geo::Circle>(QString(),
            data.basePoint.x, data.basePoint.y, data.radious));
        _object_map[_graph->container_groups().back().back()] = data.handle;
    }
    else
    {
        _combination->append(new Container<Geo::Circle>(QString(), data.basePoint.x, data.basePoint.y, data.radious));
        _object_map[_combination->back()] = data.handle;
    }
}

void DXFReaderWriter::addEllipse(const DRW_Ellipse &data)
{
    _handle_pairs.insert_or_assign(data.handle, data.parentHandle);
    if (_ignore_entity)
    {
        return;
    }
    if (_to_graph)
    {
        for (ContainerGroup &group : _graph->container_groups())
        {
            if (group.name.toStdString() == data.layer)
            {
                const double a = std::hypot(data.secPoint.x, data.secPoint.y);
                group.append(new Container<Geo::Ellipse>(QString(), data.basePoint.x, data.basePoint.y, a, a * data.ratio));
                _object_map[group.back()] = data.handle;
                return;
            }
        }
        _graph->append_group();
        _graph->container_groups().back().name = QString::fromStdString(data.layer);
        const double a = std::hypot(data.secPoint.x, data.secPoint.y);
        _graph->container_groups().back().append(new Container<Geo::Ellipse>(QString(),
            data.basePoint.x, data.basePoint.y, a, a * data.ratio));
        _object_map[_graph->container_groups().back().back()] = data.handle;
    }
    else
    {
        const double a = std::hypot(data.secPoint.x, data.secPoint.y);
        _combination->append(new Container<Geo::Ellipse>(QString(), data.basePoint.x, data.basePoint.y, a, a * data.ratio));
        _object_map[_combination->back()] = data.handle;
    }
}

void DXFReaderWriter::addLWPolyline(const DRW_LWPolyline &data)
{
    _handle_pairs.insert_or_assign(data.handle, data.parentHandle);
    if (_ignore_entity || data.vertlist.empty())
    {
        return;
    }
    if (_to_graph)
    {
        for (ContainerGroup &group : _graph->container_groups())
        {
            if (group.name.toStdString() == data.layer)
            {
                Geo::Polyline *polyline = new Geo::Polyline();
                for (size_t i = 1, count = data.vertlist.size(); i < count; ++i)
                {
                    const Bulge bulge(data.vertlist[i - 1]->x, data.vertlist[i - 1]->y,
                        data.vertlist[i]->x, data.vertlist[i]->y, data.vertlist[i - 1]->bulge);
                    if (bulge.is_line())
                    {
                        polyline->append(Geo::Point(data.vertlist[i - 1]->x, data.vertlist[i - 1]->y));
                        polyline->append(Geo::Point(data.vertlist[i]->x, data.vertlist[i]->y));
                    }
                    else
                    {
                        const Geo::Point center(bulge.center());
                        polyline->append(Geo::arc_to_polyline(center, bulge.radius(), Geo::angle(center, bulge.start), 
                            Geo::angle(center, bulge.end), bulge.is_cw(), Geo::Circle::default_down_sampling_value));
                    }
                }
                if (data.flags & 0x1)
                {
                    const Bulge bulge(data.vertlist.back()->x, data.vertlist.back()->y,
                        data.vertlist.front()->x, data.vertlist.front()->y, data.vertlist.back()->bulge);
                    if (bulge.is_line())
                    {
                        polyline->append(Geo::Point(data.vertlist.back()->x, data.vertlist.back()->y));
                        polyline->append(Geo::Point(data.vertlist.front()->x, data.vertlist.front()->y));
                    }
                    else
                    {
                        const Geo::Point center(bulge.center());
                        polyline->append(Geo::arc_to_polyline(center, bulge.radius(), Geo::angle(center, bulge.start), 
                            Geo::angle(center, bulge.end), bulge.is_cw(), Geo::Circle::default_down_sampling_value));
                    }
                }
                group.append(polyline);
                _object_map[polyline] = data.handle;
                return;
            }
        }
        _graph->append_group();
        _graph->container_groups().back().name = QString::fromStdString(data.layer);
        Geo::Polyline *polyline = new Geo::Polyline();
        for (size_t i = 1, count = data.vertlist.size(); i < count; ++i)
        {
            const Bulge bulge(data.vertlist[i - 1]->x, data.vertlist[i - 1]->y,
                data.vertlist[i]->x, data.vertlist[i]->y, data.vertlist[i - 1]->bulge);
            if (bulge.is_line())
            {
                polyline->append(Geo::Point(data.vertlist[i - 1]->x, data.vertlist[i - 1]->y));
                polyline->append(Geo::Point(data.vertlist[i]->x, data.vertlist[i]->y));
            }
            else
            {
                const Geo::Point center(bulge.center());
                polyline->append(Geo::arc_to_polyline(center, bulge.radius(), Geo::angle(center, bulge.start),
                    Geo::angle(center, bulge.end), bulge.is_cw(), Geo::Circle::default_down_sampling_value));
            }
        }
        if (data.flags & 0x1)
        {
            const Bulge bulge(data.vertlist.back()->x, data.vertlist.back()->y,
                data.vertlist.front()->x, data.vertlist.front()->y, data.vertlist.back()->bulge);
            if (bulge.is_line())
            {
                polyline->append(Geo::Point(data.vertlist.back()->x, data.vertlist.back()->y));
                polyline->append(Geo::Point(data.vertlist.front()->x, data.vertlist.front()->y));
            }
            else
            {
                const Geo::Point center(bulge.center());
                polyline->append(Geo::arc_to_polyline(center, bulge.radius(), Geo::angle(center, bulge.start),
                    Geo::angle(center, bulge.end), bulge.is_cw(), Geo::Circle::default_down_sampling_value));
            }
        }
        _graph->container_groups().back().append(polyline);
        _object_map[polyline] = data.handle;
    }
    else
    {
        Geo::Polyline *polyline = new Geo::Polyline();
        for (size_t i = 1, count = data.vertlist.size(); i < count; ++i)
        {
            const Bulge bulge(data.vertlist[i - 1]->x, data.vertlist[i - 1]->y,
                data.vertlist[i]->x, data.vertlist[i]->y, data.vertlist[i - 1]->bulge);
            if (bulge.is_line())
            {
                polyline->append(Geo::Point(data.vertlist[i - 1]->x, data.vertlist[i - 1]->y));
                polyline->append(Geo::Point(data.vertlist[i]->x, data.vertlist[i]->y));
            }
            else
            {
                const Geo::Point center(bulge.center());
                polyline->append(Geo::arc_to_polyline(center, bulge.radius(), Geo::angle(center, bulge.start),
                    Geo::angle(center, bulge.end), bulge.is_cw(), Geo::Circle::default_down_sampling_value));
            }
        }
        if (data.flags & 0x1)
        {
            const Bulge bulge(data.vertlist.back()->x, data.vertlist.back()->y,
                data.vertlist.front()->x, data.vertlist.front()->y, data.vertlist.back()->bulge);
            if (bulge.is_line())
            {
                polyline->append(Geo::Point(data.vertlist.back()->x, data.vertlist.back()->y));
                polyline->append(Geo::Point(data.vertlist.front()->x, data.vertlist.front()->y));
            }
            else
            {
                const Geo::Point center(bulge.center());
                polyline->append(Geo::arc_to_polyline(center, bulge.radius(), Geo::angle(center, bulge.start), 
                    Geo::angle(center, bulge.end), bulge.is_cw(), Geo::Circle::default_down_sampling_value));
            }
        }
        _combination->append(polyline);
        _object_map[polyline] = data.handle;
    }
}

void DXFReaderWriter::addPolyline(const DRW_Polyline &data)
{
    _handle_pairs.insert_or_assign(data.handle, data.parentHandle);
    if (_ignore_entity)
    {
        return;
    }
    if (data.flags & 0x10 || data.flags & 0x40 || data.vertlist.empty())
    {
        return;
    }

    if (_to_graph)
    {
        for (ContainerGroup &group : _graph->container_groups())
        {
            if (group.name.toStdString() == data.layer)
            {
                Geo::Polyline *polyline = new Geo::Polyline();
                for (size_t i = 1, count = data.vertlist.size(); i < count; ++i)
                {
                    const Bulge bulge(data.vertlist[i - 1]->basePoint.x, data.vertlist[i - 1]->basePoint.y,
                        data.vertlist[i]->basePoint.x, data.vertlist[i]->basePoint.y, data.vertlist[i - 1]->bulge);
                    if (bulge.is_line())
                    {
                        polyline->append(Geo::Point(data.vertlist[i - 1]->basePoint.x, data.vertlist[i - 1]->basePoint.y));
                        polyline->append(Geo::Point(data.vertlist[i]->basePoint.x, data.vertlist[i]->basePoint.y));
                    }
                    else
                    {
                        const Geo::Point center(bulge.center());
                        polyline->append(Geo::arc_to_polyline(center, bulge.radius(), Geo::angle(center, bulge.start),
                            Geo::angle(center, bulge.end), bulge.is_cw(), Geo::Circle::default_down_sampling_value));
                    }
                }
                if (data.flags & 0x1)
                {
                    const Bulge bulge(data.vertlist.back()->basePoint.x, data.vertlist.back()->basePoint.y,
                        data.vertlist.front()->basePoint.x, data.vertlist.front()->basePoint.y, data.vertlist.back()->bulge);
                    if (bulge.is_line())
                    {
                        polyline->append(Geo::Point(data.vertlist.back()->basePoint.x, data.vertlist.back()->basePoint.y));
                        polyline->append(Geo::Point(data.vertlist.front()->basePoint.x, data.vertlist.front()->basePoint.y));
                    }
                    else
                    {
                        const Geo::Point center(bulge.center());
                        polyline->append(Geo::arc_to_polyline(center, bulge.radius(), Geo::angle(center, bulge.start),
                            Geo::angle(center, bulge.end), bulge.is_cw(), Geo::Circle::default_down_sampling_value));
                    }
                }
                group.append(polyline);
                _object_map[polyline] = data.handle;
                return;
            }
        }
        _graph->append_group();
        _graph->container_groups().back().name = QString::fromStdString(data.layer);
        Geo::Polyline *polyline = new Geo::Polyline();
        for (size_t i = 1, count = data.vertlist.size(); i < count; ++i)
        {
            const Bulge bulge(data.vertlist[i - 1]->basePoint.x, data.vertlist[i - 1]->basePoint.y,
                data.vertlist[i]->basePoint.x, data.vertlist[i]->basePoint.y, data.vertlist[i - 1]->bulge);
            if (bulge.is_line())
            {
                polyline->append(Geo::Point(data.vertlist[i - 1]->basePoint.x, data.vertlist[i - 1]->basePoint.y));
                polyline->append(Geo::Point(data.vertlist[i]->basePoint.x, data.vertlist[i]->basePoint.y));
            }
            else
            {
                const Geo::Point center(bulge.center());
                polyline->append(Geo::arc_to_polyline(center, bulge.radius(), Geo::angle(center, bulge.start),
                    Geo::angle(center, bulge.end), bulge.is_cw(), Geo::Circle::default_down_sampling_value));
            }
        }
        if (data.flags & 0x1)
        {
            const Bulge bulge(data.vertlist.back()->basePoint.x, data.vertlist.back()->basePoint.y,
                data.vertlist.front()->basePoint.x, data.vertlist.front()->basePoint.y, data.vertlist.back()->bulge);
            if (bulge.is_line())
            {
                polyline->append(Geo::Point(data.vertlist.back()->basePoint.x, data.vertlist.back()->basePoint.y));
                polyline->append(Geo::Point(data.vertlist.front()->basePoint.x, data.vertlist.front()->basePoint.y));
            }
            else
            {
                const Geo::Point center(bulge.center());
                polyline->append(Geo::arc_to_polyline(center, bulge.radius(), Geo::angle(center, bulge.start),
                    Geo::angle(center, bulge.end), bulge.is_cw(), Geo::Circle::default_down_sampling_value));
            }
        }
        _graph->container_groups().back().append(polyline);
        _object_map[polyline] = data.handle;
    }
    else
    {
        Geo::Polyline *polyline = new Geo::Polyline();
        for (size_t i = 1, count = data.vertlist.size(); i < count; ++i)
        {
            const Bulge bulge(data.vertlist[i - 1]->basePoint.x, data.vertlist[i - 1]->basePoint.y,
                data.vertlist[i]->basePoint.x, data.vertlist[i]->basePoint.y, data.vertlist[i - 1]->bulge);
            if (bulge.is_line())
            {
                polyline->append(Geo::Point(data.vertlist[i - 1]->basePoint.x, data.vertlist[i - 1]->basePoint.y));
                polyline->append(Geo::Point(data.vertlist[i]->basePoint.x, data.vertlist[i]->basePoint.y));
            }
            else
            {
                const Geo::Point center(bulge.center());
                polyline->append(Geo::arc_to_polyline(center, bulge.radius(), Geo::angle(center, bulge.start), 
                    Geo::angle(center, bulge.end), bulge.is_cw(), Geo::Circle::default_down_sampling_value));
            }
        }
        if (data.flags & 0x1)
        {
            const Bulge bulge(data.vertlist.back()->basePoint.x, data.vertlist.back()->basePoint.y,
                data.vertlist.front()->basePoint.x, data.vertlist.front()->basePoint.y, data.vertlist.back()->bulge);
            if (bulge.is_line())
            {
                polyline->append(Geo::Point(data.vertlist.back()->basePoint.x, data.vertlist.back()->basePoint.y));
                polyline->append(Geo::Point(data.vertlist.front()->basePoint.x, data.vertlist.front()->basePoint.y));
            }
            else
            {
                const Geo::Point center(bulge.center());
                polyline->append(Geo::arc_to_polyline(center, bulge.radius(), Geo::angle(center, bulge.start), 
                    Geo::angle(center, bulge.end), bulge.is_cw(), Geo::Circle::default_down_sampling_value));
            }
        }
        _combination->append(polyline);
        _object_map[polyline] = data.handle;
    }
}

void DXFReaderWriter::addSpline(const DRW_Spline *data)
{
    _handle_pairs.insert_or_assign(data->handle, data->parentHandle);
    if (_ignore_entity)
    {
        return;
    }
    if (data->degree == 2)
    {
        Geo::QuadBSpline *bspline = nullptr;
        std::vector<Geo::Point> points;
        if (data->ncontrol== 0)
        {
            for (const DRW_Coord *vert: data->fitlist)
            {
                points.emplace_back(vert->x, vert->y);
            }
            bspline = new Geo::QuadBSpline(points.begin(), points.end(), true);
        }
        else
        {
            for (const DRW_Coord *vert: data->controllist)
            {
                points.emplace_back(vert->x, vert->y);
            }
            bspline = new Geo::QuadBSpline(points.begin(), points.end(), data->knotslist, false);
        }
        if (_to_graph)
        {
            for (ContainerGroup &group : _graph->container_groups())
            {
                if (group.name.toStdString() == data->layer)
                {
                    group.append(bspline);
                    _object_map[bspline] = data->handle;
                    return;
                }
            }
            _graph->append_group();
            _graph->container_groups().back().name = QString::fromStdString(data->layer);
            _graph->container_groups().back().append(bspline);
            _object_map[bspline] = data->handle;
        }
        else
        {
            _combination->append(bspline);
            _object_map[bspline] = data->handle;
        }
    }
    else
    {
        Geo::CubicBSpline *bspline = nullptr;
        std::vector<Geo::Point> points;
        if (data->ncontrol== 0)
        {
            for (const DRW_Coord *vert: data->fitlist)
            {
                points.emplace_back(vert->x, vert->y);
            }
            bspline = new Geo::CubicBSpline(points.begin(), points.end(), true);
        }
        else
        {
            for (const DRW_Coord *vert: data->controllist)
            {
                points.emplace_back(vert->x, vert->y);
            }
            bspline = new Geo::CubicBSpline(points.begin(), points.end(), data->knotslist, false);
        }
        if (_to_graph)
        {
            for (ContainerGroup &group : _graph->container_groups())
            {
                if (group.name.toStdString() == data->layer)
                {
                    group.append(bspline);
                    _object_map[bspline] = data->handle;
                    return;
                }
            }
            _graph->append_group();
            _graph->container_groups().back().name = QString::fromStdString(data->layer);
            _graph->container_groups().back().append(bspline);
            _object_map[bspline] = data->handle;
        }
        else
        {
            _combination->append(bspline);
            _object_map[bspline] = data->handle;
        }
    }
}

void DXFReaderWriter::addKnot(const DRW_Entity &data)
{
    _handle_pairs.insert_or_assign(data.handle, data.parentHandle);
}

void DXFReaderWriter::addInsert(const DRW_Insert &data)
{
    if (_to_graph)
    {
        _inserted_blocks.insert(data.name);
        _handle_pairs.insert_or_assign(data.handle, data.parentHandle);
    }
}

void DXFReaderWriter::addTrace(const DRW_Trace &data)
{
    _handle_pairs.insert_or_assign(data.handle, data.parentHandle);
}

void DXFReaderWriter::add3dFace(const DRW_3Dface &data)
{
    _handle_pairs.insert_or_assign(data.handle, data.parentHandle);
}

void DXFReaderWriter::addSolid(const DRW_Solid &data)
{
    _handle_pairs.insert_or_assign(data.handle, data.parentHandle);
}

void DXFReaderWriter::addMText(const DRW_MText &data)
{
    _handle_pairs.insert_or_assign(data.handle, data.parentHandle);
    if (_ignore_entity || data.text.empty() || std::count(data.text.begin(), data.text.end(), ' ')  == data.text.size())
    {
        return;
    }
    if (_to_graph)
    {
        for (ContainerGroup &group : _graph->container_groups())
        {
            if (group.name.toStdString() == data.layer)
            {
                group.append(new Text(data.basePoint.x, data.basePoint.y,
                    GlobalSetting::get_instance()->setting["text_size"].toInt(), 
                    QString::fromUtf8(data.text.c_str())));
                _object_map[group.back()] = data.handle;
                return;
            }
        }
        _graph->append_group();
        _graph->container_groups().back().name = QString::fromStdString(data.layer);
        _graph->container_groups().back().append(new Text(data.basePoint.x, data.basePoint.y,
            GlobalSetting::get_instance()->setting["text_size"].toInt(), 
            QString::fromUtf8(data.text.c_str())));
        _object_map[_graph->container_groups().back().back()] = data.handle;
    }
    else
    {
        _combination->append(new Text(data.basePoint.x, data.basePoint.y,
            GlobalSetting::get_instance()->setting["text_size"].toInt(), QString::fromUtf8(data.text.c_str())));
        _object_map[_combination->back()] = data.handle;
    }
}

void DXFReaderWriter::addText(const DRW_Text &data)
{
    _handle_pairs.insert_or_assign(data.handle, data.parentHandle);
    if (_ignore_entity || data.text.empty() || std::count(data.text.begin(), data.text.end(), ' ')  == data.text.size())
    {
        return;
    }
    if (_to_graph)
    {
        for (ContainerGroup &group : _graph->container_groups())
        {
            if (group.name.toStdString() == data.layer)
            {
                group.append(new Text(data.basePoint.x, data.basePoint.y,
                    GlobalSetting::get_instance()->setting["text_size"].toInt(), 
                    QString::fromUtf8(data.text.c_str())));
                _object_map[group.back()] = data.handle;
                return;
            }
        }
        _graph->append_group();
        _graph->container_groups().back().name = QString::fromStdString(data.layer);
        _graph->container_groups().back().append(new Text(data.basePoint.x, data.basePoint.y,
            GlobalSetting::get_instance()->setting["text_size"].toInt(), 
            QString::fromUtf8(data.text.c_str())));
        _object_map[_graph->container_groups().back().back()] = data.handle;
    }
    else
    {
        _combination->append(new Text(data.basePoint.x, data.basePoint.y,
            GlobalSetting::get_instance()->setting["text_size"].toInt(), QString::fromUtf8(data.text.c_str())));
        _object_map[_combination->back()] = data.handle;
    }
}

void DXFReaderWriter::addDimAlign(const DRW_DimAligned *data)
{
    _handle_pairs.insert_or_assign(data->handle, data->parentHandle);
}

void DXFReaderWriter::addDimLinear(const DRW_DimLinear *data)
{
    _handle_pairs.insert_or_assign(data->handle, data->parentHandle);
}

void DXFReaderWriter::addDimRadial(const DRW_DimRadial *data)
{
    _handle_pairs.insert_or_assign(data->handle, data->parentHandle);
}

void DXFReaderWriter::addDimDiametric(const DRW_DimDiametric *data)
{
    _handle_pairs.insert_or_assign(data->handle, data->parentHandle);
}

void DXFReaderWriter::addDimAngular(const DRW_DimAngular *data)
{
    _handle_pairs.insert_or_assign(data->handle, data->parentHandle);
}

void DXFReaderWriter::addDimAngular3P(const DRW_DimAngular3p *data)
{
    _handle_pairs.insert_or_assign(data->handle, data->parentHandle);
}

void DXFReaderWriter::addDimOrdinate(const DRW_DimOrdinate *data)
{
    _handle_pairs.insert_or_assign(data->handle, data->parentHandle);
}

void DXFReaderWriter::addLeader(const DRW_Leader *data)
{
    _handle_pairs.insert_or_assign(data->handle, data->parentHandle);
}

void DXFReaderWriter::addHatch(const DRW_Hatch *data)
{
    _handle_pairs.insert_or_assign(data->handle, data->parentHandle);
}

void DXFReaderWriter::addViewport(const DRW_Viewport &data)
{
    _handle_pairs.insert_or_assign(data.handle, data.parentHandle);
}

void DXFReaderWriter::addImage(const DRW_Image *data)
{
    _handle_pairs.insert_or_assign(data->handle, data->parentHandle);
}

void DXFReaderWriter::linkImage(const DRW_ImageDef *data)
{
    _handle_pairs.insert_or_assign(data->handle, data->parentHandle);
}

void DXFReaderWriter::addComment(const char *comment)
{}

void DXFReaderWriter::writeHeader(DRW_Header &data)
{}

void DXFReaderWriter::writeBlocks()
{}

void DXFReaderWriter::writeBlockRecords()
{}

void DXFReaderWriter::writeEntities()
{}

void DXFReaderWriter::writeLTypes()
{}

void DXFReaderWriter::writeLayers()
{}

void DXFReaderWriter::writeTextstyles()
{}

void DXFReaderWriter::writeVports()
{}

void DXFReaderWriter::writeDimstyles()
{}

void DXFReaderWriter::writeAppId()
{}

void DXFReaderWriter::check_block()
{
    for (const std::pair<Combination *, std::string> &bpair : _block_names)
    {
        if (_inserted_blocks.find(bpair.second) == _inserted_blocks.end())
        {
            _block_map1.erase(bpair.first);
            _graph->remove_object(bpair.first);
        }
    }
    for (const std::pair<Combination *, int> &bpair : _block_map1)
    {
        for (size_t i = bpair.first->size() - 1; i > 0;--i)
        {
            if (_handle_pairs.find(_object_map[(*bpair.first)[i]]) == _handle_pairs.end())
            {
                bpair.first->remove(i);
            }
        }
        if (_handle_pairs.find(_object_map[bpair.first->front()]) == _handle_pairs.end())
        {
            bpair.first->remove(bpair.first->begin());
        }
        if (bpair.first->empty())
        {
            _graph->remove_object(bpair.first);
        }
        else
        {
            bpair.first->update_border();
        }
    }
}

void DXFReaderWriter::clear_empty_group()
{
    for (size_t i = 0, count = _graph->container_groups().size(); i < count; ++i)
    {
        if (_graph->container_group(i).empty())
        {
            _graph->remove_group(i--);
            --count;
        }
    }
}