#include <math.h>
#include <stddef.h>

/*
 * Plain C statistics on an array of doubles. Nothing here touches Python
 * objects. Every loop adds the values in the same order as py_stats.py,
 * so both versions round the same way and give identical results.
 */

/**
 * sc_mean - arithmetic mean of n values
 * @v: the values
 * @n: number of values, more than 0
 *
 * Return: the mean
 */
double sc_mean(const double *v, size_t n)
{
	double total = 0.0;
	size_t i;

	for (i = 0; i < n; i++)
		total += v[i];
	return (total / (double)n);
}

/**
 * sc_sum_sq_dev - sum of squared deviations from the mean
 * @v: the values
 * @n: number of values
 * @mean: mean of the values
 *
 * Description: second pass of the two-pass variance. Dividing the result
 * by (n - ddof) gives the variance.
 *
 * Return: the sum of (v[i] - mean)^2
 */
double sc_sum_sq_dev(const double *v, size_t n, double mean)
{
	double ss = 0.0, d;
	size_t i;

	for (i = 0; i < n; i++)
	{
		d = v[i] - mean;
		ss += d * d;
	}
	return (ss);
}

/**
 * sc_min_max - smallest and largest value in one pass
 * @v: the values
 * @n: number of values, more than 0
 * @min: where to store the smallest value
 * @max: where to store the largest value
 *
 * Return: nothing
 */
void sc_min_max(const double *v, size_t n, double *min, double *max)
{
	double lo = v[0], hi = v[0];
	size_t i;

	for (i = 1; i < n; i++)
	{
		if (v[i] < lo)
			lo = v[i];
		if (v[i] > hi)
			hi = v[i];
	}
	*min = lo;
	*max = hi;
}

/**
 * sc_count_outliers - counts values further than limit from the mean
 * @v: the values
 * @n: number of values
 * @mean: mean of the values
 * @limit: threshold * standard deviation
 *
 * Return: number of values with |v[i] - mean| > limit
 */
size_t sc_count_outliers(const double *v, size_t n, double mean,
			 double limit)
{
	size_t i, count = 0;

	for (i = 0; i < n; i++)
		if (fabs(v[i] - mean) > limit)
			count++;
	return (count);
}
