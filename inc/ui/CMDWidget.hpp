#pragma once

#include <QWidget>
#include <QMouseEvent>
#include <QCompleter>


QT_BEGIN_NAMESPACE
namespace Ui { class CMDWidget;}
QT_END_NAMESPACE


class CMDWidget : public QWidget
{
    Q_OBJECT

public:
    enum CMD {ERROR_CMD, POLYLINE_CMD, CIRCLE_CMD, RECTANGLE_CMD, BEZIER_CMD, TEXT_CMD,
        CONNECT_CMD, COMBINATE_CMD, SPLIT_CMD, ROTATE_CMD, FLIPX_CMD, FLIPY_CMD,
        DELETE_CMD, COPY_CMD, CUT_CMD, PASTE_CMD, UNDO_CMD, SELECTALL_CMD};

private:
    Ui::CMDWidget *ui = nullptr;

    QPoint _last_pos;
    QWidget *_parent = nullptr;
    QStringList _cmd_list;
    QCompleter *_completer = nullptr;
    std::map<QString, CMD> _cmd_dict;

    CMD _current_cmd = CMD::ERROR_CMD, _last_cmd = CMD::ERROR_CMD;
    std::vector<double> _parameters;

private:
    void init();

protected:
    void mousePressEvent(QMouseEvent *event);

    void mouseMoveEvent(QMouseEvent *event);

    bool eventFilter(QObject *target, QEvent *event);

public:
    CMDWidget(QWidget *parent);
    ~CMDWidget();

    void clear();

    CMD cmd() const;

    bool empty() const;

    std::vector<double> &parameters();

    const std::vector<double> &parameters() const;


    bool work();

    bool get_cmd();

};