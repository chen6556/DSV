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
    enum CMD {ERROR_CMD, OPEN_CMD, APPEND_CMD, SAVE_CMD, EXIT_CMD, MAIN_CMD,
        LENGTH_CMD, POLYLINE_CMD, CIRCLE_CMD, RECTANGLE_CMD, BEZIER_CMD, TEXT_CMD,
        CONNECT_CMD, CLOSE_CMD, COMBINATE_CMD, SPLIT_CMD, ROTATE_CMD, FLIPX_CMD, FLIPY_CMD,
        MIRROR_CMD, ARRAY_CMD, LINEARRAY_CMD, RINGARRAY_CMD, OFFSET_CMD, SCALE_CMD,
        BOOLEAN_CMD, UNION_CMD, INTERSECTION_CMD, DIFFERENCE_CMD,
        DELETE_CMD, COPY_CMD, CUT_CMD, PASTE_CMD, UNDO_CMD, SELECTALL_CMD};

    enum SETTING {ABSOLUTE_SETTING, RELATIVE_SETTING};

private:
    Ui::CMDWidget *ui = nullptr;

    QPoint _last_pos;
    QWidget *_parent = nullptr;
    QStringList _cmd_list;
    QCompleter *_completer = nullptr;
    std::map<QString, CMD> _cmd_dict;
    std::map<QString, SETTING> _setting_dict;

    CMD _current_cmd = CMD::ERROR_CMD;
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

    void curve();

    void text();

    void rectangle();

    void circle();

    void rotate();

    void scale();

    void offset();

    void line_array();

    void ring_array();

};