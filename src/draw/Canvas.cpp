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
    _select_rect.clear();
}

void Canvas::bind_editer(Editer *editer)
{
    _editer = editer;
}

void Canvas::paint_cache()
{
    QPainter painter(this);
    if (_tool_flags[0] == Tool::CURVE)
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
        Geo::Circle circle(_circle_cache);
        circle.transform(_canvas_ctm[0], _canvas_ctm[3], _canvas_ctm[6], _canvas_ctm[1], _canvas_ctm[4], _canvas_ctm[7]);
        painter.drawEllipse(circle.center().coord().x - circle.radius(),
                            circle.center().coord().y - circle.radius(),
                            circle.radius() * 2, circle.radius() * 2);
    }

    QPolygonF points;
    if (!_AABBRect_cache.empty())
    {
        for (const Geo::Point &point : _AABBRect_cache)
        {
            points.append(QPointF(point.coord().x * _canvas_ctm[0] + point.coord().y * _canvas_ctm[3] + _canvas_ctm[6], 
                point.coord().x * _canvas_ctm[1] + point.coord().y * _canvas_ctm[4] + _canvas_ctm[7]));
        }
        points.pop_back();
        painter.drawPolygon(points);
    }
    if (_editer != nullptr && !_editer->point_cache().empty())
    {
        points.clear();
        for (const Geo::Point &point : _editer->point_cache())
        {
            points.append(QPointF(point.coord().x * _canvas_ctm[0] + point.coord().y * _canvas_ctm[3] + _canvas_ctm[6], 
                point.coord().x * _canvas_ctm[1] + point.coord().y * _canvas_ctm[4] + _canvas_ctm[7]));
        }
        painter.drawPolyline(points);
        if (_tool_flags[0] == Tool::CURVE)
        {
            painter.setPen(QPen(Qt::blue, 6));
            painter.drawPoints(points);
        }
    }

    if (!_reflines.empty())
    {
        painter.setPen(QPen(QColor(0, 140, 255), 3));
        for (const QLineF &line : _reflines)
        {
            painter.drawLine(line.x1() * _canvas_ctm[0] + line.y1() * _canvas_ctm[3] + _canvas_ctm[6],
                line.x1() * _canvas_ctm[1] + line.y1() * _canvas_ctm[4] + _canvas_ctm[7],
                line.x2() * _canvas_ctm[0] + line.y2() * _canvas_ctm[3] + _canvas_ctm[6],
                line.x2() * _canvas_ctm[1] + line.y2() * _canvas_ctm[4] + _canvas_ctm[7]);
        }
        _reflines.clear();
    }
}

