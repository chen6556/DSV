#include <algorithm>
#include "base/Physics.hpp"
#include "base/Algorithm.hpp"


Physics::PhysicalObject::PhysicalObject()
{
}

Physics::PhysicalObject::PhysicalObject(const double x, const double y)
    : position(x, y)
{
}

Physics::PhysicalObject::PhysicalObject(const PhysicalObject &object)
    : _mass(object._mass), _inv_mass(object._inv_mass),
    restitution(object.restitution),
    _inertia(object._inertia), _inv_inertia(object._inv_inertia), 
    position(object.position), velocity(object.velocity),
    impulse(object.impulse), force(object.force),
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
        this->restitution = object.restitution;
        this->position = object.position;
        this->impulse = object.impulse;
        this->force = object.force;
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

void Physics::PhysicalObject::add_impulse(const double x, const double y, const double rx, const double ry)
{
    if (!this->is_static)
    {
        this->velocity += (x * _inv_mass);
        this->angular_velocity += (_inv_inertia * (rx * y - ry * x));
    }
}

void Physics::PhysicalObject::add_impulse(const Physics::Vector impulse, const Physics::Vector r)
{
    if (!this->is_static)
    {
        this->velocity += (impulse * _inv_mass);
        this->angular_velocity += (_inv_inertia * r.cross(impulse));
    }
}

void Physics::PhysicalObject::update_velocity(const double dt)
{
}

void Physics::PhysicalObject::update_position(const double dt)
{
    this->last_position = this->position;
    if (!this->is_static)
    {
        this->position += (this->velocity * dt);
        this->rotation += (this->angular_velocity * dt);
    }
    this->force.clear();
    this->torques = 0;
}


void Physics::solve_velocity(std::vector<Physics::CollisionPair> &collisions)
{
    Physics::Vector velocity0, velocity1;
    for (Physics::CollisionPair &collision : collisions)
    {
        collision.v[0] = collision.object0->velocity + Physics::Vector::cross(collision.object0->angular_velocity, collision.r[0]);
        collision.v[1] = collision.object1->velocity + Physics::Vector::cross(collision.object1->angular_velocity, collision.r[1]);
        Physics::Vector dv = collision.v[0] - collision.v[1];

        const double jvt = collision.tangent.dot(dv);
        double lambda_t = collision.effective_mass_t * (-jvt);

        double max_friction = collision.friction * collision.accumulated_impulse_n;
        double new_impulse = std::clamp(collision.accumulated_impulse_t + lambda_t, -max_friction, max_friction);
        lambda_t = new_impulse - collision.accumulated_impulse_t;
        collision.accumulated_impulse_t = new_impulse;

        const Physics::Vector impulse_t = collision.tangent * lambda_t;
        collision.object0->add_impulse(impulse_t, collision.r[0]);
        collision.object1->add_impulse(-impulse_t, collision.r[1]);

        collision.v[0] = collision.object0->velocity + Physics::Vector::cross(collision.object0->angular_velocity, collision.r[0]);
        collision.v[1] = collision.object1->velocity + Physics::Vector::cross(collision.object1->angular_velocity, collision.r[1]);
        dv = collision.v[0] - collision.v[1];

        const double jvn = collision.normal.dot(dv - collision.velocity_bias);
        double lambda_n = collision.effective_mass_n * (-jvn);
        double old_normal_impulse = collision.accumulated_impulse_n;
        collision.accumulated_impulse_n = std::max(old_normal_impulse + lambda_n, 0.0);
        lambda_n = collision.accumulated_impulse_n - old_normal_impulse;

        const Physics::Vector impulse_n = collision.normal * lambda_n;
        collision.object0->add_impulse(impulse_n, collision.r[0]);
        collision.object1->add_impulse(-impulse_n, collision.r[1]);
    }
}

void Physics::solve_position(std::vector<Physics::CollisionPair> &collisions)
{
    for (Physics::CollisionPair &collision : collisions)
    {
        Physics::Vector vec = collision.point[1] - collision.point[0];

        const double bias = std::max(0.2 * (vec.dot(collision.normal) - 0.005), 0.0);
        Physics::Vector impulse = collision.normal * collision.effective_mass_n * bias;

        collision.object0->position += (impulse * collision.object0->inv_mass());
        collision.object0->rotation += (collision.object0->inv_inertia() * collision.r[0].cross(impulse));

        collision.object1->position -= (impulse * collision.object1->inv_mass());
        collision.object1->rotation -= (collision.object1->inv_inertia() * collision.r[1].cross(impulse));
    }
}


