import math

def smooth_step(x):
    x = max(0.0, min(1.0, x))
    return x*x*(3.0 - 2.0*x)

def gain_model(posterior, previous_posterior, previous_gain,
               tonality, transient, profile_std_db, excess_db,
               reduction_db=8.0, preserve=0.75, weight=1.0):
    noise_confidence = 1.0 - smooth_step((profile_std_db - 2.5)/7.5)
    instant = max(0.0, posterior - 1.0)
    alpha = 0.94 + (0.62 - 0.94)*max(0.0, min(1.0, transient))
    prior = max(0.0, alpha*previous_gain*previous_gain*previous_posterior
                + (1.0-alpha)*instant)

    stable_tonal = max(0.0, min(1.0,
        tonality*noise_confidence*(1.0-smooth_step((excess_db-1.0)/7.0))))
    harmonic = max(0.0, min(1.0,
        tonality*smooth_step((excess_db-2.0)/7.5)))

    posterior_db = 10.0*math.log10(max(posterior, 1e-12))
    posterior_presence = smooth_step((posterior_db-1.5)/9.0)
    signal_presence = max(posterior_presence, 0.94*harmonic, 0.98*transient)

    base = prior/(prior+1.0)
    strength = 0.68 + (1.28-0.68)*(reduction_db/24.0)
    random_gain = max(0.0, min(1.0, base))**strength

    tonal_prior = prior*(1.0 + (0.38-1.0)*stable_tonal)
    tonal = tonal_prior/(tonal_prior+1.0)
    tonal_gain = max(0.0, min(1.0, tonal))**(strength*1.08)
    target = random_gain + stable_tonal*(tonal_gain-random_gain)

    floor_db = reduction_db
    floor_db *= 0.68 + 0.32*noise_confidence
    floor_db *= 1.0 + 0.08*stable_tonal
    floor_db *= 1.0 - 0.52*signal_presence*preserve
    floor_db = max(0.0, min(reduction_db*1.08, floor_db))
    floor_gain = 10.0**(-floor_db/20.0)

    target = max(floor_gain, target)
    target = 1.0 - weight*(1.0-target)
    target += (1.0-target)*(1.0-noise_confidence)*0.25

    protect = min(0.96,
        harmonic*(0.18+0.70*preserve)
        + transient*(0.24+0.68*preserve)
        + posterior_presence*0.10*preserve)
    target += (1.0-target)*protect

    return max(floor_gain, min(1.0, target))

def red(g):
    return -20.0*math.log10(max(g, 1e-12))

cases = {
    "broadband_default": gain_model(1.0, 0.0, 1.0, 0.05, 0.0, 1.0, 0.0),
    "tonal_noise_default": gain_model(1.0, 0.0, 1.0, 0.95, 0.0, 1.0, 0.0),
    "wanted_harmonic_default": gain_model(8.0, 4.0, 0.7, 0.90, 0.0, 1.0, 9.0),
    "transient_default": gain_model(5.0, 1.0, 0.7, 0.20, 1.0, 1.0, 7.0),
    "unstable_profile": gain_model(1.0, 0.0, 1.0, 0.2, 0.0, 9.0, 0.0),
    "broadband_max": gain_model(1.0, 0.0, 1.0, 0.05, 0.0, 1.0, 0.0, 24.0),
    "wanted_harmonic_max": gain_model(8.0, 4.0, 0.7, 0.90, 0.0, 1.0, 9.0, 24.0),
    "transient_max": gain_model(5.0, 1.0, 0.7, 0.20, 1.0, 1.0, 7.0, 24.0),
}

reductions = {k: red(v) for k, v in cases.items()}

checks = [
    ("default broadband reduction useful", 6.0 <= reductions["broadband_default"] <= 10.0),
    ("tonal noise not protected as harmonic",
     reductions["tonal_noise_default"] > reductions["broadband_default"]),
    ("wanted harmonic protected", reductions["wanted_harmonic_default"] < 1.5),
    ("transient protected", reductions["transient_default"] < 1.0),
    ("unstable profile conservative",
     reductions["unstable_profile"] < reductions["broadband_default"] - 2.0),
    ("24 dB broadband can go deep", reductions["broadband_max"] > 20.0),
    ("24 dB harmonic still protected", reductions["wanted_harmonic_max"] < 1.5),
    ("24 dB transient still protected", reductions["transient_max"] < 1.0),
]

print("SMART DENOISE P2 MATH")
print("=====================")
failed = []
for name, ok in checks:
    print(f"[{'PASS' if ok else 'FAIL'}] {name}")
    if not ok:
        failed.append(name)

print("\nReductions:")
for name, value in reductions.items():
    print(f"  {name}: {value:.2f} dB")

raise SystemExit(1 if failed else 0)
