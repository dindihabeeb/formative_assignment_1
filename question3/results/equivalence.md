# Equivalence evidence: py_stats vs fast_stats

Python 3.12.3. 'Identical' means the same type and, for floats, the same 64 bits.

## Valid inputs

| Dataset | Call | py_stats result | Identical in fast_stats |
|---|---|---|---|
| single value | `mean(data)` | 42.0 | yes |
| single value | `variance(data)` | 0.0 | yes |
| single value | `min_max(data)` | (42.0, 42.0) | yes |
| single value | `count_outliers(data)` | 0 | yes |
| single value | `normalize(data)` | list of 1 floats | yes |
| single value | `summary(data)` | {count: 1, mean: 42.0, variance: 0.0, min: 42.0, max: 42.0, outliers: 0} | yes |
| two values | `mean(data)` | 1.5 | yes |
| two values | `variance(data)` | 0.25 | yes |
| two values | `min_max(data)` | (1.0, 2.0) | yes |
| two values | `count_outliers(data)` | 0 | yes |
| two values | `normalize(data)` | list of 2 floats | yes |
| two values | `summary(data)` | {count: 2, mean: 1.5, variance: 0.25, min: 1.0, max: 2.0, outliers: 0} | yes |
| two values | `variance(data, ddof=1)` | 0.5 | yes |
| small ints | `mean(data)` | 22.0 | yes |
| small ints | `variance(data)` | 1522.0 | yes |
| small ints | `min_max(data)` | (1.0, 100.0) | yes |
| small ints | `count_outliers(data)` | 0 | yes |
| small ints | `normalize(data)` | list of 5 floats | yes |
| small ints | `summary(data)` | {count: 5, mean: 22.0, variance: 1522.0, min: 1.0, max: 100.0, outliers: 0} | yes |
| small ints | `variance(data, ddof=1)` | 1902.5 | yes |
| mixed int and float | `mean(data)` | 0.95 | yes |
| mixed int and float | `variance(data)` | 5.96 | yes |
| mixed int and float | `min_max(data)` | (-3.0, 4.25) | yes |
| mixed int and float | `count_outliers(data)` | 0 | yes |
| mixed int and float | `normalize(data)` | list of 5 floats | yes |
| mixed int and float | `summary(data)` | {count: 5, mean: 0.95, variance: 5.96, min: -3.0, max: 4.25, outliers: 0} | yes |
| mixed int and float | `variance(data, ddof=1)` | 7.45 | yes |
| all values equal | `mean(data)` | 7.5 | yes |
| all values equal | `variance(data)` | 0.0 | yes |
| all values equal | `min_max(data)` | (7.5, 7.5) | yes |
| all values equal | `count_outliers(data)` | 0 | yes |
| all values equal | `normalize(data)` | list of 10 floats | yes |
| all values equal | `summary(data)` | {count: 10, mean: 7.5, variance: 0.0, min: 7.5, max: 7.5, outliers: 0} | yes |
| all values equal | `variance(data, ddof=1)` | 0.0 | yes |
| all negative | `mean(data)` | -27.4375 | yes |
| all negative | `variance(data)` | 1769.76171875 | yes |
| all negative | `min_max(data)` | (-100.25, -1.0) | yes |
| all negative | `count_outliers(data)` | 0 | yes |
| all negative | `normalize(data)` | list of 4 floats | yes |
| all negative | `summary(data)` | {count: 4, mean: -27.4375, variance: 1769.76171875, min: -100.25, max: -1.0, outliers: 0} | yes |
| all negative | `variance(data, ddof=1)` | 2359.6822916666665 | yes |
| 0.1 three times | `mean(data)` | 0.10000000000000002 | yes |
| 0.1 three times | `variance(data)` | 1.925929944387236e-34 | yes |
| 0.1 three times | `min_max(data)` | (0.1, 0.1) | yes |
| 0.1 three times | `count_outliers(data)` | 0 | yes |
| 0.1 three times | `normalize(data)` | list of 3 floats | yes |
| 0.1 three times | `summary(data)` | {count: 3, mean: 0.10000000000000002, variance: 1.925929944387236e-34, min: 0.1, max: 0.1, outliers: 0} | yes |
| 0.1 three times | `variance(data, ddof=1)` | 2.8888949165808538e-34 | yes |
| tiny values | `mean(data)` | 2e-300 | yes |
| tiny values | `variance(data)` | 0.0 | yes |
| tiny values | `min_max(data)` | (1e-300, 3e-300) | yes |
| tiny values | `count_outliers(data)` | 2 | yes |
| tiny values | `normalize(data)` | list of 3 floats | yes |
| tiny values | `summary(data)` | {count: 3, mean: 2e-300, variance: 0.0, min: 1e-300, max: 3e-300, outliers: 2} | yes |
| tiny values | `variance(data, ddof=1)` | 0.0 | yes |
| huge values (variance overflows) | `mean(data)` | 1.6666666666666668e+299 | yes |
| huge values (variance overflows) | `variance(data)` | inf | yes |
| huge values (variance overflows) | `min_max(data)` | (-1e+300, 1e+300) | yes |
| huge values (variance overflows) | `count_outliers(data)` | 0 | yes |
| huge values (variance overflows) | `normalize(data)` | list of 3 floats | yes |
| huge values (variance overflows) | `summary(data)` | {count: 3, mean: 1.6666666666666668e+299, variance: inf, min: -1e+300, max: 1e+300, outliers: 0} | yes |
| huge values (variance overflows) | `variance(data, ddof=1)` | inf | yes |
| full double range (range overflows) | `mean(data)` | 0.0 | yes |
| full double range (range overflows) | `variance(data)` | inf | yes |
| full double range (range overflows) | `min_max(data)` | (-1.7e+308, 1.7e+308) | yes |
| full double range (range overflows) | `count_outliers(data)` | 0 | yes |
| full double range (range overflows) | `normalize(data)` | list of 3 floats | yes |
| full double range (range overflows) | `summary(data)` | {count: 3, mean: 0.0, variance: inf, min: -1.7e+308, max: 1.7e+308, outliers: 0} | yes |
| full double range (range overflows) | `variance(data, ddof=1)` | inf | yes |
| large int | `mean(data)` | 3.3333e+19 | yes |
| large int | `variance(data)` | 2.2222444446666665e+39 | yes |
| large int | `min_max(data)` | (-1000000000000000.0, 1e+20) | yes |
| large int | `count_outliers(data)` | 0 | yes |
| large int | `normalize(data)` | list of 3 floats | yes |
| large int | `summary(data)` | {count: 3, mean: 3.3333e+19, variance: 2.2222444446666665e+39, min: -1000000000000000.0, max: 1e+20, outliers: 0} | yes |
| large int | `variance(data, ddof=1)` | 3.333366667e+39 | yes |
| tuple input | `mean(data)` | 2.9375 | yes |
| tuple input | `variance(data)` | 16.07421875 | yes |
| tuple input | `min_max(data)` | (-2.0, 9.0) | yes |
| tuple input | `count_outliers(data)` | 0 | yes |
| tuple input | `normalize(data)` | list of 4 floats | yes |
| tuple input | `summary(data)` | {count: 4, mean: 2.9375, variance: 16.07421875, min: -2.0, max: 9.0, outliers: 0} | yes |
| tuple input | `variance(data, ddof=1)` | 21.432291666666668 | yes |
| range input | `mean(data)` | 500.5 | yes |
| range input | `variance(data)` | 83333.25 | yes |
| range input | `min_max(data)` | (1.0, 1000.0) | yes |
| range input | `count_outliers(data)` | 0 | yes |
| range input | `normalize(data)` | list of 1,000 floats | yes |
| range input | `summary(data)` | {count: 1000, mean: 500.5, variance: 83333.25, min: 1.0, max: 1000.0, outliers: 0} | yes |
| range input | `variance(data, ddof=1)` | 83416.66666666667 | yes |
| generator input | `mean(data)` | 124.75 | yes |
| generator input | `variance(data)` | 5208.3125 | yes |
| generator input | `min_max(data)` | (0.0, 249.5) | yes |
| generator input | `count_outliers(data)` | 0 | yes |
| generator input | `normalize(data)` | list of 500 floats | yes |
| generator input | `summary(data)` | {count: 500, mean: 124.75, variance: 5208.3125, min: 0.0, max: 249.5, outliers: 0} | yes |
| generator input | `variance(data, ddof=1)` | 5218.75 | yes |
| bytes input (ints 0-255) | `mean(data)` | 65.25 | yes |
| bytes input (ints 0-255) | `variance(data)` | 12002.1875 | yes |
| bytes input (ints 0-255) | `min_max(data)` | (1.0, 255.0) | yes |
| bytes input (ints 0-255) | `count_outliers(data)` | 0 | yes |
| bytes input (ints 0-255) | `normalize(data)` | list of 4 floats | yes |
| bytes input (ints 0-255) | `summary(data)` | {count: 4, mean: 65.25, variance: 12002.1875, min: 1.0, max: 255.0, outliers: 0} | yes |
| bytes input (ints 0-255) | `variance(data, ddof=1)` | 16002.916666666666 | yes |
| array('d') input | `mean(data)` | 100.3587456265043 | yes |
| array('d') input | `variance(data)` | 356.007135603565 | yes |
| array('d') input | `min_max(data)` | (-48.66691874180776, 249.98174807748634) | yes |
| array('d') input | `count_outliers(data)` | 48 | yes |
| array('d') input | `normalize(data)` | list of 5,000 floats | yes |
| array('d') input | `summary(data)` | {count: 5000, mean: 100.3587456265043, variance: 356.007135603565, min: -48.66691874180776, max: 249.98174807748634, outliers: 48} | yes |
| array('d') input | `variance(data, ddof=1)` | 356.0783512738198 | yes |
| array('i') input | `mean(data)` | 5.25 | yes |
| array('i') input | `variance(data)` | 29.1875 | yes |
| array('i') input | `min_max(data)` | (-3.0, 12.0) | yes |
| array('i') input | `count_outliers(data)` | 0 | yes |
| array('i') input | `normalize(data)` | list of 4 floats | yes |
| array('i') input | `summary(data)` | {count: 4, mean: 5.25, variance: 29.1875, min: -3.0, max: 12.0, outliers: 0} | yes |
| array('i') input | `variance(data, ddof=1)` | 38.916666666666664 | yes |
| random 100,000 | `mean(data)` | 100.05920185748502 | yes |
| random 100,000 | `variance(data)` | 369.52501932025604 | yes |
| random 100,000 | `min_max(data)` | (-49.942092824935315, 249.98174807748634) | yes |
| random 100,000 | `count_outliers(data)` | 1016 | yes |
| random 100,000 | `normalize(data)` | list of 100,000 floats | yes |
| random 100,000 | `summary(data)` | {count: 100000, mean: 100.05920185748502, variance: 369.52501932025604, min: -49.942092824935315, max: 249.98174807748634, outliers: 1016} | yes |
| random 100,000 | `variance(data, ddof=1)` | 369.5287146074021 | yes |
| random 1,000,000 | `mean(data)` | 100.03429923448324 | yes |
| random 1,000,000 | `variance(data)` | 369.1901616014795 | yes |
| random 1,000,000 | `min_max(data)` | (-49.98252265800781, 249.99282492961606) | yes |
| random 1,000,000 | `count_outliers(data)` | 10143 | yes |
| random 1,000,000 | `normalize(data)` | list of 1,000,000 floats | yes |
| random 1,000,000 | `summary(data)` | {count: 1000000, mean: 100.03429923448324, variance: 369.1901616014795, min: -49.98252265800781, max: 249.99282492961606, outliers: 10143} | yes |
| random 1,000,000 | `variance(data, ddof=1)` | 369.19053079201024 | yes |
| random 100,000 | `count_outliers(data, threshold=0)` | 100000 | yes |
| random 100,000 | `summary(data, threshold=0)` | {count: 100000, mean: 100.05920185748502, variance: 369.52501932025604, min: -49.942092824935315, max: 249.98174807748634, outliers: 100000} | yes |
| random 100,000 | `count_outliers(data, threshold=1)` | 20721 | yes |
| random 100,000 | `summary(data, threshold=1)` | {count: 100000, mean: 100.05920185748502, variance: 369.52501932025604, min: -49.942092824935315, max: 249.98174807748634, outliers: 20721} | yes |
| random 100,000 | `count_outliers(data, threshold=2.5)` | 1121 | yes |
| random 100,000 | `summary(data, threshold=2.5)` | {count: 100000, mean: 100.05920185748502, variance: 369.52501932025604, min: -49.942092824935315, max: 249.98174807748634, outliers: 1121} | yes |
| random 100,000 | `count_outliers(data, threshold=6)` | 571 | yes |
| random 100,000 | `summary(data, threshold=6)` | {count: 100000, mean: 100.05920185748502, variance: 369.52501932025604, min: -49.942092824935315, max: 249.98174807748634, outliers: 571} | yes |
| random 100,000 | `count_outliers(data, threshold=10)` | 0 | yes |
| random 100,000 | `summary(data, threshold=10)` | {count: 100000, mean: 100.05920185748502, variance: 369.52501932025604, min: -49.942092824935315, max: 249.98174807748634, outliers: 0} | yes |

