#pragma once


namespace Physics
{
    class PhysicalObject
    {
    protected:
        double _mass = 0;
        double _inv_mass = 0;
        double _inertia = 0;
        double _inv_inertia = 0;
        double _recover_ratio = 1.0;

    public:
        double x_position = 0;
        double y_position = 0;
        double last_x_position = 0;
        double last_y_position = 0;
        double x_velocity = 0;
        double y_velocity = 0;
        double x_impulse = 0;
        double y_impulse = 0;
        double x_force = 0;
        double y_force = 0;

        double angular_velocity = 0;
        double rotation = 0;

        bool is_static = true;

    public:
        PhysicalObject();

        PhysicalObject(const double x, const double y);

        PhysicalObject(const PhysicalObject &object);

        PhysicalObject &operator=(const PhysicalObject &object);

        void set_mass(const double value);

        void set_inertia(const double value);

        void set_recover_ratio(const double value);

        double mass() const;

        double inv_mass() const;

        double inertia() const;

        double inv_inertia() const;

        double recover_ratio() const;

        void add_impulse(const double x, const double y, const double rx = 0, const double ry = 0);

        virtual void calculate_inertia() = 0;

        virtual void update(const double dt = 0.33);
    };


}