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
check("P4.1 keeps compact 940x540 footprint", "setSize (940, 540)" in editor_cpp)
check("Disciplined panel geometry", "captureBounds =\n        { 15, 77, 218, 330 }" in editor_cpp and "cleanBounds =\n        { 245, 77, 456, 330 }" in editor_cpp)
check("Panel renderer with layered depth", "void SmartDenoiseAudioProcessorEditor::drawPanel" in editor_cpp and "panelGradient" in editor_cpp)
check("Restrained violet-blue accents", "accentPurple" in editor_cpp and "accentBlue" in editor_cpp and "accentGradient" in editor_cpp)
check("Circular inline learn component", "class LearnCircleButton" in editor_h and ("fillEllipse (circle)" in editor_cpp or "fillEllipse (faceCircle)" in editor_cpp))
check("Learn progress ring", "ringProgress = progress" in editor_cpp and "progressArc.addCentredArc" in editor_cpp)
check("P4.2 Learn uses full 360-degree idle track", "g.drawEllipse (ringBounds, 7.0f);" in editor_cpp)
check("P4.2 Learn progress spans full two-pi", "juce::MathConstants<float>::twoPi" in editor_cpp and "ringProgress >= 0.999f" in editor_cpp)
check("P4.2 headphone has continuous headband silhouette", "headband.cubicTo" in editor_cpp and "leftCup" in editor_cpp and "rightCup" in editor_cpp)
check("Learn completes inline without popup", "drawLearnPopup" not in editor_cpp and "showLearnPopup" not in editor_cpp)
check("Learn profile ready state", '"Profile Ready"' in editor_cpp and '"Click to re-learn"' in editor_cpp)
check("Learn waveform icon", "drawWaveformIcon" in editor_cpp)
check("Primary macro has visual ticks", "tickCount = 25" in editor_cpp)
check("Primary macro shows percentage", 'juce::String (percent) + "%"' in editor_cpp)
check("Secondary knobs show percentages", "slider.getValue() * 100.0" in editor_cpp)
check("Primary active arc glow", "stroke + (primary ? 8.0f : 4.0f)" in editor_cpp)
check("Knob inner shading", "knobGradient" in editor_cpp)
check("Clean headphone icon", "void drawHeadphones" in editor_cpp and "Icon::headphones" in editor_h)
check("Clean bypass icon", "void drawBypass" in editor_cpp and "Icon::bypass" in editor_h)
check("No improvised broken icon fragments", "drawHeadphoneIcon" not in editor_cpp)
check("P3 telemetry visual bars", "drawTelemetry" in editor_cpp and "analysis.detailProtection" in editor_cpp and "analysis.tailProtection" in editor_cpp)
check("Segmented meters", "constexpr int segments = 20" in editor_cpp)
check("Meter scale ticks", "0, -12, -24, -36, -60" in editor_cpp)
check("Activity strip glow", "PathStrokeType (5.0f)" in editor_cpp and "inputHistory" in editor_h)
check("Activity center divider", "graph.getCentreX()" in editor_cpp)
check("Advanced drawer uses two-column split", "advancedBounds.getCentreX()" in editor_cpp and '"PROCESSING"' in editor_cpp and '"NOISE PROFILE"' in editor_cpp)
check("Advanced drawer removes tab/sidebar clutter", '"P3 Guard"' not in editor_cpp)
check("Advanced retains real Profile Offset", '"Profile Offset"' in editor_cpp and "profileOffset.setBounds" in editor_cpp)
check("Frozen profile authority visible", '"FROZEN / LOCKED"' in editor_cpp)
check("No fake Adaptive Mode", "Adaptive Mode" not in editor_cpp and "adaptive" not in editor_h.lower())
check("No new UI mutex", "mutex" not in editor_h.lower() and "mutex" not in editor_cpp.lower())
check("Existing processor parameter IDs retained", all(x in processor_cpp for x in ['"reduction"', '"preserve"', '"silence"', '"quality"', '"hearRemoved"', '"enabled"']))
check("Meter telemetry remains atomic", "std::atomic<float> inputPeakDb" in processor_h and "std::atomic<float> outputPeakDb" in processor_h)

failed = [name for name, ok in checks if not ok]

print("SMART DENOISE P4.2 VISUAL CORRECTION CONTRACT")
print("============================================")
for name, ok in checks:
    print(f"[{'PASS' if ok else 'FAIL'}] {name}")

print(f"Checks: {len(checks)}  Passed: {len(checks)-len(failed)}  Failed: {len(failed)}")
raise SystemExit(1 if failed else 0)
