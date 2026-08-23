# Changelog

## 0.4.0 - P4 Visual Fidelity

### Concept C premium polish
- Reworked the runtime skin to match the selected Concept C proposal more closely while keeping the proven three-step Capture → Clean → Check geometry.
- Replaced the flat bright Learn card with a dark glass-like hero action, violet/blue luminous border, waveform capture icon and softer depth treatment.
- Upgraded the primary Reduce Noise macro with a thicker gradient arc, glow pass, radial inner disc, visual ticks, white marker and explicit percentage readout.
- Preserve Detail and Silence Clean-up now use the same visual family and display percentages instead of raw 0..1 values.
- Improved typography hierarchy, spacing, panel depth, inner highlights and restrained graphite/navy surfaces.
- Added dedicated Hear Removed and Bypass icon language.
- Upgraded segmented input/output meters with scale ticks, glow and cleaner readouts.
- P3 Detail Guard and Tail Protect telemetry now use compact live status bars.
- Reworked the activity strip into a centered low-glow monitoring visualization.
- Polished Learn popup, Advanced drawer, quality selector and footer to use the same visual language.

### Product safety
- No DSP algorithm or parameter ID changes.
- Frozen Learn Noise profile remains the authority.
- P3 Detail Guard / Tail Protect remain telemetry/protection intelligence only.
- No mutex, file I/O or dynamic container was added to the audio callback.

### Validation / delivery
- Added deterministic P4 visual-fidelity source contract.
- CI continues to require P0-P3 QA, native Windows build, C++ black-box tests and pluginval strictness 5.
- Release workflow repeats Concept C + P4 gates before public publishing.
- Target public release: v0.4.0.

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
