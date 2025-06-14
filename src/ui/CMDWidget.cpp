#include <QRegularExpressionValidator>

#include "ui/CMDWidget.hpp"
#include "./ui_CMDWidget.h"
#include "io/GlobalSetting.hpp"
#include "base/Algorithm.hpp"


CMDWidget::CMDWidget(Editer *editer, Canvas *canvas, QWidget *parent)
    : QWidget(parent), _parent(parent), ui(new Ui::CMDWidget),
    _editer(editer), _canvas(canvas)
{
    ui->setupUi(this);

    init();
}

CMDWidget::~CMDWidget()
{
    delete ui;
    delete _completer;
}

void CMDWidget::init()
{
    ui->cmd->setValidator(new QRegularExpressionValidator(QRegularExpression("([A-Za-z]+)|(-?[0-9]+(.[0-9]+)?)$")));
    ui->cmd->installEventFilter(this);

    _cmd_list << QString() << "OPEN" << "APPEND" << "SAVE" << "EXIT" << "MAIN"
        << "LENGTH" << "CIRCLE" << "ELLIPSE" << "POLYLINE" << "RECTANGLE" << "BSPLINE" << "BEZIER" << "TEXT"
        << "CONNECT" << "CLOSE" << "COMBINATE" << "SPLIT"
        << "ROTATE" << "FLIPX" << "FLIPY" << "MIRROR" << "POINTMIRROR"
        << "ARRAY" << "LINEARRAY" << "RINGARRAY"
        << "UNION" << "INTERSECTION" << "DIFFERENCE"
        << "OFFSET" << "SCALE" << "FILLET" << "ABSOLUTE" << "RELATIVE"
        << "DELETE" << "COPY" << "CUT" << "PASTE" << "UNDO" << "ALL";

    _cmd_dict = {{"OPEN",CMD::Open_CMD}, {"APPEND",CMD::Append_CMD}, {"SAVE",CMD::Save_CMD}, {"EXIT",CMD::Exit_CMD},
        {"LENGTH",CMD::Length_CMD}, {"MAIN",CMD::Main_CMD},
        {"CIRCLE",CMD::Circle_CMD}, {"ELLIPSE",CMD::Ellipse_CMD},
        {"POLYLINE",CMD::Polyline_CMD}, {"RECTANGLE",CMD::Rectangle_CMD},
        {"BSPLINE",CMD::BSpline_CMD}, {"BEZIER",CMD::Bezier_CMD}, {"TEXT",CMD::Text_CMD}, {"CONNECT",CMD::Connect_CMD},
        {"COMBINATE",CMD::Combinate_CMD}, {"CLOSE",CMD::Close_CMD}, {"SPLIT",CMD::Split_CMD},
        {"ROTATE",CMD::Rotate_CMD}, {"FLIPX",CMD::FlipX_CMD}, {"FLIPY",CMD::FlipY_CMD},
        {"MIRROR",CMD::Mirror_CMD},
        {"ARRAY",CMD::Array_CMD}, {"LINEARRAY",CMD::LineArray_CMD}, {"RINGARRAY",CMD::RingArray_CMD},
        {"OFFSET",CMD::Offset_CMD}, {"SCALE", CMD::Scale_CMD}, {"FILLET",CMD::Fillet_CMD},
        {"UNION",CMD::Union_CMD}, {"INTERSECTION",CMD::Intersection_CMD}, {"DIFFERENCE",CMD::Difference_CMD},
        {"DELETE",CMD::Delete_CMD}, {"COPY",CMD::Copy_CMD}, {"CUT",CMD::Cut_CMD}, {"PASTE",CMD::Paste_CMD},
        {"UNDO",CMD::Undo_CMD}, {"ALL",CMD::SelectAll_CMD}};

    _setting_dict = {{"RELATIVE", SETTING::Relative_SETTING}, {"ABSOLUTE", SETTING::Absolute_SETTING}};

    _completer = new QCompleter(_cmd_list, this);
    _completer->setCaseSensitivity(Qt::CaseSensitivity::CaseInsensitive);
    _completer->setFilterMode(Qt::MatchFlag::MatchContains);
    _completer->setCompletionMode(QCompleter::CompletionMode::InlineCompletion);
    ui->cmd->setCompleter(_completer);

    QObject::connect(_canvas, &Canvas::tool_changed, this, &CMDWidget::refresh_tool);
}


