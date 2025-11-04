#pragma once

#include <list>

#include "base/Container.hpp"


class Graph : public Geo::Geometry
{
private:
    std::list<ContainerGroup> _container_groups;

public:
    bool modified = false;

public:
    Graph() {};

    Graph(const Graph &graph);

    const Geo::Type type() const override;

    Graph *clone() const override;

    void transfer(Graph &graph);

    void merge(Graph &graph);

    Graph &operator=(const Graph &graph);

    ContainerGroup &container_group(const size_t index = 0);

    const ContainerGroup &container_group(const size_t index = 0) const;

    std::list<ContainerGroup> &container_groups();

    const std::list<ContainerGroup> &container_groups() const;

    ContainerGroup &operator[](const size_t index);

    const ContainerGroup &operator[](const size_t index) const;

    const bool empty() const override;

    const bool empty(const size_t index) const;

    const size_t size() const;

    const size_t count(const Geo::Type type, const bool include_combinated) const;

    void clear() override;

    void clear(const size_t index);

    void transform(const double a, const double b, const double c, const double d, const double e, const double f) override;

    void transform(const double mat[6]) override;

    void translate(const double tx, const double ty) override;

    void rotate(const double x, const double y, const double rad) override; // 弧度制

    void scale(const double x, const double y, const double k) override;

    void rescale(const double x, const double y);

    Geo::AABBRect bounding_rect() const override;



    std::list<ContainerGroup>::iterator begin();

    std::list<ContainerGroup>::iterator end();

    std::list<ContainerGroup>::const_iterator begin() const;

    std::list<ContainerGroup>::const_iterator end() const;

    std::list<ContainerGroup>::const_iterator cbegin() const;

    std::list<ContainerGroup>::const_iterator cend() const;

    std::list<ContainerGroup>::reverse_iterator rbegin();

    std::list<ContainerGroup>::reverse_iterator rend();

    std::list<ContainerGroup>::const_reverse_iterator rbegin() const;

    std::list<ContainerGroup>::const_reverse_iterator rend() const;

    std::list<ContainerGroup>::const_reverse_iterator crbegin() const;

    std::list<ContainerGroup>::const_reverse_iterator crend() const;

    ContainerGroup &front();

    const ContainerGroup &front() const;

    ContainerGroup &back();

    const ContainerGroup &back() const;



    void append(Geo::Geometry *object, const size_t index = 0);

    void append_group();

    void append_group(const ContainerGroup &group);

    void append_group(const ContainerGroup &&group);

    void insert_group(const size_t index);

    void insert_group(const size_t index, const ContainerGroup &group);

    void insert_group(const size_t index, const ContainerGroup &&group);

    void remove_group(const size_t index);


    bool has_group(const QString &name) const;

    bool has_object(const QString &name) const;

    bool remove_object(const Geo::Geometry *object);


    void update_curve_shape(const double step, const double down_sampling_value);
};