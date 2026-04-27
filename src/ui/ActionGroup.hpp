#pragma once
#include <functional>
#include <QMenu>
#include <QActionGroup>
#include "./ui_MainWindow.h"


class ActionGroup
{
public:
    enum class MenuType
    {
        PolygonMenu,
        ArcMenu,
        CircleMenu,
        EllipseMenu,
        CurveMenu,
        FilletMenu,
        ConnectMenu,
        TrimMenu,
        FlipMenu,
        MirrorMenu,
        DividePointsMenu,
        DividePartsMenu,
        AlignedDimensionMenu,
        AngleDimensionMenu
    };

private:
    Ui::MainWindow *ui = nullptr;
    std::function<void(const MenuType, const int)> _callback;
    QMenu *_polygon_menu = nullptr;
    QMenu *_arc_menu = nullptr;
    QMenu *_circle_menu = nullptr;
    QMenu *_ellipse_menu = nullptr;
    QMenu *_curve_menu = nullptr;
    QMenu *_connect_menu = nullptr;
    QMenu *_trim_menu = nullptr;
    QMenu *_flip_menu = nullptr;
    QMenu *_fillet_menu = nullptr;
    QMenu *_mirror_menu = nullptr;
    QMenu *_divide_points_menu = nullptr;
    QMenu *_divide_parts_menu = nullptr;
    QMenu *_length_dimension_menu = nullptr;
    QMenu *_angle_dimension_menu = nullptr;

public:
    ActionGroup(Ui::MainWindow *ui, std::function<void(const MenuType, const int)> callback);

private:
    void init();

    void init_polygon_menu();

    void init_arc_menu();

    void init_circle_menu();

    void init_ellipse_menu();

    void init_curve_menu();

    void init_connect_menu();

    void init_trim_menu();

    void init_flip_menu();

    void init_fillet_menu();

    void init_mirror_menu();

    void init_divide_points_menu();

    void init_divide_parts_menu();

    void init_length_dimension_menu();

    void init_angle_dimension_menu();
};