#include "draw/Canvas.hpp"
#include "CanvasMenu.hpp"


CanvasMenu::CanvasMenu(Canvas *parent) : _canvas(parent), _dialog(new PropertyWidget(parent))
{
    init(parent);
}

CanvasMenu::~CanvasMenu()
{
    delete _dialog;
    delete _menu;
    delete _up;
    delete _down;
    delete _property;
    delete _text_to_polylines;
    delete _bspline_to_bezier;
    delete _bezier_to_bspline;
    delete _change_bspline_model;
}

void CanvasMenu::init(Canvas *parent)
{
    _menu = new QMenu(parent);

    _up = new QAction("Up");
    _down = new QAction("Down");
    _property = new QAction("Property");
    _text_to_polylines = new QAction("To Polylines");
    _bezier_to_bspline = new QAction("To BSpline");
    _bspline_to_bezier = new QAction("To Bezier");
    _change_bspline_model = new QAction("Show Controls");

    _menu->addAction(_up);
    _menu->addAction(_down);
    _menu->addAction(_property);
    _menu->addAction(_text_to_polylines);
    _menu->addAction(_bezier_to_bspline);
    _menu->addAction(_bspline_to_bezier);
    _menu->addAction(_change_bspline_model);
}

void CanvasMenu::exec(Geo::Geometry *object)
{
    _text_to_polylines->setVisible(dynamic_cast<Text *>(object) != nullptr);
    _bezier_to_bspline->setVisible(dynamic_cast<Geo::CubicBezier *>(object) != nullptr);
    _bspline_to_bezier->setVisible(dynamic_cast<Geo::BSpline *>(object) != nullptr);
    if (const Geo::BSpline *bspline = dynamic_cast<const Geo::BSpline *>(object))
    {
        _change_bspline_model->setVisible(true);
        _change_bspline_model->setText(bspline->controls_model ? "Show Path" : "Show Controls");
    }
    else
    {
        _change_bspline_model->setVisible(false);
    }

    if (const QAction *a = _menu->exec(QCursor::pos()); a == _up)
    {
        _canvas->editor().up(object);
        _canvas->refresh_vbo(true, object->type());
        _canvas->refresh_selected_ibo();
    }
    else if (a == _down)
    {
        _canvas->editor().down(object);
        _canvas->refresh_vbo(true, object->type());
        _canvas->refresh_selected_ibo();
    }
    else if (a == _property)
    {
        _dialog->show(object);
    }
    else if (a == _text_to_polylines)
    {
        _canvas->editor().text_to_polylines(dynamic_cast<Text *>(object));
        _canvas->refresh_vbo(true, {Geo::Type::TEXT, Geo::Type::POLYLINE});
        _canvas->refresh_selected_ibo();
    }
    else if (a == _bezier_to_bspline)
    {
        _canvas->editor().bezier_to_bspline(dynamic_cast<Geo::CubicBezier *>(object));
        CanvasOperations::CanvasOperation::tool_lines.clear();
        _canvas->refresh_vbo(true, {Geo::Type::BEZIER, Geo::Type::BSPLINE});
        _canvas->refresh_selected_ibo();
    }
    else if (a == _change_bspline_model)
    {
        static_cast<Geo::BSpline *>(object)->controls_model = !static_cast<Geo::BSpline *>(object)->controls_model;
        if (object == CanvasOperations::CanvasOperation::clicked_object)
        {
            CanvasOperations::CanvasOperation::refresh_tool_lines(object);
        }
    }
    else if (a == _bspline_to_bezier)
    {
        _canvas->editor().bspline_to_bezier(dynamic_cast<Geo::BSpline *>(object));
        CanvasOperations::CanvasOperation::tool_lines.clear();
        _canvas->refresh_vbo(true, {Geo::Type::BEZIER, Geo::Type::BSPLINE});
        _canvas->refresh_selected_ibo();
    }
}