# Benchmark: py_stats vs fast_stats

- Python 3.12.3, aarch64, 2 CPUs, Linux-6.8.0-100-generic-aarch64-with-glibc2.39
- Each value is the **average time per call** over 7 repeats (± standard deviation); each repeat runs the call in a loop for at least 0.2 s. GC off while timing.
- Data: `make_dataset(n, seed=42)`, normal values with about 1 % outliers. All three implementations were checked to give bit-identical results on the timed data.
- Speedup = Python time / C time.

## 1,000 values

| Operation | Python (list) | C (list) | C (array('d')) | Speedup, list | Speedup, array |
|---|---:|---:|---:|---:|---:|
| mean | 34.6 µs ± 0% | 1.1 µs ± 8% | 0.9 µs ± 2% | **32.0x** | **40.5x** |
| variance | 55.0 µs ± 6% | 1.6 µs ± 1% | 1.6 µs ± 5% | **33.4x** | **35.3x** |
| min_max | 39.8 µs ± 20% | 0.9 µs ± 1% | 0.7 µs ± 1% | **46.3x** | **57.6x** |
| count_outliers | 68.1 µs ± 2% | 2.0 µs ± 1% | 1.8 µs ± 7% | **34.6x** | **37.8x** |
| normalize | 54.3 µs ± 0% | 7.7 µs ± 0% | 7.6 µs ± 0% | **7.0x** | **7.2x** |
| summary | 76.5 µs ± 1% | 2.6 µs ± 2% | 2.4 µs ± 1% | **29.3x** | **31.8x** |

## 100,000 values

| Operation | Python (list) | C (list) | C (array('d')) | Speedup, list | Speedup, array |
|---|---:|---:|---:|---:|---:|
| mean | 3.63 ms ± 1% | 109.7 µs ± 0% | 83.1 µs ± 2% | **33.1x** | **43.8x** |
| variance | 5.17 ms ± 1% | 170.8 µs ± 1% | 145.1 µs ± 4% | **30.3x** | **35.7x** |
| min_max | 3.88 ms ± 5% | 91.0 µs ± 4% | 62.8 µs ± 0% | **42.6x** | **61.7x** |
| count_outliers | 6.92 ms ± 3% | 209.2 µs ± 2% | 178.5 µs ± 1% | **33.1x** | **38.8x** |
| normalize | 5.61 ms ± 2% | 933.1 µs ± 5% | 872.9 µs ± 1% | **6.0x** | **6.4x** |
| summary | 7.60 ms ± 1% | 244.3 µs ± 0% | 218.0 µs ± 1% | **31.1x** | **34.8x** |

## 1,000,000 values

| Operation | Python (list) | C (list) | C (array('d')) | Speedup, list | Speedup, array |
|---|---:|---:|---:|---:|---:|
| mean | 35.83 ms ± 1% | 1.22 ms ± 1% | 840.5 µs ± 1% | **29.5x** | **42.6x** |
| variance | 51.40 ms ± 0% | 1.84 ms ± 3% | 1.45 ms ± 1% | **27.9x** | **35.5x** |
| min_max | 35.55 ms ± 1% | 1.02 ms ± 4% | 631.3 µs ± 0% | **34.9x** | **56.3x** |
| count_outliers | 68.71 ms ± 3% | 2.22 ms ± 2% | 1.84 ms ± 3% | **31.0x** | **37.3x** |
| normalize | 60.43 ms ± 1% | 12.55 ms ± 2% | 11.82 ms ± 2% | **4.8x** | **5.1x** |
| summary | 76.30 ms ± 1% | 2.58 ms ± 1% | 2.19 ms ± 1% | **29.6x** | **34.8x** |

## Speedup (C on a list) by size

| Operation | 1,000 | 100,000 | 1,000,000 |
|---|---:|---:|---:|
| mean | 32.0x | 33.1x | 29.5x |
| variance | 33.4x | 30.3x | 27.9x |
| min_max | 46.3x | 42.6x | 34.9x |
| count_outliers | 34.6x | 33.1x | 31.0x |
| normalize | 7.0x | 6.0x | 4.8x |
| summary | 29.3x | 31.1x | 29.6x |
