# Smart Denoise DSP Invariants

These invariants define behavior that must not regress accidentally.

## Learned profile

1. Noise Learn remains explicitly user-triggered unless a product decision intentionally changes the architecture.
2. A failed/contaminated/incompatible Learn candidate never destroys the previously valid learned profile.
3. Profile replacement is transactional: build candidate -> validate quality/compatibility -> safe commit.
4. Profile compatibility metadata (sample rate, FFT/channel assumptions and versioned shape) is validated before activation.
5. No valid profile means the documented safe behavior; do not invent arbitrary fallback thresholds.

## Signal preservation

6. Denoise must reduce noise without treating wanted transients, breath/consonants, harmonics, reverb tails, cymbals, or acoustic attacks as disposable noise by default.
7. Decision-directed prior/posterior SNR behavior, transient protection, wanted-harmonic discrimination, spectral floor, reduction ceiling, frequency regularisation, cross-frame regularisation, and stereo-link semantics do not change silently.
8. Invalid internal numeric state cannot propagate NaN/Infinity into plugin output.
9. Invalid DSP state falls back to a bounded documented safe state such as unity/dry/previous-valid gain where appropriate.
10. A sonic defect is not fixed by stacking arbitrary thresholds, clamps, smoothing, or final limiting without identifying the upstream cause.

## Real-time architecture

11. `processBlock` does not use exceptions as normal control flow.
12. `processBlock` does not perform blocking I/O, UI work, thread creation, blocking locks, logging, string formatting, or uncontrolled allocation/container growth.
13. FFT/window/filter/scratch resources required for steady state are prepared/preallocated outside the callback.
14. Large learned/profile state is prepared off-thread and swapped/committed with bounded safe synchronization.
15. No per-sample blocking lock is permitted.

## Failure handling and diagnostics

16. Expected failures use compact explicit Result/status/error codes where practical.
17. JUCE/OS/library exceptions are contained at non-real-time boundaries before state reaches `processBlock`.
18. Real-time diagnostics use compact fixed-size events/counters only.
19. Diagnostic queues/rings are bounded, non-blocking to audio, and have explicit saturation policy.
20. Repeated identical errors aggregate/deduplicate/rate-limit; diagnostic failure never becomes audio/host failure.

## State ownership

21. Learned noise data has one authoritative state owner.
22. UI/host automation cannot mutate complex DSP state unsafely while audio uses it.
23. Live and Clean modes do not become independent drifting implementations of product semantics.
24. Serialization/import cannot partially mutate active DSP state before validation succeeds.

## Regression discipline

25. DSP changes require deterministic/offline regression evidence where practical, not listening impression alone.
26. Tests include low-level/silence, quiet wanted signal, normal vocal, reverb tail, hum/tonal interference, transient-rich material and stereo behavior as applicable.
27. CPU/allocation improvements cannot silently alter sonic output outside accepted tolerances.
28. Dependency/JUCE upgrades are separate qualified changes, not incidental bug-fix edits.
29. Three successive compensating heuristic patches trigger root-cause/architecture re-audit.
30. Compile success alone is never proof that real-time denoise is correct or production-ready.