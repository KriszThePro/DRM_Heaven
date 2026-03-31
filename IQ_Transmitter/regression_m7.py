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

    if not exe.exists() or not sample.exists():
        print("M7 regression: FAIL")
        print(" - missing executable or sample input")
        return 2

    profiles = ["robust", "balanced", "highrate"]
    failures = []

    for profile in profiles:
        cmd = [
            str(exe),
            str(sample),
            "--tx-drm",
            "--m6-profile",
            profile,
            "--region",
            "LAB",
            "--run-id",
            f"m7-{profile}",
            "--lab-tag",
            "regression",
        ]

        rc, stdout, stderr = run_cmd(cmd)
        if rc != 0:
            failures.append(f"{profile}: rc={rc} stderr={stderr.strip()}")
            continue

        kv = parse_kv(stdout)
        if kv.get("kpi_validation") != "pass":
            failures.append(f"{profile}: kpi_validation={kv.get('kpi_validation')}")
            continue
        if kv.get("m7_validation") != "pass":
            failures.append(f"{profile}: m7_validation={kv.get('m7_validation')}")
            continue
        if kv.get("m7_region_allowed") != "1":
            failures.append(f"{profile}: m7_region_allowed={kv.get('m7_region_allowed')}")

    # Verify OTA gate blocks FL2K mode without explicit authorization.
    gate_cmd = [
        str(exe),
        str(sample),
        "--tx-fl2k",
        "--fl2k-cmd",
        "cmd /c more > nul",
    ]
    rc, _, _ = run_cmd(gate_cmd)
    if rc == 0:
        failures.append("ota gate: expected failure without --ota-lab-authorized")

    if failures:
        print("M7 regression: FAIL")
        for f in failures:
            print(" -", f)
        return 1

    print("M7 regression: PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

