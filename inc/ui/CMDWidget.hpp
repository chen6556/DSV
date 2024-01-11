#pragma once

#include <QWidget>
#include <QMouseEvent>


QT_BEGIN_NAMESPACE
namespace Ui { class CMDWidget;}
QT_END_NAMESPACE


class CMDWidget : public QWidget
{
    Q_OBJECT

private:
    Ui::CMDWidget *ui = nullptr;

    QPoint _last_pos;
    QWidget *_parent = nullptr;

private:
    void init();

protected:
    void mousePressEvent(QMouseEvent *event);

    void mouseMoveEvent(QMouseEvent *event);


public:
    CMDWidget(QWidget *parent);
    ~CMDWidget();

};