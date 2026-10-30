"""Checks that py_stats and fast_stats give the same results and errors.

    python3 test_equivalence.py              # run the tests
    python3 test_equivalence.py --report     # also write results/equivalence.md

"Same result" means exactly the same float, compared bit for bit (both
NaN counts as the same), not just "close".
"""
import gc
import math
import struct
import sys
import unittest
from array import array

import fast_stats
import py_stats
from dataset import make_dataset

BIG_1M = make_dataset(1_000_000, seed=1)
BIG_100K = make_dataset(100_000, seed=2)

# name -> function that returns a fresh copy of the data (generators can
# only be used once)
VALID = {
    "single value": lambda: [42],
    "two values": lambda: [1, 2],
    "small ints": lambda: [1, 2, 3, 4, 100],
    "mixed int and float": lambda: [1, 2.5, -3, 4.25, 0],
    "all values equal": lambda: [7.5] * 10,
    "all negative": lambda: [-5.5, -1.0, -100.25, -3.0],
    "0.1 three times": lambda: [0.1, 0.1, 0.1],
    "tiny values": lambda: [1e-300, 2e-300, 3e-300],
    "huge values (variance overflows)": lambda: [1e300, -1e300, 5e299],
    "full double range (range overflows)": lambda: [-1.7e308, 0.0, 1.7e308],
    "large int": lambda: [10 ** 20, 1, -(10 ** 15)],
    "tuple input": lambda: (3.5, 1.25, 9.0, -2.0),
    "range input": lambda: range(1, 1001),
    "generator input": lambda: (x * 0.5 for x in range(500)),
    "bytes input (ints 0-255)": lambda: b"\x01\x02\x03\xff",
    "array('d') input": lambda: array("d", BIG_100K[:5000]),
    "array('i') input": lambda: array("i", [5, -3, 12, 7]),
    "random 100,000": lambda: BIG_100K,
    "random 1,000,000": lambda: BIG_1M,
}

# (name, function, args, kwargs, also compare the message?)
INVALID = [
    ("empty list", "mean", ([],), {}, True),
    ("empty tuple", "summary", ((),), {}, True),
    ("empty array('d')", "normalize", (array("d"),), {}, True),
    ("empty generator", "min_max", ((x for x in ()),), {}, True),
    ("None as data", "mean", (None,), {}, True),
    ("int as data", "variance", (5,), {}, True),
    ("str elements", "mean", ("abc",), {}, True),
    ("str in list", "min_max", ([1, "2"],), {}, True),
    ("None in list", "normalize", ([1, None],), {}, True),
    ("bool in list", "mean", ([True, 2],), {}, True),
    ("complex in list", "mean", ([1, 2j],), {}, True),
    ("NaN in list", "variance", ([1, float("nan")],), {}, True),
    ("inf in list", "count_outliers", ([float("inf"), 1],), {}, True),
    ("NaN in array('d')", "mean", (array("d", [1, math.nan]),), {}, True),
    ("int too big for float", "mean", ([10 ** 400],), {}, True),
    ("ddof = -1", "variance", ([1, 2, 3],), {"ddof": -1}, True),
    ("ddof = n", "variance", ([1, 2, 3],), {"ddof": 3}, True),
    ("ddof = 1 with one value", "variance", ([5],), {"ddof": 1}, True),
    ("ddof = 1.5", "variance", ([1, 2, 3],), {"ddof": 1.5}, True),
    ("threshold = -1", "count_outliers", ([1, 2],), {"threshold": -1}, True),
    ("threshold = NaN", "summary", ([1, 2],), {"threshold": math.nan}, True),
    ("threshold = '3'", "count_outliers", ([1, 2],), {"threshold": "3"},
     False),
    ("threshold = None", "summary", ([1, 2],), {"threshold": None}, False),
]

FUNCTIONS = ["mean", "variance", "min_max", "count_outliers", "normalize",
             "summary"]


