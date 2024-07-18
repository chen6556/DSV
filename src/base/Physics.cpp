#include "base/Physics.hpp"


Physics::PhysicalObject::PhysicalObject()
{
}

Physics::PhysicalObject::PhysicalObject(const double x, const double y)
    : x_position(x), y_position(y)
{
}

Physics::PhysicalObject::PhysicalObject(const PhysicalObject &object)
    : mass(object.mass), x_position(object.x_position), y_position(object.y_position),
    x_velocity(object.x_velocity),  y_velocity(object.y_velocity),
    x_acceleration(object.x_acceleration), y_acceleration(object.y_acceleration),
    angular_velocity(object.angular_velocity), rotation(object.rotation)
{
}

Physics::PhysicalObject &Physics::PhysicalObject::operator=(const PhysicalObject &object)
{
    if (this != &object)
    {
        this->mass = object.mass;
        this->x_position = object.x_position;
        this->y_position = object.y_position;
        this->x_acceleration = object.x_acceleration;
        this->y_acceleration = object.y_acceleration;
        this->rotation = object.rotation;
        this->angular_velocity = object.angular_velocity;
    }
    return *this;
}

void Physics::PhysicalObject::add_force(const double x, const double y)
{
    x_acceleration += (x / mass);
    y_acceleration += (y / mass);
}

void Physics::PhysicalObject::update()
{
    x_velocity += x_acceleration;
    y_velocity += y_acceleration;
    x_position += x_velocity;
    y_position += y_velocity;
    rotation += angular_velocity;
}