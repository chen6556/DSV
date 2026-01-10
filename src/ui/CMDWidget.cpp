#include <QRegularExpressionValidator>

#include "ui/CMDWidget.hpp"
#include "./ui_CMDWidget.h"
#include "io/GlobalSetting.hpp"
#include "base/Algorithm.hpp"



CMDWidget::CMDWidget(QWidget *parent)
    : QWidget(parent), ui(new Ui::CMDWidget)
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
    ui->cmd->setValidator(new QRegularExpressionValidator(QRegularExpression("([A-Za-z]+)|((-?[0-9]+(.[0-9]+)?,?)+)$")));
    ui->cmd->installEventFilter(this);

    _cmd_list << QString() << "ALL" << "ANGLE" << "ARRAY" << "BEZIER" << "BSPLINE" << "BLEND"
        << "CCIRCLE" << "CHAMFER" << "CLOSE" << "COMBINATE" << "CONNECT" << "COPY"
        << "CPOLYGON" << "CUT" << "DCIRCLE" << "DELETE" << "DETACH" << "DIFFERENCE"
        << "ELLIPSE" << "ELLIPSEARC"
        << "EXTEND" << "FILLET" << "FREEFILLET" << "FLIPX" << "FLIPY"
        << "IPOLYGON" << "INTERSECTION" << "LENGTH" << "LINEARRAY" << "MAIN" << "MIRROR"
        << "OFFSET" << "PASTE" << "PARC" << "PCIRCLE" << "POLYLINE" << "POINT" << "RECTANGLE"
        << "SEAARC" << "SERARC"
        << "RINGARRAY" << "ROTATE" << "SCALE" << "SCAARC" << "SAVE" << "SPLIT" << "TEXT"
        << "TRIM" << "UNDO" << "UNION" << "XOR";
    _cmd_list.sort();

    _cmd_dict = {{"LENGTH",CMD::Length_CMD}, {"ANGLE",CMD::Angle_CMD}, {"MAIN",CMD::Main_CMD},
        {"CCIRCLE",CMD::CCircle_CMD}, {"DCIRCLE",CMD::DCircle_CMD}, {"PCIRCLE",CMD::PCircle_CMD},
        {"ELLIPSE",CMD::Ellipse_CMD}, {"ELLIPSEARC",CMD::EllipseArc_CMD},
        {"POLYLINE",CMD::Polyline_CMD}, {"RECTANGLE",CMD::Rectangle_CMD}, {"POINT",CMD::Point_CMD},
        {"PARC",CMD::PArc_CMD}, {"SCAARC",CMD::SCAArc_CMD}, {"SEAARC",CMD::SEAArc_CMD}, {"SERARC",CMD::SERArc_CMD},
        {"CPOLYGON",CMD::CPolygon_CMD}, {"IPOLYGON",CMD::IPolygon_CMD},
        {"BSPLINE",CMD::BSpline_CMD}, {"BEZIER",CMD::Bezier_CMD}, {"CONNECT",CMD::Connect_CMD},
        {"BLEND",CMD::Blend_CMD},
        {"COMBINATE",CMD::Combinate_CMD}, {"CLOSE",CMD::Close_CMD}, {"DETACH",CMD::Detach_CMD},
        {"ROTATE",CMD::Rotate_CMD}, {"FLIPX",CMD::FlipX_CMD}, {"FLIPY",CMD::FlipY_CMD},
        {"TEXT",CMD::Text_CMD}, {"TRIM",CMD::Trim_CMD}, {"EXTEND",CMD::Extend_CMD}, {"MIRROR",CMD::Mirror_CMD},
        {"ARRAY",CMD::Array_CMD}, {"LINEARRAY",CMD::LineArray_CMD}, {"RINGARRAY",CMD::RingArray_CMD},
        {"OFFSET",CMD::Offset_CMD}, {"SCALE", CMD::Scale_CMD},
        {"FILLET",CMD::Fillet_CMD}, {"CHAMFER", CMD::Chamfer_CMD}, {"SPLIT",CMD::Split_CMD},
        {"UNION",CMD::Union_CMD}, {"INTERSECTION",CMD::Intersection_CMD}, {"DIFFERENCE",CMD::Difference_CMD},
        {"XOR",CMD::XOR_CMD},
        {"DELETE",CMD::Delete_CMD}, {"COPY",CMD::Copy_CMD}, {"CUT",CMD::Cut_CMD}, {"PASTE",CMD::Paste_CMD},
        {"UNDO",CMD::Undo_CMD}, {"ALL",CMD::SelectAll_CMD}};

    _setting_dict = {{"RELATIVE", SETTING::Relative_SETTING}, {"ABSOLUTE", SETTING::Absolute_SETTING}};

    _cmd_tool_dict = {{CMD::Length_CMD, CanvasOperations::Tool::Measure}, {CMD::Angle_CMD, CanvasOperations::Tool::Angle},
        {CMD::Polyline_CMD, CanvasOperations::Tool::Polyline}, {CMD::Rectangle_CMD, CanvasOperations::Tool::Rectangle},
        {CMD::CPolygon_CMD, CanvasOperations::Tool::Polygon0}, {CMD::IPolygon_CMD, CanvasOperations::Tool::Polygon1},
        {CMD::RingArray_CMD, CanvasOperations::Tool::RingArray}, {CMD::Rotate_CMD, CanvasOperations::Tool::Rotate},
        {CMD::Text_CMD, CanvasOperations::Tool::Text}, {CMD::PArc_CMD, CanvasOperations::Tool::Arc0},
        {CMD::SCAArc_CMD, CanvasOperations::Tool::Arc1}, {CMD::SEAArc_CMD, CanvasOperations::Tool::Arc2},
        {CMD::SERArc_CMD, CanvasOperations::Tool::Arc3}, {CMD::Point_CMD, CanvasOperations::Tool::Point},
        {CMD::Bezier_CMD, CanvasOperations::Tool::Bezier}, {CMD::BSpline_CMD, CanvasOperations::Tool::BSpline},
        {CMD::CCircle_CMD, CanvasOperations::Tool::Circle0},{CMD::DCircle_CMD, CanvasOperations::Tool::Circle1},
        {CMD::PCircle_CMD, CanvasOperations::Tool::Circle2}, {CMD::Ellipse_CMD, CanvasOperations::Tool::Ellipse0},
        {CMD::EllipseArc_CMD, CanvasOperations::Tool::Ellipse1},
        {CMD::Mirror_CMD, CanvasOperations::Tool::Mirror}, {CMD::Trim_CMD, CanvasOperations::Tool::Trim},
        {CMD::Extend_CMD, CanvasOperations::Tool::Extend}, {CMD::Fillet_CMD, CanvasOperations::Tool::Fillet},
        {CMD::FreeFillet_CMD, CanvasOperations::Tool::FreeFillet}, {CMD::Blend_CMD, CanvasOperations::Tool::Blend},
        {CMD::Chamfer_CMD, CanvasOperations::Tool::Chamfer}, {CMD::Split_CMD, CanvasOperations::Tool::Split},
        {CMD::Difference_CMD, CanvasOperations::Tool::ShapeDifference}};

    _tool_cmd_dict = {{CanvasOperations::Tool::Measure, CMD::Length_CMD}, {CanvasOperations::Tool::Angle, CMD::Angle_CMD},
        {CanvasOperations::Tool::Polyline, CMD::Polyline_CMD}, {CanvasOperations::Tool::Rectangle, CMD::Rectangle_CMD},
        {CanvasOperations::Tool::Polygon0, CMD::CPolygon_CMD}, {CanvasOperations::Tool::Polygon1, CMD::IPolygon_CMD},
        {CanvasOperations::Tool::BSpline, CMD::BSpline_CMD}, {CanvasOperations::Tool::Bezier, CMD::Bezier_CMD},
        {CanvasOperations::Tool::Text, CMD::Text_CMD}, {CanvasOperations::Tool::Arc0, CMD::PArc_CMD},
        {CanvasOperations::Tool::Arc1, CMD::SCAArc_CMD}, {CanvasOperations::Tool::Arc2, CMD::SEAArc_CMD},
        {CanvasOperations::Tool::Arc3, CMD::SERArc_CMD}, {CanvasOperations::Tool::Point, CMD::Point_CMD},
        {CanvasOperations::Tool::Circle0, CMD::CCircle_CMD},{CanvasOperations::Tool::Circle1, CMD::DCircle_CMD},
        {CanvasOperations::Tool::Circle2, CMD::PCircle_CMD}, {CanvasOperations::Tool::Ellipse0, CMD::Ellipse_CMD},
        {CanvasOperations::Tool::Ellipse1, CMD::EllipseArc_CMD},
        {CanvasOperations::Tool::Mirror, CMD::Mirror_CMD}, {CanvasOperations::Tool::Extend, CMD::Extend_CMD},
        {CanvasOperations::Tool::Trim, CMD::Trim_CMD}, {CanvasOperations::Tool::Split, CMD::Split_CMD},
        {CanvasOperations::Tool::Fillet, CMD::Fillet_CMD}, {CanvasOperations::Tool::FreeFillet, CMD::FreeFillet_CMD},
        {CanvasOperations::Tool::Chamfer, CMD::Chamfer_CMD}, {CanvasOperations::Tool::Blend, CMD::Blend_CMD},
        {CanvasOperations::Tool::ShapeDifference, CMD::Difference_CMD}, {CanvasOperations::Tool::Rotate, CMD::Rotate_CMD},
        {CanvasOperations::Tool::RingArray, CMD::RingArray_CMD}};

    _cmd_tips_dict = {{CMD::Angle_CMD, "Angle"}, {CMD::Bezier_CMD, "Bezier"}, {CMD::BSpline_CMD, "BSpline"},
        {CMD::CCircle_CMD, "Center-Radius Circle"}, {CMD::Combinate_CMD, "Combinate"},
        {CMD::Connect_CMD, "Connect"}, {CMD::Copy_CMD, "Copy"}, {CMD::Cut_CMD, "Cut"},
        {CMD::DCircle_CMD, "2-Point Circle"}, {CMD::Detach_CMD, "Detach"}, {CMD::Difference_CMD, "Difference"},
        {CMD::Ellipse_CMD, "Ellipse"}, {CMD::EllipseArc_CMD, "Ellipse Arc"}, {CMD::Point_CMD, "Point"},
        {CMD::Extend_CMD, "Extend"}, {CMD::Fillet_CMD, "Fillet"}, {CMD::FreeFillet_CMD, "Free Fillet"},
        {CMD::Chamfer_CMD, "Chamfer"}, {CMD::PArc_CMD, "3-Point Arc"}, {CMD::SCAArc_CMD, "Start-Center-Angle Arc"},
        {CMD::SEAArc_CMD, "Start-End-Angle Arc"}, {CMD::SERArc_CMD, "Start-End-Radius Arc"},
        {CMD::FlipX_CMD, "Flip X"}, {CMD::FlipY_CMD, "Flip Y"}, {CMD::Intersection_CMD, "Intersection"},
        {CMD::Length_CMD, "Length"}, {CMD::LineArray_CMD, "Line Array"}, {CMD::Mirror_CMD, "Mirror"},
        {CMD::Offset_CMD, "Offset"}, {CMD::Paste_CMD, "Paste"}, {CMD::PCircle_CMD, "3-Point Circle"},
        {CMD::Polyline_CMD, "Polyline"}, {CMD::Rectangle_CMD, "Rectangle"}, {CMD::RingArray_CMD, "Ring Array"},
        {CMD::CPolygon_CMD, "Polygon Circumscribed"}, {CMD::IPolygon_CMD, "Polygon Inscribed"},
        {CMD::Rotate_CMD, "Rotate"}, {CMD::Scale_CMD, "Scale"}, {CMD::Split_CMD, "Split"}, {CMD::Blend_CMD, "Blend"},
        {CMD::Text_CMD, "Text"}, {CMD::Trim_CMD, "Trim"}, {CMD::Union_CMD, "Union"}, {CMD::XOR_CMD, "XOR"}};

    _direct_cmd_list = {CMD::Error_CMD, CMD::Main_CMD,
        CMD::Connect_CMD, CMD::Close_CMD, CMD::Combinate_CMD, CMD::Detach_CMD, CMD::FlipX_CMD, CMD::FlipY_CMD,
        CMD::LineArray_CMD, CMD::Array_CMD, CMD::Offset_CMD, CMD::Scale_CMD,
        CMD::Union_CMD, CMD::Intersection_CMD, CMD::XOR_CMD,
        CMD::Delete_CMD, CMD::Copy_CMD, CMD::Cut_CMD, CMD::Paste_CMD, CMD::Undo_CMD, CMD::SelectAll_CMD};

    _completer = new QCompleter(_cmd_list, this);
    _completer->setCaseSensitivity(Qt::CaseSensitivity::CaseInsensitive);
    _completer->setFilterMode(Qt::MatchFlag::MatchContains);
    _completer->setCompletionMode(QCompleter::CompletionMode::InlineCompletion);
    ui->cmd->setCompleter(_completer);

    connect(CanvasOperations::CanvasOperation::canvas, &Canvas::tool_changed, this, &CMDWidget::refresh_tool);
    connect(CanvasOperations::CanvasOperation::canvas, &Canvas::refresh_cmd_parameters_label,
        [this]() { ui->parameter_label->setText(CanvasOperations::CanvasOperation::operation()[
            CanvasOperations::CanvasOperation::tool[0]]->cmd_tips()); });
}


