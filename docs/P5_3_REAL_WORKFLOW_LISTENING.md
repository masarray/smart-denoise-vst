# P5.3 — Real Workflow & Listening Validation

## Purpose
P5.3 is a release-validation phase. It does not introduce a new denoise algorithm. It proves that the product workflow built in P5.2 behaves correctly as a real processor and creates a reproducible listening pack for final human approval.

## Product state machine
The validated workflow is:

```text
EMPTY
  -> Learn Noise
CAPTURING (monotonic progress)
  -> accepted Learn
ACTIVE (frozen profile + spectral fingerprint)
  -> save snapshot
SAVED (Profile Bank)
  -> load compatible snapshot or restore host session
RESTORED (profile + fingerprint + denoise tuning)
```

A contaminated Relearn must return to the previous valid ACTIVE profile. A saved Live/FFT1024 snapshot must not be silently loaded into Clean/FFT2048.

## Native product workflow test
`SmartDenoiseProductTests` instantiates the real `SmartDenoiseAudioProcessor`, processes deterministic audio through `processBlock`, and verifies:

- EMPTY profile/fingerprint state;
- partial and monotonic Learn progress;
- successful ACTIVE profile and fingerprint;
- Profile Bank file persistence;
- restore of Reduction, Preserve Detail, Silence Clean-up and Profile Offset;
- host session state restore;
- incompatible Live/Clean snapshot rejection;
- contaminated Relearn preserves the frozen profile byte-for-byte.

The test uses `SMART_DENOISE_PROFILE_BANK_DIR` only as an isolated CI/test directory override. Production behavior still defaults to the normal per-user application-data Profile Bank.

## Listening pack
`SmartDenoiseListeningHarness` generates deterministic mono WAV triplets:

1. clean reference;
2. noisy input;
3. Smart Denoise output.

Fixtures:

- stationary hiss;
- 50 Hz hum + harmonics;
- broadband fan/room noise;
- speech-like harmonic program + hiss;
- transient/cymbal-like attacks + hiss;
- guitar/pluck-like attacks + fan noise;
- decaying reverb tail + hiss.

`metrics.csv` contains conservative regression guards such as noise attenuation, finite output, peak retention and tail retention. These numbers are not a substitute for listening.

## Human listening gate
Listen level-matched and judge both removal and damage. Specifically check:

- musical-noise / chirpy residuals;
- watery or phasey texture;
- pumping or breathing;
- consonant loss;
- cymbal/transient softening;
- pluck/guitar attack damage;
- reverb-tail truncation;
- timbre shift.

P5.3 automation can say the implementation is reproducible and within safety bounds. Only a human listening pass can approve the release subjectively.
