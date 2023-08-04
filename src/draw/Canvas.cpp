#include "draw/Canvas.hpp"
#include <QPalette>
#include "io/GlobalSetting.hpp"


Canvas::Canvas(QLabel **labels, QWidget *parent)
    : QWidget(parent), _info_labels(labels), _input_line(this)
{
    QPalette palette;
    palette.setColor(QPalette::Window, QColor(255, 255, 255));
    setAutoFillBackground(true);
    setPalette(palette);
    setCursor(Qt::CursorShape::CrossCursor);
    setMouseTracking(true);
    setFocusPolicy(Qt::ClickFocus);

    init();
}




void Canvas::init()
{
    _input_line.hide();

    QObject::connect(&_input_line, &QTextEdit::textChanged, this, [this]()
                     {
                                                            if (_last_clicked_obj)
                                                            {
                                                                if (dynamic_cast<Container*>(_last_clicked_obj))
                                                                {
                                                                    reinterpret_cast<Container*>(_last_clicked_obj)->set_text(_input_line.toPlainText());
                                                                }
                                                                else
                                                                {
                                                                    reinterpret_cast<CircleContainer*>(_last_clicked_obj)->set_text(_input_line.toPlainText());
                                                                }
                                                            } });
}

void Canvas::bind_editer(Editer *editer)
{
    _editer = editer;
}

void Canvas::paint_cache()
{
    QPainter painter(this);
    if (_int_flags[0] == 3)
    {
        painter.setPen(QPen(QColor(255, 140, 0), 2, Qt::DashLine));
    }
    else
    {
        painter.setPen(QPen(shape_color, 3));
    }
    painter.setBrush(QColor(250, 250, 250));

    if (!_circle_cache.empty())
    {
        painter.drawEllipse(_circle_cache.center().coord().x - _circle_cache.radius(),
                            _circle_cache.center().coord().y - _circle_cache.radius(),
                            _circle_cache.radius() * 2, _circle_cache.radius() * 2);
    }

    QPolygonF points;
    if (!_rectangle_cache.empty())
    {
        for (const Geo::Point &point : _rectangle_cache)
        {
            points.append(QPointF(point.coord().x, point.coord().y));
        }
        points.pop_back();
        painter.drawPolygon(points);
    }
    if (_editer != nullptr && !_editer->point_cache().empty())
    {
        points.clear();
        for (const Geo::Point &point : _editer->point_cache())
        {
            points.append(QPointF(point.coord().x, point.coord().y));
        }
        painter.drawPolyline(points);
        if (_int_flags[0] == 3)
        {
            painter.setPen(QPen(Qt::blue, 6));
            painter.drawPoints(points);
        }
    }

    if (GlobalSetting::get_instance()->setting()["auto_aligning"].toBool() && !_reflines.empty())
    {
        painter.setPen(QPen(QColor(0, 140, 255), 3));
        for (const QLineF &line : _reflines)
        {
            painter.drawLine(line);
        }
    }
}

