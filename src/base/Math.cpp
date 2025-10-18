#include <algorithm>
#include <cmath>
#include "base/Math.hpp"
#include <gsl/gsl_linalg.h>
#include <gsl/gsl_cblas.h>


int Math::ellipse_ellipse_f(const gsl_vector *x, void *params, gsl_vector *f)
{
    EllipseParameter *ellipse = static_cast<EllipseParameter *>(params);
    const double x0 = gsl_vector_get(x, 0);
    const double x1 = gsl_vector_get(x, 1);

    gsl_vector_set(f, 0, ellipse->a[0] * x0 * x0 + ellipse->b[0] * x0 * x1
        + ellipse->c[0] * x1 * x1 + ellipse->d[0] * x0 + ellipse->e[0] * x1 + ellipse->f[0]);
    gsl_vector_set(f, 1, ellipse->a[1] * x0 * x0 + ellipse->b[1] * x0 * x1
        + ellipse->c[1] * x1 * x1 + ellipse->d[1] * x0 + ellipse->e[1] * x1 + ellipse->f[1]);

    return GSL_SUCCESS;
}

int Math::ellipse_ellipse_df(const gsl_vector *x, void *params, gsl_matrix *j)
{
    EllipseParameter *ellipse = static_cast<EllipseParameter *>(params);
    const double x0 = gsl_vector_get(x, 0);
    const double x1 = gsl_vector_get(x, 1);

    gsl_matrix_set(j, 0, 0, 2 * ellipse->a[0] * x0 + ellipse->b[0] * x1 + ellipse->d[0]);
    gsl_matrix_set(j, 1, 0, 2 * ellipse->a[1] * x0 + ellipse->b[1] * x1 + ellipse->d[1]);
    gsl_matrix_set(j, 0, 1, 2 * ellipse->c[0] * x1 + ellipse->b[0] * x0 + ellipse->e[0]);
    gsl_matrix_set(j, 1, 1, 2 * ellipse->c[1] * x1 + ellipse->b[1] * x0 + ellipse->e[1]);

    return GSL_SUCCESS;
}

int Math::ellipse_ellipse_fdf(const gsl_vector *x, void *params, gsl_vector *f, gsl_matrix *j)
{
    Math::ellipse_ellipse_f(x, params, f);
    Math::ellipse_ellipse_df(x, params, j);
    return GSL_SUCCESS;
}

std::tuple<double, double> Math::solve_ellipse_ellipse_intersection(EllipseParameter &param, const double init_x, const double init_y)
{
    const size_t n = 2; // 方程组未知数的个数
    gsl_multiroot_function_fdf f = {&Math::ellipse_ellipse_f, &Math::ellipse_ellipse_df, &Math::ellipse_ellipse_fdf, n, &param};

    gsl_vector *x = gsl_vector_alloc(n);
    gsl_vector_set(x, 0, init_x);
	gsl_vector_set(x, 1, init_y);

    const gsl_multiroot_fdfsolver_type *t = gsl_multiroot_fdfsolver_gnewton;
	gsl_multiroot_fdfsolver *s = gsl_multiroot_fdfsolver_alloc(t, n);
    gsl_multiroot_fdfsolver_set(s, &f, x);

    int status = GSL_CONTINUE;
    size_t count = 0;
    while (status == GSL_CONTINUE && count++ < Math::MAX_ITERATION) //这个循环迭代解方程，最多迭代Math::MAX_ITERATION次
	{
		status = gsl_multiroot_fdfsolver_iterate(s);
		status = gsl_multiroot_test_residual(s->f, Math::EPSILON); //判断解是否是真实解
	}

    std::tuple<double, double> res = std::make_tuple(gsl_vector_get(s->x, 0), gsl_vector_get(s->x, 1));

    gsl_multiroot_fdfsolver_free(s);
	gsl_vector_free(x);

    return res;
}

void Math::inverse(const double *input, const size_t n, double *output)
{
    gsl_matrix_const_view a = gsl_matrix_const_view_array(input, n, n);
    gsl_matrix *tmpA = gsl_matrix_alloc(n, n);
	gsl_matrix_memcpy(tmpA, &a.matrix);
	gsl_permutation *p = gsl_permutation_alloc(n);
	int sign = n % 2 == 0 ? 1 : -1;
	gsl_linalg_LU_decomp(tmpA, p, &sign);
	gsl_matrix *inv = gsl_matrix_alloc(n, n);
	gsl_linalg_LU_invert(tmpA, p, inv);
	gsl_permutation_free(p);
	gsl_matrix_free(tmpA);
    for (size_t i = 0; i < n; ++i)
    {
        for (size_t j = 0; j < n; ++j)
        {
            output[i * n + j] = gsl_matrix_get(inv, i, j);
        }
    }
    gsl_matrix_free(inv);
}

