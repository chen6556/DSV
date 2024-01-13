#include "ui/CMDWidget.hpp"
#include "./ui_CMDWidget.h"
#include "io/GlobalSetting.hpp"
#include <QRegularExpressionValidator>



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

    _cmd_list << QString() << "OPEN" << "SAVE" << "EXIT"
        << "LENGTH" << "CIRCLE" << "POLYLINE" << "RECTANGLE" << "BEZIER" << "TEXT"
        << "CONNECT" << "CLOSE" << "COMBINATE" << "SPLIT"
        << "ROTATE" << "FLIPX" << "FLIPY" << "MIRROR" << "ARRAY" << "LINEARRAY" << "RINGARRAY"
        << "DELETE" << "COPY" << "CUT" << "PASTE" << "UNDO" << "ALL";

    _cmd_dict = {{"OPEN",CMD::OPEN_CMD}, {"SAVE",CMD::SAVE_CMD}, {"EXIT",CMD::EXIT_CMD},
        {"LENGTH",CMD::LENGTH_CMD},
        {"CIRCLE",CMD::CIRCLE_CMD}, {"POLYLINE",CMD::POLYLINE_CMD}, {"RECTANGLE",CMD::RECTANGLE_CMD}, 
        {"BEZIER",CMD::BEZIER_CMD}, {"TEXT",CMD::TEXT_CMD}, {"CONNECT",CMD::CONNECT_CMD},
        {"COMBINATE",CMD::COMBINATE_CMD}, {"CLOSE",CMD::CLOSE_CMD}, {"SPLIT",CMD::SPLIT_CMD},
        {"ROTATE",CMD::ROTATE_CMD}, {"FLIPX",CMD::FLIPX_CMD}, {"FLIPY",CMD::FLIPY_CMD}, {"MIRROR",CMD::MIRROR_CMD},
        {"ARRAY",CMD::ARRAY_CMD}, {"LINEARRAY",CMD::LINEARRAY_CMD}, {"RINGARRAY",CMD::RINGARRAY_CMD},
        {"DELETE",CMD::DELETE_CMD}, {"COPY",CMD::COPY_CMD}, {"CUT",CMD::CUT_CMD}, {"PASTE",CMD::PASTE_CMD},
        {"UNDO",CMD::UNDO_CMD}, {"ALL",CMD::SELECTALL_CMD}};

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
    case Canvas::Tool::NOTOOL:
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
            bool is_num = false;
            double value = ui->cmd->text().toDouble(&is_num);
            if (is_num)
            {
                _parameters.emplace_back(value);
            }
            else
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


