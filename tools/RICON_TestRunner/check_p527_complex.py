#!/usr/bin/env python3
"""Независимый численный check P.527-6, без записи и без утверждений о калибровке грунта.

Источник: Table2, ур.8–13 и59–70; DOCX SHA cbb067b9df36fbd164515a7f93f39e749be115cb63a1e99f47c67eda2c1db27f.
python -B -I tools/RICON_TestRunner/check_p527_complex.py
"""
import math
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "oracle"))
import sensitivity_oracle as candidate

# Константы дублируются из первоисточника намеренно: ошибка в candidate.SOILS не должна скопироваться в reference.
SOILS = [
    ("sandy_loam", 51.52, 13.42, 2.66, 1.6006),
    ("loam", 41.96, 8.53, 2.70, 1.5781),
    ("silty_loam", 30.63, 13.48, 2.59, 1.5750),
    ("silty_clay", 5.02, 47.38, 2.56, 1.4758),
]


def reference(f, t, mv, sand, clay, grain, bulk):
    theta = 300 / (273.15 + t) - 1
    static = 77.66 + 103.3 * theta
    e1 = .0671 * static
    einf = 3.52 - 7.52 * theta
    nu = 20.20 - 146.4 * theta + 316 * theta ** 2
    water = einf + (static - e1) / (1 + 1j * f / nu) + (e1 - einf) / (1 + 1j * f / (39.8 * nu))
    low = .0467 + .2204 * bulk - .004111 * sand - .006614 * clay
    high = -1.645 + 1.939 * bulk - .0225622 * sand + .01594 * clay
    sigma = high + (low - high) / (1 + 1j * f / 1.35)
    factor = 18 / f * (grain - bulk) / (grain * mv)
    free = water + factor * complex(-sigma.imag, -sigma.real)
    alpha = .65
    b1 = 1.2748 - .00519 * sand - .00152 * clay
    b2 = 1.33797 - .00603 * sand - .00166 * clay
    solid = (1.01 + .44 * grain) ** 2 - .062
    real = (1 + bulk / grain * (solid ** alpha - 1) + mv ** b1 * free.real ** alpha - mv) ** (1 / alpha)
    imag = mv ** (b2 / alpha) * (-free.imag)
    return real, imag


def main():
    maximum, count, rejected = 0.0, 0, 0
    for name, sand, clay, grain, bulk in SOILS:
        for f in [.03, .05, .088, .1, .4, 1.0]:
            for mv in [.1, .3, .5]:
                for t in [0, 23, 40]:
                    expected = reference(f, t, mv, sand, clay, grain, bulk)
                    reference_real = all(isinstance(x, (int, float)) and math.isfinite(x) for x in expected)
                    if not reference_real:
                        try:
                            actual = candidate.soil_permittivity(f, t, name, mv)
                        except ValueError:
                            rejected += 1
                            count += 1
                            continue
                        raise AssertionError(("non-real domain must be rejected", name, f, mv, t, actual))
                    actual = candidate.soil_permittivity(f, t, name, mv)
                    if len(actual) != 2 or not all(isinstance(x, (int, float)) and math.isfinite(x) for x in actual):
                        raise AssertionError((name, f, mv, t, actual))
                    error = max(abs(a - b) for a, b in zip(actual, expected))
                    if error > 1e-10:
                        raise AssertionError((name, f, mv, t, error, actual, expected))
                    maximum = max(maximum, error)
                    count += 1
    assert count == 216 and rejected == 15
    print(f"P527_COMPLEX_CROSSCHECK_PASS: {count}, real={count-rejected}, rejected={rejected}, max_abs={maximum:.12g}")


if __name__ == "__main__":
    main()