void Canvas::paint_graph()
{
    if (_editer->graph() == nullptr || _editer->graph()->empty())
    {
        return;
    }
    QPainter painter(this);
    painter.setBrush(QColor(250, 250, 250));
    const bool scale_text = GlobalSetting::get_instance()->setting()["scale_text"].toBool();
    const double suffix_text_width = 4 * (scale_text ? _ratio : 1), text_heigh_ratio = (scale_text ? _ratio : 1);
    painter.setFont(QFont("SimHei", scale_text ? 12 * _ratio : 12, QFont::Bold, true));

    QFontMetrics font_metrics(painter.font());
    QRectF text_rect;
    QPolygonF points;

    Container *container;
    CircleContainer *circlecontainer;
    Link *link;
    Geo::Polyline *polyline;

    const QPen pen_selected(selected_shape_color, 3), pen_not_selected(shape_color, 3);

    Geo::Point temp_point;
    for (const ContainerGroup &group : _editer->graph()->container_groups())
    {
        if (!group.visible())
        {
            continue;
        }
        for (const Geo::Geometry *geo : group)
        {
            link = dynamic_cast<Link *>(const_cast<Geo::Geometry *>(geo));
            if (link == nullptr || link->empty())
            {
                continue;
            }
            points.clear();
            painter.setPen(link->memo()["is_selected"].to_bool() ? pen_selected : pen_not_selected);
            if (dynamic_cast<CircleContainer *>(const_cast<Geo::Geometry *>(link->tail())) != nullptr)
            {
                temp_point = reinterpret_cast<CircleContainer *>(const_cast<Geo::Geometry *>(link->tail()))->center();
            }
            else
            {
                temp_point = link->tail()->bounding_rect().center();
            }
            points.append(QPointF(temp_point.coord().x, temp_point.coord().y));
            for (const Geo::Point &point : *link)
            {
                points.append(QPointF(point.coord().x, point.coord().y));
            }
            if (dynamic_cast<CircleContainer *>(const_cast<Geo::Geometry *>(link->head())) != nullptr)
            {
                temp_point = reinterpret_cast<CircleContainer *>(const_cast<Geo::Geometry *>(link->head()))->center();
            }
            else
            {
                temp_point = link->head()->bounding_rect().center();
            }
            points.append(QPointF(temp_point.coord().x, temp_point.coord().y));
            painter.drawPolyline(points);
        }

        for (const Geo::Geometry *geo : group)
        {
            points.clear();
            painter.setPen(geo->memo()["is_selected"].to_bool() ? pen_selected : pen_not_selected);
            switch (geo->memo()["Type"].to_int())
            {
            case 0:
                container = reinterpret_cast<Container *>(const_cast<Geo::Geometry *>(geo));
                for (const Geo::Point &point : container->shape())
                {
                    points.append(QPointF(point.coord().x, point.coord().y));
                }
                points.pop_back();
                painter.drawPolygon(points);
                if (!container->text().isEmpty())
                {
                    painter.setPen(QPen(text_color, 2));
                    text_rect = font_metrics.boundingRect(container->text());
                    text_rect.setWidth(text_rect.width() + suffix_text_width);
                    text_rect.setHeight(4 * text_heigh_ratio + 14 * (container->text().count('\n') + 1) * text_heigh_ratio);
                    text_rect.translate(points.boundingRect().center() - text_rect.center());
                    painter.drawText(text_rect, container->text(), QTextOption(Qt::AlignmentFlag::AlignCenter));
                }
                break;
            case 1:
                circlecontainer = reinterpret_cast<CircleContainer *>(const_cast<Geo::Geometry *>(geo));
                painter.drawEllipse(circlecontainer->center().coord().x - circlecontainer->radius(),
                                    circlecontainer->center().coord().y - circlecontainer->radius(),
                                    circlecontainer->radius() * 2, circlecontainer->radius() * 2);
                if (!circlecontainer->text().isEmpty())
                {
                    painter.setPen(QPen(text_color, 2));
                    text_rect = font_metrics.boundingRect(circlecontainer->text());
                    text_rect.setWidth(text_rect.width() + suffix_text_width);
                    text_rect.setHeight(4 * text_heigh_ratio + 14 * (circlecontainer->text().count('\n') + 1) * text_heigh_ratio);
                    text_rect.translate(circlecontainer->center().coord().x - text_rect.center().x(), 
                        circlecontainer->center().coord().y - text_rect.center().y());
                    painter.drawText(text_rect, circlecontainer->text(), QTextOption(Qt::AlignmentFlag::AlignCenter));
                }
                break;
            case 20:
                polyline = reinterpret_cast<Geo::Polyline *>(const_cast<Geo::Geometry *>(geo));
                for (const Geo::Point &point : *polyline)
                {
                    points.append(QPointF(point.coord().x, point.coord().y));
                }
                painter.drawPolyline(points);
                break;
            case 21:
                if (geo->memo()["is_selected"].to_bool())
                {
                    painter.setPen(QPen(QColor(255, 140, 0), 2, Qt::DashLine));
                    for (const Geo::Point &point : *reinterpret_cast<Geo::Bezier *>(const_cast<Geo::Geometry *>(geo)))
                    {
                        points.append(QPointF(point.coord().x, point.coord().y));
                    }
                    painter.drawPolyline(points);
                    painter.setPen(QPen(Qt::blue, 6));
                    painter.drawPoints(points);
                    painter.setPen(pen_selected);
                    points.clear();
                }
                for (const Geo::Point &point : reinterpret_cast<Geo::Bezier *>(const_cast<Geo::Geometry *>(geo))->shape())
                {
                    points.append(QPointF(point.coord().x, point.coord().y));
                }
                painter.drawPolyline(points);
                break;
            default:
                break;
            }
        }
    }
}

