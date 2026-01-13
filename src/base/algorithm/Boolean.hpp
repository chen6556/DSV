#pragma once
#include "base/Geometry.hpp"


namespace Geo
{
bool polygon_union(const Polygon &polygon0, const Polygon &polygon1, std::vector<Polygon> &output);

bool polygon_intersection(const Polygon &polygon0, const Polygon &polygon1, std::vector<Polygon> &output);

bool polygon_difference(const Polygon &polygon0, const Polygon &polygon1, std::vector<Polygon> &output);

bool polygon_xor(const Polygon &polygon0, const Polygon &polygon1, std::vector<Polygon> &output);


bool circle_union(const Circle &circle0, const Circle &circle1, std::vector<Arc> &output);

bool circle_intersection(const Circle &circle0, const Circle &circle1, std::vector<Arc> &output);

bool circle_difference(const Circle &circle0, const Circle &circle1, std::vector<Arc> &output);

bool circle_xor(const Circle &circle0, const Circle &circle1, std::vector<Arc> &output);


bool ellipse_union(const Ellipse &ellipse0, const Ellipse &ellipse1, std::vector<Ellipse> &output);

bool ellipse_intersection(const Ellipse &ellipse0, const Ellipse &ellipse1, std::vector<Ellipse> &output);

bool ellipse_difference(const Ellipse &ellipse0, const Ellipse &ellipse1, std::vector<Ellipse> &output);

bool ellipse_xor(const Ellipse &ellipse0, const Ellipse &ellipse1, std::vector<Ellipse> &output);


bool circle_ellipse_union(const Circle &circle, const Ellipse &ellipse, std::vector<Geo::Arc> &output0, std::vector<Geo::Ellipse> &output1);

bool circle_ellipse_intersection(const Circle &circle, const Ellipse &ellipse, std::vector<Geo::Arc> &output0,
                                 std::vector<Geo::Ellipse> &output1);

bool circle_ellipse_difference(const Circle &circle, const Ellipse &ellipse, std::vector<Geo::Arc> &output0,
                               std::vector<Geo::Ellipse> &output1);

bool ellipse_circle_difference(const Ellipse &ellipse, const Circle &circle, std::vector<Geo::Ellipse> &output0,
                               std::vector<Geo::Arc> &output1);

bool circle_ellipse_xor(const Circle &circle, const Ellipse &ellipse, std::vector<Geo::Arc> &output0, std::vector<Geo::Ellipse> &output1);


bool polygon_circle_union(const Polygon &polygon, const Circle &circle, std::vector<Geo::Polyline> &output0,
                          std::vector<Geo::Arc> &output1);

bool polygon_circle_intersection(const Polygon &polygon, const Circle &circle, std::vector<Geo::Polyline> &output0,
                                 std::vector<Geo::Arc> &output1);

bool polygon_circle_difference(const Polygon &polygon, const Circle &circle, std::vector<Geo::Polyline> &output0,
                               std::vector<Geo::Arc> &output1);

bool circle_polygon_difference(const Circle &circle, const Polygon &polygon, std::vector<Geo::Arc> &output0,
                               std::vector<Geo::Polyline> &output1);

bool polygon_circle_xor(const Polygon &polygon, const Circle &circle, std::vector<Geo::Polyline> &output0, std::vector<Geo::Arc> &output1);


bool polygon_ellipse_union(const Polygon &polygon, const Ellipse &ellipse, std::vector<Geo::Polyline> &output0,
                           std::vector<Geo::Ellipse> &output1);

bool polygon_ellipse_intersection(const Polygon &polygon, const Ellipse &ellipse, std::vector<Geo::Polyline> &output0,
                                  std::vector<Geo::Ellipse> &output1);

bool polygon_ellipse_difference(const Polygon &polygon, const Ellipse &ellipse, std::vector<Geo::Polyline> &output0,
                                std::vector<Geo::Ellipse> &output1);

bool ellipse_polygon_difference(const Ellipse &ellipse, const Polygon &polygon, std::vector<Geo::Ellipse> &output0,
                                std::vector<Geo::Polyline> &output1);

bool polygon_ellipse_xor(const Polygon &polygon, const Ellipse &ellipse, std::vector<Geo::Polyline> &output0,
                         std::vector<Geo::Ellipse> &output1);
} // namespace Geo