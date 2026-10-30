"""Build script for the fast_stats C extension.

    python3 setup.py build_ext --inplace

produces fast_stats.cpython-<version>-<platform>.so next to this file.
"""
from setuptools import Extension, setup

module = Extension(
    "fast_stats",
    sources=["src/module.c", "src/convert.c", "src/stats_core.c"],
    include_dirs=["src"],
    # -ffp-contract=off stops GCC from fusing a*b+c into one FMA
    # instruction, so C rounds exactly like Python and results match.
    # -fvisibility=hidden keeps the helper functions private: only
    # PyInit_fast_stats (marked by PyMODINIT_FUNC) is exported.
    extra_compile_args=["-O2", "-Wall", "-Wextra", "-Werror",
                        "-ffp-contract=off", "-fvisibility=hidden"],
)

setup(name="fast_stats",
      version="1.0",
      description="Statistics on numeric sequences in C",
      ext_modules=[module])
