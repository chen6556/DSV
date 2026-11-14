#include <QWindow>
#include "TitleBar.hpp"
#include "./ui_TitleBar.h"


TitleBar::TitleBar(QWidget *parent) : QWidget(parent), ui(new Ui::TitleBar)
{
    ui->setupUi(this);
}

TitleBar::~TitleBar()
{
    delete ui;
}

void TitleBar::bind_window(QWidget *window)
{
    _window = window;
}

void TitleBar::update_icon()
{
    ui->btn_max->setIcon(QIcon(_window->isMaximized() ?
        ":/icons/titlebar/normal_btn.png" : ":/icons/titlebar/max_btn.png"));
}

void TitleBar::mousePressEvent(QMouseEvent *event)
{
    QWidget::mousePressEvent(event);
    if (event->button() == Qt::MouseButton::LeftButton)
    {
        if (QWindow *handle = _window->windowHandle())
        {
            _has_handle = true;
            handle->startSystemMove();
        }
        else
        {
            _mouse_left_button_pressed = true;
            _start_pos = event->globalPosition() - _window->frameGeometry().topLeft();
        }
    }
}

void TitleBar::mouseMoveEvent(QMouseEvent *event)
{
    QWidget::mouseMoveEvent(event);
    if (_has_handle)
    {
        if (_window->isMaximized())
        {
            _window->showNormal();
            ui->btn_max->setIcon(QIcon(":/icons/titlebar/max_btn.png"));
        }
    }
    else if (_mouse_left_button_pressed)
    {
        _window->move((event->globalPosition() - _start_pos).toPoint());
        if (_window->isMaximized())
        {
            _window->showNormal();
            ui->btn_max->setIcon(QIcon(":/icons/titlebar/max_btn.png"));
        }
    }
}

void TitleBar::mouseReleaseEvent(QMouseEvent *event)
{
    QWidget::mouseReleaseEvent(event);
    if (event->button() == Qt::MouseButton::LeftButton)
    {
        _mouse_left_button_pressed = _has_handle = false;
    }
}

void TitleBar::mouseDoubleClickEvent(QMouseEvent *event)
{
    QWidget::mouseDoubleClickEvent(event);
    if (event->button() == Qt::MouseButton::LeftButton)
    {
        max_normal_window();
    }
}

void TitleBar::min_window()
{
    _window->showMinimized();
}

void TitleBar::max_normal_window()
{
    if (_window->isMaximized())
    {
        _window->showNormal();
        ui->btn_max->setIcon(QIcon(":/icons/titlebar/max_btn.png"));
    }
    else
    {
        _window->showMaximized();
        ui->btn_max->setIcon(QIcon(":/icons/titlebar/normal_btn.png"));
    }
    QEvent leave(QEvent::Type::Leave);
    QApplication::sendEvent(ui->btn_max, &leave);
}

void TitleBar::close_window()
{
    _window->close();
}