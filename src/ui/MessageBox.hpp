#pragma once
#include <QDialog>
#include <QDialogButtonBox>


QT_BEGIN_NAMESPACE
namespace Ui { class MessageBox;}
QT_END_NAMESPACE

class MessageBox : public QDialog
{
    Q_OBJECT

private:
    Ui::MessageBox *ui = nullptr;
    QDialogButtonBox::StandardButton _result = QDialogButtonBox::StandardButton::NoButton;

public:
    MessageBox(const QDialogButtonBox::StandardButtons buttons, QWidget *parent = nullptr);

    MessageBox(const QString &title, const QString &text, const QDialogButtonBox::StandardButtons buttons, QWidget *parent = nullptr);

    QDialogButtonBox::StandardButton result() const;

    static QDialogButtonBox::StandardButton question(QWidget *parent, const QString &title, const QString &text,
        const QDialogButtonBox::StandardButtons buttons = QDialogButtonBox::StandardButton::Yes | QDialogButtonBox::StandardButton::No);

    static QDialogButtonBox::StandardButton information(QWidget *parent, const QString &title, const QString &text,
        const QDialogButtonBox::StandardButtons buttons = QDialogButtonBox::StandardButton::Ok | QDialogButtonBox::StandardButton::Cancel);

    static QDialogButtonBox::StandardButton attention(QWidget *parent, const QString &title, const QString &text,
        const QDialogButtonBox::StandardButtons buttons = QDialogButtonBox::StandardButton::Ok | QDialogButtonBox::StandardButton::Cancel);

private slots:
    void button_clicked(QAbstractButton *button);
};