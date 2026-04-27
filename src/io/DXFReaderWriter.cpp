#include <limits>
#include <QRegularExpression>
#include "io/DXFReaderWriter.hpp"
#include "io/GlobalSetting.hpp"
#include "base/Algorithm.hpp"


Bulge::Bulge(const Geo::Point &s, const Geo::Point &e, const double t) : start(s), end(e), tangent(t)
{
}

Bulge::Bulge(const double x0, const double y0, const double x1, const double y1, const double t) : start(x0, y0), end(x1, y1), tangent(t)
{
}

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


DXFReaderWriter::DXFReaderWriter(Graph *graph) : _graph(graph)
{
}

DXFReaderWriter::DXFReaderWriter(Graph *graph, dxfRW *dxfrw) : _graph(graph), _dxfrw(dxfrw)
{
}

DXFReaderWriter::~DXFReaderWriter()
{
    while (!_block_store.empty())
    {
        delete _block_store.back();
        _block_store.pop_back();
    }
    if (_dxfrw == nullptr)
    {
        clear_empty_group();
    }
}

void DXFReaderWriter::addHeader(const DRW_Header *data)
{
}

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
    std::string name;
    std::transform(data.name.begin(), data.name.end(), std::back_inserter(name), [](char c) { return std::tolower(c); });
    _text_style_font.insert_or_assign(name, data.font);
}

void DXFReaderWriter::addAppId(const DRW_AppId &data)
{
    _handle_pairs.insert_or_assign(data.handle, data.parentHandle);
}

void DXFReaderWriter::addBlock(const DRW_Block &data)
{
    _handle_pairs.insert_or_assign(data.handle, data.parentHandle);
    _to_graph = false;
    _ignore_entity = (!data.visible || data.name.empty());
    const QString name = QString::fromUtf8(data.name.c_str());
    const QString mid = name.mid(1, 11);
    if (!_ignore_entity && mid.toLower() != "paper_space" && mid.toLower() != "model_space")
    {
        QString layer_name = QString::fromStdString(data.layer);
        _combination = new Combination();
        _combination->name = name;
        _block_store.push_back(_combination);
        _block_map.insert_or_assign(_combination, data.handle);
        // _block_names.insert_or_assign(_combination, name.toStdString());
        _block_name_map.insert_or_assign(name.toStdString(), _combination);
        _combination->name = layer_name;
    }
    else
    {
        _ignore_entity = true;
    }
}

void DXFReaderWriter::setBlock(const int handle)
{
}

void DXFReaderWriter::endBlock()
{
    if (_combination != nullptr && _combination->empty())
    {
        _block_map.erase(_combination);
        _block_names.erase(_combination);
        _block_name_map.erase(_block_names[_combination]);
        _graph->remove_object(_combination);
    }
    _combination = nullptr;
    _to_graph = true;
    _ignore_entity = false;
}

void DXFReaderWriter::addPoint(const DRW_Point &data)
{
    _handle_pairs.insert_or_assign(data.handle, data.parentHandle);
    if (_ignore_entity || !data.visible)
    {
        return;
    }
    if (_to_graph)
    {
        for (ContainerGroup &group : _graph->container_groups())
        {
            if (group.name.toStdString() == data.layer)
            {
                group.append(new Geo::Point(data.basePoint.x, data.basePoint.y));
                if (data.extPoint.z < 0)
                {
                    group.back()->transform(_flip_by_y_mat);
                }
                _object_map[group.back()] = data.handle;
                _handle_map[data.handle] = group.back();
                return;
            }
        }
        _graph->append_group();
        _graph->container_groups().back().name = QString::fromStdString(data.layer);
        _graph->container_groups().back().append(new Geo::Point(data.basePoint.x, data.basePoint.y));
        if (data.extPoint.z < 0)
        {
            _graph->container_groups().back().back()->transform(_flip_by_y_mat);
        }
        _object_map[_graph->container_groups().back().back()] = data.handle;
        _handle_map[data.handle] = _graph->container_groups().back().back();
    }
    else
    {
        _combination->append(new Geo::Point(data.basePoint.x, data.basePoint.y));
        if (data.extPoint.z < 0)
        {
            _combination->back()->transform(_flip_by_y_mat);
        }
        _object_map[_combination->back()] = data.handle;
        _handle_map[data.handle] = _combination->back();
    }
}

void DXFReaderWriter::addLine(const DRW_Line &data)
{
    _handle_pairs.insert_or_assign(data.handle, data.parentHandle);
    if (_ignore_entity || !data.visible)
    {
        return;
    }
    if (_to_graph)
    {
        for (ContainerGroup &group : _graph->container_groups())
        {
            if (group.name.toStdString() == data.layer)
            {
                group.append(new Geo::Polyline({{data.basePoint.x, data.basePoint.y}, {data.secPoint.x, data.secPoint.y}}));
                if (data.extPoint.z < 0)
                {
                    group.back()->transform(_flip_by_y_mat);
                }
                _object_map[group.back()] = data.handle;
                _handle_map[data.handle] = group.back();
                return;
            }
        }
        _graph->append_group();
        _graph->container_groups().back().name = QString::fromStdString(data.layer);
        _graph->container_groups().back().append(
            new Geo::Polyline({{data.basePoint.x, data.basePoint.y}, {data.secPoint.x, data.secPoint.y}}));
        if (data.extPoint.z < 0)
        {
            _graph->container_groups().back().back()->transform(_flip_by_y_mat);
        }
        _object_map[_graph->container_groups().back().back()] = data.handle;
        _handle_map[data.handle] = _graph->container_groups().back().back();
    }
    else
    {
        _combination->append(new Geo::Polyline({{data.basePoint.x, data.basePoint.y}, {data.secPoint.x, data.secPoint.y}}));
        if (data.extPoint.z < 0)
        {
            _combination->back()->transform(_flip_by_y_mat);
        }
        _object_map[_combination->back()] = data.handle;
        _handle_map[data.handle] = _combination->back();
    }
}

void DXFReaderWriter::addRay(const DRW_Ray &data)
{
    _handle_pairs.insert_or_assign(data.handle, data.parentHandle);
    if (_ignore_entity || !data.visible)
    {
        return;
    }
    if (_to_graph)
    {
        for (ContainerGroup &group : _graph->container_groups())
        {
            if (group.name.toStdString() == data.layer)
            {
                group.append(new Geo::Polyline(
                    {{data.basePoint.x, data.basePoint.y}, {data.basePoint.x + data.secPoint.x, data.basePoint.y + data.secPoint.y}}));
                if (data.extPoint.z < 0)
                {
                    group.back()->transform(_flip_by_y_mat);
                }
                _object_map[group.back()] = data.handle;
                _handle_map[data.handle] = group.back();
                return;
            }
        }
        _graph->append_group();
        _graph->container_groups().back().name = QString::fromStdString(data.layer);
        _graph->container_groups().back().append(new Geo::Polyline(
            {{data.basePoint.x, data.basePoint.y}, {data.basePoint.x + data.secPoint.x, data.basePoint.y + data.secPoint.y}}));
        if (data.extPoint.z < 0)
        {
            _graph->container_groups().back().back()->transform(_flip_by_y_mat);
        }
        _object_map[_graph->container_groups().back().back()] = data.handle;
        _handle_map[data.handle] = _graph->container_groups().back().back();
    }
    else
    {
        _combination->append(new Geo::Polyline({{data.basePoint.x, data.basePoint.y}, {data.secPoint.x, data.secPoint.y}}));
        if (data.extPoint.z < 0)
        {
            _combination->back()->transform(_flip_by_y_mat);
        }
        _object_map[_combination] = data.handle;
        _handle_map[data.handle] = _combination;
    }
}

void DXFReaderWriter::addXline(const DRW_Xline &data)
{
    _handle_pairs.insert_or_assign(data.handle, data.parentHandle);
    if (_ignore_entity || !data.visible)
    {
        return;
    }
    if (_to_graph)
    {
        for (ContainerGroup &group : _graph->container_groups())
        {
            if (group.name.toStdString() == data.layer)
            {
                group.append(new Geo::Polyline(
                    {{data.basePoint.x, data.basePoint.y}, {data.basePoint.x + data.secPoint.x, data.basePoint.y + data.secPoint.y}}));
                if (data.extPoint.z < 0)
                {
                    group.back()->transform(_flip_by_y_mat);
                }
                _object_map[group.back()] = data.handle;
                _handle_map[data.handle] = group.back();
                return;
            }
        }
        _graph->append_group();
        _graph->container_groups().back().name = QString::fromStdString(data.layer);
        _graph->container_groups().back().append(new Geo::Polyline(
            {{data.basePoint.x, data.basePoint.y}, {data.basePoint.x + data.secPoint.x, data.basePoint.y + data.secPoint.y}}));
        if (data.extPoint.z < 0)
        {
            _graph->container_groups().back().back()->transform(_flip_by_y_mat);
        }
        _object_map[_graph->container_groups().back().back()] = data.handle;
        _handle_map[data.handle] = _graph->container_groups().back().back();
    }
    else
    {
        _combination->append(new Geo::Polyline(
            {{data.basePoint.x, data.basePoint.y}, {data.basePoint.x + data.secPoint.x, data.basePoint.y + data.secPoint.y}}));
        if (data.extPoint.z < 0)
        {
            _combination->back()->transform(_flip_by_y_mat);
        }
        _object_map[_combination] = data.handle;
        _handle_map[data.handle] = _combination;
    }
}

