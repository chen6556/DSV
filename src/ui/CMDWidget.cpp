#include "ui/CMDWidget.hpp"
#include "./ui_CMDWidget.h"

#include <QRegularExpressionValidator>



CMDWidget::CMDWidget(QWidget *parent)
    : QWidget(parent), _parent(parent), ui(new Ui::CMDWidget)
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
    ui->cmd->setValidator(new QRegularExpressionValidator(QRegularExpression("([A-Za-z]+)|([0-9]+(.[0-9]+)?)$")));
    ui->cmd->installEventFilter(this);

    _cmd_list << "CIRCLE" << "POLYLINE" << "RECTANGLE" << "BEZIER" << "TEXT"
        << "CONNECT" << "COMBINATE" << "SPLIT"
        << "ROTATE" << "FLIPX" << "FLIPY"
        << "DELETE" << "COPY" << "CUT" << "PASTE" << "UNDO" << "ALL";

    _cmd_dict = {{"CIRCLE",CMD::CIRCLE_CMD}, {"POLYLINE",CMD::POLYLINE_CMD}, {"RECTANGLE",CMD::RECTANGLE_CMD}, 
        {"BEZIER",CMD::BEZIER_CMD}, {"TEXT",CMD::TEXT_CMD}, {"CONNECT",CMD::CONNECT_CMD},
        {"CONBINATE",CMD::COMBINATE_CMD}, {"SPLIT",CMD::SPLIT_CMD}, {"ROTATE",CMD::ROTATE_CMD},
        {"FLIPX",CMD::FLIPX_CMD}, {"FLIPY",CMD::FLIPY_CMD}, {"DELETE",CMD::DELETE_CMD},
        {"COPY",CMD::COPY_CMD}, {"CUT",CMD::CUT_CMD}, {"PASTE",CMD::PASTE_CMD}, {"UNDO",CMD::UNDO_CMD},
        {"ALL",CMD::SELECTALL_CMD}};

    _completer = new QCompleter(_cmd_list, this);
    _completer->setCaseSensitivity(Qt::CaseSensitivity::CaseInsensitive);
    _completer->setFilterMode(Qt::MatchFlag::MatchContains);
    _completer->setCompletionMode(QCompleter::CompletionMode::InlineCompletion);
    ui->cmd->setCompleter(_completer);
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
        if (ui->cmd->text().isEmpty())
        {
            _current_cmd = _last_cmd;
        }
        else
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
            }
        }
    }

    return QWidget::eventFilter(target, event);
}


void CMDWidget::clear()
{
    ui->cmd->clear();
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
    return ui->cmd->text().isEmpty();
}

std::vector<double> &CMDWidget::parameters()
{
    return _parameters;
}

const std::vector<double> &CMDWidget::parameters() const
{
    return _parameters;
}


bool CMDWidget::work()
{
    if (_current_cmd == CMD::ERROR_CMD)
    {
        get_cmd();
    }
    else
    {
        switch (_current_cmd)
        {
        case CMD::POLYLINE_CMD:
            break;
        case CMD::RECTANGLE_CMD:
            break;
        case CMD::CIRCLE_CMD:
            break;
        case CMD::BEZIER_CMD:
            break;
        case CMD::TEXT_CMD:
            break;

        case CMD::CONNECT_CMD:
            break;
        case CMD::COMBINATE_CMD:
            break;
        case CMD::SPLIT_CMD:
            break;
        case CMD::ROTATE_CMD:
            break;
        case CMD::FLIPX_CMD:
            break;
        case CMD::FLIPY_CMD:
            break;

        case CMD::SELECTALL_CMD:
            break;
        case CMD::DELETE_CMD:
            break;
        case CMD::COPY_CMD:
            break;
        case CMD::CUT_CMD:
            break;
        case CMD::PASTE_CMD:
            break;
        case CMD::UNDO_CMD:
            break;

        default:
            break;
        }
    }
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
        _last_cmd = _current_cmd;
        _current_cmd = result->second;
        return true;
    }
}


