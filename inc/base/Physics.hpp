#pragma once
#include <cassert>
#include <cmath>
#include <vector>


namespace Physics
{
    struct Vector
    {
        double x = 0;
        double y = 0;

        static Vector cross(double value, const Vector &vec)
        {
            return Vector(-value * vec.y, value * vec.x);
        }

        static Vector cross(const Vector &vec, double value)
        {
            return Vector(value * vec.y, -value * vec.x);
        }

        static void rotate(Vector &vec, const double x, const double y, const double rad)
        {
            vec.x -= x;
            vec.y -= y;
            const double x1 = vec.x, y1 = vec.y;
            vec.x = x1 * std::cos(rad) - y1 * std::sin(rad);
            vec.y = x1 * std::sin(rad) + y1 * std::cos(rad);
            vec.x += x;
            vec.y += y;
        }

        Vector() {}

        Vector(const double x_, const double y_)
            : x(x_), y(y_) {}

        Vector(const Vector &vec)
            : x(vec.x), y(vec.y) {}

        Vector &operator=(const double value)
        {
            x = y = value;
            return *this;
        }

        Vector &operator=(const Vector &vec)
        {
            x = vec.x;
            y = vec.y;
            return *this;
        }

        bool operator==(const double value) const
        {
            return x == value && y == value;
        }

        bool operator==(const Vector &vec) const
        {
            return x == vec.x && y == vec.y;
        }

        bool operator!=(const double value) const
        {
            return x != value || y != value;
        }

        bool operator!=(const Vector &vec) const
        {
            return x != vec.x || y != vec.y;
        }

        Vector operator+(const double value) const
        {
            return Vector(x + value, y + value);
        }

        Vector operator+(const Vector &vec) const
        {
            return Vector(x + vec.x, y + vec.y);
        }

        void operator+=(const double value)
        {
            x += value;
            y += value;
        }

        void operator+=(const Vector &vec)
        {
            x += vec.x;
            y += vec.y;
        }

        Vector operator-(const double value) const
        {
            return Vector(x - value, y - value);
        }

        Vector operator-(const Vector &vec) const
        {
            return Vector(x - vec.x, y - vec.y);
        }

        void operator-=(const double value)
        {
            x -= value;
            y -= value;
        }

        void operator-=(const Vector &vec)
        {
            x -= vec.x;
            y -= vec.y;
        }
    
        Vector operator*(const double value) const
        {
            return Vector(x * value, y * value);
        }

        void operator*=(const double value)
        {
            x *= value;
            y *= value;
        }

        Vector operator/(const double value) const
        {
            assert(value != 0);
            return Vector(x / value, y / value);
        }

        void operator/=(const double value)
        {
            assert(value != 0);
            x /= value;
            y /= value;
        }

        double dot(const double x_, const double y_) const
        {
            return x * x_ + y * y_;   
        }

        double dot(const Vector &vec) const
        {
            return x * vec.x + y * vec.y;
        }

        double cross(const double x_, const double y_) const
        {
            return x * y_ - y * x_;
        }

        double cross(const Vector &vec) const
        {
            return x * vec.y - y * vec.x;
        }

        double length() const
        {
            return std::sqrt(x * x + y * y);
        }

        void clear()
        {
            x = y = 0;
        }
    
        Vector &normalize()
        {
            const double len = std::sqrt(x * x + y * y);
            x /= len;
            y /= len;
            return *this;
        }

        Vector normalized() const
        {
            const double len = std::sqrt(x * x + y * y);
            return Vector(x / len, y / len);
        }

        Vector operator-() const
        {
            return Vector(-x, -y);
        }

        // friend Vector operator*(const double value, const Vector &vec)
        // {
        //     return Vector(vec.x * value, vec.y * value);
        // }
    };

    class PhysicalObject
    {
    protected:
        double _mass = 0;
        double _inv_mass = 0;
        double _inertia = 0;
        double _inv_inertia = 0;

    public:
        Vector position;
        Vector last_position;
        Vector velocity;
        Vector impulse;
        Vector force;

        double angular_velocity = 0;
        double rotation = 0;
        double torques = 0;

        double friction = 0.1;
        double restitution = 1.0;

        bool is_static = true;

    public:
        PhysicalObject();

        PhysicalObject(const double x, const double y);

        PhysicalObject(const PhysicalObject &object);

        PhysicalObject &operator=(const PhysicalObject &object);

        void set_mass(const double value);

        void set_inertia(const double value);

        double mass() const;

        double inv_mass() const;

        double inertia() const;

        double inv_inertia() const;

        void add_impulse(const double x, const double y, const double rx = 0, const double ry = 0);

        void add_impulse(const Vector impulse, const Vector r = Vector());

        virtual void calculate_inertia() = 0;

        virtual void update_velocity(const double dt = 0.33);

        virtual void update_position(const double dt = 0.33);
    };

    struct CollisionPair
    {
        Physics::PhysicalObject *object0 = nullptr;
        Physics::PhysicalObject *object1 = nullptr;

        Physics::Vector r[4];
        Physics::Vector point[4];
        Physics::Vector normal;
        Physics::Vector tangent;

        Physics::Vector v[4];
        Physics::Vector velocity_bias;

        double im[2];
        double ii[2];
        double rn[2];
        double rt[2];
        double kn = 0;
        double kt = 0;
        double effective_mass_n = 0;
        double effective_mass_t = 0;

        double penetration = 0;
        double restitution = 1.0;
        double friction = 0.0;

        double accumulated_impulse_n = 0;
        double accumulated_impulse_t = 0;

        int point_count = 2;
    };

    void solve_velocity(std::vector<Physics::CollisionPair> &collisions);

    void solve_position(std::vector<Physics::CollisionPair> &collisions);
}