def same_float(a, b):
    """True if a and b are the same double, bit for bit (NaN == NaN)."""
    return struct.pack("<d", a) == struct.pack("<d", b)


def same(a, b):
    """Deep, exact comparison of results (floats, ints, tuples, lists,
    dicts)."""
    if type(a) is not type(b):
        return False
    if isinstance(a, float):
        return same_float(a, b)
    if isinstance(a, (list, tuple)):
        return len(a) == len(b) and all(map(same, a, b))
    if isinstance(a, dict):
        return a.keys() == b.keys() and all(same(a[k], b[k]) for k in a)
    return a == b


def calls():
    """Every (case, function, args, kwargs) combination to compare."""
    for case, factory in VALID.items():
        for fn in FUNCTIONS:
            yield case, fn, (), {}
        if case != "single value":  # ddof=1 needs 2 values (see INVALID)
            yield case, "variance", (), {"ddof": 1}
    for t in (0, 1, 2.5, 6, 10):
        yield "random 100,000", "count_outliers", (), {"threshold": t}
        yield "random 100,000", "summary", (), {"threshold": t}


def run(module, fn, case, kwargs):
    """Call module.fn on a fresh copy of the case. Returns ("ok", value)
    or ("error", exception)."""
    try:
        return "ok", getattr(module, fn)(VALID[case](), **kwargs)
    except Exception as e:  # pylint: disable=broad-except
        return "error", e


def rss_kb():
    """Current resident memory of this process in KB (Linux)."""
    with open("/proc/self/statm") as f:
        return int(f.read().split()[1]) * 4


class TestEquivalence(unittest.TestCase):
    """Both versions must agree on every valid dataset."""

    def test_valid_datasets_identical(self):
        for case, fn, _, kwargs in calls():
            with self.subTest(case=case, fn=fn, kwargs=kwargs):
                py = run(py_stats, fn, case, kwargs)
                c = run(fast_stats, fn, case, kwargs)
                self.assertEqual(py[0], "ok", py[1])
                self.assertEqual(c[0], "ok", c[1])
                self.assertTrue(same(py[1], c[1]), f"{py[1]!r} != {c[1]!r}")

    def test_result_types(self):
        data = [1, 2, 3]
        self.assertIsInstance(fast_stats.mean(data), float)
        self.assertIsInstance(fast_stats.variance(data), float)
        self.assertIsInstance(fast_stats.min_max(data), tuple)
        self.assertIsInstance(fast_stats.count_outliers(data), int)
        self.assertIsInstance(fast_stats.normalize(data), list)
        self.assertIsInstance(fast_stats.summary(data), dict)

    def test_normalize_range(self):
        out = fast_stats.normalize(BIG_100K)
        self.assertEqual(len(out), len(BIG_100K))
        self.assertEqual(min(out), 0.0)
        self.assertEqual(max(out), 1.0)

    def test_input_not_changed(self):
        data = BIG_100K[:1000]
        copy = list(data)
        for fn in FUNCTIONS:
            getattr(fast_stats, fn)(data)
        self.assertEqual(data, copy)


class TestErrors(unittest.TestCase):
    """Both versions must refuse the same bad input the same way."""

    def test_invalid_inputs(self):
        for name, fn, args, kwargs, check_msg in INVALID:
            with self.subTest(name=name):
                errors = []
                for module in (py_stats, fast_stats):
                    with self.assertRaises(Exception) as cm:
                        getattr(module, fn)(*args, **kwargs)
                    errors.append(cm.exception)
                py, c = errors
                self.assertIs(type(py), type(c), f"{py!r} vs {c!r}")
                if check_msg:
                    self.assertEqual(str(py), str(c))