void Math::mul(const double *mat0, const size_t m0, const size_t n0, const double *mat1, const size_t n1, double *output)
{
    gsl_matrix_const_view a = gsl_matrix_const_view_array(mat0, m0, n0);
    gsl_matrix_const_view b = gsl_matrix_const_view_array(mat1, n0, n1);
    gsl_matrix_view c = gsl_matrix_view_array(output, n0, n1);
    gsl_blas_dgemm(CblasNoTrans, CblasNoTrans, 1.0, &a.matrix, &b.matrix, 0.0, &c.matrix);
}

void Math::solve(const double *mat, const size_t n, const double *b, double *output)
{
    double *a = new double[n * n];
    std::move(mat, mat + n * n, a);
    gsl_matrix_view aa = gsl_matrix_view_array(a, n, n);
    gsl_vector_const_view bb = gsl_vector_const_view_array(b, n);
    gsl_matrix *aa2 = gsl_matrix_alloc(n, n);
    int sign = n % 2 == 0 ? 1 : -1;
    gsl_permutation *p = gsl_permutation_alloc(n);
    gsl_linalg_LU_decomp(&aa.matrix, p, &sign);
    gsl_vector_view x = gsl_vector_view_array(output, n);
    gsl_linalg_LU_solve(&aa.matrix, p, &bb.vector, &x.vector);
    gsl_permutation_free(p);
    gsl_matrix_free(aa2);
    delete[] a;
}

int Math::bezier_bezier_f(const gsl_vector *v, void *params, gsl_vector *f)
{
    BezierPatameter *bezier = static_cast<BezierPatameter *>(params);
    const double t0 = gsl_vector_get(v, 0), t1 = gsl_vector_get(v, 1);
    double coord0[2] = {0, 0}, coord1[2] = {0, 0};
    for (int i = 0; i <= bezier[0].order; ++i)
    {
        coord0[0] += (bezier[0].points[i * 2] * bezier[0].values[i] * std::pow(1 - t0, bezier[0].order - i) * std::pow(t0, i));
        coord0[1] += (bezier[0].points[i * 2 + 1] * bezier[0].values[i] * std::pow(1 - t0, bezier[0].order - i) * std::pow(t0, i));
    }
    for (int i = 0; i <= bezier[1].order; ++i)
    {
        coord1[0] += (bezier[1].points[i * 2] * bezier[1].values[i] * std::pow(1 - t1, bezier[1].order - i) * std::pow(t1, i));
        coord1[1] += (bezier[1].points[i * 2 + 1] * bezier[1].values[i] * std::pow(1 - t1, bezier[1].order - i) * std::pow(t1, i));
    }
    gsl_vector_set(f, 0, coord0[0] - coord1[0]);
    gsl_vector_set(f, 1, coord0[1] - coord1[1]);
    return GSL_SUCCESS;
}

