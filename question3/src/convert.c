#include "fast_stats.h"
#include <math.h>

/**
 * item_to_double - converts one element with the same rules as py_stats
 * @item: the element (borrowed reference)
 * @i: its position, for error messages
 * @out: where to store the value
 *
 * Description: only int and float are accepted; bool is refused even
 * though it is a subclass of int. NaN and infinity are refused.
 *
 * Return: 1 on success, 0 with a Python exception set
 */
static int item_to_double(PyObject *item, Py_ssize_t i, double *out)
{
	double x;

	if (PyFloat_CheckExact(item))
		x = PyFloat_AS_DOUBLE(item);
	else if (PyBool_Check(item))
	{
		PyErr_Format(PyExc_TypeError,
			     "element %zd is bool, not int or float", i);
		return (0);
	}
	else if (PyFloat_Check(item))
		x = PyFloat_AS_DOUBLE(item);
	else if (PyLong_Check(item))
	{
		x = PyLong_AsDouble(item);
		if (x == -1.0 && PyErr_Occurred())
			return (0);
	}
	else
	{
		PyErr_Format(PyExc_TypeError,
			     "element %zd is %s, not int or float", i,
			     Py_TYPE(item)->tp_name);
		return (0);
	}
	if (!isfinite(x))
	{
		PyErr_Format(PyExc_ValueError,
			     "element %zd is not a finite number", i);
		return (0);
	}
	*out = x;
	return (1);
}

/**
 * from_buffer - uses the memory of an array('d') or similar in place
 * @obj: object that supports the buffer protocol
 * @ds: dataset to fill
 *
 * Return: 1 if used, 0 if the buffer is not 1-D doubles (no error set),
 * -1 on a bad value (exception set, buffer released)
 */
static int from_buffer(PyObject *obj, dataset_t *ds)
{
	const char *fmt;
	size_t i;

	if (PyObject_GetBuffer(obj, &ds->view,
			       PyBUF_C_CONTIGUOUS | PyBUF_FORMAT) == -1)
	{
		PyErr_Clear();
		return (0);
	}
	fmt = ds->view.format;
	if (ds->view.ndim != 1 || ds->view.itemsize != sizeof(double) ||
	    !fmt || (strcmp(fmt, "d") != 0 && strcmp(fmt, "@d") != 0))
	{
		PyBuffer_Release(&ds->view);
		return (0);
	}
	ds->has_view = 1;
	ds->values = (const double *)ds->view.buf;
	ds->n = (size_t)ds->view.len / sizeof(double);
	for (i = 0; i < ds->n; i++)
	{
		if (!isfinite(ds->values[i]))
		{
			PyErr_Format(PyExc_ValueError,
				     "element %zu is not a finite number", i);
			dataset_release(ds);
			return (-1);
		}
	}
	return (1);
}

/**
 * from_sequence - copies any sequence or iterable into a C double array
 * @obj: the Python object
 * @ds: dataset to fill
 *
 * Description: PySequence_Fast gives a list or tuple (it builds a list
 * from other iterables). The loop only calls C conversion functions, so
 * no Python code can run and change the list while it is being read.
 *
 * Return: 1 on success, 0 with a Python exception set
 */
static int from_sequence(PyObject *obj, dataset_t *ds)
{
	PyObject *seq, **items;
	Py_ssize_t n, i;

	seq = PySequence_Fast(obj, "data must be a sequence or iterable "
			      "of numbers");
	if (!seq)
		return (0);
	n = PySequence_Fast_GET_SIZE(seq);
	if (n > 0)
	{
		ds->owned = PyMem_New(double, n);
		if (!ds->owned)
		{
			Py_DECREF(seq);
			PyErr_NoMemory();
			return (0);
		}
		items = PySequence_Fast_ITEMS(seq);
		for (i = 0; i < n; i++)
		{
			if (!item_to_double(items[i], i, &ds->owned[i]))
			{
				Py_DECREF(seq);
				dataset_release(ds);
				return (0);
			}
		}
	}
	Py_DECREF(seq);
	ds->values = ds->owned;
	ds->n = (size_t)n;
	return (1);
}

/**
 * dataset_from_object - gets the values of a Python object as doubles
 * @obj: list, tuple, array('d'), or any iterable of int/float
 * @ds: dataset to fill; release it with dataset_release() after use
 *
 * Return: 1 on success, 0 with a Python exception set (nothing to release)
 */
int dataset_from_object(PyObject *obj, dataset_t *ds)
{
	int r;

	memset(ds, 0, sizeof(*ds));
	if (PyObject_CheckBuffer(obj))
	{
		r = from_buffer(obj, ds);
		if (r != 0)
			return (r == 1);
	}
	return (from_sequence(obj, ds));
}

/**
 * dataset_release - frees the copy or releases the buffer view
 * @ds: the dataset
 *
 * Return: nothing
 */
void dataset_release(dataset_t *ds)
{
	if (ds->owned)
		PyMem_Free(ds->owned);
	if (ds->has_view)
		PyBuffer_Release(&ds->view);
	ds->owned = NULL;
	ds->has_view = 0;
	ds->values = NULL;
	ds->n = 0;
}
