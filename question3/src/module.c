#include "fast_stats.h"
#include <math.h>

/**
 * get_dataset - converts a Python object and refuses an empty dataset
 * @obj: the Python object
 * @ds: dataset to fill
 *
 * Return: 1 on success, 0 with a Python exception set
 */
static int get_dataset(PyObject *obj, dataset_t *ds)
{
	if (!dataset_from_object(obj, ds))
		return (0);
	if (ds->n == 0)
	{
		dataset_release(ds);
		PyErr_SetString(PyExc_ValueError, "dataset is empty");
		return (0);
	}
	return (1);
}

/**
 * check_threshold - a threshold must be a finite number, 0 or more
 * @threshold: the value given by the caller
 *
 * Return: 1 if valid, 0 with a Python exception set
 */
static int check_threshold(double threshold)
{
	if (!isfinite(threshold) || threshold < 0.0)
	{
		PyErr_SetString(PyExc_ValueError,
				"threshold must be a finite number >= 0");
		return (0);
	}
	return (1);
}

/**
 * fs_mean - mean(data) -> float
 * @self: the module (unused)
 * @data: the dataset
 *
 * Return: a new float, or NULL with an exception set
 */
static PyObject *fs_mean(PyObject *self, PyObject *data)
{
	dataset_t ds;
	double m;

	(void)self;
	if (!get_dataset(data, &ds))
		return (NULL);
	m = sc_mean(ds.values, ds.n);
	dataset_release(&ds);
	return (PyFloat_FromDouble(m));
}

/**
 * fs_variance - variance(data, ddof=0) -> float
 * @self: the module (unused)
 * @args: positional arguments
 * @kwargs: keyword arguments
 *
 * Description: ddof=0 gives the population variance, ddof=1 the sample
 * variance. n - ddof must be at least 1.
 *
 * Return: a new float, or NULL with an exception set
 */
static PyObject *fs_variance(PyObject *self, PyObject *args, PyObject *kwargs)
{
	static char *kwlist[] = {"data", "ddof", NULL};
	PyObject *data;
	Py_ssize_t ddof = 0;
	dataset_t ds;
	double m, var;

	(void)self;
	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|n:variance", kwlist,
					 &data, &ddof))
		return (NULL);
	if (ddof < 0)
	{
		PyErr_SetString(PyExc_ValueError, "ddof must be 0 or more");
		return (NULL);
	}
	if (!get_dataset(data, &ds))
		return (NULL);
	if ((size_t)ddof >= ds.n)
	{
		dataset_release(&ds);
		PyErr_SetString(PyExc_ValueError,
				"ddof must be smaller than the number of values");
		return (NULL);
	}
	m = sc_mean(ds.values, ds.n);
	var = sc_sum_sq_dev(ds.values, ds.n, m) / (double)(ds.n - ddof);
	dataset_release(&ds);
	return (PyFloat_FromDouble(var));
}

/**
 * fs_min_max - min_max(data) -> (min, max)
 * @self: the module (unused)
 * @data: the dataset
 *
 * Return: a new tuple of two floats, or NULL with an exception set
 */
static PyObject *fs_min_max(PyObject *self, PyObject *data)
{
	dataset_t ds;
	double lo, hi;

	(void)self;
	if (!get_dataset(data, &ds))
		return (NULL);
	sc_min_max(ds.values, ds.n, &lo, &hi);
	dataset_release(&ds);
	return (Py_BuildValue("(dd)", lo, hi));
}

/**
 * fs_count_outliers - count_outliers(data, threshold=3.0) -> int
 * @self: the module (unused)
 * @args: positional arguments
 * @kwargs: keyword arguments
 *
 * Description: a value is an outlier when it is more than threshold
 * standard deviations (population) away from the mean.
 *
 * Return: a new int, or NULL with an exception set
 */
static PyObject *fs_count_outliers(PyObject *self, PyObject *args,
				   PyObject *kwargs)
{
	static char *kwlist[] = {"data", "threshold", NULL};
	PyObject *data;
	double threshold = 3.0, m, sd;
	dataset_t ds;
	size_t count;

	(void)self;
	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|d:count_outliers",
					 kwlist, &data, &threshold))
		return (NULL);
	if (!check_threshold(threshold) || !get_dataset(data, &ds))
		return (NULL);
	m = sc_mean(ds.values, ds.n);
	sd = sqrt(sc_sum_sq_dev(ds.values, ds.n, m) / (double)ds.n);
	count = sc_count_outliers(ds.values, ds.n, m, threshold * sd);
	dataset_release(&ds);
	return (PyLong_FromSize_t(count));
}

