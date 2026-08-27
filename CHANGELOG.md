# Changelog

## 0.4.0 - P4 Visual Fidelity

### Concept C premium polish
- Reworked the runtime skin to match the selected Concept C proposal more closely while keeping the proven three-step Capture → Clean → Check geometry.
- Replaced the old Learn card/popup workflow with a circular inline Learn control; no modal popup interrupts the capture flow.
- P4.2 makes the Learn progress authority a true 360-degree outer ring: the idle track is a full circle, capture progress can span the full two-pi circumference, and the 100% state is explicitly rendered as a closed ring.
- P4.2 redraws Hear Removed with a continuous over-ear headphone headband, stems and two padded ear cups so the icon remains recognizable at compact VST sizes.
- P4.3 upgrades all rotary controls to a premium dimensional material system: recessed illuminated track, lower physical shadow, machined metallic bezel, inner bevel, radial convex glass face, restrained specular reflection and a layered luminous value marker.
- P4.3 preserves the existing knob geometry and parameter mapping, so the added depth does not reopen layout collision or automation compatibility risks.
- Upgraded the primary Reduce Noise macro with a thicker gradient arc, glow pass, radial inner disc, visual ticks, white marker and explicit percentage readout.
- P4.4 retunes the knob family away from a spherical dome and toward a flatter machined-disc look with a recessed well, narrower bezel, brushed radial face texture and restrained sheen closer to premium hardware knobs.
- P4.5 matches the approved knob reference more aggressively: all remaining sphere/gloss cues and glossy marker dots are removed, the flat disc face is enlarged, the cavity/bezel are simplified, the active ring is crisper, and the face uses 144-line radial machining plus a directional satin sheen instead of dome-like specular highlights.
- P4.6 removes the translucent sheen-band artifact from every knob, moves 3D depth to the perimeter edge only, and makes Learn Noise visibly clickable with a raised circular rim, drop shadow, hover highlight and pressed-depth treatment.
- P5 removes deceptive A/B/Undo/Redo/Help chrome, removes the false profile dropdown affordance, adds professional tooltips and double-click resets, replaces redundant input/output history with real spectral-reduction activity, and compresses Advanced into one real control plus concise profile diagnostics.
- P5.1 gives the Advanced drawer collision-safe vertical space and compresses DSP ceiling copy into one line so all real diagnostics remain visible without overlapping.
- P5.3 adds native real-processor workflow validation (Empty → Capturing → Active → Saved → Restored), isolated Profile Bank persistence tests, incompatible-quality/rejected-relearn safety checks, and a deterministic WAV listening pack for hiss, hum, fan, speech-like program, transients, plucks and reverb tails.
- P5.2 adds a real Captured Profile Bank to the header. Every accepted Learn can create a timestamped snapshot containing the frozen noise profile plus Reduction, Preserve, Silence and Profile Offset, and compatible snapshots can be restored from the dropdown.
- P5.2 replaces the passive profile badge/health treatment with an explicit Empty → Capturing → Active workflow and a real 48-band spectral fingerprint generated from the learned profile itself.
- P5.2 removes failure-oriented `Try Again` language: a rejected Learn with an existing profile clearly reports that denoise remains active on the previous frozen profile, while an empty state guides the user to capture a noise-only moment.
- P5.2 fixes the Bypass glyph to use true 1:1 circular geometry rather than inheriting the rectangular icon area.
- Preserve Detail and Silence Clean-up use the same visual family and display percentages instead of raw 0..1 values.
- Improved typography hierarchy, disciplined spacing, panel depth, inner highlights and restrained graphite/navy surfaces.
- Upgraded segmented input/output meters with scale ticks, glow and cleaner readouts.
- P3 Detail Guard and Tail Protect telemetry use compact live status bars.
- Reworked the activity strip into a centered low-glow monitoring visualization.
- Simplified the Advanced drawer into a disciplined two-column layout using real controls/telemetry only.

### Product safety
- No DSP suppression algorithm or existing parameter ID changes.
- Frozen Learn Noise profile remains the authority.
- P3 Detail Guard / Tail Protect remain telemetry/protection intelligence only.
- P5.2 profile-bank file I/O runs from UI/message-thread operations only; no mutex, file I/O or dynamic container was added to the audio callback.
- Captured profiles are filtered by analysis quality so a frozen 1024-bin profile is not silently loaded onto an incompatible 2048 FFT grid, or vice versa.

### Validation / delivery
- P4.6 visual contract explicitly rejects translucent knob overlays, requires perimeter-only 3D knob edges with a flat machined face, and requires Learn Noise to render as a raised clickable circular button.
- P4.6 syntax scope was restored after the first MSVC pass exposed a missing brace in the machining clip block; final validation is run from a normal user commit.
- P5.2 adds a dedicated profile-workflow contract plus native black-box checks that both a learned profile and a restored profile publish a non-empty spectral fingerprint.
- P5.3 adds a permanent workflow/listening contract, a real processor state-machine test, reproducible listening WAV/metrics artifacts, and moves the large listening-test DSP engine off the Windows stack to avoid harness-only fast-fail without touching production DSP.
- P5.3 now includes a fast Windows validation lane plus fixture/prepare/learn/process/save checkpoints so listening-harness failures can be localized before the full VST3 + pluginval lane finishes.
- CI requires P0-P5.3 QA, native Windows build, engine + product workflow tests, mandatory listening validation pack, mandatory clean 940×540 runtime plugin screenshot and pluginval strictness 5.
- Release workflow repeats Concept C + P4 + P5 + P5.2 + P5.3 gates before public publishing.
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