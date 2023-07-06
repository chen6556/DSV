#pragma once
#include "draw/Container.hpp"


class StructureMover
{
private:



public:
	StructureMover();
	~StructureMover();

	static void move(Container *container, const double x, const double y);
};