void DXFReaderWriter::addArc(const DRW_Arc &data)
{
    _handle_pairs.insert_or_assign(data.handle, data.parentHandle);
    if (_ignore_entity || !data.visible)
    {
        return;
    }
    if (_to_graph)
    {
        for (ContainerGroup &group : _graph->container_groups())
        {
            if (group.name.toStdString() == data.layer)
            {
                Geo::Arc *arc = new Geo::Arc(data.basePoint.x, data.basePoint.y, data.radious, data.staangle, data.endangle,
                                             static_cast<bool>(data.isccw));
                if (data.extPoint.z < 0)
                {
                    arc->transform(_flip_by_y_mat);
                }
                group.append(arc);
                _object_map[arc] = data.handle;
                _handle_map[data.handle] = arc;
                return;
            }
        }
        _graph->append_group();
        _graph->container_groups().back().name = QString::fromStdString(data.layer);
        Geo::Arc *arc =
            new Geo::Arc(data.basePoint.x, data.basePoint.y, data.radious, data.staangle, data.endangle, static_cast<bool>(data.isccw));
        if (data.extPoint.z < 0)
        {
            arc->transform(_flip_by_y_mat);
        }
        _graph->container_groups().back().append(arc);
        _object_map[arc] = data.handle;
        _handle_map[data.handle] = arc;
    }
    else
    {
        Geo::Arc *arc =
            new Geo::Arc(data.basePoint.x, data.basePoint.y, data.radious, data.staangle, data.endangle, static_cast<bool>(data.isccw));
        if (data.extPoint.z < 0)
        {
            arc->transform(_flip_by_y_mat);
        }
        _combination->append(arc);
        _object_map[arc] = data.handle;
        _handle_map[data.handle] = arc;
    }
}

void DXFReaderWriter::addCircle(const DRW_Circle &data)
{
    _handle_pairs.insert_or_assign(data.handle, data.parentHandle);
    if (_ignore_entity || !data.visible)
    {
        return;
    }
    if (_to_graph)
    {
        for (ContainerGroup &group : _graph->container_groups())
        {
            if (group.name.toStdString() == data.layer)
            {
                group.append(new Geo::Circle(data.basePoint.x, data.basePoint.y, data.radious));
                if (data.extPoint.z < 0)
                {
                    group.back()->transform(_flip_by_y_mat);
                }
                _object_map[group.back()] = data.handle;
                _handle_map[data.handle] = group.back();
                return;
            }
        }
        _graph->append_group();
        _graph->container_groups().back().name = QString::fromStdString(data.layer);
        _graph->container_groups().back().append(new Geo::Circle(data.basePoint.x, data.basePoint.y, data.radious));
        if (data.extPoint.z < 0)
        {
            _graph->container_groups().back().back()->transform(_flip_by_y_mat);
        }
        _object_map[_graph->container_groups().back().back()] = data.handle;
        _handle_map[data.handle] = _graph->container_groups().back().back();
    }
    else
    {
        _combination->append(new Geo::Circle(data.basePoint.x, data.basePoint.y, data.radious));
        if (data.extPoint.z < 0)
        {
            _combination->back()->transform(_flip_by_y_mat);
        }
        _object_map[_combination->back()] = data.handle;
        _handle_map[data.handle] = _combination->back();
    }
}

void DXFReaderWriter::addEllipse(const DRW_Ellipse &data)
{
    _handle_pairs.insert_or_assign(data.handle, data.parentHandle);
    if (_ignore_entity || !data.visible)
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
                group.append(new Geo::Ellipse(data.basePoint.x, data.basePoint.y, a, a * data.ratio, data.staparam, data.endparam, true));
                group.back()->translate(-data.basePoint.x, -data.basePoint.y);
                const double s = data.secPoint.y / a, c = data.secPoint.x / a;
                group.back()->transform(c, -s, 0, s, c, 0);
                group.back()->translate(data.basePoint.x, data.basePoint.y);
                if (data.extPoint.z < 0)
                {
                    group.back()->transform(_flip_by_y_mat);
                }
                _object_map[group.back()] = data.handle;
                _handle_map[data.handle] = group.back();
                return;
            }
        }
        _graph->append_group();
        _graph->container_groups().back().name = QString::fromStdString(data.layer);
        const double a = std::hypot(data.secPoint.x, data.secPoint.y);
        _graph->container_groups().back().append(
            new Geo::Ellipse(data.basePoint.x, data.basePoint.y, a, a * data.ratio, data.staparam, data.endparam, true));
        _graph->container_groups().back().back()->translate(-data.basePoint.x, -data.basePoint.y);
        const double s = data.secPoint.y / a, c = data.secPoint.x / a;
        _graph->container_groups().back().back()->transform(c, -s, 0, s, c, 0);
        _graph->container_groups().back().back()->translate(data.basePoint.x, data.basePoint.y);
        if (data.extPoint.z < 0)
        {
            _graph->container_groups().back().back()->transform(_flip_by_y_mat);
        }
        _object_map[_graph->container_groups().back().back()] = data.handle;
        _handle_map[data.handle] = _graph->container_groups().back().back();
    }
    else
    {
        const double a = std::hypot(data.secPoint.x, data.secPoint.y);
        _combination->append(new Geo::Ellipse(data.basePoint.x, data.basePoint.y, a, a * data.ratio, data.staparam, data.endparam, true));
        _combination->back()->translate(-data.basePoint.x, -data.basePoint.y);
        const double s = data.secPoint.y / a, c = data.secPoint.x / a;
        _combination->back()->transform(c, -s, 0, s, c, 0);
        _combination->back()->translate(data.basePoint.x, data.basePoint.y);
        if (data.extPoint.z < 0)
        {
            _combination->back()->transform(_flip_by_y_mat);
        }
        _object_map[_combination->back()] = data.handle;
        _handle_map[data.handle] = _combination->back();
    }
}

void DXFReaderWriter::addLWPolyline(const DRW_LWPolyline &data)
{
    _handle_pairs.insert_or_assign(data.handle, data.parentHandle);
    if (_ignore_entity || !data.visible || data.vertlist.empty())
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
                    const Bulge bulge(data.vertlist[i - 1]->x, data.vertlist[i - 1]->y, data.vertlist[i]->x, data.vertlist[i]->y,
                                      data.vertlist[i - 1]->bulge);
                    if (bulge.is_line())
                    {
                        polyline->append(Geo::Point(data.vertlist[i - 1]->x, data.vertlist[i - 1]->y));
                        polyline->append(Geo::Point(data.vertlist[i]->x, data.vertlist[i]->y));
                    }
                    else
                    {
                        const Geo::Point center(bulge.center());
                        polyline->append(Geo::arc_to_polyline(center, bulge.radius(), Geo::angle(center, bulge.start),
                                                              Geo::angle(center, bulge.end), bulge.is_cw(),
                                                              Geo::Circle::default_down_sampling_value));
                    }
                }
                if (data.flags & 0x1)
                {
                    const Bulge bulge(data.vertlist.back()->x, data.vertlist.back()->y, data.vertlist.front()->x, data.vertlist.front()->y,
                                      data.vertlist.back()->bulge);
                    if (bulge.is_line())
                    {
                        polyline->append(Geo::Point(data.vertlist.back()->x, data.vertlist.back()->y));
                        polyline->append(Geo::Point(data.vertlist.front()->x, data.vertlist.front()->y));
                    }
                    else
                    {
                        const Geo::Point center(bulge.center());
                        polyline->append(Geo::arc_to_polyline(center, bulge.radius(), Geo::angle(center, bulge.start),
                                                              Geo::angle(center, bulge.end), bulge.is_cw(),
                                                              Geo::Circle::default_down_sampling_value));
                    }
                }
                group.append(polyline);
                if (data.extPoint.z < 0)
                {
                    polyline->transform(_flip_by_y_mat);
                }
                _object_map[polyline] = data.handle;
                _handle_map[data.handle] = polyline;
                return;
            }
        }
        _graph->append_group();
        _graph->container_groups().back().name = QString::fromStdString(data.layer);
        Geo::Polyline *polyline = new Geo::Polyline();
        for (size_t i = 1, count = data.vertlist.size(); i < count; ++i)
        {
            const Bulge bulge(data.vertlist[i - 1]->x, data.vertlist[i - 1]->y, data.vertlist[i]->x, data.vertlist[i]->y,
                              data.vertlist[i - 1]->bulge);
            if (bulge.is_line())
            {
                polyline->append(Geo::Point(data.vertlist[i - 1]->x, data.vertlist[i - 1]->y));
                polyline->append(Geo::Point(data.vertlist[i]->x, data.vertlist[i]->y));
            }
            else
            {
                const Geo::Point center(bulge.center());
                polyline->append(Geo::arc_to_polyline(center, bulge.radius(), Geo::angle(center, bulge.start),
                                                      Geo::angle(center, bulge.end), bulge.is_cw(),
                                                      Geo::Circle::default_down_sampling_value));
            }
        }
        if (data.flags & 0x1)
        {
            const Bulge bulge(data.vertlist.back()->x, data.vertlist.back()->y, data.vertlist.front()->x, data.vertlist.front()->y,
                              data.vertlist.back()->bulge);
            if (bulge.is_line())
            {
                polyline->append(Geo::Point(data.vertlist.back()->x, data.vertlist.back()->y));
                polyline->append(Geo::Point(data.vertlist.front()->x, data.vertlist.front()->y));
            }
            else
            {
                const Geo::Point center(bulge.center());
                polyline->append(Geo::arc_to_polyline(center, bulge.radius(), Geo::angle(center, bulge.start),
                                                      Geo::angle(center, bulge.end), bulge.is_cw(),
                                                      Geo::Circle::default_down_sampling_value));
            }
        }
        _graph->container_groups().back().append(polyline);
        if (data.extPoint.z < 0)
        {
            polyline->transform(_flip_by_y_mat);
        }
        _object_map[polyline] = data.handle;
        _handle_map[data.handle] = polyline;
    }
    else
    {
        Geo::Polyline *polyline = new Geo::Polyline();
        for (size_t i = 1, count = data.vertlist.size(); i < count; ++i)
        {
            const Bulge bulge(data.vertlist[i - 1]->x, data.vertlist[i - 1]->y, data.vertlist[i]->x, data.vertlist[i]->y,
                              data.vertlist[i - 1]->bulge);
            if (bulge.is_line())
            {
                polyline->append(Geo::Point(data.vertlist[i - 1]->x, data.vertlist[i - 1]->y));
                polyline->append(Geo::Point(data.vertlist[i]->x, data.vertlist[i]->y));
            }
            else
            {
                const Geo::Point center(bulge.center());
                polyline->append(Geo::arc_to_polyline(center, bulge.radius(), Geo::angle(center, bulge.start),
                                                      Geo::angle(center, bulge.end), bulge.is_cw(),
                                                      Geo::Circle::default_down_sampling_value));
            }
        }
        if (data.flags & 0x1)
        {
            const Bulge bulge(data.vertlist.back()->x, data.vertlist.back()->y, data.vertlist.front()->x, data.vertlist.front()->y,
                              data.vertlist.back()->bulge);
            if (bulge.is_line())
            {
                polyline->append(Geo::Point(data.vertlist.back()->x, data.vertlist.back()->y));
                polyline->append(Geo::Point(data.vertlist.front()->x, data.vertlist.front()->y));
            }
            else
            {
                const Geo::Point center(bulge.center());
                polyline->append(Geo::arc_to_polyline(center, bulge.radius(), Geo::angle(center, bulge.start),
                                                      Geo::angle(center, bulge.end), bulge.is_cw(),
                                                      Geo::Circle::default_down_sampling_value));
            }
        }
        _combination->append(polyline);
        if (data.extPoint.z < 0)
        {
            polyline->transform(_flip_by_y_mat);
        }
        _object_map[polyline] = data.handle;
        _handle_map[data.handle] = polyline;
    }
}

