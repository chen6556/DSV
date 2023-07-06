#pragma once
#include "base/Structure.hpp"
#include "draw/Canvas.hpp"
#include "draw/Container.hpp"
#include <QString>


// 1: LinkedList 2:BinaryTree 3:Graph

class StructureBuilder
{
private:
    Graph *_graph;
    Canvas *_canvas;    

private:
    void preoder_store_binarytree(const BinaryTreeNode<QString>* node, CircleContainer *container);

public:
    StructureBuilder(Graph *graph, Canvas *canvas);

    Graph* graph();




    LinkedList<QString>* build_linkedlist(Container *container);

    void store_linkedlist(const LinkedList<QString> &list);







    BinaryTreeNode<QString>* build_binarytree(CircleContainer *container);

    void store_binarytree(const BinaryTreeNode<QString> *tree);

};
