#include "ui/ActionGroup.hpp"


ActionGroup::ActionGroup(Ui::MainWindow *ui_, std::function<void(const MenuType, const int)> callback)
    : ui(ui_), _callback(callback)
{
    init();
}

void ActionGroup::init()
{
    // init_line_menu();
    init_circle_menu();
    init_curve_menu();
    init_fillet_menu();
}

void ActionGroup::init_line_menu()
{
    _line_menu = new QMenu(ui->line_btn);
    _line_menu->connect(_line_menu, &QMenu::triggered, [this](QAction *action)
        { _callback(MenuType::LineMenu, _line_menu->actions().indexOf(action)); });
    ui->line_btn->setMenu(_line_menu);
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

    QAction *chamfer = new QAction(QIcon(":/icons/chamfer_btn.png"),
        "Chamfer", ui->fillet_btn);
    _fillet_menu->addAction(chamfer);
}