# Legal and Regulatory Notes (M7)

This project provides transmitter tooling for laboratory development only.

## Lab-Only Policy

- OTA transmission features are **lab-only** and require explicit runtime acknowledgment.
- `--tx-fl2k` requires `--ota-lab-authorized`.
- You are responsible for local legal authorization before radiating RF energy.

## Region Tags

The application supports region tags for compliance reporting:

- `LAB` (default): internal laboratory environment
- `EU`: European deployment context
- `US`: United States deployment context
- `HU`: Hungary deployment context

Region tags are metadata for validation logs and do **not** grant legal permission.

## Spectral Checks

M7 includes software-side spectral checks to support pre-lab review:

- occupied bandwidth ratio proxy
- spur suppression proxy

Passing software checks does not replace certified RF measurements.

