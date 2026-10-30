"""Make, save and load numeric datasets for the app, tests and benchmark."""
import random


def make_dataset(n, seed=42, outlier_rate=0.01):
    """Return n floats: normal values around 100 (std 15), with about
    outlier_rate of them pushed 6 to 10 standard deviations away.

    The same seed always gives the same list.
    """
    rng = random.Random(seed)
    data = []
    for _ in range(n):
        x = rng.gauss(100.0, 15.0)
        if rng.random() < outlier_rate:
            x = 100.0 + rng.choice((-1, 1)) * rng.uniform(6, 10) * 15.0
        data.append(x)
    return data


def save_dataset(data, path):
    """Write one number per line."""
    with open(path, "w") as f:
        for x in data:
            f.write(f"{x!r}\n")


def load_dataset(path):
    """Read one number per line. Blank lines and lines starting with #
    are skipped. Whole numbers stay int, the rest become float."""
    data = []
    with open(path) as f:
        for line_no, line in enumerate(f, 1):
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            try:
                data.append(int(line))
            except ValueError:
                try:
                    data.append(float(line))
                except ValueError:
                    raise ValueError(f"{path}:{line_no}: not a number: "
                                     f"{line!r}") from None
    return data
