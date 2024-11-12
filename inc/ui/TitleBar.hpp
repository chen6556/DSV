#pragma once

#include <QWidget>
#include <QPoint>
#include <QMouseEvent>


QT_BEGIN_NAMESPACE
namespace Ui { class TitleBar;}
QT_END_NAMESPACE

class TitleBar : public QWidget
{
    Q_OBJECT
public:
    TitleBar(QWidget *parent);

public slots:
    void max_widget();

    void min_widget();

    bool close_widget();

protected:
	void mouseDoubleClickEvent(QMouseEvent *event) override;

	void mousePressEvent(QMouseEvent *event) override;

	void mouseMoveEvent(QMouseEvent *event) override;

	void mouseReleaseEvent(QMouseEvent *event) override;

private:
    Ui::TitleBar *ui = nullptr;
    QWidget *_parent = nullptr;

    bool _pressed = false;
    QPoint _move_pos;
};