class TestMemory(unittest.TestCase):
    """The C code must not leak memory or references."""

    def test_no_reference_leaks(self):
        data = [1.5, 2.5, 3.5]
        before = [sys.getrefcount(data)] + [sys.getrefcount(x) for x in data]
        for _ in range(1000):
            for fn in FUNCTIONS:
                getattr(fast_stats, fn)(data)
            for bad in ([], [1, "x"], [1, math.nan]):
                with self.assertRaises((TypeError, ValueError)):
                    fast_stats.mean(bad)
        after = [sys.getrefcount(data)] + [sys.getrefcount(x) for x in data]
        self.assertEqual(before, after)

    def test_no_memory_growth(self):
        data = BIG_100K
        arr = array("d", data)
        gc.collect()
        start = rss_kb()
        for _ in range(200):
            fast_stats.summary(data)
            fast_stats.normalize(data)
            fast_stats.summary(arr)
            with self.assertRaises(TypeError):
                fast_stats.mean(data + ["x"])
        gc.collect()
        growth_kb = rss_kb() - start
        # 200 rounds x 3 converted copies of 800 KB would leak about
        # 480 MB if the copies were never freed
        self.assertLess(growth_kb, 10_000, f"RSS grew by {growth_kb} KB")

    def test_array_buffer_released(self):
        arr = array("d", [1.0, 2.0, 3.0])
        fast_stats.summary(arr)
        with self.assertRaises(ValueError):
            fast_stats.mean(array("d", [1.0, math.inf]))
        arr.append(4.0)  # fails with BufferError if a view is still held
        self.assertEqual(len(arr), 4)


def short(value):
    """Short text of a result for the report."""
    if isinstance(value, list):
        return f"list of {len(value):,} floats"
    if isinstance(value, dict):
        return "{" + ", ".join(f"{k}: {v!r}" for k, v in value.items()) + "}"
    return repr(value)


def write_report(path):
    """Write the side-by-side evidence table."""
    lines = ["# Equivalence evidence: py_stats vs fast_stats", "",
             f"Python {sys.version.split()[0]}. 'Identical' means the same "
             "type and, for floats, the same 64 bits.", "",
             "## Valid inputs", "",
             "| Dataset | Call | py_stats result | Identical in fast_stats |",
             "|---|---|---|---|"]
    total = ok = 0
    for case, fn, _, kwargs in calls():
        py = run(py_stats, fn, case, kwargs)
        c = run(fast_stats, fn, case, kwargs)
        match = py[0] == c[0] == "ok" and same(py[1], c[1])
        total += 1
        ok += match
        arg = ", ".join(f"{k}={v}" for k, v in kwargs.items())
        lines.append(f"| {case} | `{fn}({'data' + (', ' + arg if arg else '')})` "
                     f"| {short(py[1])} | {'yes' if match else '**NO**'} |")
    lines += ["", f"**{ok} of {total} calls identical.**", "",
              "## Invalid inputs", "",
              "| Case | Call | py_stats raises | fast_stats raises | Same |",
              "|---|---|---|---|---|"]
    for name, fn, args, kwargs, check_msg in INVALID:
        errs = []
        for module in (py_stats, fast_stats):
            try:
                getattr(module, fn)(*args, **kwargs)
                errs.append(None)
            except Exception as e:  # pylint: disable=broad-except
                errs.append(e)
        py, c = errs
        if type(py) is not type(c):
            verdict = "**NO**"
        elif str(py) == str(c):
            verdict = "yes"
        elif check_msg:
            verdict = "**message differs**"
        else:
            verdict = "same type (wording from PyArg_Parse)"
        lines.append(f"| {name} | `{fn}` | `{type(py).__name__}: {py}` | "
                     f"`{type(c).__name__}: {c}` | {verdict} |")
    with open(path, "w") as f:
        f.write("\n".join(lines) + "\n")
    print(f"wrote {path}: {ok}/{total} valid calls identical")


if __name__ == "__main__":
    if "--report" in sys.argv:
        sys.argv.remove("--report")
        write_report("results/equivalence.md")
    unittest.main(verbosity=2)