void Canvas::paint_select_rect()
{
    if (_select_rect.empty())
    {
        return;
    }
    QPainter painter(this);
    painter.setPen(QPen(QColor(0, 0, 255, 140), 1));
    painter.setBrush(QColor(0, 120, 215, 10));
    
    painter.drawPolygon(QRect(_select_rect[0].coord().x, _select_rect[0].coord().y, 
        Geo::distance(_select_rect[0], _select_rect[1]), Geo::distance(_select_rect[1], _select_rect[2])));
}




void Canvas::paintEvent(QPaintEvent *event)
{
    if (!_select_rect.empty())
    {
        _editer->select(_select_rect);
    }

    paint_graph();

    paint_cache();

    paint_select_rect();
}

void Canvas::mousePressEvent(QMouseEvent *event)
{
    _mouse_pos_1 = event->localPos();
    switch (event->button())
    {
    case Qt::LeftButton:
        if (_bool_flags[1]) // paintable
        {
            switch (_int_flags[0])
            {
            case 0:
                if (!_bool_flags[2]) // not painting
                {
                    _circle_cache = Geo::Circle(_mouse_pos_1.x(), _mouse_pos_1.y(), 10);
                }
                else
                {
                    _editer->append(_circle_cache);
                    _circle_cache.clear();
                    _int_flags[1] = _int_flags[0];
                    _int_flags[0] = -1;
                    _bool_flags[1] = false;
                    emit tool_changed(_int_flags[0]);
                }
                _bool_flags[2] = !_bool_flags[2];
                break;
            case 1:
            case 3:
                if (_bool_flags[2])
                {
                    _editer->point_cache().push_back(Geo::Point(_mouse_pos_1.x(), _mouse_pos_1.y()));
                }
                else
                {
                    _editer->point_cache().push_back(Geo::Point(_mouse_pos_1.x(), _mouse_pos_1.y()));
                    _editer->point_cache().push_back(Geo::Point(_mouse_pos_1.x(), _mouse_pos_1.y()));
                    _bool_flags[2] = true;
                }
                break;
            case 2:
                if (!_bool_flags[2])
                {
                    _last_point.coord().x = _mouse_pos_1.x();
                    _last_point.coord().y = _mouse_pos_1.y();
                    _rectangle_cache = Geo::Rectangle(_mouse_pos_1.x(), _mouse_pos_1.y(), _mouse_pos_1.x() + 2, _mouse_pos_1.y() + 2);
                }
                else
                {
                    _editer->append(_rectangle_cache);
                    _rectangle_cache.clear();
                    _int_flags[1] = _int_flags[0];
                    _int_flags[0] = -1;
                    _bool_flags[1] = false;
                    emit tool_changed(_int_flags[0]);
                }
                _bool_flags[2] = !_bool_flags[2];
                break;
            default:
                break;
            }
            update();
        }
        else
        {
            _clicked_obj = _editer->select(_mouse_pos_1.x(), _mouse_pos_1.y(), false);
            std::list<Geo::Geometry *> selected_objs = _editer->selected();
            if (_clicked_obj == nullptr)
            {
                _editer->reset_selected_mark();
                _select_rect = Geo::Rectangle(_mouse_pos_1.x(), _mouse_pos_1.y(), _mouse_pos_1.x() + 1, _mouse_pos_1.y() + 1);
                _last_point.coord().x = _mouse_pos_1.x();
                _last_point.coord().y = _mouse_pos_1.y();
                _bool_flags[5] = false;
                _input_line.hide();
            }
            else
            {
                if (std::find(selected_objs.begin(), selected_objs.end(), _clicked_obj) == selected_objs.end())
                {
                    _editer->reset_selected_mark();
                    _clicked_obj->memo()["is_selected"] = true;
                }
                _bool_flags[4] = true;
                _bool_flags[5] = true;
                if (GlobalSetting::get_instance()->setting()["auto_aligning"].toBool())
                {
                    _editer->auto_aligning(_clicked_obj, _reflines);
                }
            }
            update();
        }
        break;
    case Qt::RightButton:
        break;
    case Qt::MiddleButton:
        _bool_flags[0] = true;
        break;
    default:
        break;
    }
}

