#pragma once

#include <QWidget>
#include <QMouseEvent>
#include <QCompleter>

#include "base/Editer.hpp"
#include "Draw/Canvas.hpp"


QT_BEGIN_NAMESPACE
namespace Ui { class CMDWidget;}
QT_END_NAMESPACE


class CMDWidget : public QWidget
{
    Q_OBJECT

public:
    enum CMD {ERROR_CMD, OPEN_CMD, SAVE_CMD, EXIT_CMD,
        LENGTH_CMD, POLYLINE_CMD, CIRCLE_CMD, RECTANGLE_CMD, BEZIER_CMD, TEXT_CMD,
        CONNECT_CMD, CLOSE_CMD, COMBINATE_CMD, SPLIT_CMD, ROTATE_CMD, FLIPX_CMD, FLIPY_CMD,
        MIRROR_CMD, ARRAY_CMD, LINEARRAY_CMD, RINGARRAY_CMD,
        DELETE_CMD, COPY_CMD, CUT_CMD, PASTE_CMD, UNDO_CMD, SELECTALL_CMD};

private:
    Ui::CMDWidget *ui = nullptr;

    QPoint _last_pos;
    QWidget *_parent = nullptr;
    QStringList _cmd_list;
    QCompleter *_completer = nullptr;
    std::map<QString, CMD> _cmd_dict;

    CMD _current_cmd = CMD::ERROR_CMD;
    std::vector<double> _parameters;

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


    bool work();

    bool get_cmd();

    bool get_parameter();


    void paste();

    void polyline();

    void curve();

    void text();

    void rectangle();

    void circle();

};