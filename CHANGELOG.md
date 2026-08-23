# Changelog

## 0.3.0 - release candidate

### Smart Denoise P0-P2
- Profile-relative smart expander shares denoiser spectral intelligence.
- Robust explicit 3 second Learn Noise with seven temporal groups and contaminated-frame rejection.
- Frozen compatible profile persistence with quality/confidence tracking.
- Decision-directed prior SNR spectral core.
- Stable tonal-noise versus wanted-harmonic classification.
- Per-bin transient protection, seven-bin frequency smoothing and cross-frame target smoothing.
- Bounded 0-24 dB user reduction with finite spectral floor.

### P3 multi-resolution artifact suppression
- Added secondary 512-point / 256-hop Detail Guard analysis path with no additional reported latency.
- Added short-window transient and tonal-attack protection mapped onto the primary 1024/2048 FFT grid.
- Added approximately 240 ms decaying wanted-detail/reverb-tail memory.
- Added three-frame median gain consensus that only lifts isolated deep spectral holes and never creates extra attenuation.
- Detail/tail protection relaxes consensus on wanted attacks to reduce temporal smearing.
- Tail protection contributes bounded program presence so the smart expander is less likely to close on immediate wanted tails.
- P3 analysis never mutates the frozen learned noise profile.
- FFT/window quality reconfiguration is no longer performed from the real-time audio callback.

### Validation / delivery
- Added deterministic Python P0-P3 source and behavior QA.
- Added native Windows C++ black-box engine tests.
- Added VST3 + Standalone Windows GitHub Actions build.
- Added pluginval strictness 5 validation.
- Added automated Windows release packaging workflow.
