#include "base/Physics.hpp"


Physics::PhysicalObject::PhysicalObject()
{
}

Physics::PhysicalObject::PhysicalObject(const double x, const double y)
    : x_position(x), y_position(y)
{
}

Physics::PhysicalObject::PhysicalObject(const PhysicalObject &object)
    : _mass(object._mass), _inv_mass(object._inv_mass),
    _recover_ratio(object._recover_ratio),
    _inertia(object._inertia), _inv_inertia(object._inv_inertia), 
    x_position(object.x_position), y_position(object.y_position),
    x_velocity(object.x_velocity),  y_velocity(object.y_velocity),
    x_impulse(object.x_impulse), y_impulse(object.y_impulse),
    x_force(object.x_force), y_force(object.y_force),
    angular_velocity(object.angular_velocity), rotation(object.rotation)
{
}

Physics::PhysicalObject &Physics::PhysicalObject::operator=(const PhysicalObject &object)
{
    if (this != &object)
    {
        _mass = object._mass;
        _inv_mass = object._inv_mass;
        _inertia = object._inertia;
        _inv_inertia = object._inv_inertia;
        _recover_ratio = object._recover_ratio;
        this->x_position = object.x_position;
        this->y_position = object.y_position;
        this->x_impulse = object.x_impulse;
        this->y_impulse = object.y_impulse;
        this->x_force = object.x_force;
        this->y_force = object.y_force;
        this->rotation = object.rotation;
        this->angular_velocity = object.angular_velocity;
    }
    return *this;
}

void Physics::PhysicalObject::set_mass(const double value)
{
    _mass = value;
    if (value != 0)
    {
        _inv_mass = 1.0 / value;
    }

    calculate_inertia();
}

void Physics::PhysicalObject::set_inertia(const double value)
{
    _inertia = value;
    if (value != 0)
    {
        _inv_inertia = 1.0 / value;
    }
}

void Physics::PhysicalObject::set_recover_ratio(const double value)
{
    _recover_ratio = value;
}

double Physics::PhysicalObject::mass() const
{
    return _mass;
}

double Physics::PhysicalObject::inv_mass() const
{
    return _inv_mass;
}

double Physics::PhysicalObject::inertia() const
{
    return _inertia;
}

double Physics::PhysicalObject::inv_inertia() const
{
    return _inv_inertia;
}

double Physics::PhysicalObject::recover_ratio() const
{
    return _recover_ratio;
}

void Physics::PhysicalObject::add_impulse(const double x, const double y, const double rx, const double ry)
{
    this->x_velocity += x * _inv_mass;
    this->y_force += y * _inv_mass;
    this->angular_velocity += _inv_inertia * (rx * y - ry * x);
}

void Physics::PhysicalObject::update(const double dt)
{
    this->last_x_position = this->x_position;
    this->last_y_position = this->y_position;
    this->x_position += this->x_velocity * dt;
    this->y_position += this->y_velocity * dt;
    this->rotation += this->angular_velocity * dt;
    this->x_force = this->y_force = 0;
}