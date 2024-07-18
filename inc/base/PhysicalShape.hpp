#pragma once

#include "base/Geometry.hpp"
#include "base/Physics.hpp"


namespace Physics
{
    template <typename T>
    class PhysicalShape : public PhysicalObject, public Geo::Geometry
    {
    private:
        T _shape;

    public:
        PhysicalShape() {}

        PhysicalShape(const double x, const double y)
            : PhysicalObject(x, y)
        {
        }

        PhysicalShape(const PhysicalShape &object)
            : _shape(object._shape), PhysicalObject(object)
        {
        }

        PhysicalShape(const double x, const double y, const T &shape)
            : PhysicalObject(x, y), _shape(shape)
        {
            const Geo::Point center(_shape.bounding_rect().center());
            _shape.translate(x - center.x, y - center.y);
        }

        PhysicalShape(const T &shape)
            : _shape(shape)
        {
            const Geo::Point center(_shape.bounding_rect().center());
            x_position = center.x, y_position = center.y;
        }

        const Type type() const override
        {
            if constexpr (std::is_same_v<T, Geo::Point>)
            {
                return Geo::Type::PYHSICAL_POINT;
            }
            else if constexpr (std::is_same_v<T, Geo::Polyline>)
            {
                return Geo::Type::PYHSICAL_POLYLINE;
            }
            else if constexpr (std::is_same_v<T, Geo::AABBRect>)
            {
                return Geo::Type::PYHSICAL_AABBRECT;
            }
            else if constexpr (std::is_same_v<T, Geo::Polygon>)
            {
                return Geo::Type::PYHSICAL_POLYGON;
            }
            else if constexpr (std::is_same_v<T, Geo::Triangle>)
            {
                return Geo::Type::PYHSICAL_TRIANGLE;
            }
            else if constexpr (std::is_same_v<T, Geo::Circle>)
            {
                return Geo::Type::PYHSICAL_CIRCLE;
            }
            else if constexpr (std::is_same_v<T, Geo::Line>)
            {
                return Geo::Type::PYHSICAL_LINE;
            }
            else if constexpr (std::is_same_v<T, Geo::Bezier>)
            {
                return Geo::Type::PYHSICAL_BEZIER;
            }
            else
            {
                return _shape.type();
            }
        }

        const double length() const override
        {
            return _shape.length();
        }

        const bool empty() const override
        {
            return _shape.empty();
        }

        void clear() override
        {
            return _shape.clear();
        }

        Geo::Geometry *clone() const override
        {
            return new PhysicalObject(*this);
        }

        void transform(const double a, const double b, const double c, const double d, const double e, const double f) override
        {
            const double x_ = x_position, y_ = y_position;
            x_position = a * x_ + b * y_ + c;
            y_position = d * x_ + e * y_ + f;
            _shape.transform(a, b, c, d, e, f);
        }

        void transform(const double mat[6]) override
        {
            const double x_ = x_position, y_ = y_position;
            x_position = mat[0] * x_ + mat[1] * y_ + mat[2];
            y_position = mat[3] * x_ + mat[4] * y_ + mat[5];
            _shape.transform(mat);
        }

        void translate(const double tx, const double ty) override
        {
            x_position += tx;
            y_position += ty;
            _shape.translate(tx, ty);
        }

        void rotate(const double x, const double y, const double rad) override // 弧度制
        {
            x_position -= x;
            y_position -= y;
            const double x1 = x_position, y1 = y_position;
            x_position = x1 * std::cos(rad) - y1 * std::sin(rad);
            y_position = x1 * std::sin(rad) + y1 * std::cos(rad);
            x_position += x;
            y_position += y;
            _shape.rotate(x, y, rad);
        }

        void scale(const double x, const double y, const double k) override
        {
            const double x1 = x_position, y1 = y_position;
            x_position = k * x1 + x * (1 - k);
            y_position = k * y1 + y * (1 - k);
            _shape.scale(x, y, k);
        }

        Geo::Polygon convex_hull() const override
        {
            return _shape.convex_hull();
        }

        Geo::AABBRect bounding_rect() const override
        {
            return _shape.bounding_rect();
        }

        Geo::Polygon mini_bounding_rect() const override
        {
            return _shape.mini_bounding_rect();
        }

        void set_position(const double x, const double y)
        {
            x_position = x, y_position = y;
            const Geo::Point center(_shape.bounding_rect().center());
            _shape.translate(x - center.x, y - center.y);
        }

        void set_shape(const T &shape)
        {
            _shape = shape;
            const Geo::Point center(_shape.bounding_rect().center());
            _shape.translate(x_position - center.x, y_position - center.y);
        }

        T &shape()
        {
            return _shape;
        }

        const T &shape() const
        {
            return _shape;
        }

        void update() override
        {
            x_velocity += x_acceleration;
            y_velocity += y_acceleration;
            _shape.translate(x_velocity, y_velocity);
            x_position += x_velocity;
            y_position += y_velocity;
            rotation += angular_velocity;
        }
    };

    using PhysicalPolygon = PhysicalShape<Geo::Polygon>;
    using PhysicalCircle = PhysicalShape<Geo::Circle>;
}