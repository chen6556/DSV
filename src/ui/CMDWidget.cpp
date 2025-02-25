#include <QRegularExpressionValidator>

#include "ui/CMDWidget.hpp"
#include "./ui_CMDWidget.h"
#include "io/GlobalSetting.hpp"


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

    _cmd_list << QString() << "OPEN" << "SAVE" << "EXIT" << "MAIN"
        << "LENGTH" << "CIRCLE" << "POLYLINE" << "RECTANGLE" << "BEZIER" << "TEXT"
        << "CONNECT" << "CLOSE" << "COMBINATE" << "SPLIT"
        << "ROTATE" << "FLIPX" << "FLIPY" << "MIRROR" << "ARRAY" << "LINEARRAY" << "RINGARRAY"
        << "UNION" << "INTERSECTION" << "DIFFERENCE"
        << "OFFSET" << "SCALE" << "FILLET" << "ABSOLUTE" << "RELATIVE"
        << "DELETE" << "COPY" << "CUT" << "PASTE" << "UNDO" << "ALL";

    _cmd_dict = {{"OPEN",CMD::OPEN_CMD}, {"SAVE",CMD::SAVE_CMD}, {"EXIT",CMD::EXIT_CMD},
        {"LENGTH",CMD::LENGTH_CMD}, {"MAIN",CMD::MAIN_CMD},
        {"CIRCLE",CMD::CIRCLE_CMD}, {"POLYLINE",CMD::POLYLINE_CMD}, {"RECTANGLE",CMD::RECTANGLE_CMD}, 
        {"BEZIER",CMD::BEZIER_CMD}, {"TEXT",CMD::TEXT_CMD}, {"CONNECT",CMD::CONNECT_CMD},
        {"COMBINATE",CMD::COMBINATE_CMD}, {"CLOSE",CMD::CLOSE_CMD}, {"SPLIT",CMD::SPLIT_CMD},
        {"ROTATE",CMD::ROTATE_CMD}, {"FLIPX",CMD::FLIPX_CMD}, {"FLIPY",CMD::FLIPY_CMD}, {"MIRROR",CMD::MIRROR_CMD},
        {"ARRAY",CMD::ARRAY_CMD}, {"LINEARRAY",CMD::LINEARRAY_CMD}, {"RINGARRAY",CMD::RINGARRAY_CMD},
        {"OFFSET",CMD::OFFSET_CMD}, {"SCALE", CMD::SCALE_CMD}, {"FILLET",CMD::FILLET_CMD},
        {"UNION",CMD::UNION_CMD}, {"INTERSECTION",CMD::INTERSECTION_CMD}, {"DIFFERENCE",CMD::DIFFERENCE_CMD},
        {"DELETE",CMD::DELETE_CMD}, {"COPY",CMD::COPY_CMD}, {"CUT",CMD::CUT_CMD}, {"PASTE",CMD::PASTE_CMD},
        {"UNDO",CMD::UNDO_CMD}, {"ALL",CMD::SELECTALL_CMD}};

    _setting_dict = {{"RELATIVE", SETTING::RELATIVE_SETTING}, {"ABSOLUTE", SETTING::ABSOLUTE_SETTING}};

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
    _current_cmd = CMD::ERROR_CMD;
    _completer->setCurrentRow(0);
    _parameters.clear();
}

CMDWidget::CMD CMDWidget::cmd() const
{
    std::map<QString, CMDWidget::CMD>::const_iterator result = _cmd_dict.find(ui->cmd->text().toUpper());
    if (result == _cmd_dict.end())
    {
        return CMD::ERROR_CMD;
    }
    else
    {
        return result->second;
    }
}