void Canvas::mouseReleaseEvent(QMouseEvent *event)
{
    std::swap(_mouse_pos_0, _mouse_pos_1);
    _mouse_pos_1 = event->localPos();
    switch (event->button())
    {
    case Qt::LeftButton:
        _bool_flags[4] = false;
        if (_bool_flags[1]) // paintable
        {
            if (_circle_cache.empty() && _rectangle_cache.empty() && _info_labels[1])
            {
                _info_labels[1]->clear();
            }
        }
        else
        {
            _select_rect.clear();
            _last_point.clear();
            if (_info_labels[1])
            {
                _info_labels[1]->clear();
            }
            _bool_flags[6] = false;
            _last_clicked_obj = _clicked_obj;
            _editer->auto_aligning(nullptr, _reflines);
            _clicked_obj = nullptr;
            update();
        }
        break;
    case Qt::RightButton:
        break;
    case Qt::MiddleButton:
        _bool_flags[0] = false;
        break;
    default:
        break;
    }
}

void Canvas::mouseMoveEvent(QMouseEvent *event)
{
    const double center_x = size().width() / 2.0, center_y = size().height() / 2.0;
    std::swap(_mouse_pos_0, _mouse_pos_1);
    _mouse_pos_1 = event->localPos();
    if (_info_labels[0])
    {
        _info_labels[0]->setText(std::string("X:").append(std::to_string(static_cast<int>(_mouse_pos_1.x()))).append(" Y:").append(std::to_string(static_cast<int>(_mouse_pos_1.y()))).c_str());
    }
    const double x = _mouse_pos_1.x() - _mouse_pos_0.x(), y = _mouse_pos_1.y() - _mouse_pos_0.y();
    if (_bool_flags[0]) // 视图可移动
    {
        if (!_circle_cache.empty())
        {
            _circle_cache.translate(x, y);
        }
        if (!_rectangle_cache.empty())
        {
            _rectangle_cache.translate(x, y);
        }
        if (_editer->graph() != nullptr)
        {
            _editer->graph()->translate(x, y);
            for (Geo::Point &point : _editer->point_cache())
            {
                point.translate(x, y);
            }
        }
        update();
    }
    if (_bool_flags[1] && _bool_flags[2]) // painting
    {
        switch (_int_flags[0])
        {
        case 0:
            _circle_cache.radius() = Geo::distance(_mouse_pos_1.x(), _mouse_pos_1.y(),
                                                   _circle_cache.center().coord().x, _circle_cache.center().coord().y);
            if (_info_labels[1] != nullptr)
            {
                _info_labels[1]->setText(std::string("Radius:").append(std::to_string(_circle_cache.radius())).c_str());
            }
            break;
        case 1:
            if (event->modifiers() == Qt::ControlModifier)
            {
                const Geo::Coord &coord =_editer->point_cache().at(_editer->point_cache().size() - 2).coord();
                if (std::abs(_mouse_pos_1.x() - coord.x) > std::abs(_mouse_pos_1.y() - coord.y))
                {
                    _editer->point_cache().back().coord().x = _mouse_pos_1.x();
                    _editer->point_cache().back().coord().y = coord.y;
                }
                else
                {
                    _editer->point_cache().back().coord().x = coord.x;
                    _editer->point_cache().back().coord().y = _mouse_pos_1.y();
                }
            }
            else
            {
                _editer->point_cache().back().coord().x = _mouse_pos_1.x();
                _editer->point_cache().back().coord().y = _mouse_pos_1.y();
            }
            if (_info_labels[1] != nullptr)
            {
                _info_labels[1]->setText(std::string("Length:").append(std::to_string(Geo::distance(_editer->point_cache().back(),
                                                                                                    _editer->point_cache()[_editer->point_cache().size() - 2])))
                                             .c_str());
            }
            break;
        case 2:
            _rectangle_cache = Geo::Rectangle(_last_point, Geo::Point(_mouse_pos_1.x(), _mouse_pos_1.y()));
            if (_info_labels[1] != nullptr)
            {
                _info_labels[1]->setText(std::string("Width:").append(std::to_string(std::abs(_mouse_pos_1.x() - _last_point.coord().x))).append(" Height:").append(std::to_string(std::abs(_mouse_pos_1.y() - _last_point.coord().y))).c_str());
            }
            break;
        case 3:
            if (_editer->point_cache().size() > _bezier_order && (_editer->point_cache().size() - 2) % _bezier_order == 0) 
            {
                const size_t count = _editer->point_cache().size();
                _editer->point_cache().back().coord().x = _mouse_pos_1.x();
                if (_editer->point_cache()[count - 2].coord().x == _editer->point_cache()[count - 3].coord().x)
                {
                    _editer->point_cache().back().coord().y = _mouse_pos_1.y();
                }
                else
                {
                    _editer->point_cache().back().coord().y = (_editer->point_cache()[count - 3].coord().y - _editer->point_cache()[count - 2].coord().y) /
                        (_editer->point_cache()[count - 3].coord().x - _editer->point_cache()[count - 2].coord().x) * 
                        (_mouse_pos_1.x() - _editer->point_cache()[count - 2].coord().x) + _editer->point_cache()[count - 2].coord().y;
                }
            }
            else
            {
                _editer->point_cache().back() = Geo::Point(_mouse_pos_1.x(), _mouse_pos_1.y());
            }
            break;
        default:
            if (_info_labels[1] != nullptr)
            {
                _info_labels[1]->clear();
            }
            break;
        }
        update();
    }
    else
    {
        if (_bool_flags[4])
        {
            if (!_bool_flags[6])
            {
                _editer->store_backup();
                _bool_flags[6] = true;
            }
            for (Geo::Geometry *obj : _editer->selected())
            {
                _editer->translate_points(obj, _mouse_pos_0.x(), _mouse_pos_0.y(), _mouse_pos_1.x(), _mouse_pos_1.y(), event->modifiers() == Qt::ControlModifier);
            }
            if (GlobalSetting::get_instance()->setting()["auto_aligning"].toBool())
            {
                _editer->auto_aligning(_clicked_obj, _reflines);
            }
            if (_info_labels[1])
            {
                _info_labels[1]->clear();
            }
        }
        else if (!_select_rect.empty())
        {
            _select_rect = Geo::Rectangle(_last_point.coord().x, _last_point.coord().y, _mouse_pos_1.x(), _mouse_pos_1.y());
            if (_info_labels[1])
            {
                _info_labels[1]->setText(std::string("Width:").append(std::to_string(std::abs(_mouse_pos_1.x() - _last_point.coord().x))).append(" Height:").append(std::to_string(std::abs(_mouse_pos_1.y() - _last_point.coord().y))).c_str());
            }
        }
        update();
    }
}

