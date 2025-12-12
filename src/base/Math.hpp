#pragma once
#include <tuple>
#include <gsl/gsl_vector.h>
#include <gsl/gsl_multiroots.h>


namespace Math
{
    static const double EPSILON = 1e-14;
    static const double PI = 3.14159265358979323846;
    static const int MAX_ITERATION = 10;

    void error_handle(const char *reason, const char *file, int line, int gsl_errno);


    struct EllipseParameter
    {
        double a[2] = {0};
        double b[2] = {0};
        double c[2] = {0};
        double d[2] = {0};
        double e[2] = {0};
        double f[2] = {0};
    };

    int ellipse_ellipse_f(const gsl_vector *x, void *params, gsl_vector *f);

    int ellipse_ellipse_df(const gsl_vector *x, void *params, gsl_matrix *j);

    int ellipse_ellipse_fdf(const gsl_vector *x, void *params, gsl_vector *f, gsl_matrix *j);

    std::tuple<double, double> solve_ellipse_ellipse_intersection(EllipseParameter &param, const double init_x, const double init_y);

    void inverse(const double *input, const size_t n, double *output);

    void mul(const double *mat0, const size_t m0, const size_t n0, const double *mat1, const size_t n1, double *output);

    void solve(const double *mat, const size_t n, const double *b, double *output);


    struct BezierParameter
    {
        int order = 3;
        const double *points = nullptr;
        const int *values = nullptr;
    };

    int bezier_bezier_f(const gsl_vector *v, void *params, gsl_vector *f);

    struct BSplineParameter
    {
        bool is_cubic = true;
        size_t npts = 4;
        const double *points = nullptr;
        const double *values = nullptr;
    };

    void rbasis(const bool is_cubic, const double t, const size_t npts, const double *x, double *output);

    int bspline_bspline_f(const gsl_vector *v, void *params, gsl_vector *f);

    struct BezierBSplineParameter
    {
        BezierParameter bezier;
        BSplineParameter bspline;
    };

    int bezier_bspline_f(const gsl_vector *v, void *params, gsl_vector *f);

    enum class CurveIntersectType
    {
        BezierBezier, BSplineBSpline, BezierBSpline
    };

    std::tuple<double, double> solve_curve_intersection(void *param, const CurveIntersectType type, const double init_t0, const double init_t1);


    struct EllipseFootParameter // a * cost + b * sint + c * sint * cost = 0
    {
        double a = 0;
        double b = 0;
        double c = 0;
    };

    double ellipse_foot_f(const double v, void *params);

    double solve_ellipse_foot(EllipseFootParameter &param, const double init_t);
};