void Canvas::paint_graph()
{
    if (_editer->graph() == nullptr || _editer->graph()->empty())
    {
        return;
    }
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setBrush(QColor(250, 250, 250));
    const bool show_text = GlobalSetting::get_instance()->setting()["show_text"].toBool();
    QFont font("SimSun");
    font.setPixelSize(GlobalSetting::get_instance()->setting()["text_size"].toInt());
    painter.setFont(font);

    const bool show_points = GlobalSetting::get_instance()->setting()["show_points"].toBool();

    QFontMetrics font_metrics(painter.font());
    QRectF text_rect;
    QPolygonF points;

    const Text *text;
    const Container *container;
    const CircleContainer *circlecontainer;
    const Geo::Polyline *polyline;

    const QPen pen_selected(selected_shape_color, 3), pen_not_selected(shape_color, 3);

    Geo::Point temp_point;
    Geo::Coord center;
    Geo::Circle circle;
    double radius;
    long long width, height;
    for (const ContainerGroup &group : _editer->graph()->container_groups())
    {
        if (!group.visible())
        {
            continue;
        }

        for (const Geo::Geometry *geo : group)
        {
            points.clear();
            painter.setPen(geo->is_selected() ? pen_selected : pen_not_selected);
            switch (geo->type())
            {
            case Geo::Type::TEXT:
                text = dynamic_cast<const Text *>(geo);
                if (text->is_selected())
                {
                    for (const Geo::Point &point : text->shape())
                    {
                        points.append(QPointF(point.coord().x * _canvas_ctm[0] + point.coord().y * _canvas_ctm[3] + _canvas_ctm[6],
                            point.coord().x * _canvas_ctm[1] + point.coord().y * _canvas_ctm[4] + _canvas_ctm[7]));
                    }
                    points.pop_back();
                    painter.drawPolygon(points);
                    painter.setPen(pen_not_selected);
                }
                painter.drawText(points.boundingRect(), container->text(), QTextOption(Qt::AlignmentFlag::AlignCenter));
                break;
            case Geo::Type::CONTAINER:
                container = dynamic_cast<const Container *>(geo);
                if (!is_visible(container->shape()))
                {
                    continue;
                }
                for (const Geo::Point &point : container->shape())
                {
                    points.append(QPointF(point.coord().x * _canvas_ctm[0] + point.coord().y * _canvas_ctm[3] + _canvas_ctm[6],
                        point.coord().x * _canvas_ctm[1] + point.coord().y * _canvas_ctm[4] + _canvas_ctm[7]));
                }
                points.pop_back();
                _catched_points.append(points);
                painter.drawPolygon(points);
                if (show_points)
                {
                    painter.setRenderHint(QPainter::Antialiasing);
                    painter.setPen(QPen(QColor(0, 140, 255), 6));
                    painter.drawPoints(points);
                }
                if (show_text && !container->text().isEmpty())
                {
                    painter.setPen(QPen(text_color, 2));
                    width = height = 0;
                    for (const QString &s : container->text().split('\n'))
                    {
                        width = std::max(width, font.pixelSize() * s.length());
                    }
                    if (width == 0)
                    {
                        width = font.pixelSize() * container->text().length();
                    }
                    width = std::max(20ll, width);
                    height = std::max(font_metrics.lineSpacing() * (container->text().count('\n') + 1), 20ll);
                    text_rect.setWidth(width);
                    text_rect.setHeight(height);
                    text_rect.translate(points.boundingRect().center() - text_rect.center());
                    painter.drawText(text_rect, container->text(), QTextOption(Qt::AlignmentFlag::AlignCenter));
                }
                break;
            case Geo::Type::CIRCLECONTAINER:
                circlecontainer = dynamic_cast<const CircleContainer *>(geo);
                if (!is_visible(circlecontainer->shape()))
                {
                    continue;
                }
                circle.radius() = circlecontainer->radius();
                circle.center() = circlecontainer->center();
                circle.transform(_canvas_ctm[0], _canvas_ctm[3], _canvas_ctm[6], _canvas_ctm[1], _canvas_ctm[4], _canvas_ctm[7]);
                
                center = circle.center().coord();
                radius = circle.radius();
                painter.drawEllipse(center.x - radius, center.y - radius, radius * 2, radius * 2);
                _catched_points.emplace_back(QPointF(center.x, center.y));
                _catched_points.emplace_back(QPointF(center.x, center.y + radius));
                _catched_points.emplace_back(QPointF(center.x + radius, center.y));
                _catched_points.emplace_back(QPointF(center.x, center.y - radius));
                _catched_points.emplace_back(QPointF(center.x - radius, center.y));
                if (show_points)
                {
                    painter.setRenderHint(QPainter::Antialiasing);
                    painter.setPen(QPen(QColor(0, 140, 255), 6));
                    painter.drawPoint(center.x, center.y);
                    painter.drawPoint(center.x, center.y + radius);
                    painter.drawPoint(center.x + radius, center.y);
                    painter.drawPoint(center.x, center.y - radius);
                    painter.drawPoint(center.x - radius, center.y);
                }
                if (show_text && !circlecontainer->text().isEmpty())
                {
                    painter.setPen(QPen(text_color, 2));
                    width = height = 0;
                    for (const QString &s : circlecontainer->text().split('\n'))
                    {
                        width = std::max(width, font.pointSize() * s.length());
                    }
                    if (width == 0)
                    {
                        width = font.pointSize() * circlecontainer->text().length();
                    }
                    width = std::max(20ll, width);
                    height = std::max(font_metrics.lineSpacing() * (circlecontainer->text().count('\n') + 1), 20ll);
                    text_rect.setWidth(width);
                    text_rect.setHeight(height);
                    text_rect.translate(circlecontainer->center().coord().x - text_rect.center().x(), 
                        circlecontainer->center().coord().y - text_rect.center().y());
                    painter.drawText(text_rect, circlecontainer->text(), QTextOption(Qt::AlignmentFlag::AlignCenter));
                }
                break;
            case Geo::Type::COMBINATION:
                painter.setPen(geo->is_selected() ? pen_selected : pen_not_selected);
                for (const Geo::Geometry *item : *dynamic_cast<const Combination *>(geo))
                {
                    points.clear();
                    switch (item->type())
                    {
                    case Geo::Type::TEXT:
                        text = dynamic_cast<const Text *>(item);
                        if (text->is_selected())
                        {
                            for (const Geo::Point &point : text->shape())
                            {
                                points.append(QPointF(point.coord().x * _canvas_ctm[0] + point.coord().y * _canvas_ctm[3] + _canvas_ctm[6],
                                    point.coord().x * _canvas_ctm[1] + point.coord().y * _canvas_ctm[4] + _canvas_ctm[7]));
                            }
                            points.pop_back();
                            painter.drawPolygon(points);
                            painter.setPen(pen_not_selected);
                        }
                        painter.drawText(points.boundingRect(), container->text(), QTextOption(Qt::AlignmentFlag::AlignCenter));
                        break;
                    case Geo::Type::CONTAINER:
                        container = dynamic_cast<const Container *>(item);
                        if (!is_visible(container->shape()))
                        {
                            continue;
                        }
                        for (const Geo::Point &point : container->shape())
                        {
                            points.append(QPointF(point.coord().x * _canvas_ctm[0] + point.coord().y * _canvas_ctm[3] + _canvas_ctm[6],
                                point.coord().x * _canvas_ctm[1] + point.coord().y * _canvas_ctm[4] + _canvas_ctm[7]));
                        }
                        points.pop_back();
                        _catched_points.append(points);
                        painter.drawPolygon(points);
                        if (show_points)
                        {
                            painter.setRenderHint(QPainter::Antialiasing);
                            painter.setPen(QPen(QColor(0, 140, 255), 6));
                            painter.drawPoints(points);
                        }
                        if (show_text && !container->text().isEmpty())
                        {
                            painter.setPen(QPen(text_color, 2));
                            width = height = 0;
                            for (const QString &s : container->text().split('\n'))
                            {
                                width = std::max(width, font.pixelSize() * s.length());
                            }
                            if (width == 0)
                            {
                                width = font.pixelSize() * container->text().length();
                            }
                            width = std::max(20ll, width);
                            height = std::max(font_metrics.lineSpacing() * (container->text().count('\n') + 1), 20ll);
                            text_rect.setWidth(width);
                            text_rect.setHeight(height);
                            text_rect.translate(points.boundingRect().center() - text_rect.center());
                            painter.drawText(text_rect, container->text(), QTextOption(Qt::AlignmentFlag::AlignCenter));
                        }
                        break;
                    case Geo::Type::CIRCLECONTAINER:
                        circlecontainer = dynamic_cast<const CircleContainer *>(item);
                        if (!is_visible(circlecontainer->shape()))
                        {
                            continue;
                        }
                        circle.center() = circlecontainer->center();
                        circle.radius() = circlecontainer->radius();
                        circle.transform(_canvas_ctm[0], _canvas_ctm[3], _canvas_ctm[6], _canvas_ctm[1], _canvas_ctm[4], _canvas_ctm[7]);

                        center = circle.center().coord();
                        radius = circle.radius();
                        painter.drawEllipse(center.x - radius, center.y - radius, radius * 2, radius * 2);
                        _catched_points.emplace_back(QPointF(center.x, center.y));
                        _catched_points.emplace_back(QPointF(center.x, center.y + radius));
                        _catched_points.emplace_back(QPointF(center.x + radius, center.y));
                        _catched_points.emplace_back(QPointF(center.x, center.y - radius));
                        _catched_points.emplace_back(QPointF(center.x - radius, center.y));
                        if (show_points)
                        {
                            painter.setRenderHint(QPainter::Antialiasing);
                            painter.setPen(QPen(QColor(0, 140, 255), 6));
                            painter.drawPoint(center.x, center.y);
                            painter.drawPoint(center.x, center.y + radius);
                            painter.drawPoint(center.x + radius, center.y);
                            painter.drawPoint(center.x, center.y - radius);
                            painter.drawPoint(center.x - radius, center.y);
                        }
                        if (show_text && !circlecontainer->text().isEmpty())
                        {
                            painter.setPen(QPen(text_color, 2));
                            width = height = 0;
                            for (const QString &s : circlecontainer->text().split('\n'))
                            {
                                width = std::max(width, font.pixelSize() * s.length());
                            }
                            if (width == 0)
                            {
                                width = font.pixelSize() * circlecontainer->text().length();
                            }
                            width = std::max(20ll, width);
                            height = std::max(font_metrics.lineSpacing() * (circlecontainer->text().count('\n') + 1), 20ll);
                            text_rect.setWidth(width);
                            text_rect.setHeight(height);
                            text_rect.translate(circlecontainer->center().coord().x - text_rect.center().x(), 
                                circlecontainer->center().coord().y - text_rect.center().y());
                            painter.drawText(text_rect, circlecontainer->text(), QTextOption(Qt::AlignmentFlag::AlignCenter));
                        }
                        break;
                    case Geo::Type::POLYLINE:
                        polyline = dynamic_cast<const Geo::Polyline *>(item);
                        if (!is_visible(*polyline))
                        {
                            continue;
                        } 
                        for (const Geo::Point &point : *polyline)
                        {
                            points.append(QPointF(point.coord().x * _canvas_ctm[0] + point.coord().y * _canvas_ctm[3] + _canvas_ctm[6],
                                point.coord().x * _canvas_ctm[1] + point.coord().y * _canvas_ctm[4] + _canvas_ctm[7]));
                        }
                        _catched_points.append(points);
                        painter.drawPolyline(points);
                        if (show_points)
                        {
                            painter.setRenderHint(QPainter::Antialiasing);
                            painter.setPen(QPen(QColor(0, 140, 255), 6));
                            painter.drawPoints(points);
                        }
                        break;
                    case Geo::Type::BEZIER:
                        if (!is_visible(dynamic_cast<const Geo::Bezier *>(item)->shape()))
                        {
                            continue;
                        }
                        for (const Geo::Point &point : dynamic_cast<const Geo::Bezier *>(item)->shape())
                        {
                            points.append(QPointF(point.coord().x * _canvas_ctm[0] + point.coord().y * _canvas_ctm[3] + _canvas_ctm[6],
                                point.coord().x * _canvas_ctm[1] + point.coord().y * _canvas_ctm[4] + _canvas_ctm[7]));
                        }
                        painter.drawPolyline(points);
                        if (show_points)
                        {
                            painter.setRenderHint(QPainter::Antialiasing);
                            painter.setPen(QPen(QColor(0, 140, 255), 6));
                            painter.drawPoint(points.front());
                            painter.drawPoint(points.back());
                        }
                        break;
                    default:
                        break;
                    }
                }
                break;
            case Geo::Type::POLYLINE:
                polyline = dynamic_cast<const Geo::Polyline *>(geo);
                if (!is_visible(*polyline))
                {
                    continue;
                }
                for (const Geo::Point &point : *polyline)
                {
                    points.append(QPointF(point.coord().x * _canvas_ctm[0] + point.coord().y * _canvas_ctm[3] + _canvas_ctm[6],
                        point.coord().x * _canvas_ctm[1] + point.coord().y * _canvas_ctm[4] + _canvas_ctm[7]));
                }
                _catched_points.append(points);
                painter.drawPolyline(points);
                if (show_points)
                {
                    painter.setRenderHint(QPainter::Antialiasing);
                    painter.setPen(QPen(QColor(0, 140, 255), 6));
                    painter.drawPoints(points);
                }
                break;
            case Geo::Type::BEZIER:
                if (!is_visible(dynamic_cast<const Geo::Bezier *>(geo)->shape()))
                {
                    continue;
                }
                if (geo->is_selected())
                {
                    painter.setPen(QPen(QColor(255, 140, 0), 2, Qt::DashLine));
                    for (const Geo::Point &point : *dynamic_cast<const Geo::Bezier *>(geo))
                    {
                        points.append(QPointF(point.coord().x * _canvas_ctm[0] + point.coord().y * _canvas_ctm[3] + _canvas_ctm[6],
                            point.coord().x * _canvas_ctm[1] + point.coord().y * _canvas_ctm[4] + _canvas_ctm[7]));
                    }
                    painter.drawPolyline(points);
                    painter.setPen(QPen(Qt::blue, 6));
                    painter.drawPoints(points);
                    painter.setPen(pen_selected);
                    points.clear();
                }
                for (const Geo::Point &point : dynamic_cast<const Geo::Bezier *>(geo)->shape())
                {
                    points.append(QPointF(point.coord().x * _canvas_ctm[0] + point.coord().y * _canvas_ctm[3] + _canvas_ctm[6],
                        point.coord().x * _canvas_ctm[1] + point.coord().y * _canvas_ctm[4] + _canvas_ctm[7]));
                }
                painter.drawPolyline(points);
                if (show_points)
                {
                    painter.setRenderHint(QPainter::Antialiasing);
                    painter.setPen(QPen(QColor(0, 140, 255), 6));
                    painter.drawPoint(points.front());
                    painter.drawPoint(points.back());
                }
                break;
            default:
                break;
            }
        }
    }

    for (QPointF &point : _catched_points)
    {
        Geo::Coord coord(canvas_coord_to_real_coord(point.x(), point.y()));
        point.setX(coord.x);
        point.setY(coord.y);
    }
}