int Math::bezier_bezier_df(const gsl_vector *v, void *params, gsl_matrix *j)
{
    BezierPatameter *bezier = static_cast<BezierPatameter *>(params);
    const double t0 = gsl_vector_get(v, 0), t1 = gsl_vector_get(v, 1);
    double dcoord0[2], dcoord1[2];
    dcoord0[0] = -(bezier[0].points[0] * bezier[0].values[0] * std::pow(1 - t0, bezier[0].order - 1) *  bezier[0].order);
    dcoord0[1] = -(bezier[0].points[1] * bezier[0].values[0] * std::pow(1 - t0, bezier[0].order - 1) *  bezier[0].order);
    for (int i = 1; i < bezier[0].order; ++i)
    {
        dcoord0[0] += (bezier[0].points[i * 2] * bezier[0].values[i]) * ((i - bezier[0].order) * std::pow(1 - t0, bezier[0].order - i - 1)
            * std::pow(t0, i) + std::pow(1 - t0, bezier[0].order - i) * i * std::pow(t0, i - 1));
        dcoord0[1] += (bezier[0].points[i * 2 + 1] * bezier[0].values[i]) * ((i - bezier[0].order) * std::pow(1 - t0, bezier[0].order - i - 1)
            * std::pow(t0, i) + std::pow(1 - t0, bezier[0].order - i) * i * std::pow(t0, i - 1));
    }
    dcoord0[0] += (bezier[0].points[bezier[0].order * 2] * bezier[0].values[bezier[0].order] * std::pow(t0, bezier[0].order - 1) * bezier[0].order);
    dcoord0[1] += (bezier[0].points[bezier[0].order * 2 + 1] * bezier[0].values[bezier[0].order] * std::pow(t0, bezier[0].order - 1) * bezier[0].order);

    dcoord1[0] = -(bezier[1].points[0] * bezier[1].values[0] * std::pow(1 - t1, bezier[1].order - 1) * bezier[1].order);
    dcoord1[1] = -(bezier[1].points[1] * bezier[1].values[0] * std::pow(1 - t1, bezier[1].order - 1) * bezier[1].order);
    for (int i = 1; i < bezier[1].order; ++i)
    {
        dcoord1[0] += (bezier[1].points[i * 2] * bezier[1].values[i]) * ((i - bezier[1].order) * std::pow(1 - t1, bezier[1].order - i - 1)
            * std::pow(t1, i) + (std::pow(1 - t1, bezier[1].order - i) * i * std::pow(t1, i - 1)));
        dcoord1[1] += (bezier[1].points[i * 2 + 1] * bezier[1].values[i]) * ((i - bezier[1].order) * std::pow(1 - t1, bezier[1].order - i - 1)
            * std::pow(t1, i) + (std::pow(1 - t1, bezier[1].order - i) * i * std::pow(t1, i - 1)));
    }
    dcoord1[0] += (bezier[1].points[bezier[1].order * 2] * bezier[1].values[bezier[1].order] * std::pow(t1, bezier[1].order - 1) * bezier[1].order);
    dcoord1[1] += (bezier[1].points[bezier[1].order * 2 + 1] * bezier[1].values[bezier[1].order] * std::pow(t1, bezier[1].order - 1) * bezier[1].order);

    gsl_matrix_set(j, 0, 0, dcoord0[0]);
    gsl_matrix_set(j, 1, 0, dcoord0[1]);
    gsl_matrix_set(j, 0, 1, -dcoord1[0]);
    gsl_matrix_set(j, 1, 1, -dcoord1[1]);
    return GSL_SUCCESS;
}

int Math::bezier_bezier_fdf(const gsl_vector *x, void *params, gsl_vector *f, gsl_matrix *j)
{
    Math::bezier_bezier_f(x, params, f);
    Math::bezier_bezier_df(x, params, j);
    return GSL_SUCCESS;
}

std::tuple<double, double> Math::solve_bezier_bezier_intersection(BezierPatameter param[2], const double init_t0, const double init_t1)
{
    const size_t n = 2; // 方程组未知数的个数
    // gsl_multiroot_function_fdf f = {&Math::bezier_bezier_f, &Math::bezier_bezier_df, &Math::bezier_bezier_fdf, n, param};
    gsl_multiroot_function f = {&Math::bezier_bezier_f, n, param};

    gsl_vector *x = gsl_vector_alloc(n);
    gsl_vector_set(x, 0, init_t0);
	gsl_vector_set(x, 1, init_t1);

    // const gsl_multiroot_fdfsolver_type *t = gsl_multiroot_fdfsolver_gnewton;
	// gsl_multiroot_fdfsolver *s = gsl_multiroot_fdfsolver_alloc(t, n);
    // gsl_multiroot_fdfsolver_set(s, &f, x);
    const gsl_multiroot_fsolver_type *t = gsl_multiroot_fsolver_dnewton;
    gsl_multiroot_fsolver *s = gsl_multiroot_fsolver_alloc(t, n);
    gsl_multiroot_fsolver_set(s, &f, x);

    int status = GSL_CONTINUE;
    size_t count = 0;
    while (status == GSL_CONTINUE && count++ < Math::MAX_ITERATION) //这个循环迭代解方程，最多迭代Math::MAX_ITERATION次
	{
		// status = gsl_multiroot_fdfsolver_iterate(s);
        status = gsl_multiroot_fsolver_iterate(s);
		status = gsl_multiroot_test_residual(s->f, Math::EPSILON); //判断解是否是真实解
	}

    std::tuple<double, double> res = std::make_tuple(gsl_vector_get(s->x, 0), gsl_vector_get(s->x, 1));

    // gsl_multiroot_fdfsolver_free(s);
    gsl_multiroot_fsolver_free(s);
	gsl_vector_free(x);

    return res;
}
