"""Data analysis app that runs on either the pure Python or the C version.

Examples:
    python3 app.py --impl python --size 100000
    python3 app.py --impl c --size 1000000 --threshold 4
    python3 app.py --impl c --file data.txt
    python3 app.py --impl both --size 100000
"""
import argparse
import sys
import time
from array import array

import py_stats
from dataset import load_dataset, make_dataset

try:
    import fast_stats
except ImportError:
    fast_stats = None

IMPLEMENTATIONS = {"python": py_stats, "c": fast_stats}


def analyse(impl, data, threshold):
    """Run every operation once and time it."""
    start = time.perf_counter()
    result = {
        "mean": impl.mean(data),
        "variance": impl.variance(data),
        "sample variance": impl.variance(data, ddof=1),
        "min, max": impl.min_max(data),
        "outliers": impl.count_outliers(data, threshold),
    }
    normalized = impl.normalize(data)
    result["normalized[:5]"] = [round(x, 6) for x in normalized[:5]]
    result["seconds"] = time.perf_counter() - start
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--impl", choices=["python", "c", "both"],
                        default="c")
    parser.add_argument("--size", type=int, default=100_000,
                        help="number of values to generate")
    parser.add_argument("--seed", type=int, default=42)
    parser.add_argument("--file", help="load numbers from a file instead")
    parser.add_argument("--threshold", type=float, default=3.0,
                        help="outlier threshold in standard deviations")
    parser.add_argument("--array", action="store_true",
                        help="pass the data as array('d') instead of list")
    args = parser.parse_args()

    if args.impl != "python" and fast_stats is None:
        sys.exit("fast_stats is not built: run python3 setup.py "
                 "build_ext --inplace")
    data = (load_dataset(args.file) if args.file
            else make_dataset(args.size, args.seed))
    if args.array:
        data = array("d", data)
    names = ["python", "c"] if args.impl == "both" else [args.impl]

    print(f"dataset: {len(data):,} values from "
          f"{args.file or f'make_dataset(seed={args.seed})'}")
    results = {}
    for name in names:
        try:
            results[name] = analyse(IMPLEMENTATIONS[name], data,
                                    args.threshold)
        except (TypeError, ValueError, OverflowError) as e:
            sys.exit(f"{name}: {type(e).__name__}: {e}")
        print(f"\n[{name}]")
        for key, value in results[name].items():
            print(f"  {key:16} {value}")
    if len(results) == 2:
        a, b = (dict(r) for r in results.values())
        ta, tb = a.pop("seconds"), b.pop("seconds")
        print(f"\nresults identical: {a == b}")
        print(f"speedup (python / c): {ta / tb:.1f}x")


if __name__ == "__main__":
    main()