void Canvas::wheelEvent(QWheelEvent *event)
{
    const double center_x = _mouse_pos_1.x(), center_y = _mouse_pos_1.y();
    if (event->angleDelta().y() > 0 && _ratio < 256)
    {
        _ratio *= 1.25;
        if (!_circle_cache.empty())
        {
            _circle_cache.scale(center_x, center_y, 1.25);
        }
        if (!_rectangle_cache.empty())
        {
            _rectangle_cache.scale(center_x, center_y, 1.25);
        }
        if (_editer->graph() != nullptr)
        {
            _editer->graph()->scale(center_x, center_y, 1.25);
            for (Geo::Point &point : _editer->point_cache())
            {
                point.scale(center_x, center_y, 1.25);
            }
        }
        update();
    }
    else if (event->angleDelta().y() < 0 && _ratio > (1.0 / 256.0))
    {
        _ratio *= 0.75;
        if (!_circle_cache.empty())
        {
            _circle_cache.scale(center_x, center_y, 0.75);
        }
        if (!_rectangle_cache.empty())
        {
            _rectangle_cache.scale(center_x, center_y, 0.75);
        }
        if (_editer->graph() != nullptr)
        {
            _editer->graph()->scale(center_x, center_y, 0.75);
            for (Geo::Point &point : _editer->point_cache())
            {
                point.scale(center_x, center_y, 0.75);
            }
        }
        update();
    }
    _editer->auto_aligning(_clicked_obj, _reflines);
}