void CMDWidget::refresh_tool(const Canvas::Tool tool)
{
    switch (tool)
    {
    case Canvas::Tool::NoTool:
        clear();
        break;
    default:
        break;
    }
}


void CMDWidget::mousePressEvent(QMouseEvent *event)
{
    _last_pos = event->pos();
}

void CMDWidget::mouseMoveEvent(QMouseEvent *event)
{
    move(mapToParent(event->pos()) - _last_pos);
    
    int x = pos().x(), y = pos().y(), w = width(), h = height();
    if (x < 0)
    {
        move(mapToParent(QPoint(-x, 0)));
    }
    else if (x + w > _parent->width())
    {
        move(mapToParent(QPoint(_parent->width() - x - w, 0)));
    }

    if (y < 22)
    {
        move(mapToParent(QPoint(0, 22 - y)));
    }
    else if (y + h > _parent->height() - 32)
    {
        move(mapToParent(QPoint(0, _parent->height() - 32 - y - h)));
    }
}

bool CMDWidget::eventFilter(QObject *target, QEvent *event)
{
    if (event->type() == QEvent::Type::KeyPress &&
        dynamic_cast<QKeyEvent *>(event)->key() == Qt::Key_Space)
    {
        if (!ui->cmd->text().isEmpty())
        {
            if (get_setting())
            {
                return QWidget::eventFilter(target, event);
            }

            if (!get_parameter())
            {
                ui->cmd->setText(_completer->currentCompletion());
                get_cmd();
            }
        }
        work();
    }

    return QWidget::eventFilter(target, event);
}


void CMDWidget::clear()
{
    ui->cmd->clear();
    ui->cmd_label->clear();
    ui->parameter_label->clear();
    _current_cmd = CMD::Error_CMD;
    _completer->setCurrentRow(0);
    _parameters.clear();
}

CMDWidget::CMD CMDWidget::cmd() const
{
    std::map<QString, CMDWidget::CMD>::const_iterator result = _cmd_dict.find(ui->cmd->text().toUpper());
    if (result == _cmd_dict.end())
    {
        return CMD::Error_CMD;
    }
    else
    {
        return result->second;
    }
}

bool CMDWidget::empty() const
{
    return ui->cmd->text().isEmpty() && _current_cmd == CMD::Error_CMD;
}

std::vector<double> &CMDWidget::parameters()
{
    return _parameters;
}

const std::vector<double> &CMDWidget::parameters() const
{
    return _parameters;
}

void CMDWidget::activate(const char key)
{
    ui->cmd->setText(QString(key));
    ui->cmd->setFocus();
}

void CMDWidget::show()
{
    ui->cmd->setDisabled(false);
    ui->cmd->clear();
    return QWidget::show();
}

void CMDWidget::hide()
{
    ui->cmd->setDisabled(true);
    ui->cmd->clear();
    return QWidget::hide();
}