void Canvas::paint_select_rect()
{
    QPainter painter(this);

    if (origin_visible())
    {
        painter.setPen(QPen(Qt::black, 1, Qt::DotLine));
        painter.drawLine(_canvas_ctm[6] - 20, _canvas_ctm[7], _canvas_ctm[6] + 20, _canvas_ctm[7]);
        painter.drawLine(_canvas_ctm[6], _canvas_ctm[7] - 20, _canvas_ctm[6], _canvas_ctm[7] + 20);
        painter.drawEllipse(QPoint(_canvas_ctm[6], _canvas_ctm[7]), 10, 10);
    }

    if (_select_rect.empty())
    {
        return;
    }
    
    painter.setPen(QPen(QColor(0, 0, 255, 140), 1));
    painter.setBrush(QColor(0, 120, 215, 10));

    Geo::AABBRect rect(_select_rect);
    rect.transform(_canvas_ctm[0], _canvas_ctm[3], _canvas_ctm[6], _canvas_ctm[1], _canvas_ctm[4], _canvas_ctm[7]);
    
    painter.drawPolygon(QRect(rect[3].coord().x, rect[3].coord().y, rect.width(), rect.height()));
}




void Canvas::paintEvent(QPaintEvent *event)
{
    if (!_select_rect.empty())
    {
        _editer->select(_select_rect);
    }

    _catched_points.clear();
    paint_graph();

    paint_cache();

    paint_select_rect();
}

