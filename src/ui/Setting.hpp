#pragma once

#include <QDialog>


QT_BEGIN_NAMESPACE
namespace Ui
{
class Setting;
}
QT_END_NAMESPACE

class Setting : public QDialog
{
private:
    Ui::Setting *ui = nullptr;
    int _text_size = 24;
    double _down_sampling_value = 0.02;

private:
    void init();

public slots:
    void accept() override;

public:
    Setting(QWidget *parent);
    ~Setting() override;

    void show();
    int exec() override;

    bool update_text_vbo() const;

    bool update_curve_vbo() const;
};