bool CMDWidget::work()
{
    ui->cmd->clear();
    _completer->setCurrentRow(0);
    switch (_current_cmd)
    {
    case CMD::Open_CMD:
    case CMD::Append_CMD:
    case CMD::Save_CMD:
    case CMD::Exit_CMD:
    case CMD::Main_CMD:
        emit cmd_changed(_current_cmd);
        _current_cmd = CMD::Error_CMD;
        break;

    case CMD::Length_CMD:
        _canvas->use_tool(Canvas::Tool::Measure);
        break;
    case CMD::Polyline_CMD:
        ui->cmd_label->setText("Polyline");
        polyline();
        break;
    case CMD::Rectangle_CMD:
        ui->cmd_label->setText("Rectangle");
        rectangle();
        break;
    case CMD::Circle_CMD:
        ui->cmd_label->setText("Circle");
        circle();
        break;
    case CMD::Ellipse_CMD:
        ui->cmd_label->setText("Ellipse");
        ellipse();
        break;
    case CMD::BSpline_CMD:
        ui->cmd_label->setText("BSpline");
        bspline();
        break;
    case CMD::Bezier_CMD:
        ui->cmd_label->setText("Bezier");
        bezier();
        break;
    case CMD::Text_CMD:
        ui->cmd_label->setText("Text");
        text();
        break;

    case CMD::Connect_CMD:
        if (_editer->connect(_editer->selected(), GlobalSetting::setting().catch_distance))
        {
            _canvas->refresh_vbo();
            _canvas->refresh_selected_ibo();
            _canvas->update();
        }
        _current_cmd = CMD::Error_CMD;
        break;
    case CMD::Close_CMD:
        if (_editer->close_polyline(_editer->selected()))
        {
            _canvas->refresh_vbo();
            _canvas->refresh_selected_ibo();
            _canvas->update();
        }
        _current_cmd = CMD::Error_CMD;
        break;
    case CMD::Combinate_CMD:
        if (_editer->combinate(_editer->selected()))
        {
            _canvas->refresh_vbo();
            _canvas->refresh_selected_ibo();
            _canvas->update();
        }
        _current_cmd = CMD::Error_CMD;
        break;
    case CMD::Split_CMD:
        _editer->split(_editer->selected());
        _current_cmd = CMD::Error_CMD;
        break;
    case CMD::Rotate_CMD:
        ui->cmd_label->setText("Rotate");
        rotate();
        break;
    case CMD::FlipX_CMD:
        {
            std::list<Geo::Geometry *> objects = _editer->selected();
            _editer->flip(objects, true, QApplication::keyboardModifiers() != Qt::ControlModifier, GlobalSetting::setting().ui->to_all_layers->isChecked());
            _canvas->refresh_vbo(objects.empty());
            _canvas->update();
        }
        _current_cmd = CMD::Error_CMD;
        break;
    case CMD::FlipY_CMD:
        {
            std::list<Geo::Geometry *> objects = _editer->selected();
            _editer->flip(objects, false, QApplication::keyboardModifiers() != Qt::ControlModifier, GlobalSetting::setting().ui->to_all_layers->isChecked());
            _canvas->refresh_vbo(objects.empty());
            _canvas->update();
        }
        _current_cmd = CMD::Error_CMD;
        break;
    case CMD::Scale_CMD:
        ui->cmd_label->setText("Scale");
        scale();
        break;
    case CMD::Offset_CMD:
        ui->cmd_label->setText("Offset");
        offset();
        break;
    case CMD::Fillet_CMD:
        fillet();
        break;

    case CMD::Mirror_CMD:
    case CMD::Array_CMD:
        emit cmd_changed(_current_cmd);
        _current_cmd = CMD::Error_CMD;
        break;

    case CMD::LineArray_CMD:
        ui->cmd_label->setText("Line Array");
        line_array();
        break;
    case CMD::RingArray_CMD:
        ui->cmd_label->setText("Ring Array");
        ring_array();
        break;

    case CMD::Union_CMD:
        {
            Geo::Polygon *polygon0 = nullptr, *polygon1 = nullptr;
            for (Geo::Geometry *object : _editer->selected())
            {
                if (object->type() == Geo::Type::POLYGON)
                {
                    if (polygon0 == nullptr)
                    {
                        polygon0 = dynamic_cast<Geo::Polygon *>(object);
                    }
                    else
                    {
                        polygon1 = dynamic_cast<Geo::Polygon *>(object);
                        break;
                    }
                }
            }

            if (_editer->polygon_union(polygon0, polygon1))
            {
                _canvas->refresh_vbo();
                _canvas->refresh_selected_ibo();
                _canvas->update();
            }
        }
        _current_cmd = CMD::Error_CMD;
        break;
    case CMD::Intersection_CMD:
        {
            Geo::Polygon *polygon0 = nullptr, *polygon1 = nullptr;
            for (Geo::Geometry *object : _editer->selected())
            {
                if (object->type() == Geo::Type::POLYGON)
                {
                    if (polygon0 == nullptr)
                    {
                        polygon0 = dynamic_cast<Geo::Polygon *>(object);
                    }
                    else
                    {
                        polygon1 = dynamic_cast<Geo::Polygon *>(object);
                        break;
                    }
                }
            }

            if (_editer->polygon_intersection(polygon0, polygon1))
            {
                _canvas->refresh_vbo();
                _canvas->refresh_selected_ibo();
                _canvas->update();
            }
        }
        _current_cmd = CMD::Error_CMD;
        break;
    case CMD::Difference_CMD:
        _canvas->set_operation(Canvas::Operation::PolygonDifference);
        emit cmd_changed(_current_cmd);
        _current_cmd = CMD::Error_CMD;
        ui->cmd_label->setText("Difference");
        break;

    case CMD::SelectAll_CMD:
        _editer->reset_selected_mark(true);
        _canvas->refresh_selected_ibo();
        _canvas->update();
        _current_cmd = CMD::Error_CMD;
        break;
    case CMD::Delete_CMD:
        if (_editer->remove_selected())
        {
            _canvas->refresh_vbo();
            _canvas->update();
        }
        _current_cmd = CMD::Error_CMD;
        break;
    case CMD::Copy_CMD:
        _canvas->copy();
        _current_cmd = CMD::Error_CMD;
        break;
    case CMD::Cut_CMD:
        _canvas->cut();
        _canvas->update();
        _current_cmd = CMD::Error_CMD;
        break;
    case CMD::Paste_CMD:
        ui->cmd_label->setText("Paste");
        paste();
        break;
    case CMD::Undo_CMD:
        if (!_canvas->is_painting())
        {
            _editer->undo();
            _canvas->refresh_vbo();
            _canvas->refresh_selected_ibo();
            _canvas->update();
        }
        _current_cmd = CMD::Error_CMD;
        break;

    default:
        clear();
        return false;
    }
    return true;
}