void Canvas::mouseDoubleClickEvent(QMouseEvent *event)
{
    _mouse_pos_1 = event->localPos();
    switch (event->button())
    {
    case Qt::LeftButton:
        if (_bool_flags[1] && _bool_flags[2]) // paintable and painting
        {
            _bool_flags[1] = false;
            _bool_flags[2] = false;
            switch (_int_flags[0])
            {
            case 0:
                _circle_cache.clear();
                update();
                break;
            case 1:
                if (_editer != nullptr)
                {
                    _editer->append_points();
                    update();
                }
                break;
            case 2:
                _rectangle_cache.clear();
                update();
                break;
            case 3:
                if (_editer != nullptr)
                {
                    _editer->append_bezier(_bezier_order);
                    update();
                }
                break;
            default:
                break;
            }
            _int_flags[1] = _int_flags[0];
            _int_flags[0] = -1;
            emit tool_changed(_int_flags[0]);
        }
        else
        {
            if (_bool_flags[5])
            {
                const Geo::Rectangle rect(_last_clicked_obj->bounding_rect());
                _input_line.setMaximumSize(std::max(100.0, rect.width()), std::max(100.0, rect.heigh()));
                _input_line.move(rect.center().coord().x - _input_line.rect().center().x(),
                                 rect.center().coord().y - _input_line.rect().center().y());
                _input_line.setFocus();
                if (dynamic_cast<Container *>(_last_clicked_obj))
                {
                    _input_line.setText(reinterpret_cast<Container *>(_last_clicked_obj)->text());
                }
                else
                {
                    _input_line.setText(reinterpret_cast<CircleContainer *>(_last_clicked_obj)->text());
                }
                _input_line.moveCursor(QTextCursor::End);
                _input_line.show();
            }
        }
        break;
    case Qt::RightButton:
        break;
    case Qt::MiddleButton:
        _last_point = center();
        if (!_circle_cache.empty())
        {
            _circle_cache.scale(_last_point.coord().x, _last_point.coord().y, 1.0 / _ratio);
        }
        if (!_rectangle_cache.empty())
        {
            _rectangle_cache.scale(_last_point.coord().x, _last_point.coord().y, 1.0 / _ratio);
        }
        if (_editer->graph() != nullptr && !_editer->graph()->empty())
        {
            _editer->graph()->rescale(_last_point.coord().x, _last_point.coord().y);
            _editer->graph()->translate(size().width() / 2 - _last_point.coord().x, size().height() / 2 - _last_point.coord().y);
        }
        if (!_circle_cache.empty())
        {
            _circle_cache.translate(size().width() / 2 - _last_point.coord().x, size().height() / 2 - _last_point.coord().y);
        }
        if (!_rectangle_cache.empty())
        {
            _rectangle_cache.translate(size().width() / 2 - _last_point.coord().x, size().height() / 2 - _last_point.coord().y);
        }

        _ratio = 1;
        update();
        break;
    default:
        break;
    }

    QWidget::mouseDoubleClickEvent(event);
}




void Canvas::use_tool(const int value)
{
    switch (value)
    {
    case -1:
    case 0:
    case 1:
    case 2:
    case 3:
        _int_flags[1] = _int_flags[0];
        _int_flags[0] = value;
        _bool_flags[1] = (value != -1);
        _bool_flags[2] = false;
        _editer->point_cache().clear();
        _circle_cache.clear();
        _rectangle_cache.clear();
        // setMouseTracking(value != -1);
        emit tool_changed(_int_flags[0]);
        update();
        break;
    default:
        break;
    }
}

const bool Canvas::is_painting() const
{
    return _bool_flags[2];
}

const bool Canvas::is_typing() const
{
    return _input_line.isVisible();
}

const bool Canvas::is_moving() const
{
    return _bool_flags[6];
}

const size_t Canvas::current_group() const
{
    return _editer->current_group();
}

void Canvas::set_current_group(const size_t index)
{
    assert(index < _editer->graph()->container_groups().size());
    _editer->set_current_group(index);
}

const size_t Canvas::groups_count() const
{
    return _editer->groups_count();
}

void Canvas::set_bezier_order(const size_t order)
{
    _bezier_order = order;
}
    
const size_t Canvas::bezier_order() const
{
    return _bezier_order;
}