void CMDWidget::refresh_tool(const CanvasOperations::Tool tool)
{
    switch (tool)
    {
    case CanvasOperations::Tool::Select:
        clear();
        break;
    default:
        if (auto it = _tool_cmd_dict.find(tool); it != _tool_cmd_dict.end())
        {
            if (_parameters.empty() && _current_cmd == CMD::Error_CMD)
            {
                _parameters.push_back(0);
            }
            _current_cmd = it->second;
            ui->parameter_label->setText(CanvasOperations::CanvasOperation::operation()[tool]->cmd_tips());
            if (auto it2 = _cmd_tips_dict.find(it->second); it2 != _cmd_tips_dict.end())
            {
                ui->cmd_label->setText(it2->second);
            }
            else
            {
                ui->cmd_label->clear();
            }
        }
        break;
    }
}


bool CMDWidget::eventFilter(QObject *target, QEvent *event)
{
    if (event->type() == QEvent::Type::KeyPress)
    {
        if (QKeyEvent *kevent = static_cast<QKeyEvent *>(event);
            kevent->modifiers() == Qt::KeyboardModifier::NoModifier)
        {
            switch (kevent->key())
            {
            case Qt::Key::Key_Space:
                if (ui->cmd->text().isEmpty() && _current_cmd == CMD::Error_CMD)
                {
                    _current_cmd = _last_cmd;
                }
                else
                {
                    if (get_setting())
                    {
                        return QWidget::eventFilter(target, event);
                    }
                    else if (!get_parameter())
                    {
                        ui->cmd->setText(_completer->currentCompletion());
                        get_cmd();
                    }
                }
                work();
                return true;
            case Qt::Key::Key_Tab:
                CanvasOperations::CanvasOperation::operation()[
                    CanvasOperations::CanvasOperation::tool[0]]->switch_parameters_type();
                ui->parameter_label->setText(CanvasOperations::CanvasOperation::operation()[
                    CanvasOperations::CanvasOperation::tool[0]]->cmd_tips());
                return true;
            default:
                break;
            }
        }
    }

    return QWidget::eventFilter(target, event);
}