bool CMDWidget::get_cmd()
{
    std::map<QString, CMDWidget::CMD>::const_iterator result = _cmd_dict.find(ui->cmd->text().toUpper());
    if (result == _cmd_dict.end())
    {
        return false;
    }
    else
    {
        _canvas->set_operation(Canvas::Operation::NoOperation);
        _canvas->use_tool(Canvas::Tool::NoTool);
        _current_cmd = result->second;
        return true;
    }
}

bool CMDWidget::get_parameter()
{
    if (ui->cmd->text().isEmpty())
    {
        return false;
    }
    else
    {
        bool is_num = false;
        const double value = ui->cmd->text().toDouble(&is_num);
        if (is_num)
        {
            _parameters.emplace_back(value);
        }
        return is_num;
    }
}

bool CMDWidget::get_setting()
{
    std::map<QString, CMDWidget::SETTING>::const_iterator result = _setting_dict.find(ui->cmd->text().toUpper());
    if (result == _setting_dict.cend())
    {
        return false;
    }

    switch (result->second)
    {
    case SETTING::Relative_SETTING:
    case SETTING::Absolute_SETTING:
        if (_parameters.size() < 2)
        {
            _relative = (result->second == SETTING::Relative_SETTING);
            if (_current_cmd == CMD::Polyline_CMD || _current_cmd == CMD::BSpline_CMD || _current_cmd == CMD::BSpline_CMD)
            {
                ui->parameter_label->setText(_relative ? "Relative X: Y:" : "Absolute X: Y:");
            }
            if (_relative)
            {
                if (_editer->point_cache().empty())
                {
                    _last_x = _last_y = 0;
                }
                else
                {
                    _last_x = _editer->point_cache()[_editer->point_cache().size() - 2].x;
                    _last_y = _editer->point_cache()[_editer->point_cache().size() - 2].y;
                }
            }
        }
        break;
    default:
        break;
    }
    ui->cmd->clear();
    return true;
}


