# MDI_to_IQ

`MDI_to_IQ` is a command-line tool that:

1. reads a compatible MDI AF/TAG file,
2. decodes AF packets and key TAG items,
3. writes a complex I/Q output file.

## Standards Notes

- AF/TAG parsing follows the framing model from **ETSI TS 102 821** (AF header + TAG payload items).
- Decoded TAG items include `dlfc`, `robm`, `fac_`, `sdc_`, `str0..str3`.
- CRC handling follows the AF packet CRC behavior used in DRM tooling (CRC-16 polynomial `x^16 + x^12 + x^5 + 1`).

Important: full DRM RF waveform generation across the full transmitter chain described by **ETSI ES 201 980** is outside this first implementation scope. The current converter maps decoded payload bits to complex QPSK IQ symbols as a practical baseband output stage.

## Dream Project Source Reference (Licensing Hygiene)

This implementation was developed using the `dream` project source code as technical reference for AF/TAG and CRC behavior so that decoding behavior stays aligned with established DRM tooling.

Please keep license hygiene explicit:

- `dream` is GPL-licensed,
- attribution/reference is documented here,
- if additional direct source code is copied from `dream`, distribution must remain license-compatible.

## Build

Use Visual Studio (toolset `v143`) or MSBuild.

Tracked template scripts are available at `build.example.ps1`, `clean.example.ps1`, and `pcap_to_mdi.example.ps1`.
Create local scripts as `build.ps1`, `clean.ps1`, and `pcap_to_mdi.ps1` (these are ignored by git), then run them.

```powershell
Copy-Item .\build.example.ps1 .\build.ps1
Copy-Item .\clean.example.ps1 .\clean.ps1
Copy-Item .\pcap_to_mdi.example.ps1 .\pcap_to_mdi.ps1
powershell -ExecutionPolicy Bypass -File .\build.ps1 -Configuration Debug -Platform x64
```

## Usage

### Decode MDI to IQ

```powershell
.\x64\Debug\MDI_to_IQ.exe <input.mdi> <output.iq> --format f32
```

The converter also writes a sidecar metadata file `<output.iq>.iqmeta` (contract `MDI_IQ_META_V1`) used by `IQ_Transmitter` for format/sample-count validation.

Example with provided test input file:

```powershell
.\x64\Debug\MDI_to_IQ.exe .\MDI_to_IQ\__TEST_FILES__\mf.pcap.kimenet.mdi .\mf_test_output.iq --format f32
```

Options:

- `--format f32|s16` output sample format (default: `f32`)
- `--sample-rate <Hz>` metadata parameter printed to console (default: `48000`)
- `--strict-crc` fail on CRC mismatch

### Generate deterministic sample MDI

```powershell
.\x64\Debug\MDI_to_IQ.exe --generate-sample .\sample.mdi --frames 4
```

Decode that sample:

```powershell
.\x64\Debug\MDI_to_IQ.exe .\sample.mdi .\sample.iq --format f32
```

### Test helper: convert `.pcap` to `.mdi` first

For test captures with fragmented UDP/IP traffic, first extract/reassemble AF payloads.

Using Python directly:

```powershell
python -u .\pcap_to_mdi.py .\mf.pcap .\mf_from_pcap.mdi --dst-port 1234
```

Using the PowerShell template wrapper:

```powershell
powershell -ExecutionPolicy Bypass -File .\pcap_to_mdi.example.ps1 -InputPcap .\mf.pcap -OutputMdi .\mf_from_pcap.mdi -DstPort 1234
```

Then run the normal decoder:

```powershell
.\x64\Debug\MDI_to_IQ.exe .\mf_from_pcap.mdi .\mf_from_pcap.iq --format f32
```

### Validate IQ output stream

You can validate generated IQ files with the included checker:

```powershell
python -u .\iq_validate.py --self-test
python -u .\iq_validate.py .\sample_refactor.iq --format f32
```

To enforce expected sample count from decoder logs:

```powershell
python -u .\iq_validate.py .\mf_from_pcap.iq --format f32 --expected-complex 39854908
```

The script returns exit code `0` on PASS and `1` on FAIL.

## Input / Output Format

- Input: MDI AF packet file (`AF` sync and protocol type `T`)
- Output: interleaved IQ samples:
  - `f32`: little-endian `float32` I,Q,I,Q...
  - `s16`: little-endian `int16` I,Q,I,Q...
