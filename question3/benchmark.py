"""Times py_stats against fast_stats and writes results/benchmark.md.

    python3 benchmark.py                 # sizes 1,000 100,000 1,000,000
    python3 benchmark.py --repeats 10

For every size, operation and implementation, the call is repeated in a
loop long enough to take at least --min-time seconds. That is done
--repeats times, and the average time per call over the repeats is
reported, with the standard deviation. GC is off while timing.

Three inputs are compared:
  python      py_stats on a list
  c (list)    fast_stats on the same list (C must convert every item)
  c (array)   fast_stats on array('d') (C reads the memory in place)
"""
import argparse
import gc
import os
import platform
import statistics
import sys
import time
from array import array

import fast_stats
import py_stats
from dataset import make_dataset
from test_equivalence import same

OPERATIONS = {
    "mean": lambda m, d: m.mean(d),
    "variance": lambda m, d: m.variance(d),
    "min_max": lambda m, d: m.min_max(d),
    "count_outliers": lambda m, d: m.count_outliers(d, 3.0),
    "normalize": lambda m, d: m.normalize(d),
    "summary": lambda m, d: m.summary(d, 3.0),
}


def time_call(fn, repeats, min_time):
    """Average and standard deviation of the time per call, in seconds."""
    start = time.perf_counter()
    fn()
    one = time.perf_counter() - start
    number = max(1, int(min_time / max(one, 1e-7)))
    per_call = []
    gc.disable()
    try:
        for _ in range(repeats):
            start = time.perf_counter()
            for _ in range(number):
                fn()
            per_call.append((time.perf_counter() - start) / number)
    finally:
        gc.enable()
    return statistics.mean(per_call), statistics.stdev(per_call), number


def fmt_time(seconds):
    """Pick a readable unit."""
    if seconds >= 1e-1:
        return f"{seconds * 1e3:,.0f} ms"
    if seconds >= 1e-3:
        return f"{seconds * 1e3:.2f} ms"
    return f"{seconds * 1e6:.1f} µs"


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--sizes", type=int, nargs="+",
                        default=[1_000, 100_000, 1_000_000])
    parser.add_argument("--repeats", type=int, default=7)
    parser.add_argument("--min-time", type=float, default=0.2)
    args = parser.parse_args()

    rows = []
    for n in args.sizes:
        data = make_dataset(n, seed=42)
        arr = array("d", data)
        for op, call in OPERATIONS.items():
            # correctness check on the exact data being timed
            ref = call(py_stats, data)
            if not (same(ref, call(fast_stats, data)) and
                    same(ref, call(fast_stats, arr))):
                sys.exit(f"results differ for {op} at n={n}")
            row = {"size": n, "operation": op}
            for label, module, d in (("python", py_stats, data),
                                     ("c_list", fast_stats, data),
                                     ("c_array", fast_stats, arr)):
                avg, sd, number = time_call(lambda: call(module, d),
                                            args.repeats, args.min_time)
                row[f"{label}_s"] = avg
                row[f"{label}_sd"] = sd
                row[f"{label}_calls"] = number * args.repeats
            row["speedup_list"] = row["python_s"] / row["c_list_s"]
            row["speedup_array"] = row["python_s"] / row["c_array_s"]
            rows.append(row)
            print(f"n={n:>9,}  {op:15} python {fmt_time(row['python_s']):>10}"
                  f"  c(list) {fmt_time(row['c_list_s']):>10}"
                  f"  c(array) {fmt_time(row['c_array_s']):>10}"
                  f"  speedup {row['speedup_list']:6.1f}x"
                  f" / {row['speedup_array']:6.1f}x", flush=True)

    os.makedirs("results", exist_ok=True)
    write_markdown(rows, args)
    print("wrote results/benchmark.md")


def write_markdown(rows, args):
    """Benchmark tables in Markdown."""
    lines = [
        "# Benchmark: py_stats vs fast_stats", "",
        f"- Python {platform.python_version()}, {platform.machine()}, "
        f"{os.cpu_count()} CPUs, {platform.platform()}",
        f"- Each value is the **average time per call** over {args.repeats} "
        f"repeats (± standard deviation); each repeat runs the call in a "
        f"loop for at least {args.min_time} s. GC off while timing.",
        "- Data: `make_dataset(n, seed=42)`, normal values with about 1 % "
        "outliers. All three implementations were checked to give "
        "bit-identical results on the timed data.",
        "- Speedup = Python time / C time.", ""]
    for n in sorted({r["size"] for r in rows}):
        lines += [f"## {n:,} values", "",
                  "| Operation | Python (list) | C (list) | C (array('d')) "
                  "| Speedup, list | Speedup, array |",
                  "|---|---:|---:|---:|---:|---:|"]
        for r in (r for r in rows if r["size"] == n):
            cell = (lambda k: f"{fmt_time(r[k + '_s'])} "
                              f"± {100 * r[k + '_sd'] / r[k + '_s']:.0f}%")
            lines.append(f"| {r['operation']} | {cell('python')} | "
                         f"{cell('c_list')} | {cell('c_array')} | "
                         f"**{r['speedup_list']:.1f}x** | "
                         f"**{r['speedup_array']:.1f}x** |")
        lines.append("")
    lines += ["## Speedup (C on a list) by size", "",
              "| Operation | " + " | ".join(
                  f"{n:,}" for n in sorted({r['size'] for r in rows})) + " |",
              "|---|" + "---:|" * len({r["size"] for r in rows})]
    for op in OPERATIONS:
        cells = [f"{r['speedup_list']:.1f}x" for r in rows
                 if r["operation"] == op]
        lines.append(f"| {op} | " + " | ".join(cells) + " |")
    with open("results/benchmark.md", "w") as f:
        f.write("\n".join(lines) + "\n")


if __name__ == "__main__":
    main()
