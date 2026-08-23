# AGENTS.md

## Engineering rules

- Read `CONTEXT.md` before changing DSP semantics.
- Prefer `SmartDenoiseEngine` as the highest behavioral test seam.
- Keep the audio callback allocation-free and lock-free. A non-real-time host-state operation may wait; the audio thread may not.
- Preserve the explicit frozen-profile product direction unless an ADR deliberately changes it.
- Add/adjust deterministic tests with behavioral changes.
- Run Python QA, C++ engine tests, Windows build and pluginval before declaring a release-ready change.
- Keep VST3/Standalone release packaging reproducible in GitHub Actions.

## Agent skills

This repo follows the Matt Pocock engineering-skill conventions.

- Issue tracker: see `docs/agents/issue-tracker.md`.
- Domain docs: see `docs/agents/domain.md` and `CONTEXT.md`.
- Architecture decisions: `docs/adr/`.
- Use the codebase-design vocabulary: module, interface, seam, adapter, leverage, locality.
- Work from a written spec/issue where possible, use TDD at the `SmartDenoiseEngine` seam, and perform a standards/spec code review before merge.
