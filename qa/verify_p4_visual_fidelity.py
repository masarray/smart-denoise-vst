from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
editor_cpp = (ROOT / "Source/Plugin/PluginEditor.cpp").read_text(encoding="utf-8")
editor_h = (ROOT / "Source/Plugin/PluginEditor.h").read_text(encoding="utf-8")
cmake = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
processor_h = (ROOT / "Source/Plugin/PluginProcessor.h").read_text(encoding="utf-8")
processor_cpp = (ROOT / "Source/Plugin/PluginProcessor.cpp").read_text(encoding="utf-8")

checks = []

def check(name, condition):
    checks.append((name, bool(condition)))

check("P4 release version", 'SMART_DENOISE_VERSION "0.4.0"' in cmake)
check("P4 keeps compact 940x540 footprint", "setSize (940, 540)" in editor_cpp)
check("Layered panel renderer", "void drawPanel (" in editor_cpp and "panelGradient" in editor_cpp)
check("Soft panel shadows", "juce::DropShadow" in editor_cpp)
check("Restrained violet-blue gradient", "accentPurple" in editor_cpp and "accentBlue" in editor_cpp and "accentGradient" in editor_cpp)
check("Hero learn card uses glass/dark treatment", 'name == "learn"' in editor_cpp and "accentPurple.withAlpha (0.20f)" in editor_cpp)
check("Learn capture waveform icon", "drawWaveformIcon" in editor_cpp)
check("Primary macro has visual ticks", "tickCount = 25" in editor_cpp)
check("Primary macro shows percentage", 'juce::String (percent) + "%"' in editor_cpp)
check("Secondary knobs show percentages", "slider.getValue()" in editor_cpp and '* 100.0' in editor_cpp)
check("Primary active arc has glow", "stroke + (primary ? 8.0f : 5.0f)" in editor_cpp)
check("Knob inner radial shading", "knobGradient" in editor_cpp and "true);" in editor_cpp)
check("Headphone monitoring icon", "drawHeadphoneIcon" in editor_cpp and 'hearRemoved.setName ("monitor")' in editor_cpp)
check("Bypass icon language", "drawBypassIcon" in editor_cpp and 'bypass.setName ("bypassAction")' in editor_cpp)
check("P3 telemetry uses visual bars", "drawTelemetry" in editor_cpp and "analysis.detailProtection" in editor_cpp and "analysis.tailProtection" in editor_cpp)
check("Segmented meters upgraded", "constexpr int segments = 20" in editor_cpp)
check("Meter scale ticks", "0, -12, -24, -36, -60" in editor_cpp)
check("Activity strip has glow pass", "PathStrokeType (5.0f)" in editor_cpp and "inputHistory" in editor_h)
check("Activity strip has center split", "graph.getCentreX()" in editor_cpp)
check("Learn popup uses matching panel language", "drawLearnPopup" in editor_cpp and "ui::drawPanel" in editor_cpp)
check("Advanced drawer retains real controls", "Profile Offset" in editor_cpp and "P3 detail / tail protection is automatic and profile-safe." in editor_cpp)
check("Frozen profile remains visible", "FROZEN PROFILE" in editor_cpp)
check("No fake Adaptive Mode", "Adaptive Mode" not in editor_cpp and "adaptive" not in editor_h.lower())
check("No new UI mutex", "mutex" not in editor_h.lower() and "mutex" not in editor_cpp.lower())
check("Existing processor parameters untouched by P4", '"reduction"' in processor_cpp and '"preserve"' in processor_cpp and '"silence"' in processor_cpp and '"quality"' in processor_cpp and '"hearRemoved"' in processor_cpp)
check("Meter telemetry remains atomic", "std::atomic<float> inputPeakDb" in processor_h and "std::atomic<float> outputPeakDb" in processor_h)

failed = [name for name, ok in checks if not ok]

print("SMART DENOISE P4 VISUAL FIDELITY CONTRACT")
print("========================================")
for name, ok in checks:
    print(f"[{'PASS' if ok else 'FAIL'}] {name}")

print(f"Checks: {len(checks)}  Passed: {len(checks)-len(failed)}  Failed: {len(failed)}")
raise SystemExit(1 if failed else 0)
