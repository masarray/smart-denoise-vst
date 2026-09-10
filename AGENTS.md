# AGENTS.md — Smart Denoise Production Engineering Contract

These rules apply to every AI/code agent working in this repository. Smart Denoise is a JUCE/C++ spectral DSP product. Signal integrity, transient preservation, deterministic real-time processing, learned-profile correctness, bounded CPU/memory, and regression safety are requirements from the first implementation.

## 1. Prime directive

Do not begin with a deliberately naive, disposable, prototype-only implementation when the production architecture is knowable.

Choose the smallest production-quality design that satisfies the requirement without speculative complexity.

Priority order:
1. audio/spectral correctness and signal integrity;
2. real-time safety and glitch prevention;
3. learned-profile/state correctness;
4. regression compatibility and sonic invariants;
5. bounded CPU, latency, and memory;
6. maintainability and testability.

Do not improve a meter or screenshot by changing DSP semantics without explicit evidence and acceptance criteria.

## 2. Mandatory engineering loop

For non-trivial work:

RECONNAISSANCE -> REPRODUCE/BASELINE -> ROOT CAUSE -> DSP/STATE INVARIANTS -> ARCHITECTURE IMPACT -> IMPLEMENT -> REGRESSION TEST -> FAILURE TEST -> CPU/ALLOCATION CHECK -> LISTENING/RENDER CHECK -> RELEASE BUILD

Before editing:
- identify whether the issue belongs to learning, spectral estimation, expander decision logic, gain-map generation, transient/harmonic protection, smoothing, plugin wrapper, serialization, or UI;
- trace state lifetime from `prepareToPlay`/initialization through `processBlock` and reset/reconfigure;
- locate previous-valid learned profile behavior and compatibility metadata;
- identify current QA fixtures/listening matrix and expected sonic behavior;
- determine root cause before stacking thresholds, floors, smoothing, limiters, or heuristics.

If an attempted fix fails, stop and re-audit assumptions rather than adding another compensating heuristic.

## 3. Architecture boundaries

Keep responsibilities separated:

Plugin/UI/control
-> validated parameter/state handoff
-> DSP orchestration
-> spectral analysis / learned model / denoise / expander core

Rules:
- DSP core must not depend on UI components;
- learned noise profile has one authoritative representation;
- Live/Clean FFT modes may differ in configured resolution but must share explicit semantics and compatibility rules;
- avoid duplicate signal-analysis pipelines that can drift apart;
- serialization is a boundary and must not mutate real-time state unsafely;
- preserve intentional product decisions such as static user-triggered Learn unless a task explicitly changes the product architecture.

## 4. Exception-free audio hot path

Exceptions must not be used as normal control flow in `processBlock` or functions directly reachable from the real-time processing path.

Expected/recoverable DSP conditions use compact status/result contracts rather than throw/catch.

Inside real-time processing NEVER perform:
- exception unwinding;
- blocking I/O/network/filesystem access;
- UI work;
- blocking locks/condition variables;
- thread creation;
- expensive initialization;
- uncontrolled allocation/deallocation or container growth;
- string formatting/JSON/stack traces/synchronous logging.

Preallocate and prepare FFTs, windows, overlap buffers, gain maps, scratch arrays, filter state, and other expensive resources outside the steady-state callback.

Use `noexcept` only where the complete reachable path genuinely honors it. Do not globally disable exceptions merely to satisfy this contract.

JUCE/OS/library exceptions may occur at non-real-time boundaries; catch them there and convert them to explicit application/DSP state before audio processing resumes.

## 5. Result-oriented failure handling

Expected failures should use explicit result/status values.

Examples:
- invalid/incompatible learned profile;
- failed Learn capture;
- insufficient valid frames;
- sample-rate/FFT/channel mismatch;
- invalid parameter state;
- non-finite input/intermediate value;
- serialization validation failure;
- unavailable optional resource.

Use a consistent error taxonomy rather than creating unrelated result classes per function.

When failure detail is not needed in the hot path, prefer a compact enum/status over heap-owning strings.

A failed Learn must not destroy a previously valid profile. Treat profile replacement transactionally:
CAPTURE/VALIDATE -> BUILD CANDIDATE -> VERIFY COMPATIBILITY/QUALITY -> ATOMIC/SAFE COMMIT -> otherwise RETAIN PREVIOUS VALID PROFILE.

When no valid profile exists, preserve the documented safe behavior (for example unity behavior where already specified) rather than inventing arbitrary thresholds.

## 6. Bounded asynchronous diagnostics

Audio/spectral hot paths may emit only compact machine-readable diagnostic events into a bounded non-blocking queue/ring.

A real-time diagnostic event should contain stable error code plus tiny numeric context only; do not allocate or format human-readable messages on the audio thread.

The diagnostic system must be:
- bounded;
- non-blocking for the audio producer;
- allocation-free on the producer path where practical;
- deduplicated/rate-limited/aggregated under repeated errors;
- observational only.

A lower-priority consumer may format/persist/display diagnostics.

Queue saturation must use an explicit drop/coalesce policy. Diagnostic failure must never become audio failure, dropout, deadlock, or host instability.