void CMDWidget::paste()
{
    switch (_parameters.size())
    {
    case 0:
        _parameters.emplace_back(0);
        break;
    case 1:
        _canvas->paste();
        _canvas->update();
        clear();
        break;
    case 2:
        ui->parameter_label->setText("X:" + QString::number(_parameters[1]) + " Y:");
        break;
    case 3:
        _canvas->paste(_parameters[1], _parameters[2]);
        _canvas->update();
        clear();
        break;
    default:
        break;
    }
}

void CMDWidget::polyline()
{
    switch (_parameters.size())
    {
    case 0:
        _canvas->use_tool(Canvas::Tool::Polyline);
        _parameters.emplace_back(0);
        ui->parameter_label->setText(_relative ? "Relative X: Y:" : "Absolute X: Y:");
        _last_x = _last_y = 0;
        break;
    case 1:
        _parameters.clear();
        ui->parameter_label->clear();
        _editer->point_cache().back() = _editer->point_cache()[_editer->point_cache().size() - 2];
        _editer->point_cache().emplace_back(_editer->point_cache().back());
        _canvas->polyline_cmd();
        break;
    case 2:
        ui->parameter_label->setText((_relative ? "Relative X:" : "Absolute X:") + QString::number(_parameters[1]) + " Y:");
        break;
    case 3:
        ui->parameter_label->setText(_relative ? "Relative X: Y:" : "Absolute X: Y:");
        if (_relative)
        {
            _canvas->polyline_cmd(_parameters[1] + _last_x, _parameters[2] + _last_y);
            _last_x += _parameters[1];
            _last_y += _parameters[2];
        }
        else
        {
            _canvas->polyline_cmd(_parameters[1], _parameters[2]);
        }
        _parameters.pop_back();
        _parameters.pop_back();
        break;
    default:
        break;
    }
}

void CMDWidget::bspline()
{
    switch (_parameters.size())
    {
    case 0:
        _canvas->use_tool(Canvas::Tool::BSpline);
        _parameters.emplace_back(0);
        ui->parameter_label->setText(_relative ? "Relative X: Y:" : "Absolute X: Y:");
        _last_x = _last_y = 0;
        break;
    case 1:
        _parameters.clear();
        ui->parameter_label->clear();
        _editer->point_cache().back() = _editer->point_cache()[_editer->point_cache().size() - 2];
        _editer->point_cache().emplace_back(_editer->point_cache().back());
        _canvas->polyline_cmd();
        break;
    case 2:
        ui->parameter_label->setText((_relative ? "Relative X:" : "Absolute X:") + QString::number(_parameters[1]) + " Y:");
        break;
    case 3:
        ui->parameter_label->setText(_relative ? "Relative X: Y:" : "Absolute X: Y:");
        if (_relative)
        {
            _canvas->polyline_cmd(_parameters[1] + _last_x, _parameters[2] + _last_y);
            _last_x += _parameters[1];
            _last_y += _parameters[2];
        }
        else
        {
            _canvas->polyline_cmd(_parameters[1], _parameters[2]);
        }
        _parameters.pop_back();
        _parameters.pop_back();
        break;
    default:
        break;
    }
}

void CMDWidget::bezier()
{
    switch (_parameters.size())
    {
    case 0:
        _canvas->use_tool(Canvas::Tool::Bezier);
        _parameters.emplace_back(0);
        ui->parameter_label->setText(_relative ? "Relative X: Y:" : "Absolute X: Y:");
        _last_x = _last_y = 0;
        break;
    case 1:
        _parameters.clear();
        ui->parameter_label->clear();
        _editer->point_cache().back() = _editer->point_cache()[_editer->point_cache().size() - 2];
        _editer->point_cache().emplace_back(_editer->point_cache().back());
        _canvas->polyline_cmd();
        break;
    case 2:
        ui->parameter_label->setText((_relative ? "Relative X:" : "Absolute X:") + QString::number(_parameters[1]) + " Y:");
        break;
    case 3:
        ui->parameter_label->setText(_relative ? "Relative X: Y:" : "Absolute X: Y:");
        if (_relative)
        {
            _canvas->polyline_cmd(_parameters[1] + _last_x, _parameters[2] + _last_y);
            _last_x += _parameters[1];
            _last_y += _parameters[2];
        }
        else
        {
            _canvas->polyline_cmd(_parameters[1], _parameters[2]);
        }
        _parameters.pop_back();
        _parameters.pop_back();
        break;
    default:
        break;
    }
}

