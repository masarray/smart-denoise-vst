# Smart Denoise architecture — P0 to P2

## Product rule

The noise profile is **static** after user Learn.

There is no adaptive background profile update. Decision-directed SNR memory is
allowed because it estimates short-term signal presence; it does not mutate the
learned noise profile.

## Processing flow

```text
input
  -> 24 Hz subsonic cleanup
  -> STFT 1024/2048
  -> frozen robust noise profile
  -> posterior SNR
  -> decision-directed prior SNR
  -> random / stable-tonal classification
  -> bounded spectral attenuation
  -> harmonic/transient protection
  -> 7-bin frequency regularisation
  -> cross-frame target regularisation
  -> iSTFT
  -> profile-relative smart expander
  -> output
```

## P0 detector

The expander consumes the same spectral analysis produced by the denoiser.

Open candidates include:

- weighted excess above learned profile;
- active-band occupancy;
- transient confidence;
- harmonic/program structure.

Close requires all quiet conditions to remain true through a hold interval.

Profile unavailable = expander bypass. There is no synthetic broadband noise-floor
fallback.

## P1 learner

Learn defaults to 3 seconds.

Accepted frames are assigned to seven temporal groups. Each FFT bin is represented by
the robust centre of those group means. Short transient/high-level contamination is
rejected before accumulation.

Profile confidence is derived from:

- accepted-frame ratio;
- broadband level stability;
- spectral inter-group stability.

A failed Learn keeps the previous valid profile.

## P2 spectral core

For each bin:

```text
posterior = current_power / learned_noise_power
instant_prior = max(posterior - 1, 0)
previous_decision = previous_gain^2 * previous_posterior
prior = alpha * previous_decision + (1-alpha) * instant_prior
```

`alpha` becomes shorter on transient bins.

Stable tonal energy close to the learned profile is treated more like hum/buzz.
Tonal energy that rises clearly above the profile is treated more like wanted
harmonic content.

Reduction is bounded. The user range is 0–24 dB, but frequency weighting, profile
confidence, signal presence and Preserve prevent 24 dB from being applied uniformly
across the spectrum.

No FFT bin is hard-zeroed.
