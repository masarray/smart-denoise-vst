# Smart Denoise P3 — Multi-Resolution Detail Guard

## Purpose

P3 reduces musical-noise / chirpy / watery artifacts without replacing the frozen P1 noise profile and without adding plug-in latency.

The P2 STFT remains the audio reconstruction authority. P3 adds a second, shorter analysis scale that is used only to protect wanted detail and stabilize the P2 gain map.

## Signal path

```text
input
  |-- primary STFT 1024 / 2048 --------------------------|
  |     frozen profile -> P2 gain -> P3 consensus -> iFFT| -> output
  |
  `-- secondary FFT 512 / hop 256 -> detail + tail guard-'
```

The 512-point path never performs inverse FFT and never writes audio. Therefore reported latency remains the primary FFT latency: 1024 samples in Live mode and 2048 samples in Clean mode.

## 512-point Detail Guard

The secondary analysis runs at 50% overlap. For each stereo-linked short-window bin it tracks:

- positive magnitude rise;
- local spectral tonality;
- a fast attack / slower release protection state;
- a 240 ms decaying tail memory.

This path intentionally does not compare against or update the learned noise profile. It answers a different question: **does this bin currently look like fresh wanted detail or the continuation of recent wanted detail?**

The short-bin protection mask is linearly mapped onto the primary FFT grid. P2 uses the mapped protection to shorten decision-directed memory and to release attenuation toward unity faster.

## Three-frame gain consensus

P2 already performs seven-bin frequency smoothing and asymmetric temporal smoothing. P3 adds a robust three-frame consensus before that temporal smoother.

For every primary bin:

1. take the current frequency-smoothed gain and the previous two targets;
2. compute the median target;
3. only lift the current gain toward that median;
4. never use consensus to create deeper attenuation;
5. reduce/disable consensus when the Detail Guard or tail memory says wanted detail is present.

This specifically attacks isolated one-frame deep gain holes, a major cause of metallic/chirpy residuals.

## Reverb-tail memory

A strong short-window event seeds a bounded tail memory with an approximately 240 ms time constant. The tail memory:

- relaxes gain consensus;
- contributes to program-presence telemetry;
- helps keep the profile-relative smart expander from closing on the immediate tail of wanted material.

The memory always decays. It is not a latch and it is not a noise estimator.

## Frozen-profile guarantee

P3 does not write:

- `profilePower`;
- `profileVarianceDb2`;
- `profileValid`;
- persisted profile metadata.

Learn/Relearn remains the only operation that changes the noise profile.

## Real-time constraints

P3 adds fixed-size arrays only. The audio callback still has:

- no mutex;
- no file I/O;
- no heap allocation;
- no FFT/window configuration;
- no profile serialization;
- no dynamic containers.

The additional cost is one 512-point analysis FFT every 256 samples plus O(N) short-bin classification and primary-bin consensus arithmetic.

## Acceptance tests

P3 is not considered complete until all of the following are green:

1. P0/P1/P2 source contract.
2. P2 mathematical behavior model.
3. P3 source contract.
4. P3 consensus/tail behavior model.
5. Native Windows VST3 + Standalone compile.
6. C++ black-box engine tests.
7. pluginval strictness 5.
8. Listening test for speech consonants, cymbals, guitar attacks, reverb tails, stationary hiss and hum.
