# Smart Denoise Real-Time Performance Budget

This document defines regression guardrails for the same reference environment/build. It does not claim universal CPU performance on every DAW or machine.

## 1. Audio callback deadline

For block size `N` and sample rate `Fs`:

`deadline_ms = 1000 * N / Fs`

Targets on the reference system:
- steady-state DSP should target <=50% of its available callback deadline;
- p99/worst observed callback processing approaching >=80% of the deadline requires investigation even without an observed dropout;
- no new sustained xrun/dropout is acceptable in the qualified host workflow.

## 2. Real-time allocations

After prepare/warm-up:
- target zero project-owned heap allocations inside steady-state `processBlock`;
- FFT/window/scratch/gain/profile working buffers are retained/reused;
- no logging/string/JSON or lazy initialization is allowed in `processBlock`;
- any unavoidable third-party allocation in the hot path must be measured and documented.

## 3. Algorithmic work

- Do not calculate duplicate transforms when an existing validated transform/result can be reused safely.
- Work per block must remain bounded by configured FFT/block/channel dimensions, not accumulated session length.
- Live and Clean quality changes must have explicit CPU/latency consequences.
- SIMD/vectorization is accepted only after correctness tests and profiling, with a portable fallback where required.

## 4. State/Learn budget

Learn/profile construction and validation must not perform heavy work on the audio callback.

Candidate learned state is prepared outside real-time processing and committed only after validation succeeds. Failed Learn retains previous valid state without blocking audio.

## 5. Diagnostic budget

- audio producer diagnostic work is bounded O(1);
- diagnostic queue/ring capacity is bounded;
- queue saturation never blocks `processBlock`;
- repeated events aggregate/deduplicate/rate-limit outside the audio callback;
- human-readable formatting/persistence is non-real-time work.

## 6. CPU/memory regression threshold

For the same Release build, host, sample rate, block size, channel count, quality mode, learned profile and test signal:
- callback p95/p99 should not regress >10% without explicit review;
- sustained CPU should not regress >10% without explicit review;
- steady/peak working set should not regress >10% without explicit review;
- steady-state allocation volume should not regress >10% without explicit review.

The 10% threshold is a review trigger, not an automatic rejection when a measured quality/correctness benefit justifies the cost.

## 7. Sonic-performance coupling

Performance optimization must preserve accepted sonic/numerical behavior. Measure both cost and signal result.

Relevant quality evidence may include:
- output peak/headroom;
- non-finite sample count;
- reduction/gain-map statistics;
- transient attenuation/recovery behavior;
- stereo-link behavior;
- Learn success/rejection behavior;
- offline render comparison/tolerance.

Do not improve CPU by silently reducing required spectral resolution or disabling protection logic.

## 8. Benchmark record

Performance-sensitive PRs should record:

```text
Base commit:
Candidate commit:
Machine/OS:
Host:
Release build:
Sample rate:
Block size:
Channels:
Mode (Live/Clean):
Learn profile/test signal:
Warm-up/run duration:

Metric                    Base       Candidate       Delta
---------------------------------------------------------
callback p50              ...        ...             ...
callback p95              ...        ...             ...
callback p99/max          ...        ...             ...
process CPU               ...        ...             ...
RT allocations            ...        ...             ...
working set               ...        ...             ...
xruns/dropouts            ...        ...             ...
non-finite outputs        ...        ...             ...
```

Do not compare Debug against Release or different signals/hosts and label the result an optimization.

## 9. Qualification rule

A performance claim requires measured before/after evidence. A denoise-quality claim requires signal/listening evidence. Production acceptance requires both when the change affects both.