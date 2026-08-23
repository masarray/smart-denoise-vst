# Smart Denoise domain context

## Product intent

Smart Denoise is a compact live/DAW spectral denoiser built around an **explicit frozen noise profile**. A user captures a few seconds of noise with Learn Noise, then the profile remains unchanged until Relearn.

This repository is the product authority. `masarray/Sonkupik-Live` is only the donor/reference history for P0–P2.

## Domain glossary

**Learn Noise** — explicit user action that captures roughly three seconds of noise-only audio.

**Frozen profile** — the learned per-bin noise power and variance after Learn. It never adapts in the background.

**Profile quality** — confidence score for a Learn, derived from accepted-frame ratio, level stability and spectral stability.

**Profile rejection** — a contaminated Learn does not replace a previous valid profile.

**Spectral reducer** — the STFT denoiser that calculates a stereo-linked gain map from the frozen profile.

**Decision-directed prior SNR** — P2 short-term signal estimate combining previous applied gain/posterior SNR with current posterior SNR. This memory may evolve every frame; it does not mutate the frozen noise profile.

**Stable tonal noise** — narrow-band energy close to the frozen profile, such as hum/buzz, that may receive slightly stronger suppression.

**Wanted harmonic** — tonal energy clearly above the profile, treated as likely music/voice structure and protected.

**Preserve** — user control that increases harmonic/transient/detail protection.

**Reduction** — maximum requested spectral attenuation. Range is 0–24 dB, but actual per-bin attenuation is limited by frequency weighting, profile confidence and program protection.

**Silence** — profile-relative smart-expander depth used only after the spectral reducer. It is not a full-band hard gate.

**Program presence** — shared P0 analysis estimating whether structured wanted content is present.

**Hear Removed** — monitor mode returning delayed dry minus denoised wet, for judging what is being removed.

**Live** — FFT 1024 quality mode.

**Clean** — FFT 2048 quality mode.

## Architecture

`SmartDenoiseEngine` is the primary deep module and the preferred behavioral test seam.

Behind that seam:

1. fixed FFT/STFT;
2. robust P1 profile learning;
3. profile persistence/compatibility;
4. P2 decision-directed spectral gain;
5. transient/harmonic protection;
6. time-frequency regularisation;
7. profile-relative P0 smart expander.

The plug-in processor is an adapter between host/APVTS state and the engine seam. The editor is an adapter for human control/display.

## Invariants

- No adaptive/MCRA/minimum-statistics mutation of the frozen profile.
- No hard-zero spectral bins.
- No audio-thread heap allocation or blocking mutex.
- Expander decisions use shared learned-profile analysis, not arbitrary full-band RMS thresholds.
- A bad Relearn preserves a previous valid profile.
- Profile restore requires compatible sample rate, FFT grid and physical channel count.
- Quality is a setup-level control. FFT/window rebuild must not occur as ordinary high-rate automation inside `processBlock`.
- VST3 + Standalone on Windows x64 is the first release target.

## Verification vocabulary

A release candidate is not "done" until:

1. source-contract QA passes;
2. mathematical P2 QA passes;
3. C++ engine black-box tests pass;
4. Windows VST3/Standalone build succeeds;
5. pluginval strictness 5 passes on the VST3;
6. release artifacts are produced by GitHub Actions.
