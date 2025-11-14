#pragma once
#include <functional>
#include <QMenu>
#include <QActionGroup>
#include "./ui_MainWindow.h"


class ActionGroup
{
public:
    enum class MenuType {PolygonMenu, ArcMenu, CircleMenu, EllipseMenu, CurveMenu, FilletMenu};

private:
    Ui::MainWindow *ui = nullptr;
    std::function<void(const MenuType, const int)> _callback;
    QMenu *_polygon_menu = nullptr;
    QMenu *_arc_menu = nullptr;
    QMenu *_circle_menu = nullptr;
    QMenu *_ellipse_menu = nullptr;
    QMenu *_curve_menu = nullptr;
    QMenu *_fillet_menu = nullptr;

public:
    ActionGroup(Ui::MainWindow *ui, std::function<void(const MenuType, const int)> callback);

private:
    void init();

    void init_polygon_menu();

    void init_arc_menu();

    void init_circle_menu();

    void init_ellipse_menu();

    void init_curve_menu();

    void init_fillet_menu();
};