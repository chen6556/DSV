#pragma once

#include <QWidget>
#include <QMouseEvent>
#include <QCompleter>

#include "base/Editer.hpp"
#include "draw/Canvas.hpp"


QT_BEGIN_NAMESPACE
namespace Ui { class CMDWidget;}
QT_END_NAMESPACE


class CMDWidget : public QWidget
{
    Q_OBJECT

public:
    enum class CMD {Error_CMD, Open_CMD, Append_CMD, Save_CMD, Exit_CMD, Main_CMD,
        Length_CMD, Angle_CMD,
        Polyline_CMD, Circle_CMD, Ellipse_CMD, Rectangle_CMD, BSpline_CMD, Bezier_CMD, Text_CMD,
        Connect_CMD, Close_CMD, Combinate_CMD, Split_CMD, Rotate_CMD, FlipX_CMD, FlipY_CMD,
        Mirror_CMD, Array_CMD, LineArray_CMD, RingArray_CMD,
        Offset_CMD, Scale_CMD, Fillet_CMD, Trim_CMD, Extend_CMD,
        Union_CMD, Intersection_CMD, Difference_CMD, XOR_CMD,
        Delete_CMD, Copy_CMD, Cut_CMD, Paste_CMD, Undo_CMD, SelectAll_CMD};

    enum class SETTING {Absolute_SETTING, Relative_SETTING};

private:
    Ui::CMDWidget *ui = nullptr;

    QPoint _last_pos;
    QWidget *_parent = nullptr;
    QStringList _cmd_list;
    QCompleter *_completer = nullptr;
    std::map<QString, CMD> _cmd_dict;
    std::map<QString, SETTING> _setting_dict;

    CMD _current_cmd = CMD::Error_CMD;
    std::vector<double> _parameters;
    bool _relative = false;
    double _last_x, _last_y;

    Editer *_editer = nullptr;
    Canvas *_canvas = nullptr;

private:
    void init();

private slots:
    void refresh_tool(const Canvas::Tool tool);

protected:
    void mousePressEvent(QMouseEvent *event);

    void mouseMoveEvent(QMouseEvent *event);

    bool eventFilter(QObject *target, QEvent *event);

public:
signals:
    void cmd_changed(const CMD);

public:
    CMDWidget(Editer *editer, Canvas *canvas, QWidget *parent);
    ~CMDWidget();

    void clear();

    CMD cmd() const;

    bool empty() const;

    std::vector<double> &parameters();

    const std::vector<double> &parameters() const;

    void activate(const char key);

    void show();

    void hide();


    bool work();

    bool get_cmd();

    bool get_parameter();

    bool get_setting();


    void paste();

    void polyline();

    void bspline();

    void bezier();

    void text();

    void rectangle();

    void circle();

    void ellipse();

    void rotate();

    void scale();

    void offset();

    void fillet();

    void line_array();

    void ring_array();

};