bool CMDWidget::work()
{
    ui->cmd->clear();
    _completer->setCurrentRow(0);
    switch (_current_cmd)
    {
    case CMD::OPEN_CMD:
    case CMD::SAVE_CMD:
    case CMD::EXIT_CMD:
        emit cmd_changed(_current_cmd);
        _current_cmd = CMD::ERROR_CMD;
        break;

    case CMD::LENGTH_CMD:
        _canvas->use_tool(Canvas::Tool::MEASURE);
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
        if (_editer->connect(GlobalSetting::get_instance()->setting()["catch_distance"].toDouble()))
        {
            _canvas->refresh_vbo();
            _canvas->refresh_selected_ibo();
            _canvas->update();
        }
        break;
    case CMD::CLOSE_CMD:
        if (_editer->close_polyline())
        {
            _canvas->refresh_vbo();
            _canvas->refresh_selected_ibo();
            _canvas->update();
        }
        break;
    case CMD::COMBINATE_CMD:
        if (_editer->combinate())
        {
            _canvas->refresh_vbo();
            _canvas->refresh_selected_ibo();
            _canvas->update();
        }
        break;
    case CMD::SPLIT_CMD:
        _editer->split();
        break;
    case CMD::ROTATE_CMD:
        ui->cmd_label->setText("Rotate");
        rotate();
        break;
    case CMD::FLIPX_CMD:
        {
            const bool unitary = _editer->selected_count() == 0;
            _editer->flip(true, unitary, GlobalSetting::get_instance()->ui()->to_all_layers->isChecked());
            _canvas->refresh_vbo(unitary);
            _canvas->update();
        }
        break;
    case CMD::FLIPY_CMD:
        {
            const bool unitary = _editer->selected_count() == 0;
            _editer->flip(false, unitary, GlobalSetting::get_instance()->ui()->to_all_layers->isChecked());
            _canvas->refresh_vbo(unitary);
            _canvas->update();
        }
        break;
    case CMD::MIRROR_CMD:
    case CMD::ARRAY_CMD:
        emit cmd_changed(_current_cmd);
        break;
    case CMD::LINEARRAY_CMD:
        break;
    case CMD::RINGARRAY_CMD:
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
            _editer->load_backup();
            _canvas->refresh_vbo();
            _canvas->refresh_selected_ibo();
            _canvas->update();
        }
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
        _canvas->set_operation(Canvas::Operation::NOOPERATION);
        _canvas->use_tool(Canvas::Tool::NOTOOL);
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
        const double value = ui->cmd->text().toDouble();
        if (is_num)
        {
            _parameters.emplace_back(value);
        }
        return is_num;
    }
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
        _canvas->use_tool(Canvas::Tool::POLYLINE);
        _parameters.emplace_back(0);
        break;
    case 1:
        _parameters.clear();
        _editer->point_cache().back() = _editer->point_cache()[_editer->point_cache().size() - 2];
        _editer->point_cache().emplace_back(_editer->point_cache().back());
        _canvas->polyline_cmd();
        break;
    case 2:
        ui->parameter_label->setText("X:" + QString::number(_parameters[1]) + " Y:");
        break;
    case 3:
        ui->parameter_label->clear();
        _canvas->polyline_cmd(_parameters[1], _parameters[2]);
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
        _canvas->use_tool(Canvas::Tool::CURVE);
        _parameters.emplace_back(0);
        break;
    case 1:
        _parameters.clear();
        _editer->point_cache().back() = _editer->point_cache()[_editer->point_cache().size() - 2];
        _editer->point_cache().emplace_back(_editer->point_cache().back());
        _canvas->polyline_cmd();
        break;
    case 2:
        ui->parameter_label->setText("X:" + QString::number(_parameters[1]) + " Y:");
        break;
    case 3:
        ui->parameter_label->clear();
        _canvas->polyline_cmd(_parameters[1], _parameters[2]);
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
        _canvas->use_tool(Canvas::Tool::TEXT);
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
        _canvas->use_tool(Canvas::Tool::RECT);
        _parameters.emplace_back(0);
        break;
    case 1:
        _canvas->rect_cmd();
        _parameters.emplace_back(0);
        _parameters.emplace_back(0);
        _parameters.emplace_back(0);
        break;
    case 2:
        ui->parameter_label->setText("X:" + QString::number(_parameters[1]) + " Y:");
        break;
    case 3:
        ui->parameter_label->setText("X:" + QString::number(_parameters[1]) + " Y:" + QString::number(_parameters[2]));
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
        _canvas->use_tool(Canvas::Tool::CIRCLE);
        _parameters.emplace_back(0);
        break;
    case 1:
        {
            const Geo::Coord coord(_canvas->mouse_position());
            _parameters.emplace_back(coord.x);
            _parameters.emplace_back(coord.y);
            _canvas->circle_cmd(_parameters[1], _parameters[2]);
        }
        break;
    case 2:
        ui->parameter_label->setText("X:" + QString::number(_parameters[1]) + " Y:");
        break;
    case 3:
        ui->parameter_label->setText("X:" + QString::number(_parameters[1])
            + " Y:" + QString::number(_parameters[2]));
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
        break;
    case 1:
        clear();
        break;
    case 2:
        {
            const bool unitary = _editer->selected_count() == 0;
            _editer->rotate(_parameters[1], unitary, GlobalSetting::get_instance()->ui()->to_all_layers->isChecked());
            _canvas->refresh_vbo(unitary);
            _parameters.pop_back();
            _canvas->update();
        }
        break;
    default:
        break;
    }
}