void CMDWidget::clear()
{
    ui->cmd->clear();
    ui->cmd_label->clear();
    ui->parameter_label->clear();
    ui->absolute_label->clear();
    _current_cmd = CMD::Error_CMD;
    _completer->setCurrentRow(0);
    _parameters.clear();
}

CMDWidget::CMD CMDWidget::cmd() const
{
    if (auto result = _cmd_dict.find(ui->cmd->text().toUpper()); result == _cmd_dict.end())
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

void CMDWidget::activate(const char key)
{
    ui->cmd->setText(QString(key));
    ui->cmd->setFocus();
}

void CMDWidget::work_last_cmd()
{
    work(_last_cmd);
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
    if (std::find(_direct_cmd_list.begin(), _direct_cmd_list.end(), _current_cmd) != _direct_cmd_list.end())
    {
        switch (_current_cmd)
        {
        case CMD::Main_CMD:
        case CMD::Array_CMD:
            emit cmd_changed(_current_cmd);
            _current_cmd = CMD::Error_CMD;
            break;
        case CMD::Connect_CMD:
            connect_polyline();
            _current_cmd = CMD::Error_CMD;
            break;
        case CMD::Close_CMD:
            close_polyline();
            _current_cmd = CMD::Error_CMD;
            break;
        case CMD::Combinate_CMD:
            combinate();
            _current_cmd = CMD::Error_CMD;
            break;
        case CMD::Detach_CMD:
            detach();
            _current_cmd = CMD::Error_CMD;
            break;
        case CMD::FlipX_CMD:
            flip_x();
            _current_cmd = CMD::Error_CMD;
            break;
        case CMD::FlipY_CMD:
            flip_y();
            _current_cmd = CMD::Error_CMD;
            break;
        case CMD::Scale_CMD:
            scale();
            break;
        case CMD::Offset_CMD:
            offset();
            break;
        case CMD::Union_CMD:
            shape_union();
            _current_cmd = CMD::Error_CMD;
            break;
        case CMD::Intersection_CMD:
            shape_intersection();
            _current_cmd = CMD::Error_CMD;
            break;
        case CMD::XOR_CMD:
            shape_xor();
            _current_cmd = CMD::Error_CMD;
            break;
        case CMD::LineArray_CMD:
            line_array();
            break;
        case CMD::SelectAll_CMD:
            CanvasOperations::CanvasOperation::editer->reset_selected_mark(true);
            CanvasOperations::CanvasOperation::canvas->refresh_selected_ibo();
            CanvasOperations::CanvasOperation::canvas->update();
            _current_cmd = CMD::Error_CMD;
            break;
        case CMD::Delete_CMD:
            delete_selected_objects();
            _current_cmd = CMD::Error_CMD;
            break;
        case CMD::Copy_CMD:
            CanvasOperations::CanvasOperation::canvas->copy();
            _current_cmd = CMD::Error_CMD;
            break;
        case CMD::Cut_CMD:
            CanvasOperations::CanvasOperation::canvas->cut();
            CanvasOperations::CanvasOperation::canvas->update();
            _current_cmd = CMD::Error_CMD;
            break;
        case CMD::Paste_CMD:
            paste();
            break;
        case CMD::Undo_CMD:
            CanvasOperations::CanvasOperation::editer->undo();
            CanvasOperations::CanvasOperation::canvas->refresh_vbo(true);
            CanvasOperations::CanvasOperation::canvas->refresh_selected_ibo();
            CanvasOperations::CanvasOperation::canvas->update();
            _current_cmd = CMD::Error_CMD;
            break;
        default:
            clear();
            return false;
        }
    }
    else
    {
        if (_parameters.empty())
        {
            ui->absolute_label->setText(CanvasOperations::CanvasOperation::absolute_coord ? "Absolute" : "Relative");
            CanvasOperations::CanvasOperation::canvas->use_tool(_cmd_tool_dict[_current_cmd]);
            _parameters.push_back(0);
        }
        else
        {
            read_parameters(_parameters.size() - 1);
        }
    }
    if (CanvasOperations::CanvasOperation::tool[0] != CanvasOperations::Tool::Select)
    {
        ui->parameter_label->setText(CanvasOperations::CanvasOperation::operation()[
            CanvasOperations::CanvasOperation::tool[0]]->cmd_tips());
    }
    return true;
}

bool CMDWidget::work(const CMD cmd)
{
    if (_current_cmd != CMD::Error_CMD)
    {
        clear();
    }
    _last_cmd = _current_cmd = cmd;
    return work();
}

bool CMDWidget::get_cmd()
{
    if (auto result = _cmd_dict.find(ui->cmd->text().toUpper()); result == _cmd_dict.end())
    {
        return false;
    }
    else
    {
        CanvasOperations::CanvasOperation::canvas->use_tool(CanvasOperations::Tool::Select);
        _last_cmd = _current_cmd = result->second;
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
        bool result = false;
        for (const QString &str : ui->cmd->text().split(','))
        {
            bool is_num = false;
            const double value = str.toDouble(&is_num);
            result = result || is_num;
            if (is_num)
            {
                _parameters.push_back(value);
            }
        }
        return result;
    }
}

bool CMDWidget::get_setting()
{
    if (auto result = _setting_dict.find(ui->cmd->text().toUpper()); result == _setting_dict.cend())
    {
        return false;
    }
    else
    {
        switch (result->second)
        {
        case SETTING::Relative_SETTING:
            CanvasOperations::CanvasOperation::absolute_coord = false;
            ui->absolute_label->setText("Relative");
            break;
        case SETTING::Absolute_SETTING:
            CanvasOperations::CanvasOperation::absolute_coord = true;
            ui->absolute_label->setText("Absolute");
            break;
        default:
            break;
        }
        ui->cmd->clear();
        return true;
    }
}


void CMDWidget::read_parameters(const int count)
{
    if (_parameters.size() <= 1)
    {
        if (CanvasOperations::CanvasOperation::operation()[CanvasOperations::CanvasOperation::tool[0]]->read_parameters(nullptr, 0))
        {
            CanvasOperations::CanvasOperation::canvas->update();
        }
    }
    else
    {
        if (_parameters.size() >= count + 1)
        {
            double *params = new double[count];
            for (int i = 0; i < count; ++i)
            {
                params[i] = _parameters[i + 1];
            }
            CanvasOperations::CanvasOperation::operation()[CanvasOperations::CanvasOperation::tool[0]]->read_parameters(params, count);
            delete[] params;
            CanvasOperations::CanvasOperation::canvas->update();
        }
        _parameters.erase(_parameters.begin() + 1, _parameters.end());
    }

    if (CanvasOperations::CanvasOperation::tool[0] == CanvasOperations::Tool::Select)
    {
        CanvasOperations::CanvasOperation::canvas->use_tool(CanvasOperations::Tool::Select);
    }
}

void CMDWidget::paste()
{
    switch (_parameters.size())
    {
    case 0:
        _parameters.emplace_back(0);
        ui->cmd_label->setText("Paste");
        ui->parameter_label->setText("(x, y):");
        break;
    case 1:
        CanvasOperations::CanvasOperation::canvas->paste();
        CanvasOperations::CanvasOperation::canvas->update();
        clear();
        break;
    case 2:
        _parameters.pop_back();
        break;
    default:
        CanvasOperations::CanvasOperation::canvas->paste(_parameters[1], _parameters[2]);
        CanvasOperations::CanvasOperation::canvas->update();
        clear();
        break;
    }
}

void CMDWidget::delete_selected_objects()
{
    std::set<Geo::Type> types;
    for (const Geo::Geometry *object : CanvasOperations::CanvasOperation::editer->selected())
    {
        if (const Combination *combination = dynamic_cast<const Combination *>(object))
        {
            for (const Geo::Geometry *item : *combination)
            {
                types.insert(item->type());
            }
        }
        else
        {
            types.insert(object->type());
        }
    }
    if (!types.empty())
    {
        CanvasOperations::CanvasOperation::operation().clear();
        CanvasOperations::CanvasOperation::editer->remove_selected();
        CanvasOperations::CanvasOperation::canvas->refresh_vbo(types, true);
        CanvasOperations::CanvasOperation::canvas->refresh_selected_ibo();
        CanvasOperations::CanvasOperation::canvas->update();
    }
}

void CMDWidget::connect_polyline()
{
    std::vector<Geo::Geometry *> objects = CanvasOperations::CanvasOperation::editer->selected();
    std::set<Geo::Type> types;
    for (const Geo::Geometry *object : objects)
    {
        if (Geo::Type type = object->type(); type == Geo::Type::POLYLINE
            || type == Geo::Type::BEZIER || type == Geo::Type::BSPLINE)
        {
            types.insert(type);
        }
    }
    if (CanvasOperations::CanvasOperation::editer->connect(objects, GlobalSetting::setting().catch_distance))
    {
        CanvasOperations::CanvasOperation::canvas->refresh_vbo(types, true);
        CanvasOperations::CanvasOperation::canvas->refresh_selected_ibo();
    }
}

void CMDWidget::close_polyline()
{
    std::vector<Geo::Geometry *> objects = CanvasOperations::CanvasOperation::editer->selected();
    std::set<Geo::Type> types;
    for (const Geo::Geometry *object : objects)
    {
        if (Geo::Type type = object->type(); type == Geo::Type::POLYLINE
            || type == Geo::Type::BEZIER || type == Geo::Type::BSPLINE)
        {
            types.insert(type);
        }
    }
    types.insert(Geo::Type::POLYGON);
    if (CanvasOperations::CanvasOperation::editer->close_polyline(objects))
    {
        CanvasOperations::CanvasOperation::canvas->refresh_vbo(types, true);
        CanvasOperations::CanvasOperation::canvas->refresh_selected_ibo();
    }
}

void CMDWidget::combinate()
{
    if (std::vector<Geo::Geometry *> objects = CanvasOperations::CanvasOperation::editer->selected();
        CanvasOperations::CanvasOperation::editer->combinate(objects))
    {
        std::set<Geo::Type> types;
        for (const Geo::Geometry *object : objects)
        {
            if (const Combination *combination = dynamic_cast<const Combination *>(object))
            {
                for (const Geo::Geometry *item : *combination)
                {
                    types.insert(item->type());
                }
            }
            else
            {
                types.insert(object->type());
            }
        }
        CanvasOperations::CanvasOperation::canvas->refresh_vbo(types, true);
        CanvasOperations::CanvasOperation::canvas->refresh_selected_ibo();
    }
}

void CMDWidget::detach()
{
    std::vector<Geo::Geometry *> objects = CanvasOperations::CanvasOperation::editer->selected();
    std::set<Geo::Type> types;
    for (const Geo::Geometry *object : objects)
    {
        if (const Combination *combination = dynamic_cast<const Combination *>(object))
        {
            for (const Geo::Geometry *item : *combination)
            {
                types.insert(item->type());
            }
        }
    }
    if (!types.empty())
    {
        CanvasOperations::CanvasOperation::editer->detach(objects);
        CanvasOperations::CanvasOperation::canvas->refresh_vbo(types, true);
        CanvasOperations::CanvasOperation::canvas->refresh_selected_ibo();
    }
}

void CMDWidget::scale()
{
    switch (_parameters.size())
    {
    case 0:
        _parameters.emplace_back(0);
        ui->cmd_label->setText("Scale");
        ui->parameter_label->setText("ratio:");
        break;
    case 1:
        clear();
        break;
    default:
        if (_parameters.back() > 0)
        {
            if (std::vector<Geo::Geometry *> objects = CanvasOperations::CanvasOperation::editer->selected();
                CanvasOperations::CanvasOperation::editer->scale(objects,
                QApplication::keyboardModifiers() != Qt::KeyboardModifier::ControlModifier, _parameters.back()))
            {
                std::set<Geo::Type> types;
                for (const Geo::Geometry *object : objects)
                {
                    if (const Combination *combination = dynamic_cast<const Combination *>(object))
                    {
                        for (const Geo::Geometry *item : *combination)
                        {
                            types.insert(item->type());
                        }
                    }
                    else
                    {
                        types.insert(object->type());
                    }
                }
                CanvasOperations::CanvasOperation::canvas->refresh_vbo(types, false);
                if (objects.size() == 1)
                {
                    CanvasOperations::CanvasOperation::canvas->refresh_selected_ibo(objects.front());
                }
                CanvasOperations::CanvasOperation::canvas->update();
                clear();
            }
            else
            {
                _parameters.pop_back();
            }
        }
        else
        {
            _parameters.pop_back();
        }
        break;
    }
}

void CMDWidget::offset()
{
    switch (_parameters.size())
    {
    case 0:
        _parameters.emplace_back(0);
        ui->cmd_label->setText("Offset");
        ui->parameter_label->setText("distance:");
        break;
    case 1:
        clear();
        break;
    default:
        if (_parameters.back() != 0)
        {
            if (std::vector<Geo::Geometry *> objects = CanvasOperations::CanvasOperation::editer->selected();
                CanvasOperations::CanvasOperation::editer->offset(objects, _parameters.back(),
                static_cast<Geo::Offset::JoinType>(GlobalSetting::setting().offset_join_type),
                static_cast<Geo::Offset::EndType>(GlobalSetting::setting().offset_end_type)))
            {
                std::set<Geo::Type> types;
                for (const Geo::Geometry *object : objects)
                {
                    types.insert(object->type());
                }
                CanvasOperations::CanvasOperation::canvas->refresh_vbo(types, true);
                CanvasOperations::CanvasOperation::canvas->update();
                clear();
            }
            else
            {
                _parameters.pop_back();
            }
        }
        else
        {
            _parameters.pop_back();
        }
        break;
    }
}

void CMDWidget::line_array()
{
    switch (_parameters.size())
    {
    case 0:
        _parameters.emplace_back(0);
        ui->cmd_label->setText("Line Array");
        ui->parameter_label->setText("(X-count, Y-count, X-space, Y-space):");
        break;
    case 1:
        if (std::vector<Geo::Geometry *> objects = CanvasOperations::CanvasOperation::editer->selected();
            CanvasOperations::CanvasOperation::editer->line_array(objects,
            GlobalSetting::setting().ui->array_x_item->value(), GlobalSetting::setting().ui->array_y_item->value(),
            GlobalSetting::setting().ui->array_x_space->value(), GlobalSetting::setting().ui->array_y_space->value()))
        {
            std::set<Geo::Type> types;
            for (const Geo::Geometry *object : objects)
            {
                if (const Combination *combination = dynamic_cast<const Combination *>(object))
                {
                    for (const Geo::Geometry *item : *combination)
                    {
                        types.insert(item->type());
                    }
                }
                else
                {
                    types.insert(object->type());
                }
            }
            CanvasOperations::CanvasOperation::canvas->refresh_vbo(types, true);
            CanvasOperations::CanvasOperation::canvas->refresh_selected_ibo();
            CanvasOperations::CanvasOperation::canvas->update();
        }
        clear();
        break;
    default:
        if (_parameters.size() >= 5 && std::abs(_parameters[1]) >= 1 && std::abs(_parameters[2]) >= 1)
        {
            if (std::vector<Geo::Geometry *> objects = CanvasOperations::CanvasOperation::editer->selected();
                CanvasOperations::CanvasOperation::editer->line_array(objects, _parameters[1],
                _parameters[2], _parameters[3], _parameters[4]))
            {
                std::set<Geo::Type> types;
                for (const Geo::Geometry *object : objects)
                {
                    if (const Combination *combination = dynamic_cast<const Combination *>(object))
                    {
                        for (const Geo::Geometry *item : *combination)
                        {
                            types.insert(item->type());
                        }
                    }
                    else
                    {
                        types.insert(object->type());
                    }
                }
                CanvasOperations::CanvasOperation::canvas->refresh_vbo(types, true);
                CanvasOperations::CanvasOperation::canvas->refresh_selected_ibo();
                CanvasOperations::CanvasOperation::canvas->update();
            }
            clear();
        }
        else
        {
            _parameters.erase(_parameters.begin() + 1, _parameters.end());
        }
        break;
    }
}

void CMDWidget::flip_x()
{
    std::vector<Geo::Geometry *> objects = CanvasOperations::CanvasOperation::editer->selected();
    CanvasOperations::CanvasOperation::editer->flip(objects, true,
        QApplication::keyboardModifiers() != Qt::ControlModifier, GlobalSetting::setting().ui->to_all_layers->isChecked());
    std::set<Geo::Type> types;
    for (const Geo::Geometry *object : objects)
    {
        if (const Combination *combination = dynamic_cast<const Combination *>(object))
        {
            for (const Geo::Geometry *item : *combination)
            {
                types.insert(item->type());
            }
        }
        else
        {
            types.insert(object->type());
        }
    }
    if (types.empty())
    {
        CanvasOperations::CanvasOperation::canvas->refresh_vbo(false);
    }
    else
    {
        CanvasOperations::CanvasOperation::canvas->refresh_vbo(types, false);
    }
    if (objects.size() == 1)
    {
        CanvasOperations::CanvasOperation::canvas->refresh_selected_ibo(objects.front());
    }
    CanvasOperations::CanvasOperation::operation().clear();
    CanvasOperations::CanvasOperation::canvas->update();
}

void CMDWidget::flip_y()
{
    std::vector<Geo::Geometry *> objects = CanvasOperations::CanvasOperation::editer->selected();
    CanvasOperations::CanvasOperation::editer->flip(objects, false,
        QApplication::keyboardModifiers() != Qt::ControlModifier, GlobalSetting::setting().ui->to_all_layers->isChecked());
    std::set<Geo::Type> types;
    for (const Geo::Geometry *object : objects)
    {
        if (const Combination *combination = dynamic_cast<const Combination *>(object))
        {
            for (const Geo::Geometry *item : *combination)
            {
                types.insert(item->type());
            }
        }
        else
        {
            types.insert(object->type());
        }
    }
    if (types.empty())
    {
        CanvasOperations::CanvasOperation::canvas->refresh_vbo(false);
    }
    else
    {
        CanvasOperations::CanvasOperation::canvas->refresh_vbo(types, false);
    }
    if (objects.size() == 1)
    {
        CanvasOperations::CanvasOperation::canvas->refresh_selected_ibo(objects.front());
    }
    CanvasOperations::CanvasOperation::operation().clear();
    CanvasOperations::CanvasOperation::canvas->update();
}

void CMDWidget::shape_intersection()
{
    Geo::Geometry *shape0 = nullptr, *shape1 = nullptr;
    std::set<Geo::Type> types;
    for (Geo::Geometry *object : CanvasOperations::CanvasOperation::editer->selected())
    {
        if (const Geo::Type type = object->type(); type != Geo::Type::POLYGON
            && type != Geo::Type::CIRCLE && type != Geo::Type::ELLIPSE)
        {
            continue;
        }
        else
        {
            if (type == Geo::Type::ELLIPSE && static_cast<const Geo::Ellipse *>(object)->is_arc())
            {
                continue;
            }
            types.insert(type);
        }
        if (shape0 == nullptr)
        {
            shape0 = object;
        }
        else
        {
            shape1 = object;
            break;
        }
    }

    if (CanvasOperations::CanvasOperation::editer->shape_intersection(shape0, shape1))
    {
        if (types.find(Geo::Type::POLYGON) != types.end() &&
            (types.find(Geo::Type::CIRCLE) != types.end() || types.find(Geo::Type::ELLIPSE) != types.end()))
        {
            types.insert(Geo::Type::POLYLINE);
        }
        CanvasOperations::CanvasOperation::canvas->refresh_vbo(types, true);
        CanvasOperations::CanvasOperation::canvas->refresh_selected_ibo();
        CanvasOperations::CanvasOperation::canvas->update();
    }
}

void CMDWidget::shape_union()
{
    Geo::Geometry *shape0 = nullptr, *shape1 = nullptr;
    std::set<Geo::Type> types;
    for (Geo::Geometry *object : CanvasOperations::CanvasOperation::editer->selected())
    {
        if (const Geo::Type type = object->type(); type != Geo::Type::POLYGON
            && type != Geo::Type::CIRCLE && type != Geo::Type::ELLIPSE)
        {
            continue;
        }
        else
        {
            if (type == Geo::Type::ELLIPSE && static_cast<const Geo::Ellipse *>(object)->is_arc())
            {
                continue;
            }
            types.insert(type);
        }
        if (shape0 == nullptr)
        {
            shape0 = object;
        }
        else
        {
            shape1 = object;
            break;
        }
    }

    if (CanvasOperations::CanvasOperation::editer->shape_union(shape0, shape1))
    {
        if (types.find(Geo::Type::POLYGON) != types.end() &&
            (types.find(Geo::Type::CIRCLE) != types.end() || types.find(Geo::Type::ELLIPSE) != types.end()))
        {
            types.insert(Geo::Type::POLYLINE);
        }
        CanvasOperations::CanvasOperation::canvas->refresh_vbo(types, true);
        CanvasOperations::CanvasOperation::canvas->refresh_selected_ibo();
        CanvasOperations::CanvasOperation::canvas->update();
    }
}

void CMDWidget::shape_xor()
{
    Geo::Geometry *shape0 = nullptr, *shape1 = nullptr;
    std::set<Geo::Type> types;
    for (Geo::Geometry *object : CanvasOperations::CanvasOperation::editer->selected())
    {
        if (const Geo::Type type = object->type(); type != Geo::Type::POLYGON
            && type != Geo::Type::CIRCLE && type != Geo::Type::ELLIPSE)
        {
            continue;
        }
        else
        {
            if (type == Geo::Type::ELLIPSE && static_cast<const Geo::Ellipse *>(object)->is_arc())
            {
                continue;
            }
            types.insert(type);
        }
        if (shape0 == nullptr)
        {
            shape0 = object;
        }
        else
        {
            shape1 = object;
            break;
        }
    }

    if (CanvasOperations::CanvasOperation::editer->shape_xor(shape0, shape1))
    {
        if (types.find(Geo::Type::POLYGON) != types.end() &&
            (types.find(Geo::Type::CIRCLE) != types.end() || types.find(Geo::Type::ELLIPSE) != types.end()))
        {
            types.insert(Geo::Type::POLYLINE);
        }
        CanvasOperations::CanvasOperation::canvas->refresh_vbo(types, true);
        CanvasOperations::CanvasOperation::canvas->refresh_selected_ibo();
        CanvasOperations::CanvasOperation::canvas->update();
    }
}
