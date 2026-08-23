# Smart Denoise — P0/P1/P2 source handoff

This repository is a clean extraction/refactor of the Smart Denoise work prototyped in
`masarray/Sonkupik-Live` on the P0–P2 denoise branch.

Source donor head used for the handoff:

- repository: `masarray/Sonkupik-Live`
- branch: `agent/p0-profile-relative-expander`
- donor head: `2d86348e0f8cd699a8b05d2526c959f1946b6ac1`
- donor PR: `#3`

The donor PR does **not** need to be merged into Sonkupik-Live to use this repository.

## What is included

### P0 — shared spectral intelligence + smart expander

The expander is not driven by a full-band RMS threshold. It uses the same learned
noise model as the spectral denoiser:

- weighted excess above learned noise;
- active-band occupancy;
- transient probability;
- harmonic probability;
- program-presence confidence;
- OPEN / HOLD / CLOSE hysteresis;
- no arbitrary `-72 dB` fallback when no profile exists.

If there is no valid learned profile, the expander returns to unity.

### P1 — robust static Learn Noise

Noise learning is explicitly user-triggered and frozen after capture.

- 3 second default Learn;
- Live FFT 1024;
- Clean FFT 2048;
- seven temporal groups;
- robust median-of-means profile;
- transient/high-level contaminated-frame rejection;
- profile variance/confidence;
- failed Learn does not destroy a previously valid profile;
- serialized profile contains sample-rate / FFT / channel compatibility metadata.

There is deliberately **no adaptive/MCRA noise-profile update** in this product direction.

### P2 — decision-directed spectral denoise

- posterior SNR + decision-directed prior SNR;
- shorter prior-memory on transients;
- profile-variance driven confidence;
- stable tonal-noise vs wanted-harmonic discrimination;
- bounded spectral floor;
- 0–24 dB user reduction ceiling;
- per-bin transient protection;
- seven-bin triangular frequency regularisation;
- cross-frame gain regularisation;
- stereo-linked gain map;
- fast release toward unity to protect attacks.

## Build

Requirements:

- CMake 3.22+
- Git
- Visual Studio 2022 with Desktop C++ workload on Windows

Example:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

The CMake file pins JUCE `8.0.15` for the initial handoff. Change
`SMART_DENOISE_JUCE_TAG` only after validating the build against another JUCE release.

## Current product status

This is a **P0–P2 engineering handoff**, not a release candidate.

The core DSP and plugin wrapper are present, but native Windows/JUCE build, real
audio listening tests, CPU profiling, DAW validation and installer/release workflow
still need to be performed in the new repository.

Recommended first listening matrix:

1. Learn 3 s of pure room/hiss noise.
2. Test silence.
3. Very quiet speech / breath / consonants.
4. Normal vocal.
5. Reverb tail.
6. 50/60 Hz hum plus harmonics.
7. Cymbals / acoustic guitar / transient-rich material.
8. Compare `Hear Removed`.
9. Sweep Reduction from 0 to 24 dB.
10. Compare Live 1024 vs Clean 2048.

## License

The source handoff preserves the GPL direction of the donor repository.
See `LICENSE`.
