"""Pure Python statistics: the baseline for the fast_stats C extension.

Every function follows the same rules as the C version:

* data may be a list, tuple, array('d') or any iterable of int/float
* bool, str, None and other types raise TypeError
* NaN and infinity raise ValueError
* an empty dataset raises ValueError

The loops add values in the same order as the C code, so both versions
round the same way and return identical floats.

Only plain Python loops are used, not built-ins such as sum(), min() or
max(), which are themselves written in C.
"""
import math
import operator


def _values(data):
    """Check data and return it as a list of floats."""
    if not isinstance(data, (list, tuple)):
        try:
            data = list(data)
        except TypeError:
            raise TypeError("data must be a sequence or iterable "
                            "of numbers") from None
    values = []
    for i, x in enumerate(data):
        if type(x) is float:
            pass
        elif isinstance(x, bool):
            raise TypeError(f"element {i} is bool, not int or float")
        elif isinstance(x, (int, float)):
            x = float(x)
        else:
            raise TypeError(f"element {i} is {type(x).__name__}, "
                            "not int or float")
        if not math.isfinite(x):
            raise ValueError(f"element {i} is not a finite number")
        values.append(x)
    if not values:
        raise ValueError("dataset is empty")
    return values


def _threshold(threshold):
    """Check the outlier threshold and return it as a float."""
    if isinstance(threshold, (str, bytes, bytearray)):
        raise TypeError("threshold must be a number")
    threshold = float(threshold)
    if not math.isfinite(threshold) or threshold < 0.0:
        raise ValueError("threshold must be a finite number >= 0")
    return threshold


def _mean(values):
    total = 0.0
    for x in values:
        total += x
    return total / len(values)


def _sum_sq_dev(values, m):
    ss = 0.0
    for x in values:
        d = x - m
        ss += d * d
    return ss


def _min_max(values):
    lo = hi = values[0]
    for x in values:
        if x < lo:
            lo = x
        if x > hi:
            hi = x
    return lo, hi


def _count_outliers(values, m, limit):
    count = 0
    for x in values:
        if abs(x - m) > limit:
            count += 1
    return count


def mean(data):
    """Arithmetic mean."""
    return _mean(_values(data))


def variance(data, ddof=0):
    """Two-pass variance. ddof=0: population, ddof=1: sample."""
    ddof = operator.index(ddof)
    if ddof < 0:
        raise ValueError("ddof must be 0 or more")
    values = _values(data)
    if ddof >= len(values):
        raise ValueError("ddof must be smaller than the number of values")
    m = _mean(values)
    return _sum_sq_dev(values, m) / (len(values) - ddof)


def min_max(data):
    """(min, max) as a tuple of floats."""
    return _min_max(_values(data))


def count_outliers(data, threshold=3.0):
    """Number of values with |x - mean| > threshold * std (population)."""
    threshold = _threshold(threshold)
    values = _values(data)
    m = _mean(values)
    sd = math.sqrt(_sum_sq_dev(values, m) / len(values))
    return _count_outliers(values, m, threshold * sd)


def normalize(data):
    """(x - min) / (max - min) as a new list; all 0.0 if max == min."""
    values = _values(data)
    lo, hi = _min_max(values)
    rng = hi - lo
    if rng == 0.0:
        return [0.0] * len(values)
    return [(x - lo) / rng for x in values]


def summary(data, threshold=3.0):
    """Every statistic from one validated copy of the data."""
    threshold = _threshold(threshold)
    values = _values(data)
    n = len(values)
    m = _mean(values)
    var = _sum_sq_dev(values, m) / n
    lo, hi = _min_max(values)
    return {"count": n, "mean": m, "variance": var, "min": lo, "max": hi,
            "outliers": _count_outliers(values, m, threshold * math.sqrt(var))}