/**
 * fs_normalize - normalize(data) -> list of floats in [0, 1]
 * @self: the module (unused)
 * @data: the dataset
 *
 * Description: (x - min) / (max - min). If all values are equal the
 * range is 0, and every result is 0.0.
 *
 * Return: a new list, or NULL with an exception set
 */
static PyObject *fs_normalize(PyObject *self, PyObject *data)
{
	dataset_t ds;
	PyObject *list, *item;
	double lo, hi, range;
	size_t i;

	(void)self;
	if (!get_dataset(data, &ds))
		return (NULL);
	sc_min_max(ds.values, ds.n, &lo, &hi);
	range = hi - lo;
	list = PyList_New((Py_ssize_t)ds.n);
	for (i = 0; list && i < ds.n; i++)
	{
		item = PyFloat_FromDouble(range == 0.0 ? 0.0 :
					  (ds.values[i] - lo) / range);
		if (!item)
			Py_CLEAR(list);
		else
			PyList_SET_ITEM(list, (Py_ssize_t)i, item);
	}
	dataset_release(&ds);
	return (list);
}

/**
 * fs_summary - summary(data, threshold=3.0) -> dict with every statistic
 * @self: the module (unused)
 * @args: positional arguments
 * @kwargs: keyword arguments
 *
 * Description: converts the data once and computes everything from the
 * same C array. variance is the population variance (ddof=0).
 *
 * Return: a new dict, or NULL with an exception set
 */
static PyObject *fs_summary(PyObject *self, PyObject *args, PyObject *kwargs)
{
	static char *kwlist[] = {"data", "threshold", NULL};
	PyObject *data;
	double threshold = 3.0, m, var, lo, hi;
	dataset_t ds;
	size_t outliers, n;

	(void)self;
	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|d:summary", kwlist,
					 &data, &threshold))
		return (NULL);
	if (!check_threshold(threshold) || !get_dataset(data, &ds))
		return (NULL);
	n = ds.n;
	m = sc_mean(ds.values, n);
	var = sc_sum_sq_dev(ds.values, n, m) / (double)n;
	sc_min_max(ds.values, n, &lo, &hi);
	outliers = sc_count_outliers(ds.values, n, m, threshold * sqrt(var));
	dataset_release(&ds);
	return (Py_BuildValue("{s:n,s:d,s:d,s:d,s:d,s:n}", "count",
			      (Py_ssize_t)n, "mean", m, "variance", var,
			      "min", lo, "max", hi, "outliers",
			      (Py_ssize_t)outliers));
}

static PyMethodDef FastStatsMethods[] = {
	{"mean", fs_mean, METH_O,
	 "mean(data) -> float\n\nArithmetic mean."},
	{"variance", (PyCFunction)(void (*)(void))fs_variance,
	 METH_VARARGS | METH_KEYWORDS,
	 "variance(data, ddof=0) -> float\n\n"
	 "Two-pass variance; ddof=0 population, ddof=1 sample."},
	{"min_max", fs_min_max, METH_O,
	 "min_max(data) -> (min, max)"},
	{"count_outliers", (PyCFunction)(void (*)(void))fs_count_outliers,
	 METH_VARARGS | METH_KEYWORDS,
	 "count_outliers(data, threshold=3.0) -> int\n\n"
	 "Number of values with |x - mean| > threshold * std (population)."},
	{"normalize", fs_normalize, METH_O,
	 "normalize(data) -> list\n\n"
	 "(x - min) / (max - min); all 0.0 if every value is equal."},
	{"summary", (PyCFunction)(void (*)(void))fs_summary,
	 METH_VARARGS | METH_KEYWORDS,
	 "summary(data, threshold=3.0) -> dict\n\n"
	 "count, mean, variance (ddof=0), min, max and outliers in one call."},
	{NULL, NULL, 0, NULL}
};

static struct PyModuleDef fast_stats_module = {
	PyModuleDef_HEAD_INIT,
	"fast_stats",
	"Statistics on numeric sequences, written in C with the CPython C API.\n"
	"Accepts lists, tuples, any iterable of int/float, and reads\n"
	"array('d') objects in place through the buffer protocol.",
	-1,
	FastStatsMethods,
	NULL, NULL, NULL, NULL
};

/**
 * PyInit_fast_stats - called by Python on "import fast_stats"
 *
 * Return: the new module object, or NULL on failure
 */
PyMODINIT_FUNC PyInit_fast_stats(void)
{
	PyObject *m = PyModule_Create(&fast_stats_module);

	if (m && PyModule_AddStringConstant(m, "__version__", "1.0") < 0)
		Py_CLEAR(m);
	return (m);
}