void CMDWidget::text()
{
    switch (_parameters.size())
    {
    case 0:
        _canvas->use_tool(Canvas::Tool::Text);
        ui->parameter_label->setText("X: Y:");
        break;
    case 1:
        ui->parameter_label->setText("X:" + QString::number(_parameters[0]) + " Y:");
        break;
    case 2:
        _canvas->text_cmd(_parameters[0], _parameters[1]);
        clear();
        break;
    default:
        break;
    }
}

void CMDWidget::rectangle()
{
    switch (_parameters.size())
    {
    case 0:
        _canvas->use_tool(Canvas::Tool::Rect);
        _parameters.emplace_back(0);
        ui->parameter_label->setText("X: Y:");
        break;
    case 1:
        _canvas->rect_cmd();
        _parameters.emplace_back(0);
        _parameters.emplace_back(0);
        _parameters.emplace_back(0);
        ui->parameter_label->setText("W: H:");
        break;
    case 2:
        ui->parameter_label->setText("X:" + QString::number(_parameters[1]) + " Y:");
        break;
    case 3:
        ui->parameter_label->setText("X:" + QString::number(_parameters[1])
            + " Y:" + QString::number(_parameters[2]) + " W: H:");
        _parameters.emplace_back(0);
        _canvas->rect_cmd(_parameters[1], _parameters[2]);
        _canvas->update();
        break;
    case 4:
        _canvas->rect_cmd();
        clear();
        break;
    case 5:
        ui->parameter_label->setText("W:" + QString::number(_parameters[4]) + " H:");
        break;
    case 6:
        _canvas->rect_cmd(_parameters[4], _parameters[5]);
        _canvas->update();
        clear();
        break;
    default:
        break;
    }
}

void CMDWidget::circle()
{
    switch (_parameters.size())
    {
    case 0:
        _canvas->use_tool(Canvas::Tool::Circle);
        _parameters.emplace_back(0);
        ui->parameter_label->setText("X: Y:");
        break;
    case 1:
        {
            const Geo::Point coord(_canvas->mouse_position());
            _parameters.emplace_back(coord.x);
            _parameters.emplace_back(coord.y);
            _canvas->circle_cmd(_parameters[1], _parameters[2]);
            ui->parameter_label->setText("X:" + QString::number(_parameters[1])
                + " Y:" + QString::number(_parameters[2]) + " R:");
        }
        break;
    case 2:
        ui->parameter_label->setText("X:" + QString::number(_parameters[1]) + " Y:");
        break;
    case 3:
        ui->parameter_label->setText("X:" + QString::number(_parameters[1])
            + " Y:" + QString::number(_parameters[2]) + " R:");
        _canvas->circle_cmd(_parameters[1], _parameters[2]);
        break;
    case 4:
        _canvas->circle_cmd(_parameters[1], _parameters[2], _parameters[3]);
        break;
    default:
        break;
    }
}