void DXFReaderWriter::addPolyline(const DRW_Polyline &data)
{
    _handle_pairs.insert_or_assign(data.handle, data.parentHandle);
    if (_ignore_entity || !data.visible)
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
                    const Bulge bulge(data.vertlist[i - 1]->basePoint.x, data.vertlist[i - 1]->basePoint.y, data.vertlist[i]->basePoint.x,
                                      data.vertlist[i]->basePoint.y, data.vertlist[i - 1]->bulge);
                    if (bulge.is_line())
                    {
                        polyline->append(Geo::Point(data.vertlist[i - 1]->basePoint.x, data.vertlist[i - 1]->basePoint.y));
                        polyline->append(Geo::Point(data.vertlist[i]->basePoint.x, data.vertlist[i]->basePoint.y));
                    }
                    else
                    {
                        const Geo::Point center(bulge.center());
                        polyline->append(Geo::arc_to_polyline(center, bulge.radius(), Geo::angle(center, bulge.start),
                                                              Geo::angle(center, bulge.end), bulge.is_cw(),
                                                              Geo::Circle::default_down_sampling_value));
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
                                                              Geo::angle(center, bulge.end), bulge.is_cw(),
                                                              Geo::Circle::default_down_sampling_value));
                    }
                }
                group.append(polyline);
                if (data.extPoint.z < 0)
                {
                    polyline->transform(_flip_by_y_mat);
                }
                _object_map[polyline] = data.handle;
                _handle_map[data.handle] = polyline;
                return;
            }
        }
        _graph->append_group();
        _graph->container_groups().back().name = QString::fromStdString(data.layer);
        Geo::Polyline *polyline = new Geo::Polyline();
        for (size_t i = 1, count = data.vertlist.size(); i < count; ++i)
        {
            const Bulge bulge(data.vertlist[i - 1]->basePoint.x, data.vertlist[i - 1]->basePoint.y, data.vertlist[i]->basePoint.x,
                              data.vertlist[i]->basePoint.y, data.vertlist[i - 1]->bulge);
            if (bulge.is_line())
            {
                polyline->append(Geo::Point(data.vertlist[i - 1]->basePoint.x, data.vertlist[i - 1]->basePoint.y));
                polyline->append(Geo::Point(data.vertlist[i]->basePoint.x, data.vertlist[i]->basePoint.y));
            }
            else
            {
                const Geo::Point center(bulge.center());
                polyline->append(Geo::arc_to_polyline(center, bulge.radius(), Geo::angle(center, bulge.start),
                                                      Geo::angle(center, bulge.end), bulge.is_cw(),
                                                      Geo::Circle::default_down_sampling_value));
            }
        }
        if (data.flags & 0x1)
        {
            const Bulge bulge(data.vertlist.back()->basePoint.x, data.vertlist.back()->basePoint.y, data.vertlist.front()->basePoint.x,
                              data.vertlist.front()->basePoint.y, data.vertlist.back()->bulge);
            if (bulge.is_line())
            {
                polyline->append(Geo::Point(data.vertlist.back()->basePoint.x, data.vertlist.back()->basePoint.y));
                polyline->append(Geo::Point(data.vertlist.front()->basePoint.x, data.vertlist.front()->basePoint.y));
            }
            else
            {
                const Geo::Point center(bulge.center());
                polyline->append(Geo::arc_to_polyline(center, bulge.radius(), Geo::angle(center, bulge.start),
                                                      Geo::angle(center, bulge.end), bulge.is_cw(),
                                                      Geo::Circle::default_down_sampling_value));
            }
        }
        _graph->container_groups().back().append(polyline);
        if (data.extPoint.z < 0)
        {
            polyline->transform(_flip_by_y_mat);
        }
        _object_map[polyline] = data.handle;
        _handle_map[data.handle] = polyline;
    }
    else
    {
        Geo::Polyline *polyline = new Geo::Polyline();
        for (size_t i = 1, count = data.vertlist.size(); i < count; ++i)
        {
            const Bulge bulge(data.vertlist[i - 1]->basePoint.x, data.vertlist[i - 1]->basePoint.y, data.vertlist[i]->basePoint.x,
                              data.vertlist[i]->basePoint.y, data.vertlist[i - 1]->bulge);
            if (bulge.is_line())
            {
                polyline->append(Geo::Point(data.vertlist[i - 1]->basePoint.x, data.vertlist[i - 1]->basePoint.y));
                polyline->append(Geo::Point(data.vertlist[i]->basePoint.x, data.vertlist[i]->basePoint.y));
            }
            else
            {
                const Geo::Point center(bulge.center());
                polyline->append(Geo::arc_to_polyline(center, bulge.radius(), Geo::angle(center, bulge.start),
                                                      Geo::angle(center, bulge.end), bulge.is_cw(),
                                                      Geo::Circle::default_down_sampling_value));
            }
        }
        if (data.flags & 0x1)
        {
            const Bulge bulge(data.vertlist.back()->basePoint.x, data.vertlist.back()->basePoint.y, data.vertlist.front()->basePoint.x,
                              data.vertlist.front()->basePoint.y, data.vertlist.back()->bulge);
            if (bulge.is_line())
            {
                polyline->append(Geo::Point(data.vertlist.back()->basePoint.x, data.vertlist.back()->basePoint.y));
                polyline->append(Geo::Point(data.vertlist.front()->basePoint.x, data.vertlist.front()->basePoint.y));
            }
            else
            {
                const Geo::Point center(bulge.center());
                polyline->append(Geo::arc_to_polyline(center, bulge.radius(), Geo::angle(center, bulge.start),
                                                      Geo::angle(center, bulge.end), bulge.is_cw(),
                                                      Geo::Circle::default_down_sampling_value));
            }
        }
        _combination->append(polyline);
        if (data.extPoint.z < 0)
        {
            polyline->transform(_flip_by_y_mat);
        }
        _object_map[polyline] = data.handle;
        _handle_map[data.handle] = polyline;
    }
}

