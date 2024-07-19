#pragma once


namespace Physics
{
    class PhysicalObject
    {
    public:
        double mass = 0;
        double x_position = 0;
        double y_position = 0;
        double x_delta = 0;
        double y_delta = 0;
        double x_velocity = 0;
        double y_velocity = 0;
        double x_acceleration = 0;
        double y_acceleration = 0;
        double angular_velocity = 0;
        double rotation = 0;
        double recover_ratio = 1.0;

        bool is_static = true;

    public:
        PhysicalObject();

        PhysicalObject(const double x, const double y);

        PhysicalObject(const PhysicalObject &object);

        PhysicalObject &operator=(const PhysicalObject &object);

        void add_force(const double x, const double y);

        virtual void update();
    };


}