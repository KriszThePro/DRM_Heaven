# MDI_to_IQ

`MDI_to_IQ` is a small command-line tool in this solution that:

1. reads an MDI AF/TAG file,
2. decodes AF packets and selected TAG items,
3. writes a complex I/Q output file.

## Standards Notes

- AF/TAG packet parsing is implemented around the framing used by **ETSI TS 102 821** (AF packet + TAG payload model).
- Parsed TAG items include commonly used fields (`dlfc`, `robm`, `fac_`, `sdc_`, `str0..str3`) used in DRM distribution workflows.
- The current conversion stage maps decoded payload bits to complex QPSK symbols and writes interleaved IQ samples (`f32` or `s16`).

Important: full DRM RF waveform generation per **ETSI ES 201 980** transmitter chain (complete OFDM framing, pilots, interleaving, and channel coding from service source) is outside this first implementation scope.

## Dream Project Source Reference

This work was developed with technical reference to the `dream` project source tree (especially AF/TAG framing and CRC handling behavior) to stay aligned with practical DRM MDI decoding behavior.

Please keep license hygiene in mind:

- `dream` is GPL-licensed,
- this repository should keep attribution/usage notes clear,
- if you copy additional code from `dream`, ensure your distribution remains license-compatible.

## Build

Use Visual Studio (solution already configured with `v143` toolset), or MSBuild.

## Usage

### Decode MDI to IQ

```powershell
.\x64\Debug\MDI_to_IQ.exe <input.mdi> <output.iq> --format f32
```

Options:

- `--format f32|s16` output sample format (default: `f32`)
- `--sample-rate <Hz>` metadata parameter printed to console (default: `48000`)
- `--strict-crc` fail immediately on CRC mismatch

### Generate a deterministic sample MDI file

```powershell
.\x64\Debug\MDI_to_IQ.exe --generate-sample .\sample.mdi --frames 4
```

Then decode it:

```powershell
.\x64\Debug\MDI_to_IQ.exe .\sample.mdi .\sample.iq --format f32
```

## Input/Output

- Input: MDI-compatible AF packet file (`AF` sync, TAG protocol type `T`)
- Output: interleaved IQ file:
  - `f32`: little-endian `float32` I,Q,I,Q...
  - `s16`: little-endian `int16` I,Q,I,Q...