void DXFReaderWriter::addSpline(const DRW_Spline *data)
{
    _handle_pairs.insert_or_assign(data->handle, data->parentHandle);
    if (_ignore_entity || !data->visible)
    {
        return;
    }
    if (data->degree == 2)
    {
        Geo::QuadBSpline *bspline = nullptr;
        std::vector<Geo::Point> points;
        if (data->ncontrol == 0)
        {
            for (const DRW_Coord *vert : data->fitlist)
            {
                points.emplace_back(vert->x, vert->y);
            }
            bspline = new Geo::QuadBSpline(points.begin(), points.end(), true);
        }
        else
        {
            for (const DRW_Coord *vert : data->controllist)
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
                    _handle_map[data->handle] = bspline;
                    return;
                }
            }
            _graph->append_group();
            _graph->container_groups().back().name = QString::fromStdString(data->layer);
            _graph->container_groups().back().append(bspline);
            _object_map[bspline] = data->handle;
            _handle_map[data->handle] = bspline;
        }
        else
        {
            _combination->append(bspline);
            _object_map[bspline] = data->handle;
            _handle_map[data->handle] = bspline;
        }
    }
    else
    {
        Geo::CubicBSpline *bspline = nullptr;
        std::vector<Geo::Point> points;
        if (data->ncontrol == 0)
        {
            for (const DRW_Coord *vert : data->fitlist)
            {
                points.emplace_back(vert->x, vert->y);
            }
            bspline = new Geo::CubicBSpline(points.begin(), points.end(), true);
        }
        else
        {
            for (const DRW_Coord *vert : data->controllist)
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
                    _handle_map[data->handle] = bspline;
                    return;
                }
            }
            _graph->append_group();
            _graph->container_groups().back().name = QString::fromStdString(data->layer);
            _graph->container_groups().back().append(bspline);
            _object_map[bspline] = data->handle;
            _handle_map[data->handle] = bspline;
        }
        else
        {
            _combination->append(bspline);
            _object_map[bspline] = data->handle;
            _handle_map[data->handle] = bspline;
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
        _handle_pairs.insert_or_assign(data.handle, data.parentHandle);
        if (_block_name_map.find(data.name) != _block_name_map.end())
        {
            Combination *combination = new Combination();
            for (const Geo::Geometry *obj : *_block_name_map[data.name])
            {
                combination->append(obj->clone());
            }
            combination->scale(data.basePoint.x, data.basePoint.y, data.xscale);
            combination->translate(data.basePoint.x, data.basePoint.y);
            combination->rotate(data.basePoint.x, data.basePoint.y, data.angle);
            combination->update_border();
            _graph->append(combination);
        }
    }
    else
    {
        if (_block_name_map.find(data.name) != _block_name_map.end())
        {
            Combination combination;
            for (const Geo::Geometry *obj : *_block_name_map[data.name])
            {
                combination.append(obj->clone());
            }
            combination.scale(data.basePoint.x, data.basePoint.y, data.xscale);
            combination.translate(data.basePoint.x, data.basePoint.y);
            combination.rotate(data.basePoint.x, data.basePoint.y, data.angle);
            _combination->append(&combination);
            _combination->update_border();
        }
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
    const QString txt = to_native_string(QString::fromUtf8(data.text.c_str()));
    _handle_pairs.insert_or_assign(data.handle, data.parentHandle);
    if (_ignore_entity || !data.visible || txt.isEmpty() || txt.count(' ') == txt.size())
    {
        return;
    }

    std::string style;
    std::transform(data.style.begin(), data.style.end(), std::back_inserter(style), [](char c) { return std::tolower(c); });
    if (_to_graph)
    {
        for (ContainerGroup &group : _graph->container_groups())
        {
            if (group.name.toStdString() == data.layer)
            {
                Text *text = nullptr;
                if (_text_style_font.find(style) == _text_style_font.end())
                {
                    QFont font("SimSun");
                    font.setPointSizeF(data.height);
                    text = new Text(data.basePoint.x, data.basePoint.y, font, txt, 0);
                }
                else
                {
                    QFont font(QString::fromStdString(_text_style_font[style]));
                    font.setPointSizeF(data.height);
                    text = new Text(data.basePoint.x, data.basePoint.y, font, txt, 0);
                }
                text->rotate(text->anchor().x, text->anchor().y, Geo::degree_to_rad(data.angle));
                group.append(text);
                _object_map[group.back()] = data.handle;
                _handle_map[data.handle] = group.back();
                return;
            }
        }
        _graph->append_group();
        _graph->container_groups().back().name = QString::fromStdString(data.layer);
        Text *text = nullptr;
        if (_text_style_font.find(style) == _text_style_font.end())
        {
            QFont font("SimSun");
            font.setPointSizeF(data.height);
            text = new Text(data.basePoint.x, data.basePoint.y, font, txt, 0);
        }
        else
        {
            QFont font(QString::fromStdString(_text_style_font[style]));
            font.setPointSizeF(data.height);
            text = new Text(data.basePoint.x, data.basePoint.y, font, txt, 0);
        }
        text->rotate(text->anchor().x, text->anchor().y, Geo::degree_to_rad(data.angle));
        _graph->container_groups().back().append(text);
        _object_map[_graph->container_groups().back().back()] = data.handle;
        _handle_map[data.handle] = _graph->container_groups().back().back();
    }
    else
    {
        Text *text = nullptr;
        if (_text_style_font.find(style) == _text_style_font.end())
        {
            QFont font("SimSun");
            font.setPointSizeF(data.height);
            text = new Text(data.basePoint.x, data.basePoint.y, font, txt, 0);
        }
        else
        {
            QFont font(QString::fromStdString(_text_style_font[style]));
            font.setPointSizeF(data.height);
            text = new Text(data.basePoint.x, data.basePoint.y, font, txt, 0);
        }
        text->rotate(text->anchor().x, text->anchor().y, Geo::degree_to_rad(data.angle));
        _combination->append(text);
        _object_map[_combination->back()] = data.handle;
        _handle_map[data.handle] = _combination->back();
    }
}

void DXFReaderWriter::addText(const DRW_Text &data)
{
    const QString txt = to_native_string(QString::fromUtf8(data.text.c_str()));
    _handle_pairs.insert_or_assign(data.handle, data.parentHandle);
    if (_ignore_entity || !data.visible || txt.isEmpty() || txt.count(' ') == txt.size())
    {
        return;
    }

    std::string style;
    std::transform(data.style.begin(), data.style.end(), std::back_inserter(style), [](char c) { return std::tolower(c); });
    if (_to_graph)
    {
        for (ContainerGroup &group : _graph->container_groups())
        {
            if (group.name.toStdString() == data.layer)
            {
                Text *text = nullptr;
                if (_text_style_font.find(style) == _text_style_font.end())
                {
                    QFont font("SimSun");
                    font.setPointSizeF(data.height);
                    text = new Text(data.basePoint.x, data.basePoint.y, font, txt, 3);
                }
                else
                {
                    QFont font(QString::fromStdString(_text_style_font[style]));
                    font.setPointSizeF(data.height);
                    text = new Text(data.basePoint.x, data.basePoint.y, font, txt, 3);
                }
                text->rotate(text->anchor().x, text->anchor().y, Geo::degree_to_rad(data.angle));
                group.append(text);
                _object_map[group.back()] = data.handle;
                _handle_map[data.handle] = group.back();
                return;
            }
        }
        _graph->append_group();
        _graph->container_groups().back().name = QString::fromStdString(data.layer);
        Text *text = nullptr;
        if (_text_style_font.find(style) == _text_style_font.end())
        {
            QFont font("SimSun");
            font.setPointSizeF(data.height);
            text = new Text(data.basePoint.x, data.basePoint.y, font, txt, 3);
        }
        else
        {
            QFont font(QString::fromStdString(_text_style_font[style]));
            font.setPointSizeF(data.height);
            text = new Text(data.basePoint.x, data.basePoint.y, font, txt, 3);
        }
        text->rotate(text->anchor().x, text->anchor().y, Geo::degree_to_rad(data.angle));
        _graph->container_groups().back().append(text);
        _object_map[_graph->container_groups().back().back()] = data.handle;
        _handle_map[data.handle] = _graph->container_groups().back().back();
    }
    else
    {
        Text *text = nullptr;
        if (_text_style_font.find(style) == _text_style_font.end())
        {
            QFont font("SimSun");
            font.setPointSizeF(data.height);
            text = new Text(data.basePoint.x, data.basePoint.y, font, txt, 3);
        }
        else
        {
            QFont font(QString::fromStdString(_text_style_font[style]));
            font.setPointSizeF(data.height);
            text = new Text(data.basePoint.x, data.basePoint.y, font, txt, 3);
        }
        text->rotate(text->anchor().x, text->anchor().y, Geo::degree_to_rad(data.angle));
        _combination->append(text);
        _object_map[_combination->back()] = data.handle;
        _handle_map[data.handle] = _combination->back();
    }
}

void DXFReaderWriter::addDimAlign(const DRW_DimAligned *data)
{
    _handle_pairs.insert_or_assign(data->handle, data->parentHandle);
    if (_ignore_entity || !data->visible)
    {
        return;
    }
    if (const DRW_Coord coords[] = {data->getDef1Point(), data->getDef2Point(), data->getDimPoint()}; _to_graph)
    {
        const Geo::Point points[3] = {Geo::Point(coords[0].x, coords[0].y), Geo::Point(coords[1].x, coords[1].y),
                                      Geo::Point(coords[2].x, coords[2].y)};
        const double height = Geo::distance(points[2], points[0], points[1], true);
        for (ContainerGroup &group : _graph->container_groups())
        {
            if (group.name.toStdString() == data->layer)
            {
                group.append(
                    new Dim::DimAligned(points[0], points[1], Geo::is_on_left(points[2], points[0], points[1]) ? height : -height));
                _object_map[group.back()] = data->handle;
                _handle_map[data->handle] = group.back();
                return;
            }
        }
        _graph->append_group();
        _graph->container_groups().back().name = QString::fromStdString(data->layer);
        _graph->container_groups().back().append(
            new Dim::DimAligned(points[0], points[1], Geo::is_on_left(points[2], points[0], points[1]) ? height : -height));
        _object_map[_graph->container_groups().back().back()] = data->handle;
        _handle_map[data->handle] = _graph->container_groups().back().back();
    }
    else
    {
        const Geo::Point points[3] = {Geo::Point(coords[0].x, coords[0].y), Geo::Point(coords[1].x, coords[1].y),
                                      Geo::Point(coords[2].x, coords[2].y)};
        const double height = Geo::distance(points[2], points[0], points[1], true);
        _combination->append(
            new Dim::DimAligned(points[0], points[1], Geo::is_on_left(points[2], points[0], points[1]) ? height : -height));
        _object_map[_combination->back()] = data->handle;
        _handle_map[data->handle] = _combination->back();
    }
}