void Canvas::mousePressEvent(QMouseEvent *event)
{
    _mouse_pos_1 = event->localPos();
    const double real_x1 = _mouse_pos_1.x() * _view_ctm[0] + _mouse_pos_1.y() * _view_ctm[3] + _view_ctm[6];
    const double real_y1 = _mouse_pos_1.x() * _view_ctm[1] + _mouse_pos_1.y() * _view_ctm[4] + _view_ctm[7];
    switch (event->button())
    {
    case Qt::LeftButton:
        if (is_paintable()) // paintable
        {
            switch (_tool_flags[0])
            {
            case Tool::CIRCLE:
                if (!is_painting()) // not painting
                {
                    _circle_cache = Geo::Circle(real_x1, real_y1, 10);
                }
                else
                {
                    _editer->append(_circle_cache);
                    _circle_cache.clear();
                    _tool_flags[1] = _tool_flags[0];
                    _tool_flags[0] = Tool::NONE;
                    _bool_flags[1] = false; // moveable
                    emit tool_changed(_tool_flags[0]);
                }
                _bool_flags[2] = !_bool_flags[2]; // painting
                break;
            case Tool::POLYLINE:
            case Tool::CURVE:
                if (is_painting())
                {
                    _editer->point_cache().emplace_back(Geo::Point(real_x1, real_y1));
                }
                else
                {
                    _editer->point_cache().emplace_back(Geo::Point(real_x1, real_y1));
                    _editer->point_cache().emplace_back(Geo::Point(real_x1, real_y1));
                    _bool_flags[2] = true; // painting
                }
                break;
            case Tool::RECT:
                if (!is_painting())
                {
                    _last_point.coord().x = real_x1;
                    _last_point.coord().y = real_y1;
                    _AABBRect_cache = Geo::AABBRect(real_x1, real_y1, real_x1 + 2, real_y1 + 2);
                }
                else
                {
                    _editer->append(_AABBRect_cache);
                    _AABBRect_cache.clear();
                    _tool_flags[1] = _tool_flags[0];
                    _tool_flags[0] = Tool::NONE;
                    _bool_flags[1] = false; // paintable
                    emit tool_changed(_tool_flags[0]);
                }
                _bool_flags[2] = !_bool_flags[2]; // painting
                break;
            default:
                break;
            }
            update();
        }
        else
        {
            const bool reset = !(GlobalSetting::get_instance()->setting()["multiple_select"].toBool()
                || event->modifiers() == Qt::ControlModifier);
            _clicked_obj = _editer->select(real_x1, real_y1, reset);
            std::list<Geo::Geometry *> selected_objs = _editer->selected();
            if (_clicked_obj == nullptr)
            {
                _editer->reset_selected_mark();
                _select_rect = Geo::AABBRect(real_x1, real_y1, real_x1 + 1, real_y1 + 1);
                _last_point.coord().x = real_x1;
                _last_point.coord().y = real_y1;
                _bool_flags[5] = false; // is obj selected
                if (_input_line.isVisible() && _last_clicked_obj != nullptr)
                {
                    if (dynamic_cast<Container *>(_last_clicked_obj) != nullptr)
                    {
                        dynamic_cast<Container *>(_last_clicked_obj)->set_text(_input_line.toPlainText());   
                    }
                    else
                    {
                        dynamic_cast<CircleContainer *>(_last_clicked_obj)->set_text(_input_line.toPlainText());
                    }
                }
            }
            else
            {
                if (std::find(selected_objs.begin(), selected_objs.end(), _clicked_obj) == selected_objs.end())
                {
                    _editer->reset_selected_mark();
                    _clicked_obj->is_selected() = true;
                }
                _bool_flags[4] = true; // is obj moveable
                _bool_flags[5] = true; // is obj selected
                bool catched_point = false;
                if (GlobalSetting::get_instance()->setting()["cursor_catch"].toBool())
                {
                    const double catch_distance = GlobalSetting::get_instance()->setting()["catch_distance"].toDouble();
                    for (const QPointF &point : _catched_points)
                    {
                        if (Geo::distance(point.x(), point.y(), real_x1, real_y1) < catch_distance)
                        {
                            catched_point = true;
                            Geo::Coord coord(real_coord_to_view_coord(point.x(), point.y()));
                            _mouse_pos_1.setX(coord.x);
                            _mouse_pos_1.setY(coord.y);
                            QCursor::setPos(this->mapToGlobal(_mouse_pos_1).x(), this->mapToGlobal(_mouse_pos_1).y());
                            break;
                        }
                    }
                }
                if (!catched_point && GlobalSetting::get_instance()->setting()["auto_aligning"].toBool())
                {
                    _editer->auto_aligning(_clicked_obj, real_x1, real_y1, _reflines,
                        GlobalSetting::get_instance()->setting()["active_layer_catch_only"].toBool());
                }
            }
            _input_line.clear();
            _input_line.hide();
            update();
        }
        break;
    case Qt::RightButton:
        break;
    case Qt::MiddleButton:
        _bool_flags[0] = true; // view moveable
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
        _bool_flags[4] = false; // is obj moveable
        if (is_paintable()) // paintable
        {
            if (_circle_cache.empty() && _AABBRect_cache.empty() && _info_labels[1])
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
            _bool_flags[6] = false; // is moving obj
            _last_clicked_obj = _clicked_obj;
            update();
        }
        break;
    case Qt::RightButton:
        break;
    case Qt::MiddleButton:
        _bool_flags[0] = false; // view moveable
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
    double mat[9];
    std::memcpy(mat, _view_ctm, sizeof(double) * 9);
    const double real_x1 = _mouse_pos_1.x() * _view_ctm[0] + _mouse_pos_1.y() * _view_ctm[3] + _view_ctm[6];
    const double real_y1 = _mouse_pos_1.x() * _view_ctm[1] + _mouse_pos_1.y() * _view_ctm[4] + _view_ctm[7];
    const double real_x0 = _mouse_pos_0.x() * _view_ctm[0] + _mouse_pos_0.y() * _view_ctm[3] + _view_ctm[6];
    const double real_y0 = _mouse_pos_0.x() * _view_ctm[1] + _mouse_pos_0.y() * _view_ctm[4] + _view_ctm[7];
    const double canvas_x0 = real_x0 * _canvas_ctm[0] + real_y0 * _canvas_ctm[3] + _canvas_ctm[6];
    const double canvas_y0 = real_x0 * _canvas_ctm[1] + real_y0 * _canvas_ctm[4] + _canvas_ctm[7];
    const double canvas_x1 = real_x1 * _canvas_ctm[0] + real_y1 * _canvas_ctm[3] + _canvas_ctm[6];
    const double canvas_y1 = real_x1 * _canvas_ctm[1] + real_y1 * _canvas_ctm[4] + _canvas_ctm[7];
    if (_info_labels[0])
    {
        _info_labels[0]->setText(std::string("X:").append(std::to_string(static_cast<int>(real_x1))).append(" Y:").append(std::to_string(static_cast<int>(real_y1))).c_str());
    }
    if (is_view_moveable()) // 视图可移动
    {
        _canvas_ctm[6] += (canvas_x1 - canvas_x0), _canvas_ctm[7] += (canvas_y1 - canvas_y0);
        _view_ctm[6] -= (real_x1 - real_x0), _view_ctm[7] -= (real_y1 - real_y0);
        _visible_area.translate(real_x0 - real_x1, real_y0 - real_y1);
        update();
    }
    if (is_paintable() && is_painting()) // painting
    {
        switch (_tool_flags[0])
        {
        case Tool::CIRCLE:
            _circle_cache.radius() = Geo::distance(real_x1, real_y1,
                                                   _circle_cache.center().coord().x, _circle_cache.center().coord().y);
            if (_info_labels[1] != nullptr)
            {
                _info_labels[1]->setText(std::string("Radius:").append(std::to_string(_circle_cache.radius())).c_str());
            }
            break;
        case Tool::POLYLINE:
            if (event->modifiers() == Qt::ControlModifier)
            {
                const Geo::Coord &coord =_editer->point_cache().at(_editer->point_cache().size() - 2).coord();
                if (std::abs(real_x1 - coord.x) > std::abs(real_y1 - coord.y))
                {
                    _editer->point_cache().back().coord().x = real_x1;
                    _editer->point_cache().back().coord().y = coord.y;
                }
                else
                {
                    _editer->point_cache().back().coord().x = coord.x;
                    _editer->point_cache().back().coord().y = real_y1;
                }
            }
            else
            {
                _editer->point_cache().back().coord().x = real_x1;
                _editer->point_cache().back().coord().y = real_y1;
            }
            if (_info_labels[1] != nullptr)
            {
                _info_labels[1]->setText(std::string("Length:").append(std::to_string(Geo::distance(_editer->point_cache().back(),
                                                                                                    _editer->point_cache()[_editer->point_cache().size() - 2])))
                                             .c_str());
            }
            break;
        case Tool::RECT:
            _AABBRect_cache = Geo::AABBRect(_last_point, Geo::Point(real_x1, real_y1));
            if (_info_labels[1] != nullptr)
            {
                _info_labels[1]->setText(std::string("Width:").append(std::to_string(std::abs(real_x1 - _last_point.coord().x))).append(" Height:").append(std::to_string(std::abs(real_y1 - _last_point.coord().y))).c_str());
            }
            break;
        case Tool::CURVE:
            if (_editer->point_cache().size() > _bezier_order && (_editer->point_cache().size() - 2) % _bezier_order == 0) 
            {
                const size_t count = _editer->point_cache().size();
                if (_editer->point_cache()[count - 2].coord().x == _editer->point_cache()[count - 3].coord().x)
                {
                    _editer->point_cache().back().coord().x = _editer->point_cache()[count - 2].coord().x;
                    _editer->point_cache().back().coord().y = real_y1;
                }
                else
                {
                    _editer->point_cache().back().coord().x = real_x1;
                    _editer->point_cache().back().coord().y = (_editer->point_cache()[count - 3].coord().y - _editer->point_cache()[count - 2].coord().y) /
                        (_editer->point_cache()[count - 3].coord().x - _editer->point_cache()[count - 2].coord().x) * 
                        (_mouse_pos_1.x() - _editer->point_cache()[count - 2].coord().x) + _editer->point_cache()[count - 2].coord().y;
                }
            }
            else
            {
                _editer->point_cache().back() = Geo::Point(real_x1, real_y1);
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
        if (is_obj_moveable())
        {
            if (!is_moving_obj())
            {
                _editer->store_backup();
                _bool_flags[6] = true; // is moving obj
            }
            for (Geo::Geometry *obj : _editer->selected())
            {
                _editer->translate_points(obj, real_x0, real_y0, real_x1, real_y1, event->modifiers() == Qt::ControlModifier);
            }
            if (event->modifiers() != Qt::ControlModifier && GlobalSetting::get_instance()->setting()["auto_aligning"].toBool())
            {
                _editer->auto_aligning(_clicked_obj, real_x1, real_y1, _reflines,
                    GlobalSetting::get_instance()->setting()["active_layer_catch_only"].toBool());
            }
            if (_info_labels[1])
            {
                _info_labels[1]->clear();
            }
        }
        else if (!_select_rect.empty())
        {
            _select_rect = Geo::AABBRect(_last_point.coord().x, _last_point.coord().y, real_x1, real_y1);
            if (_info_labels[1])
            {
                _info_labels[1]->setText(std::string("Width:").append(std::to_string(std::abs(real_x1 - _last_point.coord().x))).append(" Height:").append(std::to_string(std::abs(real_y1 - _last_point.coord().y))).c_str());
            }
        }
        update();
    }

    if (GlobalSetting::get_instance()->setting()["cursor_catch"].toBool() && _clicked_obj == nullptr)
    {
        const bool value = GlobalSetting::get_instance()->setting()["active_layer_catch_only"].toBool();
        Geo::Coord pos(real_x1, real_y1);
        if (_editer->auto_aligning(pos, _reflines, value))
        {
            const double k = mat[0] * mat[4] - mat[3] * mat[1];
            const double x = (mat[4] * (pos.x - mat[6]) - mat[3] * (pos.y - mat[7])) / k;
            const double y = (mat[0] * (pos.y - mat[7]) - mat[1] * (pos.x - mat[6])) / k;
            _mouse_pos_1.setX(x);
            _mouse_pos_1.setY(y);
            QCursor::setPos(this->mapToGlobal(_mouse_pos_1).x(), this->mapToGlobal(_mouse_pos_1).y());
        }
    }
}

void Canvas::wheelEvent(QWheelEvent *event)
{
    const double real_x = _mouse_pos_1.x() * _view_ctm[0] + _mouse_pos_1.y() * _view_ctm[3] + _view_ctm[6];
    const double real_y = _mouse_pos_1.x() * _view_ctm[1] + _mouse_pos_1.y() * _view_ctm[4] + _view_ctm[7];
    const double canvas_x = real_x * _canvas_ctm[0] + real_y * _canvas_ctm[3] + _canvas_ctm[6];
    const double canvas_y = real_x * _canvas_ctm[1] + real_y * _canvas_ctm[4] + _canvas_ctm[7];
    if (event->angleDelta().y() > 0 && _ratio < 256)
    {
        _ratio *= 1.25;
        _canvas_ctm[0] *= 1.25;
        _canvas_ctm[4] *= 1.25;
        _canvas_ctm[6] = _canvas_ctm[6] * 1.25 - canvas_x * 0.25;
        _canvas_ctm[7] = _canvas_ctm[7] * 1.25 - canvas_y * 0.25;

        _view_ctm[0] *= 0.8;
        _view_ctm[4] *= 0.8;
        _view_ctm[6] = _view_ctm[6] * 0.8 + real_x * 0.2;
        _view_ctm[7] = _view_ctm[7] * 0.8 + real_y * 0.2;

        _visible_area.scale(real_x, real_y, 0.8);
        update();
    }
    else if (event->angleDelta().y() < 0 && _ratio > (1.0 / 256.0))
    {
        _ratio *= 0.8;
        _canvas_ctm[0] *= 0.8;
        _canvas_ctm[4] *= 0.8;
        _canvas_ctm[6] = _canvas_ctm[6] * 0.8 + canvas_x * 0.2;
        _canvas_ctm[7] = _canvas_ctm[7] * 0.8 + canvas_y * 0.2;

        _view_ctm[0] *= 1.25;
        _view_ctm[4] *= 1.25;
        _view_ctm[6] = _view_ctm[6] * 1.25 - real_x * 0.25;
        _view_ctm[7] = _view_ctm[7] * 1.25 - real_y * 0.25;

        _visible_area.scale(real_x, real_y, 1.25);
        update();
    }
    _editer->set_ratio(_ratio);
    _editer->auto_aligning(_clicked_obj, _reflines);
}

void Canvas::mouseDoubleClickEvent(QMouseEvent *event)
{
    _mouse_pos_1 = event->localPos();
    switch (event->button())
    {
    case Qt::LeftButton:
        if (is_paintable() && is_painting()) // paintable and painting
        {
            _bool_flags[1] = false; // paintable
            _bool_flags[2] = false; // painting
            switch (_tool_flags[0])
            {
            case Tool::CIRCLE:
                _circle_cache.clear();
                update();
                break;
            case Tool::POLYLINE:
                if (_editer != nullptr)
                {
                    _editer->append_points();
                    update();
                }
                break;
            case Tool::RECT:
                _AABBRect_cache.clear();
                update();
                break;
            case Tool::CURVE:
                if (_editer != nullptr)
                {
                    _editer->append_bezier(_bezier_order);
                    update();
                }
                break;
            default:
                break;
            }
            _tool_flags[1] = _tool_flags[0];
            _tool_flags[0] = Tool::NONE;
            emit tool_changed(_tool_flags[0]);
        }
        else
        {
            if (is_obj_selected() && (_last_clicked_obj->type() == Geo::Type::TEXT
                || _last_clicked_obj->type() == Geo::Type::CONTAINER
                || _last_clicked_obj->type() == Geo::Type::CIRCLECONTAINER))
            {
                Geo::AABBRect rect(_last_clicked_obj->bounding_rect());
                rect.transform(_canvas_ctm[0], _canvas_ctm[3], _canvas_ctm[6], _canvas_ctm[1], _canvas_ctm[4], _canvas_ctm[7]);
                _input_line.setMaximumSize(std::max(100.0, rect.width()), std::max(100.0, rect.height()));
                _input_line.move(rect.center().coord().x - _input_line.rect().center().x(),
                                 rect.center().coord().y - _input_line.rect().center().y());
                _input_line.setFocus();
                switch (_last_clicked_obj->type())
                {
                case Geo::Type::TEXT:
                    _input_line.setText(dynamic_cast<Text *>(_last_clicked_obj)->text());
                    break;
                case Geo::Type::CONTAINER:
                    _input_line.setText(dynamic_cast<Container *>(_last_clicked_obj)->text());
                    break;
                case Geo::Type::CIRCLECONTAINER:
                    _input_line.setText(dynamic_cast<CircleContainer *>(_last_clicked_obj)->text());
                    break;
                default:
                    break;
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
        _editer->set_ratio(1.0);
        show_overview();
        update();
        break;
    default:
        break;
    }
}

void Canvas::show_overview()
{
    Graph *graph = _editer->graph();
    if (graph->empty())
    {
        update();
        return;
    }
    // 获取graph的边界
    Geo::AABBRect bounding_area = graph->bounding_rect();
    // 整个绘图控件区域作为显示区域
    QRect view_area = this->geometry();
    // 选择合适的缩放倍率
    double height_ratio = view_area.height() / bounding_area.height();
    double width_ratio = view_area.width() / bounding_area.width();
    _ratio = std::min(height_ratio, width_ratio);
    // 缩放减少2%，使其与边界留出一些空间
    _ratio = _ratio * 0.98;

    // 置于控件中间
    double x_offset = (view_area.width() - bounding_area.width() * _ratio) / 2 - bounding_area.left() * _ratio;
    double y_offset = (view_area.height() - bounding_area.height() * _ratio) / 2 - bounding_area.top() * _ratio;

    _canvas_ctm[0] = _canvas_ctm[4] = _ratio;
    _canvas_ctm[1] = _canvas_ctm[2] = _canvas_ctm[3] = _canvas_ctm[5] = 0;
    _canvas_ctm[8] = 1;
    _canvas_ctm[6] = x_offset;
    _canvas_ctm[7] = y_offset;

    _view_ctm[0] = _view_ctm[4] = 1 / _ratio;
    _view_ctm[1] = _view_ctm[2] = _view_ctm[3] = _view_ctm[5] = 0;
    _view_ctm[8] = 1;
    _view_ctm[6] = -x_offset / _ratio;
    _view_ctm[7] = -y_offset / _ratio;

    // 可视区域为显示控件区域的反变换
    double x0=0, y0=0, x1=view_area.width(), y1=view_area.height();
    _visible_area = Geo::AABBRect(
        x0 * _view_ctm[0] + y0 * _view_ctm[3] + _view_ctm[6],
        x0 * _view_ctm[1] + y0 * _view_ctm[4] + _view_ctm[7],
        x1 * _view_ctm[0] + y1 * _view_ctm[3] + _view_ctm[6],
        x1 * _view_ctm[1] + y1 * _view_ctm[4] + _view_ctm[7]);

    update();
}


void Canvas::resizeEvent(QResizeEvent *event)
{
    const QRect rect(this->geometry());
    _visible_area = Geo::AABBRect(0, 0, rect.width(), rect.height());
    _visible_area.transform(_view_ctm[0], _view_ctm[3], _view_ctm[6], _view_ctm[1], _view_ctm[4], _view_ctm[7]);
    return QWidget::resizeEvent(event);
}



void Canvas::use_tool(const Tool tool)
{
    _tool_flags[1] = _tool_flags[0];
    _tool_flags[0] = tool;
    _bool_flags[1] = (tool != Tool::NONE); // paintable
    _bool_flags[2] = false; // painting
    _editer->point_cache().clear();
    _circle_cache.clear();
    _AABBRect_cache.clear();
    emit tool_changed(_tool_flags[0]);
    update();
}

void Canvas::show_origin()
{
    _bool_flags[7] = true;
}

void Canvas::hide_origin()
{
    _bool_flags[7] = false;
}

bool Canvas::origin_visible() const
{
    return _bool_flags[7];
}

const bool Canvas::is_view_moveable() const
{
    return _bool_flags[0];
}

const bool Canvas::is_paintable() const
{
    return _bool_flags[1];
}

const bool Canvas::is_painting() const
{
    return _bool_flags[2];
}

const bool Canvas::is_typing() const
{
    return _input_line.isVisible();
}

const bool Canvas::is_catching_cursor() const
{
    return _bool_flags[3];
}

const bool Canvas::is_obj_moveable() const
{
    return _bool_flags[4];
}

const bool Canvas::is_obj_selected() const
{
    return _bool_flags[5];
}

const bool Canvas::is_moving_obj() const
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




double Canvas::ratio() const
{
    return _ratio;
}

Geo::Point Canvas::center() const
{
    if (_circle_cache.empty() && _AABBRect_cache.empty() && (_editer->graph() == nullptr || _editer->graph()->empty()))
    {
        return Geo::Point();
    }

    double x0 = DBL_MAX, y0 = DBL_MAX, x1 = (-FLT_MAX), y1 = (-FLT_MAX);
    for (const Geo::Point &point : _AABBRect_cache)
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

Geo::AABBRect Canvas::bounding_rect() const
{
    if (_circle_cache.empty() && _AABBRect_cache.empty() && (_editer->graph() == nullptr || _editer->graph()->empty()))
    {
        return Geo::AABBRect();
    }

    double x0 = DBL_MAX, y0 = DBL_MAX, x1 = (-FLT_MAX), y1 = (-FLT_MAX);
    for (const Geo::Point &point : _AABBRect_cache)
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
        return Geo::AABBRect(x0, y0, x1, y1);
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

    return Geo::AABBRect(x0, y0, x1, y1);
}

Geo::Coord Canvas::mouse_position() const
{
    return Geo::Coord(_mouse_pos_1.x(), _mouse_pos_1.y());
}

const bool Canvas::empty() const
{
    return _circle_cache.empty() && _AABBRect_cache.empty() &&
           (_editer == nullptr || _editer->graph() == nullptr || _editer->graph()->empty());
}

void Canvas::cancel_painting()
{
    _bool_flags[1] = false; // paintable
    _bool_flags[2] = false; // painting
    _tool_flags[1] = _tool_flags[0];
    _tool_flags[0] = Tool::NONE;
    emit tool_changed(_tool_flags[0]);
    _editer->point_cache().clear();
    _circle_cache.clear();
    _AABBRect_cache.clear();
    update();
}

void Canvas::use_last_tool()
{
    if (is_painting())
    {
        return;
    }
    _tool_flags[0] = _tool_flags[1];
    if (_tool_flags[0] != Tool::NONE)
    {
        _bool_flags[1] = true; // paintable
        emit tool_changed(_tool_flags[0]);
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
    if (_editer->paste((_mouse_pos_1.x() - _stored_mouse_pos.x()) / _ratio, (_mouse_pos_1.y() - _stored_mouse_pos.y()) / _ratio))
    {
        update();
    }
}



bool Canvas::is_visible(const Geo::Point &point) const
{
    return point.coord().x > _visible_area.left() && point.coord().x < _visible_area.right()
        && point.coord().y > _visible_area.bottom() && point.coord().y < _visible_area.top();
}

bool Canvas::is_visible(const Geo::Polyline &polyline) const
{
    if (!Geo::is_intersected(_visible_area, polyline.bounding_rect()))
    {
        return false;
    }
    if (is_visible(polyline.front()))
    {
        return true;
    }
    const Geo::Point center(_visible_area.center());
    const double len = std::min(_visible_area.width(), _visible_area.height());
    for (size_t i = 1, count = polyline.size(); i < count; ++i)
    {
        if (is_visible(polyline[i]) || Geo::distance(center, polyline[i - 1], polyline[i]) < len)
        {
            return true;
        }
    }
    return false;
}

bool Canvas::is_visible(const Geo::Polygon &polygon) const
{
    if (!Geo::is_intersected(_visible_area, polygon.bounding_rect()))
    {
        return false;
    }
    const Geo::Point center(_visible_area.center());
    const double len = std::min(_visible_area.width(), _visible_area.height());
    for (size_t i = 1, count = polygon.size(); i < count; ++i)
    {
        if (is_visible(polygon[i - 1]) || Geo::distance(center, polygon[i - 1], polygon[i]) < len)
        {
            return true;
        }
    }
    for (const Geo::Point &point : _visible_area)
    {
        if (Geo::is_inside(point, polygon))
        {
            return true;
        }
    }
    return false;
}

bool Canvas::is_visible(const Geo::Circle &circle) const
{
    return circle.center().coord().x > _visible_area.left() - circle.radius() &&
        circle.center().coord().x < _visible_area.right() + circle.radius() &&
        circle.center().coord().y > _visible_area.bottom() - circle.radius() &&
        circle.center().coord().y < _visible_area.top() + circle.radius();
}


Geo::Coord Canvas::real_coord_to_view_coord(const Geo::Coord &input) const
{
    const double k = _view_ctm[0] * _view_ctm[4] - _view_ctm[3] * _view_ctm[1];
    return {(_view_ctm[4] * (input.x - _view_ctm[6]) - _view_ctm[3] * (input.y - _view_ctm[7])) / k,
        (_view_ctm[0] * (input.y - _view_ctm[7]) - _view_ctm[1] * (input.x - _view_ctm[6])) / k};
}

Geo::Coord Canvas::real_coord_to_view_coord(const double x, const double y) const
{
    const double k = _view_ctm[0] * _view_ctm[4] - _view_ctm[3] * _view_ctm[1];
    return {(_view_ctm[4] * (x - _view_ctm[6]) - _view_ctm[3] * (y - _view_ctm[7])) / k,
        (_view_ctm[0] * (y - _view_ctm[7]) - _view_ctm[1] * (x - _view_ctm[6])) / k};
}

Geo::Coord Canvas::canvas_coord_to_real_coord(const Geo::Coord &input) const
{
    const double t = (input.y - _canvas_ctm[7] - _canvas_ctm[1] / _canvas_ctm[0] * (input.x - _canvas_ctm[6])) /
        (_canvas_ctm[4] - _canvas_ctm[1] / _canvas_ctm[0] * _canvas_ctm[3]);
    return {(input.x - _canvas_ctm[6] - _canvas_ctm[3] * t) / _canvas_ctm[0], t};
}

Geo::Coord Canvas::canvas_coord_to_real_coord(const double x, const double y) const
{
    const double t = (y - _canvas_ctm[7] - _canvas_ctm[1] / _canvas_ctm[0] * (x - _canvas_ctm[6])) /
        (_canvas_ctm[4] - _canvas_ctm[1] / _canvas_ctm[0] * _canvas_ctm[3]);
    return {(x - _canvas_ctm[6] - _canvas_ctm[3] * t) / _canvas_ctm[0], t};
}