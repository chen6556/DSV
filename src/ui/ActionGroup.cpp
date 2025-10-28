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