#include "simulation/StructureBuilder.hpp"


StructureBuilder::StructureBuilder(Graph *graph, Canvas *canvas)
    : _graph(graph), _canvas(canvas)
{
}

Graph *StructureBuilder::graph()
{
    return _graph;
}

LinkedList<QString> *StructureBuilder::build_linkedlist(Container *container)
{
    LinkedList<QString> *list = new LinkedList<QString>({container->text()});
    std::vector<Geo::Geometry *> *related = &container->related();
    Container *cur_container = container;
    bool flag = true;
    while (flag)
    {
        flag = false;
        for (size_t i = 0, count = related->size(); i < count; ++i)
        {
            if (dynamic_cast<Link *>((*related)[i]) && reinterpret_cast<Link *>((*related)[i])->tail() == cur_container && dynamic_cast<Container *>(reinterpret_cast<Link *>((*related)[i])->head()))
            {
                cur_container = reinterpret_cast<Container *>(reinterpret_cast<Link *>((*related)[i])->head());
                list->append(cur_container->text());
                flag = true;
                related = &cur_container->related();
                break;
            }
        }
    }
    cur_container = nullptr;
    return list;
}

void StructureBuilder::store_linkedlist(const LinkedList<QString> &list)
{
    const Geo::Coord coord(_canvas->mouse_position());
    int step = 0;
    Container *last_container = new Container("Head Node", Geo::Rectangle(coord.x, coord.y, coord.x + 80, coord.y + 19));
    last_container->shape_fixed() = true;
    _graph->append(last_container, _canvas->current_group());

    Container *cur_container;
    for (const QString &txt : list)
    {
        ++step;
        cur_container = new Container(txt, Geo::Rectangle(coord.x + step * 100, coord.y, coord.x + 80 + step * 100, coord.y + 19));
        _graph->append(cur_container, _canvas->current_group());
        _graph->append(new Link(Geo::Polyline(), last_container, cur_container), _canvas->current_group());
        last_container->related().push_back(_graph->container_group(_canvas->current_group()).back());
        cur_container->related().push_back(_graph->container_group(_canvas->current_group()).back());
        cur_container->shape_fixed() = true;
        last_container = cur_container;
    }
}

void StructureBuilder::preoder_store_binarytree(const BinaryTreeNode<QString> *node, CircleContainer *container)
{

    if (node != nullptr)
    {
        CircleContainer *circlecontainer;
        container->set_text(node->get_data());
        _graph->append(new CircleContainer(QString(), Geo::Circle(container->center() + Geo::Point(-14, 24), 10)));
        _graph->container_group(_canvas->current_group()).back()->memo()["is_selected"] = false;
        _graph->container_group(_canvas->current_group()).back()->memo()["is_left_node"] = true;
        circlecontainer = reinterpret_cast<CircleContainer *>(_graph->container_group(_canvas->current_group()).back());
        _graph->append(new Link(Geo::Polyline(), container, _graph->container_group(_canvas->current_group()).back()));
        _graph->container_group(_canvas->current_group()).back()->memo()["is_selected"] = false;
        preoder_store_binarytree(node->get_left(), circlecontainer);

        _graph->append(new CircleContainer(QString(), Geo::Circle(container->center() + Geo::Point(14, 24), 10)));
        _graph->container_group(_canvas->current_group()).back()->memo()["is_selected"] = false;
        _graph->container_group(_canvas->current_group()).back()->memo()["is_left_node"] = false;
        circlecontainer = reinterpret_cast<CircleContainer *>(_graph->container_group(_canvas->current_group()).back());
        _graph->append(new Link(Geo::Polyline(), container, _graph->container_group(_canvas->current_group()).back()));
        _graph->container_group(_canvas->current_group()).back()->memo()["is_selected"] = false;
        preoder_store_binarytree(node->get_right(), circlecontainer);
    }
    else
    {
        std::vector<Geo::Geometry *>::iterator link_it = _graph->container_group(_canvas->current_group()).begin();
        while (dynamic_cast<Link *>(*link_it) == nullptr ||
               reinterpret_cast<Link *>(*link_it)->head() != container)
        {
            ++link_it;
        }
        Link *link = reinterpret_cast<Link *>(*link_it);
        link->tail()->related().erase(std::find(link->tail()->related().begin(), link->tail()->related().end(), link));
        link->head()->related().erase(std::find(link->head()->related().begin(), link->head()->related().end(), link));
        _graph->container_group(_canvas->current_group()).remove(link_it);

        std::vector<Geo::Geometry *>::iterator container_it = _graph->container_group(_canvas->current_group()).begin();
        while (*container_it != container)
        {
            ++container_it;
        }
        _graph->container_group(_canvas->current_group()).remove(container_it);
    }
}

BinaryTreeNode<QString> *StructureBuilder::build_binarytree(CircleContainer *container)
{
    BinaryTreeNode<QString> *tree = new BinaryTreeNode<QString>(container->text());

    for (Geo::Geometry *geo : container->related())
    {
        if (dynamic_cast<Link *>(geo) && dynamic_cast<CircleContainer *>(reinterpret_cast<Link *>(geo)->head()) != nullptr && reinterpret_cast<CircleContainer *>(reinterpret_cast<Link *>(geo)->head()) != container)
        {
            if (reinterpret_cast<Link *>(geo)->head()->memo().at("is_left_node").to_bool() && tree->get_left() == nullptr)
            {
                tree->set_left(build_binarytree(reinterpret_cast<CircleContainer *>(reinterpret_cast<Link *>(geo)->head())));
            }
            else if (!reinterpret_cast<Link *>(geo)->head()->memo().at("is_left_node").to_bool() && tree->get_right() == nullptr)
            {
                tree->set_right(build_binarytree(reinterpret_cast<CircleContainer *>(reinterpret_cast<Link *>(geo)->head())));
            }
            if (tree->get_left() != nullptr && tree->get_right() != nullptr)
            {
                break;
            }
        }
    }

    return tree;
}

void StructureBuilder::store_binarytree(const BinaryTreeNode<QString> *tree)
{
    const Geo::Coord coord(_canvas->mouse_position());
    _graph->append(new CircleContainer(QString(), Geo::Circle(coord.x, coord.y, 10)));

    preoder_store_binarytree(tree, reinterpret_cast<CircleContainer *>(_graph->container_group(_canvas->current_group()).back()));
}




