from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
header = (ROOT / "Source/DSP/SmartDenoiseEngine.h").read_text(encoding="utf-8")
source = (ROOT / "Source/DSP/SmartDenoiseEngine.cpp").read_text(encoding="utf-8")
processor_cpp = (ROOT / "Source/Plugin/PluginProcessor.cpp").read_text(encoding="utf-8")
processor_h = (ROOT / "Source/Plugin/PluginProcessor.h").read_text(encoding="utf-8")
processor = processor_cpp + "\n" + processor_h

checks = []

def check(name, condition):
    checks.append((name, bool(condition)))

# P0
check("P0 shared frame analysis", "NoiseFrameAnalysis" in header)
check("P0 profile-relative excess", "weightedExcessDb" in source)
check("P0 occupancy", "activeBandRatio" in source)
check("P0 open/close hysteresis", "openCandidate" in source and "closeCandidate" in source)
check("P0 profile missing bypass", "! analysis.profileReady" in source)

# P1
check("P1 FFT 1024/2048", "live1024" in header and "clean2048" in header)
check("P1 seven temporal groups", "profileGroupCount = 7" in header)
check("P1 robust centre", "robustCentre" in source)
check("P1 contamination rejection", "transientScore > 0.82f" in source and "levelDeltaDb > 2.5f" in source and "levelDeltaDb > 7.0f" in source)
check("P1 profile quality", "candidateQuality >= 0.25f" in source)
check("P1 failed learn preserves profile", "profileValidBeforeLearning" in source)
check("P1 profile serialization", "serialiseProfile" in source and "restoreProfile" in source)

# P2
check("P2 24 dB ceiling", "maxReductionDb = 24.0f" in header)
check("P2 decision-directed prior", "previousDecision" in source and "instantaneousPrior" in source)
check("P2 transient-shortened memory", "0.94f" in source and "0.62f" in source)
check("P2 profile variance confidence", "noiseConfidence" in source and "profileVarianceDb2" in source)
check("P2 tonal/harmonic split", "stableTonalNoise" in source and "harmonicSignal" in source)
check("P2 finite spectral floor", "spectralFloor" in source)
check("P2 seven-bin smoothing", "1.0f / 16.0f" in source and "bin - 3" in source and "bin + 3" in source)
check("P2 cross-frame smoothing", "previousFrequencyGain" in source and "target2d" in source)
check("P2 stereo-linked map", "rightChannel" in source and "linkedPower" in source)

# Wrapper
check("VST wrapper has Learn command", "startNoiseLearn" in processor)
check("Wrapper exposes 24 dB range", "maxReductionDb" in processor)
check("No vector in audio engine", "std::vector" not in source)
check("No mutex in audio engine", "mutex" not in source.lower())

failed = [name for name, ok in checks if not ok]

print("SMART DENOISE P0-P2 CONTRACT")
print("============================")
for name, ok in checks:
    print(f"[{'PASS' if ok else 'FAIL'}] {name}")

print(f"Checks: {len(checks)}  Passed: {len(checks)-len(failed)}  Failed: {len(failed)}")
raise SystemExit(1 if failed else 0)