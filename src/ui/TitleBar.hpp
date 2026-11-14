#pragma once

#include <QWidget>
#include <QPointF>
#include <QMouseEvent>


QT_BEGIN_NAMESPACE
namespace Ui
{
class TitleBar;
}
QT_END_NAMESPACE

class TitleBar : public QWidget
{
    Q_OBJECT

public:
    TitleBar(QWidget *parent = nullptr);

    ~TitleBar() override;

    void bind_window(QWidget *window);

    void update_icon();

protected:
    void mousePressEvent(QMouseEvent *event) override;

    void mouseMoveEvent(QMouseEvent *event) override;

    void mouseReleaseEvent(QMouseEvent *event) override;

    void mouseDoubleClickEvent(QMouseEvent *event) override;

private slots:
    void min_window();

    void max_normal_window();

    void close_window();

private:
    Ui::TitleBar *ui = nullptr;
    QWidget *_window = nullptr;
    bool _mouse_left_button_pressed = false;
    bool _has_handle = false;
    QPointF _start_pos;
};