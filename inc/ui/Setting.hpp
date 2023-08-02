#pragma once
#include <QDialog>


QT_BEGIN_NAMESPACE
namespace Ui { class Setting;}
QT_END_NAMESPACE

class Setting : public QDialog
{
private:
    Ui::Setting *ui = nullptr;

private:
    void init();

public slots:
    void accept();

public:
    Setting(QWidget *parent);
    ~Setting();

    void show();
};