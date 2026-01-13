#include <QPushButton>
#include "ui/MessageBox.hpp"
#include "./ui_MessageBox.h"
#include "ui/WinUITool.hpp"


MessageBox::MessageBox(const QDialogButtonBox::StandardButtons buttons, QWidget *parent) : QDialog(parent), ui(new Ui::MessageBox)
{
    ui->setupUi(this);
    ui->buttonBox->setStandardButtons(buttons);
#ifdef _WIN64
    WinUITool::set_caption_color(winId(), 0x3C3C3D);
#endif
}

MessageBox::MessageBox(const QString &title, const QString &text, const QDialogButtonBox::StandardButtons buttons, QWidget *parent)
    : MessageBox(buttons, parent)
{
    setWindowTitle(title);
    ui->lb_text->setText(text);
}

QDialogButtonBox::StandardButton MessageBox::result() const
{
    return _result;
}

QDialogButtonBox::StandardButton MessageBox::question(QWidget *parent, const QString &title, const QString &text,
                                                      const QDialogButtonBox::StandardButtons buttons)
{
    MessageBox dialog(title, text, buttons, parent);
    QPixmap pixmap(":/icons/help.svg");
    pixmap.scaled(dialog.ui->lb_icon->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    dialog.ui->lb_icon->setPixmap(pixmap);
    dialog.exec();
    return dialog.result();
}

QDialogButtonBox::StandardButton MessageBox::information(QWidget *parent, const QString &title, const QString &text,
                                                         const QDialogButtonBox::StandardButtons buttons)
{
    MessageBox dialog(title, text, buttons, parent);
    QPixmap pixmap(":/icons/info.svg");
    pixmap.scaled(dialog.ui->lb_icon->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    dialog.ui->lb_icon->setPixmap(pixmap);
    dialog.exec();
    return dialog.result();
}

QDialogButtonBox::StandardButton MessageBox::attention(QWidget *parent, const QString &title, const QString &text,
                                                       const QDialogButtonBox::StandardButtons buttons)
{
    MessageBox dialog(title, text, buttons, parent);
    QPixmap pixmap(":/icons/attention.svg");
    pixmap.scaled(dialog.ui->lb_icon->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    dialog.ui->lb_icon->setPixmap(pixmap);
    dialog.exec();
    return dialog.result();
}

void MessageBox::button_clicked(QAbstractButton *button)
{
    _result = ui->buttonBox->standardButton(button);
    return done(_result);
}