bool CMDWidget::empty() const
{
    return ui->cmd->text().isEmpty() && _current_cmd == CMD::ERROR_CMD;
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
    case CMD::OPEN_CMD:
    case CMD::SAVE_CMD:
    case CMD::EXIT_CMD:
    case CMD::MAIN_CMD:
        emit cmd_changed(_current_cmd);
        _current_cmd = CMD::ERROR_CMD;
        break;

    case CMD::LENGTH_CMD:
        _canvas->use_tool(Canvas::Tool::Measure);
        break;
    case CMD::POLYLINE_CMD:
        ui->cmd_label->setText("Polyline");
        polyline();
        break;
    case CMD::RECTANGLE_CMD:
        ui->cmd_label->setText("Rectangle");
        rectangle();
        break;
    case CMD::CIRCLE_CMD:
        ui->cmd_label->setText("Circle");
        circle();
        break;
    case CMD::BEZIER_CMD:
        ui->cmd_label->setText("Curve");
        curve();
        break;
    case CMD::TEXT_CMD:
        ui->cmd_label->setText("Text");
        text();
        break;

    case CMD::CONNECT_CMD:
        if (_editer->connect(_editer->selected(), GlobalSetting::get_instance()->setting["catch_distance"].toDouble()))
        {
            _canvas->refresh_vbo();
            _canvas->refresh_selected_ibo();
            _canvas->update();
        }
        _current_cmd = CMD::ERROR_CMD;
        break;
    case CMD::CLOSE_CMD:
        if (_editer->close_polyline(_editer->selected()))
        {
            _canvas->refresh_vbo();
            _canvas->refresh_selected_ibo();
            _canvas->update();
        }
        _current_cmd = CMD::ERROR_CMD;
        break;
    case CMD::COMBINATE_CMD:
        if (_editer->combinate(_editer->selected()))
        {
            _canvas->refresh_vbo();
            _canvas->refresh_selected_ibo();
            _canvas->update();
        }
        _current_cmd = CMD::ERROR_CMD;
        break;
    case CMD::SPLIT_CMD:
        _editer->split(_editer->selected());
        _current_cmd = CMD::ERROR_CMD;
        break;
    case CMD::ROTATE_CMD:
        ui->cmd_label->setText("Rotate");
        rotate();
        break;
    case CMD::FLIPX_CMD:
        {
            std::list<Geo::Geometry *> objects = _editer->selected();
            _editer->flip(objects, true, QApplication::keyboardModifiers() != Qt::ControlModifier, GlobalSetting::get_instance()->ui->to_all_layers->isChecked());
            _canvas->refresh_vbo(objects.empty());
            _canvas->update();
        }
        _current_cmd = CMD::ERROR_CMD;
        break;
    case CMD::FLIPY_CMD:
        {
            std::list<Geo::Geometry *> objects = _editer->selected();
            _editer->flip(objects, false, QApplication::keyboardModifiers() != Qt::ControlModifier, GlobalSetting::get_instance()->ui->to_all_layers->isChecked());
            _canvas->refresh_vbo(objects.empty());
            _canvas->update();
        }
        _current_cmd = CMD::ERROR_CMD;
        break;
    case CMD::SCALE_CMD:
        ui->cmd_label->setText("Scale");
        scale();
        break;
    case CMD::OFFSET_CMD:
        ui->cmd_label->setText("Offset");
        offset();
        break;
    case CMD::FILLET_CMD:
        fillet();
        break;

    case CMD::MIRROR_CMD:
    case CMD::ARRAY_CMD:
        emit cmd_changed(_current_cmd);
        _current_cmd = CMD::ERROR_CMD;
        break;

    case CMD::LINEARRAY_CMD:
        ui->cmd_label->setText("Line Array");
        line_array();
        break;
    case CMD::RINGARRAY_CMD:
        ui->cmd_label->setText("Ring Array");
        ring_array();
        break;

    case CMD::UNION_CMD:
        {
            Container *container0 = nullptr, *container1 = nullptr;
            for (Geo::Geometry *object : _editer->selected())
            {
                if (object->type() == Geo::Type::CONTAINER)
                {
                    if (container0 == nullptr)
                    {
                        container0 = dynamic_cast<Container *>(object);
                    }
                    else
                    {
                        container1 = dynamic_cast<Container *>(object);
                        break;
                    }
                }
            }

            if (_editer->polygon_union(container0, container1))
            {
                _canvas->refresh_vbo();
                _canvas->refresh_selected_ibo();
                _canvas->update();
            }
        }
        _current_cmd = CMD::ERROR_CMD;
        break;
    case CMD::INTERSECTION_CMD:
        {
            Container *container0 = nullptr, *container1 = nullptr;
            for (Geo::Geometry *object : _editer->selected())
            {
                if (object->type() == Geo::Type::CONTAINER)
                {
                    if (container0 == nullptr)
                    {
                        container0 = dynamic_cast<Container *>(object);
                    }
                    else
                    {
                        container1 = dynamic_cast<Container *>(object);
                        break;
                    }
                }
            }

            if (_editer->polygon_intersection(container0, container1))
            {
                _canvas->refresh_vbo();
                _canvas->refresh_selected_ibo();
                _canvas->update();
            }
        }
        _current_cmd = CMD::ERROR_CMD;
        break;
    case CMD::DIFFERENCE_CMD:
        _canvas->set_operation(Canvas::Operation::PolygonDifference);
        emit cmd_changed(_current_cmd);
        _current_cmd = CMD::ERROR_CMD;
        ui->cmd_label->setText("Difference");
        break;

    case CMD::SELECTALL_CMD:
        _editer->reset_selected_mark(true);
        _canvas->refresh_selected_ibo();
        _canvas->update();
        _current_cmd = CMD::ERROR_CMD;
        break;
    case CMD::DELETE_CMD:
        if (_editer->remove_selected())
        {
            _canvas->refresh_vbo();
            _canvas->update();
        }
        _current_cmd = CMD::ERROR_CMD;
        break;
    case CMD::COPY_CMD:
        _canvas->copy();
        _current_cmd = CMD::ERROR_CMD;
        break;
    case CMD::CUT_CMD:
        _canvas->cut();
        _canvas->update();
        _current_cmd = CMD::ERROR_CMD;
        break;
    case CMD::PASTE_CMD:
        ui->cmd_label->setText("Paste");
        paste();
        break;
    case CMD::UNDO_CMD:
        if (!_canvas->is_painting())
        {
            _editer->undo();
            _canvas->refresh_vbo();
            _canvas->refresh_selected_ibo();
            _canvas->update();
        }
        _current_cmd = CMD::ERROR_CMD;
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
    case SETTING::RELATIVE_SETTING:
    case SETTING::ABSOLUTE_SETTING:
        if (_parameters.size() < 2)
        {
            _relative = (result->second == SETTING::RELATIVE_SETTING);
            if (_current_cmd == CMD::POLYLINE_CMD || _current_cmd == CMD::BEZIER_CMD)
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

void CMDWidget::curve()
{
    switch (_parameters.size())
    {
    case 0:
        _canvas->use_tool(Canvas::Tool::Curve);
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
            GlobalSetting::get_instance()->ui->rotate_angle->setValue(_parameters[1]);
            _editer->rotate(objects, _parameters[1], QApplication::keyboardModifiers() != Qt::ControlModifier, GlobalSetting::get_instance()->ui->to_all_layers->isChecked());
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
        GlobalSetting::get_instance()->ui->scale_sbx->setValue(_parameters.back());
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
        GlobalSetting::get_instance()->ui->offset_sbx->setValue(_parameters.back());
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
        GlobalSetting::get_instance()->ui->canvas->set_operation(Canvas::Operation::Fillet);
        ui->cmd_label->setText("Fillet Radius: " + QString::number(
            GlobalSetting::get_instance()->ui->fillet_sbx->value()));
        _parameters.push_back(0);
        break;
    case 1:
        clear();
        GlobalSetting::get_instance()->ui->canvas->set_operation(Canvas::Operation::NoOperation);
        break;
    case 2:
        ui->cmd_label->setText("Fillet Radius: " + QString::number(_parameters.back()));
        GlobalSetting::get_instance()->ui->fillet_sbx->setValue(_parameters.back());
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
        GlobalSetting::get_instance()->ui->array_x_item->setValue(_parameters[0]);
        break;
    case 2:
        ui->parameter_label->setText("X Items:" + QString::number(_parameters[0]) + " Y Items:"
            + QString::number(_parameters[1]) + " X Space:");
        GlobalSetting::get_instance()->ui->array_x_space->setValue(_parameters[1]);
        break;
    case 3:
        ui->parameter_label->setText("X Items:" + QString::number(_parameters[0]) + " Y Items:"
            + QString::number(_parameters[1]) + " X Space:" +  QString::number(_parameters[2]) + " Y Space:");
        GlobalSetting::get_instance()->ui->array_y_item->setValue(_parameters[2]);
        break;
    case 4:
        GlobalSetting::get_instance()->ui->array_y_space->setValue(_parameters[3]);
        if (_editer->line_array(_editer->selected(), _parameters[0], _parameters[1], _parameters[2], _parameters[3]))
        {
            _canvas->refresh_vbo();
            _canvas->refresh_selected_ibo();
            _canvas->update();
        }
        _parameters.clear();
        ui->parameter_label->clear();
        ui->cmd_label->clear();
        _current_cmd = CMD::ERROR_CMD;
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
        emit cmd_changed(CMD::RINGARRAY_CMD);
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
        GlobalSetting::get_instance()->ui->array_item->setValue(_parameters[2]);
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
        _current_cmd = CMD::ERROR_CMD;
        break;
    default:
        break;
    }
}

