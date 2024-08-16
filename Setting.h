#ifndef SETTING_H
#define SETTING_H

#include <QDialog>


QT_BEGIN_NAMESPACE
namespace Ui { class Setting;}
QT_END_NAMESPACE

class Setting : public QDialog
{
    Q_OBJECT
public:
    explicit Setting(QWidget *parent = nullptr);
    ~Setting();

    void show();
    int exec();

public slots:
    void accept();

private:
    void init();

private:
    Ui::Setting *ui = nullptr;
};

#endif // SETTING_H
