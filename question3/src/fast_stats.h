#ifndef FAST_STATS_H
#define FAST_STATS_H

#define PY_SSIZE_T_CLEAN
#include <Python.h>

/**
 * struct dataset_s - The numbers of a Python object, seen as C doubles
 *
 * @values: Pointer to the first value
 * @n: Number of values
 * @owned: PyMem buffer holding a converted copy of the values, or NULL
 * @view: Buffer view, when the values are read in place (array('d'))
 * @has_view: 1 if @view must be released
 */
typedef struct dataset_s
{
	const double	*values;
	size_t		n;
	double		*owned;
	Py_buffer	view;
	int		has_view;
} dataset_t;

/* convert.c */
int dataset_from_object(PyObject *obj, dataset_t *ds);
void dataset_release(dataset_t *ds);

/* stats_core.c */
double sc_mean(const double *v, size_t n);
double sc_sum_sq_dev(const double *v, size_t n, double mean);
void sc_min_max(const double *v, size_t n, double *min, double *max);
size_t sc_count_outliers(const double *v, size_t n, double mean,
			 double limit);

#endif /* FAST_STATS_H */