void DXFReaderWriter::addDimLinear(const DRW_DimLinear *data)
{
    _handle_pairs.insert_or_assign(data->handle, data->parentHandle);
    if (_ignore_entity || !data->visible)
    {
        return;
    }
    if (const DRW_Coord coords[] = {data->getDef1Point(), data->getDef2Point(), data->getDimPoint()}; _to_graph)
    {
        const Geo::Point points[3] = {Geo::Point(coords[0].x, coords[0].y), Geo::Point(coords[1].x, coords[1].y),
                                      Geo::Point(coords[2].x, coords[2].y)};
        const bool horizontal = data->getAngle() == 0;
        const double height =
            horizontal ? (coords[2].y - (coords[0].y + coords[1].y) / 2) : (coords[2].x - (coords[0].x + coords[1].x) / 2);
        for (ContainerGroup &group : _graph->container_groups())
        {
            if (group.name.toStdString() == data->layer)
            {
                group.append(new Dim::DimLinear(points[0], points[1], horizontal, height));
                _object_map[group.back()] = data->handle;
                _handle_map[data->handle] = group.back();
                return;
            }
        }
        _graph->append_group();
        _graph->container_groups().back().name = QString::fromStdString(data->layer);
        _graph->container_groups().back().append(new Dim::DimLinear(points[0], points[1], horizontal, height));
        _object_map[_graph->container_groups().back().back()] = data->handle;
        _handle_map[data->handle] = _graph->container_groups().back().back();
    }
    else
    {
        const Geo::Point points[3] = {Geo::Point(coords[0].x, coords[0].y), Geo::Point(coords[1].x, coords[1].y),
                                      Geo::Point(coords[2].x, coords[2].y)};
        const bool horizontal = data->getAngle() == 0;
        const double height =
            horizontal ? (coords[2].y - (coords[0].y + coords[1].y) / 2) : (coords[2].x - (coords[0].x + coords[1].x) / 2);
        _combination->append(new Dim::DimLinear(points[0], points[1], horizontal, height));
        _object_map[_combination->back()] = data->handle;
        _handle_map[data->handle] = _combination->back();
    }
}

void DXFReaderWriter::addDimRadial(const DRW_DimRadial *data)
{
    _handle_pairs.insert_or_assign(data->handle, data->parentHandle);
    if (_ignore_entity || !data->visible)
    {
        return;
    }
    if (const DRW_Coord coords[] = {data->getCenterPoint(), data->getDiameterPoint(), data->getTextPoint()}; _to_graph)
    {
        const Geo::Point points[3] = {Geo::Point(coords[0].x, coords[0].y), Geo::Point(coords[1].x, coords[1].y),
                                      Geo::Point(coords[2].x, coords[2].y)};
        for (ContainerGroup &group : _graph->container_groups())
        {
            if (group.name.toStdString() == data->layer)
            {
                group.append(new Dim::DimRadius(points[0], points[1], Geo::distance(points[0], points[2])));
                _object_map[group.back()] = data->handle;
                _handle_map[data->handle] = group.back();
                return;
            }
        }
        _graph->append_group();
        _graph->container_groups().back().name = QString::fromStdString(data->layer);
        _graph->container_groups().back().append(new Dim::DimRadius(points[0], points[1], Geo::distance(points[0], points[2])));
        _object_map[_graph->container_groups().back().back()] = data->handle;
        _handle_map[data->handle] = _graph->container_groups().back().back();
    }
    else
    {
        const Geo::Point points[3] = {Geo::Point(coords[0].x, coords[0].y), Geo::Point(coords[1].x, coords[1].y),
                                      Geo::Point(coords[2].x, coords[2].y)};
        _combination->append(new Dim::DimRadius(points[0], points[1], Geo::distance(points[0], points[2])));
        _object_map[_combination->back()] = data->handle;
        _handle_map[data->handle] = _combination->back();
    }
}

void DXFReaderWriter::addDimDiametric(const DRW_DimDiametric *data)
{
    _handle_pairs.insert_or_assign(data->handle, data->parentHandle);
    if (_ignore_entity || !data->visible)
    {
        return;
    }
    if (const DRW_Coord coords[] = {data->getTextPoint(), data->getDiameter1Point(), data->getDiameter2Point()}; _to_graph)
    {
        const Geo::Point points[] = {Geo::Point(coords[0].x, coords[0].y), Geo::Point(coords[1].x, coords[1].y),
                                     Geo::Point(coords[2].x, coords[2].y),
                                     Geo::Point((coords[1].x + coords[2].x) / 2, (coords[1].y + coords[2].y) / 2)};
        for (ContainerGroup &group : _graph->container_groups())
        {
            if (group.name.toStdString() == data->layer)
            {
                group.append(new Dim::DimDiameter(
                    points[3], Geo::distance(points[3], points[1]) < Geo::distance(points[3], points[2]) ? points[1] : points[2],
                    Geo::distance(points[0], points[3])));
                _object_map[group.back()] = data->handle;
                _handle_map[data->handle] = group.back();
                return;
            }
        }
        _graph->append_group();
        _graph->container_groups().back().name = QString::fromStdString(data->layer);
        _graph->container_groups().back().append(new Dim::DimDiameter(
            points[3], Geo::distance(points[3], points[1]) < Geo::distance(points[3], points[2]) ? points[1] : points[2],
            Geo::distance(points[0], points[3])));
        _object_map[_graph->container_groups().back().back()] = data->handle;
        _handle_map[data->handle] = _graph->container_groups().back().back();
    }
    else
    {
        const Geo::Point points[] = {Geo::Point(coords[0].x, coords[0].y), Geo::Point(coords[2].x, coords[2].y),
                                     Geo::Point(coords[1].x, coords[1].y),
                                     Geo::Point((coords[1].x + coords[2].x) / 2, (coords[1].y + coords[2].y) / 2)};
        _combination->append(new Dim::DimDiameter(
            points[3], Geo::distance(points[3], points[1]) < Geo::distance(points[3], points[2]) ? points[1] : points[2],
            Geo::distance(points[0], points[3])));
        _object_map[_combination->back()] = data->handle;
        _handle_map[data->handle] = _combination->back();
    }
}

void DXFReaderWriter::addDimAngular(const DRW_DimAngular *data)
{
    _handle_pairs.insert_or_assign(data->handle, data->parentHandle);
    const Geo::Point line1[2] = {Geo::Point(data->getFirstLine1().x, data->getFirstLine1().y),
                                 Geo::Point(data->getFirstLine2().x, data->getFirstLine2().y)};
    const Geo::Point line2[2] = {Geo::Point(data->getSecondLine1().x, data->getSecondLine1().y),
                                 Geo::Point(data->getSecondLine2().x, data->getSecondLine2().y)};
    const Geo::Point arc_anchor(data->getDimPoint().x, data->getDimPoint().y);
    if (_to_graph)
    {
        for (ContainerGroup &group : _graph->container_groups())
        {
            if (group.name.toStdString() == data->layer)
            {
                group.append(new Dim::DimAngle(line1[0], line1[1], line2[0], line2[1], arc_anchor));
                _object_map[group.back()] = data->handle;
                _handle_map[data->handle] = group.back();
                return;
            }
        }
        _graph->append_group();
        _graph->container_groups().back().name = QString::fromStdString(data->layer);
        _graph->container_groups().back().append(new Dim::DimAngle(line1[0], line1[1], line2[0], line2[1], arc_anchor));
        _object_map[_graph->container_groups().back().back()] = data->handle;
        _handle_map[data->handle] = _graph->container_groups().back().back();
    }
    else
    {
        _combination->append(new Dim::DimAngle(line1[0], line1[1], line2[0], line2[1], arc_anchor));
        _object_map[_combination->back()] = data->handle;
        _handle_map[data->handle] = _combination->back();
    }
}

void DXFReaderWriter::addDimAngular3P(const DRW_DimAngular3p *data)
{
    _handle_pairs.insert_or_assign(data->handle, data->parentHandle);
    const Geo::Point vertexes[3] = {Geo::Point(data->getFirstLine().x, data->getFirstLine().y),
                                    Geo::Point(data->getVertexPoint().x, data->getVertexPoint().y),
                                    Geo::Point(data->getSecondLine().x, data->getSecondLine().y)};
    const Geo::Point arc_anchor(data->getDefPoint().x, data->getDefPoint().y);
    const double radius = Geo::distance(vertexes[1], arc_anchor);
    const double extend_ratio = 1.0 / std::cos(std::abs(Geo::angle(vertexes[0], vertexes[1], vertexes[2])) / 2.0);
    const Geo::Point extend_vertexes[2] = {vertexes[0] + (vertexes[0] - vertexes[1]).normalize() * radius * extend_ratio,
                                           vertexes[2] + (vertexes[2] - vertexes[1]).normalize() * radius * extend_ratio};
    if (_to_graph)
    {
        for (ContainerGroup &group : _graph->container_groups())
        {
            if (group.name.toStdString() == data->layer)
            {
                Dim::DimAngle *dim = new Dim::DimAngle(vertexes[0], vertexes[1], vertexes[2], radius, vertexes[0], vertexes[2]);
                dim->set_minor_arc(Geo::is_inside(arc_anchor, extend_vertexes[0], vertexes[1], extend_vertexes[1]));
                group.append(dim);
                _object_map[group.back()] = data->handle;
                _handle_map[data->handle] = group.back();
                return;
            }
        }
        _graph->append_group();
        _graph->container_groups().back().name = QString::fromStdString(data->layer);
        Dim::DimAngle *dim = new Dim::DimAngle(vertexes[0], vertexes[1], vertexes[2], radius, vertexes[0], vertexes[2]);
        dim->set_minor_arc(Geo::is_inside(arc_anchor, extend_vertexes[0], vertexes[1], extend_vertexes[1]));
        _graph->container_groups().back().append(dim);
        _object_map[_graph->container_groups().back().back()] = data->handle;
        _handle_map[data->handle] = _graph->container_groups().back().back();
    }
    else
    {
        Dim::DimAngle *dim = new Dim::DimAngle(vertexes[0], vertexes[1], vertexes[2], radius, vertexes[0], vertexes[2]);
        dim->set_minor_arc(Geo::is_inside(arc_anchor, extend_vertexes[0], vertexes[1], extend_vertexes[1]));
        _combination->append(dim);
        _object_map[_combination->back()] = data->handle;
        _handle_map[data->handle] = _combination->back();
    }
}

