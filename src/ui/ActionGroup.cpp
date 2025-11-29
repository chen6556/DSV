#include "ui/ActionGroup.hpp"


ActionGroup::ActionGroup(Ui::MainWindow *ui_, std::function<void(const MenuType, const int)> callback)
    : ui(ui_), _callback(callback)
{
    init();
}

void ActionGroup::init()
{
    init_polygon_menu();
    init_arc_menu();
    init_circle_menu();
    init_ellipse_menu();
    init_curve_menu();
    init_fillet_menu();
}

void ActionGroup::init_polygon_menu()
{
    _polygon_menu = new QMenu(ui->polyline_btn);
    _polygon_menu->connect(_polygon_menu, &QMenu::triggered, [this](QAction *action)
        {
            _callback(MenuType::PolygonMenu, _polygon_menu->actions().indexOf(action));
            ui->polyline_btn->setIcon(action->icon());
            ui->polyline_btn->setToolTip(action->text());
            ui->polyline_btn->setDefaultAction(action);
        });
    ui->polyline_btn->setMenu(_polygon_menu);

    QAction *polyline = new QAction(QIcon(":/icons/polygon/polyline_btn.png"),
        "Polyline", ui->polyline_btn);
    _polygon_menu->addAction(polyline);
    ui->polyline_btn->setDefaultAction(polyline);

    QAction *rectangle = new QAction(QIcon(":/icons/polygon/rectangle_btn.png"),
        "Rectangle", ui->polyline_btn);
    _polygon_menu->addAction(rectangle);

    QAction *circumscribed = new QAction(QIcon(":/icons/polygon/"
        "polygon_circumscribed_btn.png"), "Polygon Circumscribed", ui->polyline_btn);
    _polygon_menu->addAction(circumscribed);

    QAction *inscribed = new QAction(QIcon(":/icons/polygon/"
        "polygon_inscribed_btn.png"), "Polygon Inscribed", ui->polyline_btn);
    _polygon_menu->addAction(inscribed);
}

void ActionGroup::init_arc_menu()
{
    _arc_menu = new QMenu(ui->arc_btn);
    _arc_menu->connect(_arc_menu, &QMenu::triggered, [this](QAction *action)
        {
            _callback(MenuType::ArcMenu, _arc_menu->actions().indexOf(action));
            ui->arc_btn->setIcon(action->icon());
            ui->arc_btn->setToolTip(action->text());
            ui->arc_btn->setDefaultAction(action);
        });
    ui->arc_btn->setMenu(_arc_menu);

    QAction *arc_3p = new QAction(QIcon(":/icons/arc/3p_arc_btn.png"),
        "3-Point Arc", ui->arc_btn);
    _arc_menu->addAction(arc_3p);
    ui->arc_btn->setDefaultAction(arc_3p);

    QAction *start_center_angle_arc = new QAction(QIcon(":/icons/arc/sca_arc_btn.png"),
        "Start-Center-Angle Arc", ui->arc_btn);
    _arc_menu->addAction(start_center_angle_arc);

    QAction *start_end_angle_arc = new QAction(QIcon(":/icons/arc/sea_arc_btn.png"),
        "Start-End-Angle Arc", ui->arc_btn);
    _arc_menu->addAction(start_end_angle_arc);
    
    QAction *start_end_radius_arc = new QAction(QIcon(":/icons/arc/ser_arc_btn.png"),
        "Start-End-Radius Arc", ui->arc_btn);
    _arc_menu->addAction(start_end_radius_arc);
}

void ActionGroup::init_circle_menu()
{
    _circle_menu = new QMenu(ui->circle_btn);
    _circle_menu->connect(_circle_menu, &QMenu::triggered, [this](QAction *action)
        {
            _callback(MenuType::CircleMenu, _circle_menu->actions().indexOf(action));
            ui->circle_btn->setIcon(action->icon());
            ui->circle_btn->setToolTip(action->text());
            ui->circle_btn->setDefaultAction(action);
        });
    ui->circle_btn->setMenu(_circle_menu);

    QAction *circle_r = new QAction(QIcon(":/icons/circle/circle_btn.png"),
        "Center-Radius", ui->circle_btn);
    _circle_menu->addAction(circle_r);
    ui->circle_btn->setDefaultAction(circle_r);

    QAction *circle_2p = new QAction(QIcon(":/icons/circle/2p_circle_btn.png"),
        "2-Point", ui->circle_btn);
    _circle_menu->addAction(circle_2p);

    QAction *circle_3p = new QAction(QIcon(":/icons/circle/3p_circle_btn.png"),
        "3-Point", ui->circle_btn);
    _circle_menu->addAction(circle_3p);
}

void ActionGroup::init_ellipse_menu()
{
    _ellipse_menu = new QMenu(ui->ellipse_btn);
    _ellipse_menu->connect(_ellipse_menu, &QMenu::triggered, [this](QAction *action)
        {
            _callback(MenuType::EllipseMenu, _ellipse_menu->actions().indexOf(action));
            ui->ellipse_btn->setIcon(action->icon());
            ui->ellipse_btn->setToolTip(action->text());
            ui->ellipse_btn->setDefaultAction(action);
        });
    ui->ellipse_btn->setMenu(_ellipse_menu);

    QAction *ellipse = new QAction(QIcon(":/icons/ellipse/ellipse_btn.png"),
        "Ellipse", ui->ellipse_btn);
    _ellipse_menu->addAction(ellipse);
    ui->ellipse_btn->setDefaultAction(ellipse);

    QAction *arc = new QAction(QIcon(":/icons/ellipse/ellipse_arc_btn.png"),
        "Ellipse Arc", ui->ellipse_btn);
    _ellipse_menu->addAction(arc);
}

void ActionGroup::init_curve_menu()
{
    _curve_menu = new QMenu(ui->curve_btn);
    _curve_menu->connect(_curve_menu, &QMenu::triggered, [this](QAction *action)
        {
            _callback(MenuType::CurveMenu, _curve_menu->actions().indexOf(action));
            ui->curve_btn->setIcon(action->icon());
            ui->curve_btn->setToolTip(action->text());
            ui->curve_btn->setDefaultAction(action);
        });
    ui->curve_btn->setMenu(_curve_menu);

    QAction *bspline = new QAction(QIcon(":/icons/curve_btn.png"),
        "BSpline", ui->curve_btn);
    _curve_menu->addAction(bspline);
    ui->curve_btn->setDefaultAction(bspline);

    QAction *bezier = new QAction(QIcon(":/icons/bezier_btn.png"),
        "Bezier", ui->curve_btn);
    _curve_menu->addAction(bezier);
}

void ActionGroup::init_fillet_menu()
{
    _fillet_menu = new QMenu(ui->fillet_btn);
    _fillet_menu->connect(_fillet_menu, &QMenu::triggered, [this](QAction *action)
        {
            _callback(MenuType::FilletMenu, _fillet_menu->actions().indexOf(action));
            ui->fillet_btn->setIcon(action->icon());
            ui->fillet_btn->setToolTip(action->text());
            ui->fillet_btn->setDefaultAction(action);
        });
    ui->fillet_btn->setMenu(_fillet_menu);

    QAction *fillet = new QAction(QIcon(":/icons/fillet_btn.png"),
        "Fillet", ui->fillet_btn);
    _fillet_menu->addAction(fillet);
    ui->fillet_btn->setDefaultAction(fillet);

    QAction *chamfer = new QAction(QIcon(":/icons/chamfer_btn.png"),
        "Chamfer", ui->fillet_btn);
    _fillet_menu->addAction(chamfer);
}