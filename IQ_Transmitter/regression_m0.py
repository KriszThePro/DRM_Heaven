#!/usr/bin/env python3
"""M0 regression harness for MDI_to_IQ <-> IQ_Transmitter IQ contract.

Checks:
1) MDI_to_IQ emits both .iq and .iqmeta
2) IQ_Transmitter auto-loads metadata and decodes successfully
3) Forcing wrong --format fails with metadata mismatch
"""

from __future__ import annotations

import os
import subprocess
import sys
import tempfile
from pathlib import Path


def run(cmd: list[str], cwd: Path, expect_ok: bool = True) -> subprocess.CompletedProcess[str]:
    p = subprocess.run(cmd, cwd=str(cwd), text=True, capture_output=True)
    if expect_ok and p.returncode != 0:
        raise RuntimeError(
            f"Command failed ({p.returncode}): {' '.join(cmd)}\nSTDOUT:\n{p.stdout}\nSTDERR:\n{p.stderr}"
        )
    return p


def parse_report(path: Path) -> dict[str, str]:
    out: dict[str, str] = {}
    for line in path.read_text(encoding="utf-8", errors="replace").splitlines():
        if "=" in line:
            k, v = line.split("=", 1)
            out[k.strip()] = v.strip()
    return out


def main() -> int:
    repo_root = Path(__file__).resolve().parents[1]
    mdi_exe = repo_root / "x64" / "Debug" / "MDI_to_IQ.exe"
    tx_exe = repo_root / "x64" / "Debug" / "IQ_Transmitter.exe"

    if not mdi_exe.exists() or not tx_exe.exists():
        print("Build outputs not found. Build Debug|x64 first.", file=sys.stderr)
        print(f"Missing: {mdi_exe if not mdi_exe.exists() else tx_exe}", file=sys.stderr)
        return 1

    with tempfile.TemporaryDirectory(prefix="m0_regression_") as td:
        tdir = Path(td)
        mdi = tdir / "sample.mdi"
        iq = tdir / "sample.iq"
        iqmeta = tdir / "sample.iq.iqmeta"
        decoded = tdir / "decoded.bin"
        report = tdir / "decode_report.txt"

        # Deterministic sample generation
        run([str(mdi_exe), "--generate-sample", str(mdi), "--frames", "3"], repo_root)

        # Produce IQ + sidecar metadata
        run([str(mdi_exe), str(mdi), str(iq), "--format", "f32"], repo_root)

        if not iq.exists() or iq.stat().st_size == 0:
            raise RuntimeError("Expected IQ file missing or empty")
        if not iqmeta.exists() or iqmeta.stat().st_size == 0:
            raise RuntimeError("Expected IQ metadata sidecar missing or empty")

        # Decode with auto metadata
        run([str(tx_exe), str(iq), str(decoded), "--report", str(report)], repo_root)

        if not decoded.exists() or decoded.stat().st_size == 0:
            raise RuntimeError("Decoded output missing or empty")
        if not report.exists() or report.stat().st_size == 0:
            raise RuntimeError("Decode report missing or empty")

        kv = parse_report(report)
        if kv.get("meta_contract") != "MDI_IQ_META_V1":
            raise RuntimeError(f"Unexpected meta_contract in report: {kv.get('meta_contract')}")
        if kv.get("meta_format") != "f32":
            raise RuntimeError(f"Unexpected meta_format in report: {kv.get('meta_format')}")

        # Force wrong format: should fail early
        bad = run([str(tx_exe), str(iq), str(tdir / "bad.bin"), "--format", "s16"], repo_root, expect_ok=False)
        combined = (bad.stdout or "") + "\n" + (bad.stderr or "")
        if bad.returncode == 0:
            raise RuntimeError("Expected non-zero exit code for format mismatch")
        if "IQ format mismatch with metadata" not in combined:
            raise RuntimeError("Expected metadata mismatch error message was not found")

    print("M0 regression: PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

