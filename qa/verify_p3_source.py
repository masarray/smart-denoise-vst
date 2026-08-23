from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
header = (ROOT / "Source/DSP/SmartDenoiseEngine.h").read_text(encoding="utf-8")
source = (ROOT / "Source/DSP/SmartDenoiseEngine.cpp").read_text(encoding="utf-8")

checks = []


def check(name, condition):
    checks.append((name, bool(condition)))


check("512-point secondary detail FFT", "detailFftSize = 512" in header and "detailFft.configure (detailFftSize)" in source)
check("detail FFT is analysis-only", "processDetailFrame" in source and "detailFft.inverse" not in source)
check("detail cadence is 50 percent hop", "detailHopSize = detailFftSize / 2" in header)
check("detail ring is fixed-size", "detailInputRing" in header and "detailWork" in header)
check("detail protection telemetry", "frameDetailProtection" in header and "detailProtection" in header)
check("tail protection telemetry", "frameTailProtection" in header and "tailProtection" in header)
check("short-window transient detector", "riseRatio" in source and "detailProtectionState" in source)
check("short-window tonal attack detector", "tonalAttack" in source)
check("240 ms tail memory", "sampleRate * 0.240" in source and "detailTailMemory" in source)
check("detail maps to primary grid", "detailProtectionForPrimaryBin" in source and "tailProtectionForPrimaryBin" in source)
check("three-frame gain history", "gainHistoryOne" in header and "gainHistoryTwo" in header)
check("median gain consensus", "const float median = current + historyOne + historyTwo - minimum - maximum" in source)
check("consensus only lifts holes", "const float lifted = juce::jmax (current, median)" in source)
check("wanted detail relaxes consensus", "consensusStrength = 0.86f * (1.0f - wantedGuard)" in source)
check("tail informs program presence", "0.48f * averageTailProtection" in source)
check("quality reconfigure removed from callback", "void SmartDenoiseEngine::process (" in source and "// Quality/FFT reconfiguration is setup-level work" in source)

start = source.index("void SmartDenoiseEngine::processDetailFrame() noexcept")
end = source.index("void SmartDenoiseEngine::processFrame() noexcept", start)
detail_body = source[start:end]
check("detail analysis does not mutate learned profile", "profilePower" not in detail_body and "profileVarianceDb2" not in detail_body and "profileValid.store" not in detail_body)

process_start = source.index("void SmartDenoiseEngine::process (\n")
process_body = source[process_start:]
check("audio callback contains no dynamic vector", "std::vector" not in process_body)
check("audio callback contains no mutex", "mutex" not in process_body.lower())
check("P3 adds no reported latency", "getLatencySamples() const noexcept { return activeFftSize.load(); }" in header)

failed = [name for name, ok in checks if not ok]

print("SMART DENOISE P3 SOURCE CONTRACT")
print("================================")
for name, ok in checks:
    print(f"[{'PASS' if ok else 'FAIL'}] {name}")
print(f"Checks: {len(checks)}  Passed: {len(checks)-len(failed)}  Failed: {len(failed)}")
raise SystemExit(1 if failed else 0)
