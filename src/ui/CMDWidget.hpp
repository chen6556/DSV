#pragma once

#include <QWidget>
#include <QMouseEvent>
#include <QCompleter>
#include "draw/CanvasOperation.hpp"


QT_BEGIN_NAMESPACE
namespace Ui { class CMDWidget;}
QT_END_NAMESPACE


class CMDWidget : public QWidget
{
    Q_OBJECT

public:
    enum class CMD {Error_CMD, Main_CMD, Array_CMD,
        Delete_CMD, Copy_CMD, Cut_CMD, Paste_CMD, Undo_CMD, SelectAll_CMD,
        Length_CMD, Angle_CMD, Polyline_CMD, CCircle_CMD, DCircle_CMD, PCircle_CMD,
        PArc_CMD, SCAArc_CMD, SEAArc_CMD, SERArc_CMD, Ellipse_CMD, EllipseArc_CMD, 
        Rectangle_CMD, CPolygon_CMD, IPolygon_CMD, Point_CMD, BSpline_CMD, Bezier_CMD, Text_CMD,
        Connect_CMD, Blend_CMD, Close_CMD, Combinate_CMD, Detach_CMD, Rotate_CMD, FlipX_CMD, FlipY_CMD,
        Mirror_CMD, Offset_CMD, Scale_CMD, Fillet_CMD, FreeFillet_CMD, Chamfer_CMD,
        Trim_CMD, Extend_CMD, Split_CMD,
        Union_CMD, Intersection_CMD, Difference_CMD, XOR_CMD, LineArray_CMD, RingArray_CMD};

    enum class SETTING {Absolute_SETTING, Relative_SETTING};

private:
    Ui::CMDWidget *ui = nullptr;

    QStringList _cmd_list;
    QCompleter *_completer = nullptr;
    std::unordered_map<QString, CMD> _cmd_dict;
    std::unordered_map<QString, SETTING> _setting_dict;
    std::unordered_map<CMD, CanvasOperations::Tool> _cmd_tool_dict;
    std::unordered_map<CanvasOperations::Tool, CMD> _tool_cmd_dict;
    std::unordered_map<CMD, QString> _cmd_tips_dict;
    std::vector<CMD> _direct_cmd_list;

    CMD _last_cmd = CMD::Error_CMD;
    CMD _current_cmd = CMD::Error_CMD;
    std::vector<double> _parameters;

private:
    void init();

private slots:
    void refresh_tool(const CanvasOperations::Tool tool);

protected:
    bool eventFilter(QObject *target, QEvent *event);

public:
signals:
    void cmd_changed(const CMD);

public:
    CMDWidget(QWidget *parent);
    ~CMDWidget();

    void clear();

    CMD cmd() const;

    bool empty() const;

    void activate(const char key);

    void work_last_cmd();

    void show();

    void hide();


    bool work();

    bool work(const CMD cmd);

    bool get_cmd();

    bool get_parameter();

    bool get_setting();


    void read_parameters(const int count);

    void paste();

    void delete_selected_objects();

    void connect_polyline();

    void close_polyline();

    void combinate();

    void detach();

    void scale();

    void offset();

    void line_array();

    void flip_x();

    void flip_y();

    void shape_intersection();

    void shape_union();

    void shape_xor();
};