**142 of 142 calls identical.**

## Invalid inputs

| Case | Call | py_stats raises | fast_stats raises | Same |
|---|---|---|---|---|
| empty list | `mean` | `ValueError: dataset is empty` | `ValueError: dataset is empty` | yes |
| empty tuple | `summary` | `ValueError: dataset is empty` | `ValueError: dataset is empty` | yes |
| empty array('d') | `normalize` | `ValueError: dataset is empty` | `ValueError: dataset is empty` | yes |
| empty generator | `min_max` | `ValueError: dataset is empty` | `ValueError: dataset is empty` | yes |
| None as data | `mean` | `TypeError: data must be a sequence or iterable of numbers` | `TypeError: data must be a sequence or iterable of numbers` | yes |
| int as data | `variance` | `TypeError: data must be a sequence or iterable of numbers` | `TypeError: data must be a sequence or iterable of numbers` | yes |
| str elements | `mean` | `TypeError: element 0 is str, not int or float` | `TypeError: element 0 is str, not int or float` | yes |
| str in list | `min_max` | `TypeError: element 1 is str, not int or float` | `TypeError: element 1 is str, not int or float` | yes |
| None in list | `normalize` | `TypeError: element 1 is NoneType, not int or float` | `TypeError: element 1 is NoneType, not int or float` | yes |
| bool in list | `mean` | `TypeError: element 0 is bool, not int or float` | `TypeError: element 0 is bool, not int or float` | yes |
| complex in list | `mean` | `TypeError: element 1 is complex, not int or float` | `TypeError: element 1 is complex, not int or float` | yes |
| NaN in list | `variance` | `ValueError: element 1 is not a finite number` | `ValueError: element 1 is not a finite number` | yes |
| inf in list | `count_outliers` | `ValueError: element 0 is not a finite number` | `ValueError: element 0 is not a finite number` | yes |
| NaN in array('d') | `mean` | `ValueError: element 1 is not a finite number` | `ValueError: element 1 is not a finite number` | yes |
| int too big for float | `mean` | `OverflowError: int too large to convert to float` | `OverflowError: int too large to convert to float` | yes |
| ddof = -1 | `variance` | `ValueError: ddof must be 0 or more` | `ValueError: ddof must be 0 or more` | yes |
| ddof = n | `variance` | `ValueError: ddof must be smaller than the number of values` | `ValueError: ddof must be smaller than the number of values` | yes |
| ddof = 1 with one value | `variance` | `ValueError: ddof must be smaller than the number of values` | `ValueError: ddof must be smaller than the number of values` | yes |
| ddof = 1.5 | `variance` | `TypeError: 'float' object cannot be interpreted as an integer` | `TypeError: 'float' object cannot be interpreted as an integer` | yes |
| threshold = -1 | `count_outliers` | `ValueError: threshold must be a finite number >= 0` | `ValueError: threshold must be a finite number >= 0` | yes |
| threshold = NaN | `summary` | `ValueError: threshold must be a finite number >= 0` | `ValueError: threshold must be a finite number >= 0` | yes |
| threshold = '3' | `count_outliers` | `TypeError: threshold must be a number` | `TypeError: must be real number, not str` | same type (wording from PyArg_Parse) |
| threshold = None | `summary` | `TypeError: float() argument must be a string or a real number, not 'NoneType'` | `TypeError: must be real number, not NoneType` | same type (wording from PyArg_Parse) |
