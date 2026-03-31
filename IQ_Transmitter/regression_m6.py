#!/usr/bin/env python3
import subprocess
from pathlib import Path


def run_cmd(cmd):
    p = subprocess.run(cmd, capture_output=True, text=True)
    return p.returncode, p.stdout, p.stderr


def parse_kv(text):
    out = {}
    for line in text.splitlines():
        if "=" in line:
            k, v = line.split("=", 1)
            out[k.strip()] = v.strip()
    return out


def main():
    repo = Path(__file__).resolve().parents[1]
    exe = repo / "x64" / "Debug" / "IQ_Transmitter.exe"
    sample = repo / "sample_refactor.iq"
    out_dir = repo / "m6_kpi_runs"
    out_dir.mkdir(exist_ok=True)

    if not exe.exists():
        print(f"Missing executable: {exe}")
        return 2
    if not sample.exists():
        print(f"Missing sample input: {sample}")
        return 2

    profiles = ["robust", "balanced", "highrate"]
    failures = []

    for profile in profiles:
        kpi_path = out_dir / f"kpi_{profile}.txt"
        cmd = [
            str(exe),
            str(sample),
            "--tx-drm",
            "--m6-profile",
            profile,
            "--run-id",
            f"m6-{profile}",
            "--lab-tag",
            "regression",
            "--kpi-out",
            str(kpi_path),
        ]

        rc, stdout, stderr = run_cmd(cmd)
        if rc != 0:
            failures.append(f"{profile}: process failed rc={rc} stderr={stderr.strip()}")
            continue

        kv = parse_kv(stdout)
        if kv.get("kpi_validation") != "pass":
            failures.append(f"{profile}: kpi_validation={kv.get('kpi_validation')}")
            continue
        if kv.get("kpi_lock_proxy") != "1" or kv.get("kpi_decode_proxy") != "1":
            failures.append(f"{profile}: lock/decode proxy failed")
            continue

        if not kpi_path.exists():
            failures.append(f"{profile}: missing KPI file {kpi_path}")

    if failures:
        print("M6 regression: FAIL")
        for f in failures:
            print(" -", f)
        return 1

    print("M6 regression: PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

