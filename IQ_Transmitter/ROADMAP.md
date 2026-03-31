# IQ_Transmitter Roadmap

This roadmap tracks the staged implementation from current utility behavior to a DRM-compliant transmitter chain.

## M0 - Contract Freeze (Current)

- [x] Add IQ sidecar metadata contract (`MDI_IQ_META_V1`)
- [x] Write metadata from `MDI_to_IQ` as `<output.iq>.iqmeta`
- [x] Read/validate metadata in `IQ_Transmitter`
- [x] Allow explicit override (`--format`) and metadata controls (`--meta`, `--no-meta`)
- [x] Add regression test vectors with known metadata + expected decode reports (`IQ_Transmitter/regression_m0.py`)

Acceptance:

- `MDI_to_IQ` emits `.iq` and `.iqmeta`
- `IQ_Transmitter` auto-loads metadata when available
- format/sample-count mismatch is detected early

## M1 - TX Pipeline Skeleton

- [x] Separate decode utility path from future DRM TX chain classes
- [x] Add stage interfaces: coder, interleaver, frame builder, OFDM mapper
- [x] Add per-stage dump switches for deterministic diagnostics (`--dump-stages`, `--dump-dir`)

## M2 - Channel Coding + Interleaving

- [x] Implement MSC/SDC/FAC coding path
- [x] Add interleaver implementation and unit tests
- [x] Verify bit-exact vectors for coding/interleaving

## M3 - FAC/SDC/MSC Framing

- [x] Build DRM logical frame assembly
- [x] Add consistency checks for lengths and service mapping
- [x] Export debug dumps per logical frame

## M4 - OFDM + Pilots + Guard Interval

- [x] Carrier mapping
- [x] Pilot insertion
- [x] IFFT + guard interval
- [x] Symbol timing and frame sequencing

## M5 - FL2K Conditioning

- [x] Add TX scaling/headroom control
- [x] Add optional clipping / simple shaping path
- [x] Add runtime underrun/overrun telemetry

## M6 - Receiver Validation

- [x] Define lock/decode KPIs
- [x] Run loopback/lab tests and capture reproducible logs
- [x] Track mode/profile-specific performance

## M7 - Compliance and Legal Gate

- [x] Spectral checks (occupied bandwidth, spurs)
- [x] Document legal/regulatory constraints per deployment region
- [x] Mark OTA features as lab-only until authorized
