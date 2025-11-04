#pragma once
#include <functional>
#include <QMenu>
#include <QActionGroup>
#include "./ui_MainWindow.h"


class ActionGroup
{
public:
    enum class MenuType {LineMenu, CircleMenu, CurveMenu, FilletMenu};

private:
    Ui::MainWindow *ui = nullptr;
    std::function<void(const MenuType, const int)> _callback;
    QMenu *_line_menu = nullptr;
    QMenu *_circle_menu = nullptr;
    QMenu *_curve_menu = nullptr;
    QMenu *_fillet_menu = nullptr;

public:
    ActionGroup(Ui::MainWindow *ui, std::function<void(const MenuType, const int)> callback);

private:
    void init();

    void init_line_menu();

    void init_circle_menu();

    void init_curve_menu();

    void init_fillet_menu();
};