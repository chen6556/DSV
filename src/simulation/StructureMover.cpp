#include "simulation/StructureMover.hpp"


StructureMover::StructureMover()
{

}

StructureMover::~StructureMover()
{
}

void StructureMover::move(Container *container, const double x, const double y)
{
    container->translate(x, y);
    std::vector<Geo::Geometry *> *related = &container->related();
    Container *cur_container = container;
    bool flag = true;
    while (flag)
    {
        flag = false;
        for (size_t i = 0, count = related->size(); i < count; ++i)
        {
            if (dynamic_cast<Link *>((*related)[i]) != nullptr && reinterpret_cast<Link *>((*related)[i])->tail() == cur_container
                && dynamic_cast<Container *>(reinterpret_cast<Link *>((*related)[i])->head()) != nullptr)
            {
                cur_container = reinterpret_cast<Container *>(reinterpret_cast<Link *>((*related)[i])->head());
                cur_container->translate(x, y);
                flag = true;
                related = &cur_container->related();
                break;
            }
        }
    }
}