Geo::Point Canvas::center() const
{
    if (_circle_cache.empty() && _rectangle_cache.empty() && (_editer->graph() == nullptr || _editer->graph()->empty()))
    {
        return Geo::Point();
    }

    double x0 = DBL_MAX, y0 = DBL_MAX, x1 = (-FLT_MAX), y1 = (-FLT_MAX);
    for (const Geo::Point &point : _rectangle_cache)
    {
        x0 = std::min(x0, point.coord().x);
        y0 = std::min(y0, point.coord().y);
        x1 = std::max(x1, point.coord().x);
        y1 = std::max(y1, point.coord().y);
    }
    if (!_circle_cache.empty())
    {
        x0 = std::min(x0, _circle_cache.center().coord().x - _circle_cache.radius());
        y0 = std::min(y0, _circle_cache.center().coord().y - _circle_cache.radius());
        x1 = std::max(x1, _circle_cache.center().coord().x + _circle_cache.radius());
        y1 = std::max(y1, _circle_cache.center().coord().y + _circle_cache.radius());
    }

    if (_editer->graph() == nullptr || _editer->graph()->empty())
    {
        return Geo::Point((x0 + x1) / 2, (y0 + y1) / 2);
    }

    for (const ContainerGroup &group : _editer->graph()->container_groups())
    {
        for (const Geo::Point &point : group.bounding_rect())
        {
            x0 = std::min(x0, point.coord().x);
            y0 = std::min(y0, point.coord().y);
            x1 = std::max(x1, point.coord().x);
            y1 = std::max(y1, point.coord().y);
        }
    }

    return Geo::Point((x0 + x1) / 2, (y0 + y1) / 2);
}

Geo::Rectangle Canvas::bounding_rect() const
{
    if (_circle_cache.empty() && _rectangle_cache.empty() && (_editer->graph() == nullptr || _editer->graph()->empty()))
    {
        return Geo::Rectangle();
    }

    double x0 = DBL_MAX, y0 = DBL_MAX, x1 = (-FLT_MAX), y1 = (-FLT_MAX);
    for (const Geo::Point &point : _rectangle_cache)
    {
        x0 = std::min(x0, point.coord().x);
        y0 = std::min(y0, point.coord().y);
        x1 = std::max(x1, point.coord().x);
        y1 = std::max(y1, point.coord().y);
    }
    if (!_circle_cache.empty())
    {
        x0 = std::min(x0, _circle_cache.center().coord().x - _circle_cache.radius());
        y0 = std::min(y0, _circle_cache.center().coord().y - _circle_cache.radius());
        x1 = std::max(x1, _circle_cache.center().coord().x + _circle_cache.radius());
        y1 = std::max(y1, _circle_cache.center().coord().y + _circle_cache.radius());
    }

    if (_editer->graph() == nullptr || _editer->graph()->empty())
    {
        return Geo::Rectangle(x0, y0, x1, y1);
    }

    for (const ContainerGroup &group : _editer->graph()->container_groups())
    {
        for (const Geo::Point &point : group.bounding_rect())
        {
            x0 = std::min(x0, point.coord().x);
            y0 = std::min(y0, point.coord().y);
            x1 = std::max(x1, point.coord().x);
            y1 = std::max(y1, point.coord().y);
        }
    }

    return Geo::Rectangle(x0, y0, x1, y1);
}

Geo::Coord Canvas::mouse_position() const
{
    return Geo::Coord(_mouse_pos_1.x(), _mouse_pos_1.y());
}

const bool Canvas::empty() const
{
    return _circle_cache.empty() && _rectangle_cache.empty() &&
           (_editer == nullptr || _editer->graph() == nullptr || _editer->graph()->empty());
}

void Canvas::cancel_painting()
{
    _bool_flags[1] = false;
    _bool_flags[2] = false;
    _int_flags[1] = _int_flags[0];
    _int_flags[0] = -1;
    emit tool_changed(_int_flags[0]);
    _editer->point_cache().clear();
    _circle_cache.clear();
    _rectangle_cache.clear();
    update();
}

void Canvas::use_last_tool()
{
    if (_bool_flags[2])
    {
        return;
    }
    _int_flags[0] = _int_flags[1];
    if (_int_flags[0] >= 0)
    {
        _bool_flags[1] = true;
        emit tool_changed(_int_flags[0]);
    }
}

void Canvas::set_info_labels(QLabel **labels)
{
    _info_labels = labels;
}

void Canvas::copy()
{
    _stored_mouse_pos = _mouse_pos_1;
    _editer->copy_selected();
}

void Canvas::cut()
{
    _stored_mouse_pos = _mouse_pos_1;
    _editer->cut_selected();
}

void Canvas::paste()
{
    if (_editer->paste(_mouse_pos_1.x() - _stored_mouse_pos.x(), _mouse_pos_1.y() - _stored_mouse_pos.y()))
    {
        update();
    }
}