void CMDWidget::ellipse()
{
    switch (_parameters.size())
    {
    case 0:
        _canvas->use_tool(Canvas::Tool::Ellipse);
        _parameters.emplace_back(0);
        ui->parameter_label->setText("X: Y:");
        break;
    case 1:
        {
            _parameters.emplace_back(_canvas->mouse_position().x);
            _parameters.emplace_back(_canvas->mouse_position().y);
            _parameters.emplace_back(0);
            ui->parameter_label->setText("X:" + QString::number(_parameters[1])
            + " Y:" + QString::number(_parameters[2]) + " angle:");
            _canvas->ellipse_cmd(_parameters[1], _parameters[2]);
        }
        break;
    case 2:
        ui->parameter_label->setText("X:" + QString::number(_parameters[1]) + " Y:");
        break;
    case 3:
        _parameters.emplace_back(0);
        ui->parameter_label->setText("X:" + QString::number(_parameters[1])
            + " Y:" + QString::number(_parameters[2]) + " angle:");
        _canvas->ellipse_cmd(_parameters[1], _parameters[2]);
        break;
    case 4:
        {
            const Geo::Point coord(_canvas->mouse_position());
            const Geo::Point center(_parameters[1], _parameters[2]);
            _parameters.emplace_back(Geo::angle(center, coord));
            _parameters.emplace_back(0);
            _parameters.emplace_back(Geo::distance(coord, center));
            _parameters.emplace_back(0);
            _canvas->ellipse_cmd(_parameters[1], _parameters[2], _parameters[4], _parameters[6]);
            ui->parameter_label->setText("X:" + QString::number(_parameters[1])
                + " Y:" + QString::number(_parameters[2]) + " angle:" + QString::number(_parameters[4])
                + " a:" + QString::number(_parameters[6]) + " b:");
        }
        break;
    case 5:
        {
            ui->parameter_label->setText("X:" + QString::number(_parameters[1])
                + " Y:" + QString::number(_parameters[2]) + " angle:" + QString::number(_parameters[4]) + " a:");
            _parameters.emplace_back(0);
        }
        break;
    case 6:
        {
            const Geo::Point coord(_canvas->mouse_position());
            const Geo::Point center(_parameters[1], _parameters[2]);
            _parameters.emplace_back(Geo::distance(coord, center));
            _parameters.emplace_back(0);
            ui->parameter_label->setText("X:" + QString::number(_parameters[1])
                + " Y:" + QString::number(_parameters[2]) + " angle:" + QString::number(_parameters[4])
                + " a:" + QString::number(_parameters[6]) + " b:");
            _canvas->ellipse_cmd(_parameters[1], _parameters[2], _parameters[4], _parameters[6]);
        }
        break;
    case 7:
        {
            _parameters.emplace_back(0);
            ui->parameter_label->setText("X:" + QString::number(_parameters[1])
                + " Y:" + QString::number(_parameters[2]) + " angle:" + QString::number(_parameters[4])
                + " a:" + QString::number(_parameters[6]) + " b:");
            _canvas->ellipse_cmd(_parameters[1], _parameters[2], _parameters[4], _parameters[6]);
        }
        break;
    case 8:
        _canvas->ellipse_cmd(_parameters[1], _parameters[2], _parameters[4], _parameters[6], 
            Geo::distance(_canvas->mouse_position(), Geo::Point(_parameters[1], _parameters[2])));
        break;
    case 9:
        _canvas->ellipse_cmd(_parameters[1], _parameters[2], _parameters[4], _parameters[6], _parameters[8]);
        break;
    default:
        break;
    }
}

void CMDWidget::rotate()
{
    switch (_parameters.size())
    {
    case 0:
        _parameters.emplace_back(0);
        ui->parameter_label->setText("Degree:");
        break;
    case 1:
        clear();
        break;
    case 2:
        {
            std::list<Geo::Geometry *> objects = _editer->selected();
            GlobalSetting::setting().ui->rotate_angle->setValue(_parameters[1]);
            _editer->rotate(objects, _parameters[1], QApplication::keyboardModifiers() != Qt::ControlModifier, GlobalSetting::setting().ui->to_all_layers->isChecked());
            _canvas->refresh_vbo(objects.empty());
            _parameters.pop_back();
            _canvas->update();
        }
        break;
    default:
        break;
    }
}

void CMDWidget::scale()
{
    switch (_parameters.size())
    {
    case 0:
        _parameters.emplace_back(0);
        ui->parameter_label->setText("Rate:");
        break;
    case 1:
        clear();
        break;
    case 2:
        GlobalSetting::setting().ui->scale_sbx->setValue(_parameters.back());
        _editer->scale(_editer->selected(), QApplication::keyboardModifiers() != Qt::ControlModifier, _parameters.back());
        _canvas->refresh_vbo();
        _canvas->refresh_selected_ibo();
        _parameters.pop_back();
        _canvas->update();
        break;
    default:
        break;
    }
}

