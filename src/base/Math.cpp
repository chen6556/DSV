#include "base/Math.hpp"


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