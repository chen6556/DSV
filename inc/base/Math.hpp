#pragma once
#include <tuple>
#include <gsl/gsl_vector.h>
#include <gsl/gsl_multiroots.h>


namespace Math
{
    static const double EPSILON = 1e-10;
    static const size_t MAX_ITERATION = 1000;

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
};