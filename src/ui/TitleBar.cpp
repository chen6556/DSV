#include "ui/TitleBar.hpp"
#include "./ui_TitleBar.h"


TitleBar::TitleBar(QWidget *parent)
    : ui(new Ui::TitleBar),
    _parent(parent)
{
    ui->setupUi(this);
    
}

void TitleBar::max_widget()
{
    if (_maximized)
    {
        _parent->showNormal();
        ui->max_btn->setIcon(QIcon(":/icons/max_btn_0.svg"));
    }
    else
    {
        _parent->showMaximized();
        ui->max_btn->setIcon(QIcon(":/icons/max_btn_1.svg"));
    }
    _maximized = !_maximized;
}

void TitleBar::min_widget()
{
    _parent->showMinimized();
}

bool TitleBar::close_widget()
{
    _parent->close();
    return QWidget::close();
}

void TitleBar::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (Qt::LeftButton == event->button())
	{	
        max_widget();
	}

	QWidget::mouseDoubleClickEvent(event);
}

void TitleBar::mousePressEvent(QMouseEvent *event)
{
    if (Qt::LeftButton == event->button())
	{
		_pressed = true;
		_move_pos = event->globalPos();
	}

    return QWidget::mousePressEvent(event);
}

void TitleBar::mouseMoveEvent(QMouseEvent *event)
{
    if (_pressed)
	{
		QPoint target = event->globalPos();
		_parent->move(_parent->pos() + target - _move_pos);
		_move_pos = target;
        _maximized = false;
	}

    return QWidget::mouseMoveEvent(event);
}

void TitleBar::mouseReleaseEvent(QMouseEvent *event)
{
    _pressed = false;
    if (!_maximized)
    {   
        ui->max_btn->setIcon(QIcon(":/icons/max_btn_0.svg"));
    }

	QWidget::mouseReleaseEvent(event);
}