## 7. Spectral/DSP numerical safety

Defend explicitly against:
- NaN/Infinity/denormals;
- divide-by-zero/log-of-zero;
- invalid sample rate/block/channel count;
- incompatible FFT/profile dimensions;
- unstable smoothing/filter state;
- out-of-range SNR/probability/gain values;
- gain explosions or clipping;
- stale state after reset/reconfigure.

Clamp only where mathematically/product-appropriate. Do not use arbitrary clamps to hide an upstream defect.

Invalid internal state should fall back to bounded safe output (for example unity/dry/previous-valid gain state where appropriate) rather than propagating non-finite samples.

## 8. Preserve denoise product invariants

Unless explicitly changed by the task, preserve:
- static user-triggered Learn behavior;
- failed Learn retains previous valid profile;
- profile compatibility metadata;
- decision-directed prior/posterior SNR architecture;
- transient protection;
- wanted-harmonic versus stable tonal-noise discrimination;
- bounded spectral floor and user reduction ceiling;
- frequency and cross-frame regularisation;
- stereo-linked gain semantics where currently defined;
- fast release toward unity for attack preservation.

Do not silently replace the learned-noise architecture with adaptive/MCRA behavior.

Any change to thresholds/weights/smoothing must include evidence for noise reduction AND wanted-signal preservation.

## 9. Performance and allocation discipline

Spectral processing is performance-sensitive by default.

Avoid per-block/per-bin heap churn, duplicate FFT transforms, unnecessary full-spectrum copies, repeated window construction, and recalculating invariants that can be prepared/cached safely.

Prefer contiguous buffers, retained capacity, reused scratch arrays, and deterministic bounded loops.

Use SIMD/vectorization only after correctness tests and profiling identify a worthwhile hotspot. Preserve numerical/sonic tolerances and portable fallbacks.

Do not add threads or object pools speculatively. Parallel work must not create scheduling jitter that harms the host audio deadline.

## 10. Parameter/state synchronization

UI/host automation must not mutate complex DSP structures unsafely while `processBlock` is using them.

Use atomics, immutable snapshots, double-buffered/candidate-state commit, or another bounded synchronization mechanism appropriate to the state size.

Do not take blocking locks from `processBlock`.

Large learned profiles/settings must be prepared off the audio thread and swapped/committed safely.

## 11. Serialization and compatibility

Serialized learned profiles/settings must validate version, sample rate, FFT size, channel assumptions, lengths, ranges, and finite numeric values before becoming active.

Malformed persisted data must not crash the host or poison current DSP state.

Do not invent missing spectral/profile values silently. Return a structured incompatibility/failure and retain safe previous/default state.

## 12. Testing and listening discipline

Every DSP bug fix should add/update a deterministic regression fixture where practical.

Test not just average noise reduction but preservation of:
- silence behavior;
- very quiet speech/breath/consonants;
- normal vocal;
- reverb tails;
- 50/60 Hz hum and harmonics;
- cymbals/acoustic/transient-rich material;
- stereo image;
- `Hear Removed` behavior;
- reduction sweep;
- Live versus Clean mode.

Listening tests complement, not replace, deterministic offline renders, numerical checks, CPU profiling, and DAW/host validation.

## 13. Performance contract

For meaningful DSP changes, measure where practical:
- callback/block processing time versus deadline;
- sustained CPU load;
- allocation count/rate in steady state;
- latency;
- memory working set;
- non-finite sample count;
- output peak/headroom;
- reduction/gain-map behavior;
- Learn completion/failure behavior.

Do not claim optimization without before/after evidence.

## 14. Change discipline

Prefer the smallest coherent root-cause change.

Do not:
- stack arbitrary thresholds to fix one clip;
- rewrite the DSP because one edge case is difficult;
- mix unrelated UI/release refactoring into a DSP bug fix;
- duplicate analysis/profile state;
- upgrade JUCE or other dependencies incidentally;
- add logging to `processBlock` as a debugging shortcut.

Instrumentation for real-time defects must use bounded counters/snapshots/diagnostic events and be removable or production-safe.

## 15. Definition of done

A task is not complete because CMake succeeds.

Validate as applicable:
CLEAN RELEASE BUILD
+ UNIT/OFFLINE DSP TESTS
+ REGRESSION FIXTURES
+ MALFORMED/PROFILE FAILURE TESTS
+ STEADY-STATE ALLOCATION CHECK
+ CPU/LATENCY CHECK
+ NON-FINITE/HEADROOM CHECK
+ DAW/HOST VALIDATION
+ LISTENING MATRIX

Never claim a check was run when it was not.

## 16. Completion report

Report:
- Changed;
- Root cause;
- DSP/state architecture decision;
- Result/error contract affected;
- regression protection;
- measured CPU/allocation/sonic impact;
- exact validation executed;
- genuine remaining limitations.

## Final rule

Think like the engineer responsible for transparent real-time denoise across millions of audio blocks, not like a prototype generator tuning one test file.

Preserve wanted signal. Keep `processBlock` deterministic. Make expected failures explicit. Commit learned state transactionally. Keep diagnostics bounded and off the audio thread. Measure before claiming improvement.