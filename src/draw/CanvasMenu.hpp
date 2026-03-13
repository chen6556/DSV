#pragma once

#include <QMenu>
#include <QAction>
#include "ui/PropertyWidget.hpp"


class Canvas;

class CanvasMenu
{
private:
    Canvas *_canvas = nullptr;
    PropertyWidget *_dialog = nullptr;
    QMenu *_menu = nullptr; 
    QAction *_up = nullptr;
    QAction *_down = nullptr;
    QAction *_property = nullptr;
    QAction *_text_to_polylines = nullptr;
    QAction *_bezier_to_bspline = nullptr;
    QAction *_bspline_to_bezier = nullptr;
    QAction *_change_bspline_model = nullptr;

public:
    CanvasMenu(Canvas *parent);

    ~CanvasMenu();

    void init(Canvas *parent);

    void exec(Geo::Geometry *object);
};