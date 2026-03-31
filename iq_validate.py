#!/usr/bin/env python3
"""Validate IQ output files produced by MDI_to_IQ.

Checks:
- interleaved I/Q layout sanity
- optional expected complex sample count
- value ranges
- DC offset
- I/Q RMS balance
- nearest-QPSK-point distance (for current mapper output)

Exit code:
- 0 => PASS
- 1 => FAIL
"""

from __future__ import annotations

import argparse
import array
import math
import os
import random
import struct
import sys
from typing import Tuple


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Validate IQ stream metrics")
    parser.add_argument("iq_file", nargs="?", help="Path to IQ file")
    parser.add_argument("--format", dest="fmt", choices=["f32", "s16"], default="f32", help="IQ sample format")
    parser.add_argument("--expected-complex", type=int, default=None, help="Expected number of complex samples")
    parser.add_argument("--max-abs", type=float, default=1.05, help="Maximum allowed absolute I/Q value")
    parser.add_argument("--max-dc", type=float, default=0.05, help="Maximum allowed absolute DC offset")
    parser.add_argument("--max-rms-imbalance", type=float, default=0.10, help="Maximum allowed relative RMS imbalance")
    parser.add_argument("--max-qpsk-distance", type=float, default=0.20, help="Maximum allowed mean nearest-QPSK distance")
    parser.add_argument("--no-qpsk-check", action="store_true", help="Disable nearest-QPSK distance check")
    parser.add_argument("--self-test", action="store_true", help="Run built-in synthetic test and exit")
    return parser.parse_args()


def read_iq(path: str, fmt: str) -> Tuple[list[float], list[float]]:
    if fmt == "f32":
        item_size = 4
        unpack = "f"
    elif fmt == "s16":
        item_size = 2
        unpack = "h"
    else:
        raise ValueError(f"Unsupported format: {fmt}")

    file_size = os.path.getsize(path)
    if file_size % (2 * item_size) != 0:
        raise ValueError("File size is not divisible by one interleaved IQ pair")

    if fmt == "f32":
        arr = array.array("f")
        with open(path, "rb") as f:
            arr.fromfile(f, file_size // item_size)
        vals = [float(v) for v in arr]
    else:
        arr = array.array("h")
        with open(path, "rb") as f:
            arr.fromfile(f, file_size // item_size)
        vals = [float(v) / 32767.0 for v in arr]

    if len(vals) % 2 != 0:
        raise ValueError("Odd scalar sample count (broken I/Q interleave)")

    i_vals = vals[0::2]
    q_vals = vals[1::2]
    return i_vals, q_vals


def mean(xs: list[float]) -> float:
    return sum(xs) / float(len(xs)) if xs else 0.0


def rms(xs: list[float]) -> float:
    if not xs:
        return 0.0
    return math.sqrt(sum(v * v for v in xs) / float(len(xs)))


def mean_nearest_qpsk_distance(i_vals: list[float], q_vals: list[float]) -> float:
    inv = 1.0 / math.sqrt(2.0)
    points = [(inv, inv), (-inv, inv), (-inv, -inv), (inv, -inv)]

    total = 0.0
    n = len(i_vals)
    for i, q in zip(i_vals, q_vals):
        dmin = min(math.hypot(i - pi, q - pq) for pi, pq in points)
        total += dmin
    return total / float(n) if n else 0.0


def validate(
    i_vals: list[float],
    q_vals: list[float],
    expected_complex: int | None,
    max_abs: float,
    max_dc: float,
    max_rms_imbalance: float,
    max_qpsk_distance: float,
    qpsk_check: bool,
) -> int:
    failures: list[str] = []

    n = len(i_vals)
    if n == 0:
        failures.append("No complex samples found")

    if expected_complex is not None and n != expected_complex:
        failures.append(f"Expected {expected_complex} complex samples but got {n}")

    i_min = min(i_vals) if i_vals else 0.0
    i_max = max(i_vals) if i_vals else 0.0
    q_min = min(q_vals) if q_vals else 0.0
    q_max = max(q_vals) if q_vals else 0.0

    if max(abs(i_min), abs(i_max), abs(q_min), abs(q_max)) > max_abs:
        failures.append(f"Value range exceeds max_abs={max_abs}")

    i_mean = mean(i_vals)
    q_mean = mean(q_vals)
    if abs(i_mean) > max_dc or abs(q_mean) > max_dc:
        failures.append(f"DC offset too high (|Imean|={abs(i_mean):.6f}, |Qmean|={abs(q_mean):.6f}, max_dc={max_dc})")

    i_rms = rms(i_vals)
    q_rms = rms(q_vals)
    if (i_rms + q_rms) > 0.0:
        rel_imbalance = abs(i_rms - q_rms) / ((i_rms + q_rms) * 0.5)
    else:
        rel_imbalance = 0.0

    if rel_imbalance > max_rms_imbalance:
        failures.append(f"RMS imbalance too high ({rel_imbalance:.6f} > {max_rms_imbalance})")

    qpsk_dist = None
    if qpsk_check:
        qpsk_dist = mean_nearest_qpsk_distance(i_vals, q_vals)
        if qpsk_dist > max_qpsk_distance:
            failures.append(f"Mean nearest-QPSK distance too high ({qpsk_dist:.6f} > {max_qpsk_distance})")

    print(f"complex_samples={n}")
    print(f"I_min={i_min:.6f} I_max={i_max:.6f} Q_min={q_min:.6f} Q_max={q_max:.6f}")
    print(f"I_mean={i_mean:.6f} Q_mean={q_mean:.6f}")
    print(f"I_rms={i_rms:.6f} Q_rms={q_rms:.6f} rel_rms_imbalance={rel_imbalance:.6f}")
    if qpsk_dist is not None:
        print(f"mean_nearest_qpsk_distance={qpsk_dist:.6f}")

    if failures:
        print("VALIDATION: FAIL")
        for item in failures:
            print(f" - {item}")
        return 1

    print("VALIDATION: PASS")
    return 0


def run_self_test() -> int:
    inv = 1.0 / math.sqrt(2.0)
    points = [(inv, inv), (-inv, inv), (-inv, -inv), (inv, -inv)]

    i_vals: list[float] = []
    q_vals: list[float] = []
    random.seed(42)
    for _ in range(20000):
        i, q = random.choice(points)
        i_vals.append(i)
        q_vals.append(q)

    print("Running synthetic QPSK self-test...")
    return validate(
        i_vals,
        q_vals,
        expected_complex=20000,
        max_abs=1.05,
        max_dc=0.05,
        max_rms_imbalance=0.10,
        max_qpsk_distance=0.20,
        qpsk_check=True,
    )


def main() -> int:
    args = parse_args()

    if args.self_test:
        return run_self_test()

    if not args.iq_file:
        print("Error: iq_file is required unless --self-test is used", file=sys.stderr)
        return 1

    if not os.path.exists(args.iq_file):
        print(f"Error: IQ file not found: {args.iq_file}", file=sys.stderr)
        return 1

    i_vals, q_vals = read_iq(args.iq_file, args.fmt)

    return validate(
        i_vals,
        q_vals,
        expected_complex=args.expected_complex,
        max_abs=args.max_abs,
        max_dc=args.max_dc,
        max_rms_imbalance=args.max_rms_imbalance,
        max_qpsk_distance=args.max_qpsk_distance,
        qpsk_check=not args.no_qpsk_check,
    )


if __name__ == "__main__":
    raise SystemExit(main())