void DXFReaderWriter::addDimOrdinate(const DRW_DimOrdinate *data)
{
    _handle_pairs.insert_or_assign(data->handle, data->parentHandle);
    if (_ignore_entity || !data->visible)
    {
        return;
    }
    const DRW_Coord coords[2] = {data->getFirstLine(), data->getSecondLine()};
    if (const Geo::Point points[2] = {Geo::Point(coords[0].x, coords[0].y), Geo::Point(coords[1].x, coords[1].y)}; _to_graph)
    {
        for (ContainerGroup &group : _graph->container_groups())
        {
            if (group.name.toStdString() == data->layer)
            {
                Dim::DimOrdinate *dim = new Dim::DimOrdinate(points[0], points[1]);
                group.append(dim);
                _object_map[group.back()] = data->handle;
                _handle_map[data->handle] = group.back();
                return;
            }
        }
        _graph->append_group();
        _graph->container_groups().back().name = QString::fromStdString(data->layer);
        Dim::DimOrdinate *dim = new Dim::DimOrdinate(points[0], points[1]);
        _graph->container_groups().back().append(dim);
        _object_map[_graph->container_groups().back().back()] = data->handle;
        _handle_map[data->handle] = _graph->container_groups().back().back();
    }
    else
    {
        Dim::DimOrdinate *dim = new Dim::DimOrdinate(points[0], points[1]);
        _combination->append(dim);
        _object_map[_combination->back()] = data->handle;
        _handle_map[data->handle] = _combination->back();
    }
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
{
}

void DXFReaderWriter::writeHeader(DRW_Header &data)
{
}

void DXFReaderWriter::writeBlocks()
{
    for (Combination *combination : _block_store)
    {
        DRW_Block block;
        block.name = combination->name.toStdString();
        block.basePoint.x = 0.0;
        block.basePoint.y = 0.0;
        block.basePoint.z = 0.0;
        block.flags = 1; // flag for unnamed block
        _dxfrw->writeBlock(&block);
        const Geo::AABBRectParams rect = combination->aabbrect_params();
        const Geo::Point anchor(rect.left, rect.bottom);
        for (const Geo::Geometry *object : *combination)
        {
            Geo::Geometry *moved_object = object->clone();
            moved_object->translate(-anchor.x, -anchor.y);
            write_geometry_object(moved_object);
            delete moved_object;
        }
    }
    _block_store.clear();
}

void DXFReaderWriter::writeBlockRecords()
{
    prepare_blocks();
    for (Combination *combination : _block_store)
    {
        _dxfrw->writeBlockRecord(combination->name.toStdString());
    }
}

void DXFReaderWriter::writeEntities()
{
    for (const ContainerGroup &group : _graph->container_groups())
    {
        _current_group = &group;
        for (const Geo::Geometry *object : group)
        {
            write_geometry_object(object);
        }
        _current_group = nullptr;
    }
}

void DXFReaderWriter::writeLTypes()
{
}

void DXFReaderWriter::writeLayers()
{
    DRW_Layer lay;
    for (const ContainerGroup &group : _graph->container_groups())
    {
        lay.reset();

        lay.name = group.name.toStdString();
        _dxfrw->writeLayer(&lay);
    }
}

void DXFReaderWriter::writeTextstyles()
{
    DRW_Textstyle style;
    style.name = "Standard";
    style.font = "SimSun";
    _dxfrw->writeTextstyle(&style);
}

void DXFReaderWriter::writeVports()
{
    DRW_Vport vp;
    vp.name = "*Active";
    vp.grid = 1;
    vp.gridSpacing.x = 50;
    vp.gridSpacing.y = 50;
    {
        const Geo::AABBRectParams rect = _graph->aabbrect_params();
        vp.height = (rect.top - rect.bottom) * 1.1;
        vp.ratio = (rect.right - rect.left) / (rect.top - rect.bottom);
        vp.center.x = (rect.left + rect.right) / 2;
        vp.center.y = (rect.top + rect.bottom) / 2;
    }
    _dxfrw->writeVport(&vp);
}

void DXFReaderWriter::writeDimstyles()
{
}

void DXFReaderWriter::writeAppId()
{
    DRW_AppId ai;
    ai.name = "DSV";
    _dxfrw->writeAppId(&ai);
}

void DXFReaderWriter::write_geometry_object(const Geo::Geometry *object)
{
    switch (object->type())
    {
    case Geo::Type::AABBRECT:
        break;
    case Geo::Type::BEZIER:
        write_bezier(static_cast<const Geo::CubicBezier *>(object));
        break;
    case Geo::Type::BSPLINE:
        write_bspline(static_cast<const Geo::BSpline *>(object));
        break;
    case Geo::Type::CIRCLE:
        write_circle(static_cast<const Geo::Circle *>(object));
        break;
    case Geo::Type::COMBINATION:
        write_insert(static_cast<const Combination *>(object),
                     Geo::Point(object->aabbrect_params().left, object->aabbrect_params().bottom));
        break;
    case Geo::Type::CONTAINERGROUP:
        break;
    case Geo::Type::ELLIPSE:
        write_ellipse(static_cast<const Geo::Ellipse *>(object));
        break;
    case Geo::Type::ARC:
        write_arc(static_cast<const Geo::Arc *>(object));
        break;
    case Geo::Type::POINT:
        write_point(static_cast<const Geo::Point *>(object));
        break;
    case Geo::Type::POLYGON:
        write_polygon(static_cast<const Geo::Polygon *>(object));
        break;
    case Geo::Type::POLYLINE:
        write_polyline(static_cast<const Geo::Polyline *>(object));
        break;
    case Geo::Type::TEXT:
        write_text(static_cast<const Text *>(object));
        break;
    case Geo::Type::TRIANGLE:
        break;
    case Geo::Type::DIMENSION:
        {
            const Dim::Dimension *dim = static_cast<const Dim::Dimension *>(object);
            switch (dim->dim_type())
            {
            case Dim::Type::ALIGNED:
                write_aligned_dim(static_cast<const Dim::DimAligned *>(dim));
                break;
            case Dim::Type::LINEAR:
                write_linear_dim(static_cast<const Dim::DimLinear *>(dim));
                break;
            case Dim::Type::ANGLE:
                write_angle_dim(static_cast<const Dim::DimAngle *>(dim));
                break;
            case Dim::Type::DIAMETER:
                write_diameter_dim(static_cast<const Dim::DimDiameter *>(dim));
                break;
            case Dim::Type::ORDINATE:
                write_ordinate_dim(static_cast<const Dim::DimOrdinate *>(dim));
                break;
            case Dim::Type::RADIUS:
                write_radius_dim(static_cast<const Dim::DimRadius *>(dim));
                break;
            default:
                break;
            }
        }
        break;
    default:
        break;
    }
}

void DXFReaderWriter::write_bezier(const Geo::CubicBezier *bezier)
{
    const Geo::CubicBSpline bspline = Geo::bezier_to_bspline(*bezier);
    write_bspline(&bspline);
}

void DXFReaderWriter::write_bspline(const Geo::BSpline *bspline)
{
    DRW_Spline sp;

    // dxf spline group code=70
    // bit coded: 1: closed; 2: periodic; 4: rational; 8: planar; 16:linear
    sp.flags = 0x1008;

    std::vector<DRW_Coord> control_points, path_points;
    for (const Geo::Point &v : bspline->control_points)
    {
        control_points.emplace_back(v.x, v.y, 0);
    }
    for (const Geo::Point &v : bspline->path_points)
    {
        path_points.emplace_back(v.x, v.y, 0);
    }
    // write spline control points:
    for (size_t i = 0, count = control_points.size(); i < count; ++i)
    {
        sp.controllist.push_back(&control_points[i]);
    }
    // write spline fit points:
    for (size_t i = 0, count = path_points.size(); i < count; ++i)
    {
        sp.fitlist.push_back(&path_points[i]);
    }

    sp.nfit = sp.fitlist.size();
    sp.ncontrol = sp.controllist.size();
    sp.degree = dynamic_cast<const Geo::CubicBSpline *>(bspline) == nullptr ? 2 : 3;

    // knot vector from RS_Spline
    sp.knotslist = bspline->knots();
    sp.nknots = sp.knotslist.size();

    sp.layer = _current_group == nullptr ? "0" : _current_group->name.toStdString();
    sp.lineType = "CONTINUOUS";
    _dxfrw->writeSpline(&sp);
}

void DXFReaderWriter::write_circle(const Geo::Circle *circle)
{
    DRW_Circle c;
    c.layer = _current_group == nullptr ? "0" : _current_group->name.toStdString();
    c.lineType = "CONTINUOUS";
    c.basePoint.x = circle->x;
    c.basePoint.y = circle->y;
    c.radious = circle->radius;
    _dxfrw->writeCircle(&c);
}

void DXFReaderWriter::write_ellipse(const Geo::Ellipse *ellipse)
{
    DRW_Ellipse el;
    el.layer = _current_group == nullptr ? "0" : _current_group->name.toStdString();
    el.lineType = "CONTINUOUS";
    el.basePoint.x = ellipse->center().x;
    el.basePoint.y = ellipse->center().y;
    el.secPoint.x = ellipse->a1().x - el.basePoint.x;
    el.secPoint.y = ellipse->a1().y - el.basePoint.y;
    el.ratio = ellipse->lengthb() / ellipse->lengtha();
    el.staparam = ellipse->arc_param0();
    el.endparam = ellipse->arc_param1();
    _dxfrw->writeEllipse(&el);
}

void DXFReaderWriter::write_polygon(const Geo::Polygon *polygon)
{
    DRW_LWPolyline pol;
    for (const Geo::Point &point : *polygon)
    {
        pol.addVertex(DRW_Vertex2D(point.x, point.y, 0));
    }
    pol.flags = 1;
    pol.vertexnum = pol.vertlist.size();
    pol.layer = _current_group == nullptr ? "0" : _current_group->name.toStdString();
    pol.lineType = "CONTINUOUS";
    _dxfrw->writeLWPolyline(&pol);
}

void DXFReaderWriter::write_polyline(const Geo::Polyline *polyline)
{
    DRW_LWPolyline pol;
    for (const Geo::Point &point : *polyline)
    {
        pol.addVertex(DRW_Vertex2D(point.x, point.y, 0));
    }
    pol.addVertex(DRW_Vertex2D(polyline->back().x, polyline->back().y, 0));
    pol.vertexnum = pol.vertlist.size();
    pol.layer = _current_group == nullptr ? "0" : _current_group->name.toStdString();
    pol.lineType = "CONTINUOUS";
    _dxfrw->writeLWPolyline(&pol);
}

void DXFReaderWriter::write_text(const Text *text)
{
    if (text->text().isEmpty())
    {
        return;
    }
    if (text->text().count('\n') == 0)
    {
        DRW_Text t;
        t.layer = _current_group == nullptr ? "0" : _current_group->name.toStdString();
        t.lineType = "CONTINUOUS";
        t.style = "Standard";
        t.secPoint.x = text->shape(0).x;
        t.secPoint.y = text->shape(0).y;
        t.basePoint.x = text->shape(3).x;
        t.basePoint.y = text->shape(3).y;
        t.widthscale = 1.0;
        t.height = text->font().pointSizeF();
        t.text = to_dxf_string(text->text()).toStdString();
        t.angle = Geo::rad_to_degree(text->angle());
        t.alignH = DRW_Text::HAlign::HLeft;
        t.alignV = DRW_Text::VAlign::VBaseLine;
        _dxfrw->writeText(&t);
    }
    else
    {
        DRW_MText t;
        t.layer = _current_group == nullptr ? "0" : _current_group->name.toStdString();
        t.lineType = "CONTINUOUS";
        t.style = "Standard";
        t.basePoint.x = text->shape(0).x;
        t.basePoint.y = text->shape(0).y;
        t.secPoint.x = text->shape(2).x;
        t.secPoint.y = text->shape(2).y;
        t.widthscale = 1.0;
        t.height = text->font().pointSizeF();
        t.text = to_dxf_string(text->text()).toStdString();
        t.angle = Geo::rad_to_degree(text->angle());
        t.alignH = DRW_Text::HAlign::HLeft;
        t.alignV = DRW_Text::VAlign::VTop;
        _dxfrw->writeMText(&t);
    }
}

void DXFReaderWriter::write_arc(const Geo::Arc *arc)
{
    DRW_Arc a;
    a.layer = _current_group == nullptr ? "0" : _current_group->name.toStdString();
    a.lineType = "CONTINUOUS";
    a.basePoint.x = arc->x;
    a.basePoint.y = arc->y;
    a.radious = arc->radius;
    a.isccw = !arc->is_cw();
    if (a.isccw)
    {
        a.staangle = Geo::angle(Geo::Point(arc->x, arc->y), arc->control_points[0]);
        a.endangle = Geo::angle(Geo::Point(arc->x, arc->y), arc->control_points[2]);
    }
    else
    {
        a.staangle = Geo::angle(Geo::Point(arc->x, arc->y), arc->control_points[2]);
        a.endangle = Geo::angle(Geo::Point(arc->x, arc->y), arc->control_points[0]);
    }
    _dxfrw->writeArc(&a);
}

void DXFReaderWriter::write_point(const Geo::Point *point)
{
    DRW_Point p;
    p.layer = _current_group == nullptr ? "0" : _current_group->name.toStdString();
    p.basePoint.x = point->x;
    p.basePoint.y = point->y;
    _dxfrw->writePoint(&p);
}

void DXFReaderWriter::prepare_blocks()
{
    for (ContainerGroup &group : _graph->container_groups())
    {
        for (Geo::Geometry *object : group)
        {
            if (Combination *combination = dynamic_cast<Combination *>(object))
            {
                _block_store.push_back(combination);
            }
        }
    }

    std::set<QString> names;
    for (Combination *combination : _block_store)
    {
        if (!combination->name.isEmpty())
        {
            names.insert(combination->name);
        }
    }

    int index = 0;
    for (Combination *combination : _block_store)
    {
        if (combination->name.isEmpty())
        {
            QString name = QString::number(index++);
            while (names.find(name) != names.end())
            {
                name = QString::number(index++);
            }
            combination->name = name;
        }
    }

    std::vector<Geo::AABBRect> rects;
    for (const Combination *combination : _block_store)
    {
        Geo::AABBRect rect = combination->bounding_rect();
        rect.translate(-rect.left(), -rect.bottom());
        rects.emplace_back(rect);
    }
    for (size_t i = 0, count = _block_store.size(); i < count; ++i)
    {
        const Geo::AABBRect &rect0 = rects[i];
        for (size_t j = i + 1; j < count; ++j)
        {
            if (_block_store[i]->size() != _block_store[j]->size())
            {
                continue;
            }
            const Geo::AABBRect &rect1 = rects[j];
            if (rect0[0] != rect1[0] || rect0[2] != rect1[2])
            {
                continue;
            }
            bool is_same = true;
            for (size_t k = 0, count1 = _block_store[i]->size(); k < count1 && is_same; ++k)
            {
                if (_block_store[i]->at(k)->type() != _block_store[j]->at(k)->type())
                {
                    is_same = false;
                    break;
                }
                switch (_block_store[i]->at(k)->type())
                {
                case Geo::Type::ARC:
                    if (const Geo::Arc *arc0 = static_cast<const Geo::Arc *>(_block_store[i]->at(k)),
                        *arc1 = static_cast<const Geo::Arc *>(_block_store[j]->at(k));
                        arc0->radius == arc1->radius && arc0->x - rect0[3].x == arc1->x - rect1[3].x &&
                        arc0->y - rect0[3].y == arc1->y - rect1[3].y)
                    {
                        for (int m = 0; m < 3; ++m)
                        {
                            if (arc0->control_points[m].x - rect0[3].x != arc1->control_points[m].x - rect1[3].x ||
                                arc0->control_points[m].y - rect0[3].y != arc1->control_points[m].y - rect1[3].y)
                            {
                                is_same = false;
                                break;
                            }
                        }
                    }
                    else
                    {
                        is_same = false;
                    }
                    break;
                case Geo::Type::BEZIER:
                    if (const Geo::CubicBezier *bezier0 = static_cast<const Geo::CubicBezier *>(_block_store[i]->at(k)),
                        *bezier1 = static_cast<const Geo::CubicBezier *>(_block_store[j]->at(k));
                        bezier0->size() == bezier1->size())
                    {
                        for (size_t m = 0, count2 = bezier0->size(); m < count2; ++m)
                        {
                            if (bezier0->at(m).x - rect0[3].x != bezier1->at(m).x - rect1[3].x ||
                                bezier0->at(m).y - rect0[3].y != bezier1->at(m).y - rect1[3].y)
                            {
                                is_same = false;
                                break;
                            }
                        }
                    }
                    else
                    {
                        is_same = false;
                    }
                    break;
                case Geo::Type::BSPLINE:
                    if (const Geo::BSpline *bspline0 = static_cast<const Geo::BSpline *>(_block_store[i]->at(k)),
                        *bspline1 = static_cast<const Geo::BSpline *>(_block_store[j]->at(k));
                        bspline0->control_points.size() == bspline1->control_points.size() && bspline0->knots() == bspline1->knots())
                    {
                        for (size_t m = 0, count2 = bspline0->control_points.size(); m < count2; ++m)
                        {
                            if (bspline0->control_points[m].x - rect0[3].x != bspline1->control_points[m].x - rect1[3].x ||
                                bspline0->control_points[m].y - rect0[3].y != bspline1->control_points[m].y - rect1[3].y)
                            {
                                is_same = false;
                                break;
                            }
                        }
                    }
                    else
                    {
                        is_same = false;
                    }
                    break;
                case Geo::Type::CIRCLE:
                    if (const Geo::Circle *circle0 = static_cast<const Geo::Circle *>(_block_store[i]->at(k)),
                        *circle1 = static_cast<const Geo::Circle *>(_block_store[j]->at(k));
                        circle0->radius != circle1->radius || circle0->x - rect0[3].x != circle1->x - rect1[3].x ||
                        circle0->y - rect0[3].y != circle1->y - rect1[3].y)
                    {
                        is_same = false;
                    }
                    break;
                case Geo::Type::ELLIPSE:
                    if (const Geo::Ellipse *ellipse0 = static_cast<const Geo::Ellipse *>(_block_store[i]->at(k)),
                        *ellipse1 = static_cast<const Geo::Ellipse *>(_block_store[j]->at(k));
                        ellipse0->lengtha() != ellipse1->lengtha() || ellipse0->lengthb() != ellipse1->lengthb() ||
                        ellipse0->arc_param0() != ellipse1->arc_param0() || ellipse0->arc_param1() != ellipse1->arc_param1() ||
                        ellipse0->center().x - rect0[3].x != ellipse1->center().x - rect1[3].x ||
                        ellipse0->center().y - rect0[3].y != ellipse1->center().y - rect1[3].y)
                    {
                        is_same = false;
                    }
                    break;
                case Geo::Type::POINT:
                    if (const Geo::Point *point0 = static_cast<const Geo::Point *>(_block_store[i]->at(k)),
                        *point1 = static_cast<const Geo::Point *>(_block_store[j]->at(k));
                        point0->x - rect0[3].x != point1->x - rect1[3].x || point0->y - rect0[3].y != point1->y - rect1[3].y)
                    {
                        is_same = false;
                    }
                    break;
                case Geo::Type::POLYGON:
                case Geo::Type::POLYLINE:
                    if (const Geo::Polyline *polyline0 = static_cast<const Geo::Polyline *>(_block_store[i]->at(k)),
                        *polyline1 = static_cast<const Geo::Polyline *>(_block_store[j]->at(k));
                        polyline0->size() != polyline1->size())
                    {
                        is_same = false;
                    }
                    else
                    {
                        for (size_t m = 0, count2 = polyline0->size(); m < count2; ++m)
                        {
                            if (polyline0->at(m).x - rect0[3].x != polyline1->at(m).x - rect1[3].x ||
                                polyline0->at(m).y - rect0[3].y != polyline1->at(m).y - rect1[3].y)
                            {
                                is_same = false;
                                break;
                            }
                        }
                    }
                    break;
                case Geo::Type::TEXT:
                    if (const Text *text0 = static_cast<const Text *>(_block_store[i]->at(k)),
                        *text1 = static_cast<const Text *>(_block_store[j]->at(k));
                        text0->text() != text1->text() || text0->height() != text1->height() ||
                        text0->anchor() - rect0[3] != text1->anchor() - rect1[3])
                    {
                        is_same = false;
                    }
                    break;
                default:
                    break;
                }
            }
            if (is_same)
            {
                _block_store[j]->name = _block_store[i]->name;
                _block_store.erase(_block_store.begin() + j);
                rects.erase(rects.begin() + j);
                --count;
                --j;
            }
        }
    }
}

void DXFReaderWriter::write_insert(const Combination *combination, const Geo::Point &insertion_point)
{
    DRW_Insert insert;
    insert.layer = _current_group == nullptr ? "0" : _current_group->name.toStdString();
    insert.lineType = "CONTINUOUS";
    insert.name = combination->name.toStdString();
    insert.basePoint.x = insertion_point.x;
    insert.basePoint.y = insertion_point.y;
    insert.basePoint.z = 0.0;
    _dxfrw->writeInsert(&insert);
}

void DXFReaderWriter::write_aligned_dim(const Dim::DimAligned *dim)
{
    DRW_DimAligned d;
    d.type = 1;
    d.setDef1Point(DRW_Coord(dim->anchor[0].x, dim->anchor[0].y, 0));
    d.setDef2Point(DRW_Coord(dim->anchor[1].x, dim->anchor[1].y, 0));
    d.setDimPoint(DRW_Coord(dim->label.x, dim->label.y, 0));
    _dxfrw->writeDimension(&d);
}

void DXFReaderWriter::write_linear_dim(const Dim::DimLinear *dim)
{
    DRW_DimLinear d;
    d.type = 0;
    d.setDef1Point(DRW_Coord(dim->anchor[0].x, dim->anchor[0].y, 0));
    d.setDef2Point(DRW_Coord(dim->anchor[1].x, dim->anchor[1].y, 0));
    d.setDimPoint(DRW_Coord(dim->label.x, dim->label.y, 0));
    d.setAngle(dim->is_horizontal() ? 0.0 : 90.0);
    _dxfrw->writeDimension(&d);
}

void DXFReaderWriter::write_angle_dim(const Dim::DimAngle *dim)
{
    if (dim->point(1) == dim->point(2))
    {
        DRW_DimAngular3p d;
        d.type = 5;
        d.setFirstLine(DRW_Coord(dim->point(0).x, dim->anchor[0].y, 0));
        d.setSecondLine(DRW_Coord(dim->point(3).x, dim->point(3).y, 0));
        d.SetVertexPoint(DRW_Coord(dim->point(1).x, dim->point(1).y, 0));
        d.setDimPoint(DRW_Coord(dim->label.x, dim->label.y, 0));
        _dxfrw->writeDimension(&d);
    }
    else
    {
        DRW_DimAngular d;
        d.type = 2;
        d.setFirstLine1(DRW_Coord(dim->point(0).x, dim->point(0).y, 0));
        d.setFirstLine2(DRW_Coord(dim->point(1).x, dim->point(1).y, 0));
        d.setSecondLine1(DRW_Coord(dim->point(2).x, dim->point(2).y, 0));
        d.setSecondLine2(DRW_Coord(dim->point(3).x, dim->point(3).y, 0));
        d.setDimPoint(DRW_Coord(dim->label.x, dim->label.y, 0));
        _dxfrw->writeDimension(&d);
    }
}

void DXFReaderWriter::write_diameter_dim(const Dim::DimDiameter *dim)
{
    DRW_DimDiametric d;
    d.type = 3;
    d.setDiameter1Point(DRW_Coord(dim->anchor[0].x * 2 - dim->anchor[1].x, dim->anchor[0].y * 2 - dim->anchor[1].y, 0));
    d.setDiameter2Point(DRW_Coord(dim->anchor[1].x, dim->anchor[1].y, 0));
    d.setTextPoint(DRW_Coord(dim->label.x, dim->label.y, 0));
    _dxfrw->writeDimension(&d);
}

void DXFReaderWriter::write_radius_dim(const Dim::DimRadius *dim)
{
    DRW_DimRadial d;
    d.type = 4;
    d.setCenterPoint(DRW_Coord(dim->anchor[0].x, dim->anchor[0].y, 0));
    d.setDiameterPoint(DRW_Coord(dim->anchor[1].x, dim->anchor[1].y, 0));
    d.setTextPoint(DRW_Coord(dim->label.x, dim->label.y, 0));
    _dxfrw->writeDimension(&d);
}

void DXFReaderWriter::write_ordinate_dim(const Dim::DimOrdinate *dim)
{
    DRW_DimOrdinate d;
    d.type = 6;
    d.setOriginPoint(DRW_Coord(0, 0, 0));
    d.setFirstLine(DRW_Coord(dim->root(1).x, dim->root(1).y, 0));
    d.setSecondLine(DRW_Coord(dim->anchor[1].x, dim->anchor[1].y, 0));
    _dxfrw->writeDimension(&d);
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

QString DXFReaderWriter::to_dxf_string(const QString &txt)
{
    QString res;
    int j = 0;
    for (size_t i = 0, count = txt.length(); i < count; ++i)
    {
        int c = txt.at(i).unicode();
        if (c > 175 || c < 11)
        {
            res.append(txt.mid(j, i - j));
            j = i;
            switch (c)
            {
            case 0x0A:
                res += "\\P";
                break;
                // diameter:
            case 0x2205: // RLZ: Empty_set, diameter is 0x2300 need to add in all fonts
            case 0x2300:
                res += "%%C";
                break;
                // degree:
            case 0x00B0:
                res += "%%D";
                break;
                // plus/minus
            case 0x00B1:
                res += "%%P";
                break;
            default:
                --j;
                break;
            }
            ++j;
        }
    }
    res.append(txt.mid(j));
    return res;
}

QString DXFReaderWriter::to_native_string(const QString &txt)
{
    QString res;
    // Ignore font tags:
    int j = 0;
    for (size_t i = 0, count = txt.length(); i < count; ++i)
    {
        if (txt.at(i).unicode() == 0x7B)
        { // is '{' ?
            if (txt.at(i + 1).unicode() == 0x5c)
            { // and is "{\" ?
                // check known codes
                if ((txt.at(i + 2).unicode() == 0x66) || // is "\f" ?
                    (txt.at(i + 2).unicode() == 0x48) || // is "\H" ?
                    (txt.at(i + 2).unicode() == 0x43)    // is "\C" ?
                )
                {
                    // found tag, append parsed part
                    res.append(txt.mid(j, i - j));
                    qsizetype pos = txt.indexOf(QChar(0x7D), i + 3); // find '}'
                    if (pos < 0)
                    {
                        break; //'}' not found
                    }
                    QString tmp = txt.mid(i + 1, pos - i - 1);
                    do
                    {
                        tmp = tmp.remove(0, tmp.indexOf(QChar{0x3B}, 0) + 1); // remove to ';'
                    } while (tmp.startsWith("\\f") || tmp.startsWith("\\H") || tmp.startsWith("\\C"));
                    res.append(tmp);
                    i = j = pos;
                    ++j;
                }
            }
        }
    }
    res.append(txt.mid(j));

    // Line feed:
    res = res.replace(QRegularExpression("\\\\P"), "\n");
    // Space:
    res = res.replace(QRegularExpression("\\\\~"), " ");
    // Tab:
    res = res.replace(QRegularExpression("\\^I"), "    "); // RLZ: change 4 spaces for \t when mtext have support for tab
    // diameter:
    res = res.replace(QRegularExpression("%%[cC]"), QChar(0x2300)); // RLZ: Empty_set is 0x2205, diameter is 0x2300 need to add in all fonts
    // degree:
    res = res.replace(QRegularExpression("%%[dD]"), QChar(0x00B0));
    // plus/minus
    res = res.replace(QRegularExpression("%%[pP]"), QChar(0x00B1));

    return res;
}