void CMDWidget::offset()
{
    switch (_parameters.size())
    {
    case 0:
        _parameters.emplace_back(0);
        ui->parameter_label->setText("Distance:");
        break;
    case 1:
        clear();
        break;
    case 2:
        GlobalSetting::setting().ui->offset_sbx->setValue(_parameters.back());
        _editer->offset(_editer->selected(), _parameters.back());
        _canvas->refresh_vbo();
        _canvas->refresh_selected_ibo();
        _canvas->update();
        clear();
        break;
    default:
        break;
    }
}

void CMDWidget::fillet()
{
    switch (_parameters.size())
    {
    case 0:
        GlobalSetting::setting().ui->canvas->set_operation(Canvas::Operation::Fillet);
        ui->cmd_label->setText("Fillet Radius: " + QString::number(GlobalSetting::setting().ui->fillet_sbx->value()));
        _parameters.push_back(0);
        break;
    case 1:
        clear();
        GlobalSetting::setting().ui->canvas->set_operation(Canvas::Operation::NoOperation);
        break;
    case 2:
        ui->cmd_label->setText("Fillet Radius: " + QString::number(_parameters.back()));
        GlobalSetting::setting().ui->fillet_sbx->setValue(_parameters.back());
        _parameters.pop_back();
        break;
    default:
        break;
    }
}

void CMDWidget::line_array()
{
    switch (_parameters.size())
    {
    case 0:
        ui->parameter_label->setText("X Items:");
        break;
    case 1:
        ui->parameter_label->setText("X Items:" + QString::number(_parameters[0]) + " Y Items:");
        GlobalSetting::setting().ui->array_x_item->setValue(_parameters[0]);
        break;
    case 2:
        ui->parameter_label->setText("X Items:" + QString::number(_parameters[0]) + " Y Items:"
            + QString::number(_parameters[1]) + " X Space:");
        GlobalSetting::setting().ui->array_x_space->setValue(_parameters[1]);
        break;
    case 3:
        ui->parameter_label->setText("X Items:" + QString::number(_parameters[0]) + " Y Items:"
            + QString::number(_parameters[1]) + " X Space:" +  QString::number(_parameters[2]) + " Y Space:");
        GlobalSetting::setting().ui->array_y_item->setValue(_parameters[2]);
        break;
    case 4:
        GlobalSetting::setting().ui->array_y_space->setValue(_parameters[3]);
        if (_editer->line_array(_editer->selected(), _parameters[0], _parameters[1], _parameters[2], _parameters[3]))
        {
            _canvas->refresh_vbo();
            _canvas->refresh_selected_ibo();
            _canvas->update();
        }
        _parameters.clear();
        ui->parameter_label->clear();
        ui->cmd_label->clear();
        _current_cmd = CMD::Error_CMD;
        break;
    default:
        break;
    }
}

void CMDWidget::ring_array()
{
    switch (_parameters.size())
    {
    case 0:
        emit cmd_changed(CMD::RingArray_CMD);
        ui->parameter_label->setText("Center X:");
        break;
    case 1:
        ui->parameter_label->setText("Center X:"+ QString::number(_parameters[0]) + " Y:");
        break;
    case 2:
        ui->parameter_label->setText("Center X:" + QString::number(_parameters[0]) + " Y:"
            + QString::number(_parameters[1]) + " Items:");
        break;
    case 3:
        GlobalSetting::setting().ui->array_item->setValue(_parameters[2]);
        if (_editer->ring_array(_editer->selected(), _parameters[0], _parameters[1], _parameters[2]))
        {
            _canvas->refresh_vbo();
            _canvas->refresh_selected_ibo();
            _canvas->update();
        }
        _parameters.clear();
        ui->parameter_label->clear();
        ui->cmd_label->clear();
        emit _canvas->tool_changed(Canvas::Tool::NoTool);
        _current_cmd = CMD::Error_CMD;
        break;
    default:
        break;
    }
}

