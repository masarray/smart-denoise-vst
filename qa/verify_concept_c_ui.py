from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
editor_h = (ROOT / "Source/Plugin/PluginEditor.h").read_text(encoding="utf-8")
editor_cpp = (ROOT / "Source/Plugin/PluginEditor.cpp").read_text(encoding="utf-8")
processor_h = (ROOT / "Source/Plugin/PluginProcessor.h").read_text(encoding="utf-8")
processor_cpp = (ROOT / "Source/Plugin/PluginProcessor.cpp").read_text(encoding="utf-8")

checks = []

def check(name, condition):
    checks.append((name, bool(condition)))

check("Concept C base footprint", "setSize (940, 540)" in editor_cpp)
check("Three-step Capture section", '"CAPTURE"' in editor_cpp and "drawCaptureSection" in editor_cpp)
check("Three-step Clean section", '"CLEAN"' in editor_cpp and "drawCleanSection" in editor_cpp)
check("Three-step Check section", '"CHECK"' in editor_cpp and "drawCheckSection" in editor_cpp)
check("Primary Reduce Noise macro", '"Reduce Noise"' in editor_cpp and 'setName ("primary")' in editor_cpp)
check("Preserve Detail control", '"Preserve Detail"' in editor_cpp)
check("Silence Clean-up control", '"Silence Clean-up"' in editor_cpp)
check("Learn Noise 3s workflow", '"Learn Noise\\n3s"' in editor_h and "startNoiseLearn" in editor_cpp)
check("Frozen profile feedback", '"FROZEN PROFILE"' in editor_cpp and "Frozen" in editor_cpp)
check("Hear Removed monitoring", '"Hear Removed"' in editor_h and '"hearRemoved"' in editor_cpp)
check("Host-safe bypass uses existing enabled parameter", 'getParameter (\n                "enabled")' in editor_cpp)
check("P3 Detail Guard feedback", '"P3 DETAIL GUARD"' in editor_cpp and "analysis.detailProtection" in editor_cpp)
check("P3 Tail Protect feedback", '"TAIL PROTECT"' in editor_cpp and "analysis.tailProtection" in editor_cpp)
check("Input/output meter telemetry", "inputPeakDb" in processor_h and "outputPeakDb" in processor_h)
check("Meter telemetry is atomic", "std::atomic<float> inputPeakDb" in processor_h and "std::atomic<float> outputPeakDb" in processor_h)
check("Processor measures before and after DSP", "inputPeakDb.store" in processor_cpp and "outputPeakDb.store" in processor_cpp)
check("Activity strip", "drawActivityStrip" in editor_cpp and "inputHistory" in editor_h and "outputHistory" in editor_h)
check("Advanced drawer expands below compact UI", "shouldShow ? 700 : 540" in editor_cpp and "drawAdvancedDrawer" in editor_cpp)
check("Advanced exposes real Profile Offset", '"Profile Offset"' in editor_cpp and 'state, "thresholdOffset", profileOffset' in editor_cpp)
check("Quality selector uses existing quality parameter", 'state, "quality", quality' in editor_cpp)
check("No fake adaptive mode", "Adaptive Mode" not in editor_cpp and "adaptive" not in editor_h.lower())
check("No mutex introduced in UI telemetry", "mutex" not in processor_h.lower() and "mutex" not in processor_cpp.lower())

failed = [name for name, ok in checks if not ok]

print("SMART DENOISE CONCEPT C UI CONTRACT")
print("===================================")
for name, ok in checks:
    print(f"[{'PASS' if ok else 'FAIL'}] {name}")

print(f"Checks: {len(checks)}  Passed: {len(checks)-len(failed)}  Failed: {len(failed)}")
raise SystemExit(1 if failed else 0)
