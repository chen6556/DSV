#pragma once

#include <list>
#include <tuple>
#include <vector>
#include <string>

#include "draw/Graph.hpp"


namespace UndoStack
{
    class Command
    {
    public:
        virtual ~Command() {};

        virtual void undo(Graph *graph = nullptr) = 0;
    };


    class CommandStack
    {
    private:
        std::list<Command *> _commands;
        size_t _count = 3;

        Graph *_graph = nullptr;

    public:
        void set_count(const size_t count);

        void set_graph(Graph *graph);

        void push_command(Command *command);

        void clear();

        void undo();
    };


    class ObjectCommand : public Command
    {
    private:
        // object, group index, object index
        std::vector<std::tuple<Geo::Geometry *, size_t, size_t>> _add_items;
        std::vector<std::tuple<Geo::Geometry *, size_t, size_t>> _remove_items;

    public:
        ObjectCommand(const std::vector<std::tuple<Geo::Geometry *, size_t, size_t>> &items, const bool add);

        ObjectCommand(Geo::Geometry *object, const size_t group, const size_t index, const bool add);

        ObjectCommand(const std::vector<std::tuple<Geo::Geometry *, size_t, size_t>> &add_items,
            const std::vector<std::tuple<Geo::Geometry *, size_t, size_t>> &remove_items);

        ~ObjectCommand();

        void undo(Graph *graph = nullptr) override;
    };


    class TranslateCommand : public Command
    {
    private:
        std::vector<Geo::Geometry *> _items;
        double _dx = 0, _dy = 0;

    public:
        TranslateCommand(const std::vector<Geo::Geometry *> &objects, const double x, const double y);

        TranslateCommand(Geo::Geometry *object, const double x, const double y);

        void undo(Graph *graph = nullptr) override;
    };


    class TransformCommand : public Command
    {
    private:
        std::vector<Geo::Geometry *> _items;
        double _invmat[6];

    public:
        TransformCommand(const std::vector<Geo::Geometry *> &objects, const double mat[6]);

        TransformCommand(Geo::Geometry *object, const double mat[6]);

        void undo(Graph *graph = nullptr) override;
    };


    class ChangeShapeCommand : public Command
    {
    private:
        std::vector<std::tuple<double, double>> _shape;
        Geo::Geometry *_object;

    public:
        ChangeShapeCommand(Geo::Geometry *object, const std::vector<std::tuple<double, double>> &shape);

        void undo(Graph *graph = nullptr) override;
    };


    class RotateCommand : public Command
    {
    private:
        std::vector<Geo::Geometry *> _items;
        double _x, _y, _rad;
        bool _unitary;

    public:
        RotateCommand(const std::vector<Geo::Geometry *> &objects, const double x, const double y, const double rad, const bool unitary);

        RotateCommand(Geo::Geometry *object, const double x, const double y, const double rad);

        void undo(Graph *graph = nullptr) override;
    };


    class ScaleCommand : public Command
    {
    private:
        std::vector<Geo::Geometry *> _items;
        double _x, _y, _k;
        bool _unitary;

    public:
        ScaleCommand(const std::vector<Geo::Geometry *> &objects, const double x, const double y, const double k, const bool unitary);

        ScaleCommand(Geo::Geometry *object, const double x, const double y, const double k);

        void undo(Graph *graph = nullptr) override;
    };


    class CombinateCommand : public Command
    {
    public:
        // combination, object index, objects in combination
        std::vector<std::tuple<Combination *, size_t, std::vector<Geo::Geometry *>>> _items;
        Combination *_combination = nullptr;
        size_t _group_index = 0;

    public:
        CombinateCommand(const std::vector<std::tuple<Combination *, size_t>> &combinations, const size_t index);

        CombinateCommand(Combination *combination, const std::vector<std::tuple<Combination *, size_t, std::vector<Geo::Geometry *>>> &items, const size_t index);

        ~CombinateCommand();

        void undo(Graph *graph = nullptr) override; 
    };


    class FlipCommand : public Command
    {
    private:
        std::vector<Geo::Geometry *> _items;
        double _x, _y;
        bool _direction;
        bool _unitary;

    public:
        FlipCommand(const std::vector<Geo::Geometry *> &objects, const double x, const double y, const bool direction, const bool unitary);

        FlipCommand(Geo::Geometry *object, const double x, const double y, const bool direction);

        void undo(Graph *graph = nullptr) override;
    };


    class ConnectCommand : public Command
    {
    private:
        std::vector<std::tuple<Geo::Geometry *, size_t>> _items;
        const Geo::Polyline *_polyline = nullptr; 
        size_t _group_index = 0;

    public:
        ConnectCommand(const std::vector<std::tuple<Geo::Geometry *, size_t>> &polylines, const Geo::Polyline *polyline, const size_t index);

        ~ConnectCommand();

        void undo(Graph *graph = nullptr) override;
    };


    class GroupCommand : public Command
    {
    private:
        size_t _index = 0;
        bool _add = true;
        ContainerGroup _group;

    public:
        GroupCommand(const size_t index, const bool add);

        GroupCommand(const size_t index, const bool add, ContainerGroup &group);

        void undo(Graph *graph = nullptr) override;
    };


    class ReorderGroupCommand : public Command
    {
    private:
        size_t _from = 0, _to = 0;

    public:
        ReorderGroupCommand(const size_t from, const size_t to);

        void undo(Graph *graph = nullptr) override;
    };


    class RenameGroupCommand : public Command
    {
    private:
        size_t _index;
        QString _old_name;

    public:
        RenameGroupCommand(const size_t index, const QString old_name);

        void undo(Graph *graph = nullptr);
    };


    class TextChangedCommand : public Command
    {
    private:
        QString _text;
        Geo::Geometry *_item = nullptr;

    public:
        TextChangedCommand(Geo::Geometry *item, const QString &text);

        void undo(Graph *graph = nullptr) override;
    };
}