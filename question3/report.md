# CPython C Extension for Data Analysis: Technical Report

Ubuntu 24.04 (Docker), AArch64, 2 vCPUs, Python 3.12.3, GCC 13.3.0.
`py_stats.py` is the pure Python version, `fast_stats` (`src/`) the C
extension, and `app.py --impl python|c|both` the application.

**Definitions (identical in both versions)**
- **Variance:** two-pass, `Σ(x − mean)² / (n − ddof)`.
- **Outlier:** a value with `|x − mean| > threshold × σ`, where σ is the
  population standard deviation.
- **Normalize:** `(x − min) / (max − min)`, or all `0.0` if every value
  is equal.

**Accepted input:** any sequence or iterable of `int`/`float`. Other types,
including `bool`, raise `TypeError`. NaN/infinity and empty data raise
`ValueError`.

## Compilation

```sh
python3 setup.py build_ext --inplace     # builds fast_stats.cpython-312-aarch64-linux-gnu.so
python3 test_equivalence.py --report     # equivalence evidence
python3 benchmark.py                     # benchmark results
```

This needs `python3-dev` and setuptools. `setup.py` adds
`-O2 -Wall -Wextra -Werror -ffp-contract=off -fvisibility=hidden`.

## Correctness

- **142 of 142** calls on 19 datasets return **bit-identical** results
  (`results/equivalence.md`). The datasets include 1 value, all-equal
  values, ints and floats mixed, `1e±300`, generators, `range`,
  `array('d')`, and 100,000 and 1,000,000 random values.
- This holds because both versions add values in the same order, and
  `-ffp-contract=off` stops GCC from fusing `d*d + ss` into an FMA, which
  rounds differently.
- **23 invalid inputs** raise the same exception type in both. 21 also give
  the same message; the two `threshold` type errors are worded by
  `PyArg_ParseTupleAndKeywords`.
- **Memory tests** show no reference leaks and no memory growth over 200
  calls on 100,000 values. All 8 tests pass (`results/test_output.txt`).

## Performance

Each value is the average of 7 repeats (`results/benchmark.md`).
Speedup = Python ÷ C.

| Operation | Python, 1 M | C (list), 1 M | Speedup 1,000 | Speedup 100,000 | Speedup 1,000,000 |
|---|---:|---:|---:|---:|---:|
| mean | 35.8 ms | 1.22 ms | 32.0× | 33.1× | 29.5× |
| variance | 51.4 ms | 1.84 ms | 33.4× | 30.3× | 27.9× |
| min_max | 35.6 ms | 1.02 ms | 46.3× | 42.6× | 34.9× |
| count_outliers | 68.7 ms | 2.22 ms | 34.6× | 33.1× | 31.0× |
| normalize | 60.4 ms | 12.6 ms | 7.0× | 6.0× | 4.8× |

- **Size.** Both versions are O(n): Python time grows 10× from 100,000 to
  1,000,000 values. C saves a constant **cost per value**, about 36 ns in
  Python (interpreter dispatch, type checks, float objects) against about
  1 ns in C. So the speedup stays at 30–46×.
- **Large lists.** At 1 M values the speedup drops a little (`min_max`
  46× → 35×). The 1 M float objects (about 24 MB) no longer fit in the CPU
  cache, and C must follow a pointer to each one to convert it. Passed as
  `array('d')`, which C reads in place with no conversion, `mean` stays at
  about 43× at every size.
- **`normalize` gains only about 5×.** It must create one new Python
  float per value. That allocation, and freeing it later, costs the same
  in C as in Python, so it limits the total (Amdahl's law).
- **Exactness costs some speed.** The sum loop is also held back by
  matching Python exactly. `-ffast-math` could vectorise it, but that
  changes the rounding.

## How the C API connects Python to C

1. **Compiling.** setuptools compiles each `.c` file with `-fPIC` against
   `Python.h`, and links them with `-shared` into a `.so`. The name suffix
   `.cpython-312-aarch64-linux-gnu` marks the Python version and CPU it
   works with.
2. **Importing.** `import fast_stats` finds the `.so` on `sys.path`,
   `dlopen()`s it, and calls `PyInit_fast_stats`, the only exported
   symbol (`nm -D`). The `.so` does not link against libpython (`readelf -d`
   shows only libc). Its `Py*` calls are filled in from the running
   interpreter at load time.
3. **Building the module.** `PyInit_fast_stats` calls `PyModule_Create`.
   Its `PyMethodDef` table maps each Python name to a C function and a
   calling convention (`METH_O`, or `METH_VARARGS | METH_KEYWORDS` with
   `PyArg_ParseTupleAndKeywords`).
4. **Each call.**
   - The interpreter calls the C wrapper with the GIL held.
   - The wrapper validates and converts the data
     (`PySequence_Fast`, `PyFloat_AS_DOUBLE`/`PyLong_AsDouble`, or
     `PyObject_GetBuffer` for `array('d')`) into a `PyMem_New` buffer.
   - It runs the plain C maths, frees the buffer, and returns a new
     reference (`PyFloat_FromDouble`, `Py_BuildValue`, a `PyList`).
   - On error it sets an exception and returns `NULL`, which Python
     raises.

## Trade-offs and limitations

**Trade-offs of using C**
- **Speed:** about 30× faster for numeric loops, but only about 5× when the
  result is a list of Python objects.
- **Risk:** a memory or reference-count bug crashes or leaks instead of
  raising a Python exception.
- **Maintenance:** about 580 lines of C against 140 lines of Python, and
  the two must be kept in step.
- **Portability:** the `.so` works only on CPython 3.12 for aarch64. It
  needs a compiler and Python headers to rebuild for other systems.
- **Debugging:** C bugs need `gdb`; there are no tracebacks.

**Limitations of this implementation**
- **Memory:** lists and generators are copied in full into a C array (8
  bytes per value extra).
- **Threads:** the GIL is not released during the maths.
- **Number limits:** ints above 2⁵³ lose precision. Values above about
  1e154 make the variance `inf`. A range wider than the largest double
  makes `normalize` give `nan`. Both versions behave the same and do not
  warn.
- **Benchmark noise:** results come from a 2-vCPU VM. Most cells vary by
  less than 5 %; the worst is ± 20 %.
