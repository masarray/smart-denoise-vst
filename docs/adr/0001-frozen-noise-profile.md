# ADR 0001 — Explicit frozen noise profile

Status: accepted

## Context

Smart Denoise targets live vocal/music cleanup where a user can identify a representative noise-only section. Earlier research considered adaptive minimum-statistics/MCRA tracking.

## Decision

The product profile is captured only by explicit **Learn Noise** and remains frozen until **Relearn**.

Decision-directed prior-SNR state may evolve frame by frame because it estimates wanted-signal presence. It must not mutate the learned profile.

## Consequences

- Behavior is predictable and easy to A/B with Hear Removed.
- A contaminated environment requires deliberate Relearn.
- Profile persistence is meaningful across sessions when sample rate, FFT grid and channel count match.
- Adaptive MCRA/minimum-statistics profile tracking is out of scope unless this ADR is superseded.
