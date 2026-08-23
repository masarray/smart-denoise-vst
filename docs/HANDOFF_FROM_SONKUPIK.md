# Handoff from Sonkupik-Live

This package was prepared as a new-repository handoff after P0–P2 work had originally
been prototyped directly inside Sonkupik-Live.

## Donor reference

- Repo: `masarray/Sonkupik-Live`
- Branch: `agent/p0-profile-relative-expander`
- P0–P2 donor head: `2d86348e0f8cd699a8b05d2526c959f1946b6ac1`
- Draft PR: `#3`

The donor branch should be treated as the laboratory/reference implementation.
The new Smart Denoise repository should become the product authority.

## What was intentionally not copied

The handoff does not copy:

- Sonkupik visualizer;
- Smart Enhancer / ASK-P engine;
- Sonkupik routing/UI;
- license/trial UI;
- application tray code;
- unrelated presets;
- VST host slots.

## What was retained/refactored

- P0 profile-relative smart expander decisions;
- P1 7-group robust frozen noise profile;
- P1 profile quality / rejected Learn behavior;
- P1 profile persistence format concept;
- P2 decision-directed SNR;
- P2 profile confidence;
- P2 tonal-noise / harmonic distinction;
- P2 24 dB bounded spectral floor;
- P2 transient protection;
- P2 seven-bin frequency smoothing;
- P2 cross-frame smoothing;
- P2 stereo-linked gain map.

## Important

This extraction is deliberately cleaner than copying the entire donor branch. It is
therefore a refactor/handoff, not a byte-for-byte archive of Sonkupik-Live.
