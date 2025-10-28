#pragma once
#include <functional>
#include <QMenu>
#include <QActionGroup>
#include "./ui_MainWindow.h"


class ActionGroup
{
public:
    enum class MenuType {LineMenu, CircleMenu};

private:
    Ui::MainWindow *ui = nullptr;
    std::function<void(const MenuType, const int)> _callback;
    QMenu *_line_menu = nullptr;
    QMenu *_circle_menu = nullptr;

public:
    ActionGroup(Ui::MainWindow *ui, std::function<void(const MenuType, const int)> callback);

private:
    void init();

    void init_line_menu();

    void init_circle_menu();
};