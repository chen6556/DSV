#include "ui/CMDWidget.hpp"
#include "./ui_CMDWidget.h"

#include <QRegularExpressionValidator>



CMDWidget::CMDWidget(QWidget *parent)
    : QWidget(parent), _parent(parent), ui(new Ui::CMDWidget)
{
    ui->setupUi(this);

    init();
}

CMDWidget::~CMDWidget()
{

}

void CMDWidget::init()
{
    ui->cmd->setValidator(new QRegularExpressionValidator(QRegularExpression("([A-Za-z]+)|([0-9]+(.[0-9]+)?)$")));
}


void CMDWidget::mousePressEvent(QMouseEvent *event)
{
    _last_pos = event->pos();
}

void CMDWidget::mouseMoveEvent(QMouseEvent *event)
{
    move(mapToParent(event->pos()) - _last_pos);
    
    int x = pos().x(), y = pos().y(), w = width(), h = height();
    if (x < 0)
    {
        move(mapToParent(QPoint(-x, 0)));
    }
    else if (x + w > _parent->width())
    {
        move(mapToParent(QPoint(_parent->width() - x - w, 0)));
    }

    if (y < 22)
    {
        move(mapToParent(QPoint(0, 22 - y)));
    }
    else if (y + h > _parent->height() - 32)
    {
        move(mapToParent(QPoint(0, _parent->height() - 32 - y - h)));
    }
}