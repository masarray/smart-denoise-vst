from pathlib import Path

root = Path(__file__).resolve().parents[1]
path = root / "tests" / "SmartDenoiseListeningHarness.cpp"
text = path.read_text(encoding="utf-8")

if "#include <memory>" not in text:
    text = text.replace("#include <limits>\n", "#include <limits>\n#include <memory>\n", 1)

old = '''    smartdenoise::SmartDenoiseEngine engine;\n    engine.prepare (sampleRate, blockSize, channels);\n    engine.setSilenceAmount (0.0f);\n    engine.setReductionDb (reductionDb);\n    engine.setPreserve (preserve);\n    learnProfile (engine, profileNoise);\n\n    if (! engine.hasProfile())\n        throw std::runtime_error (\"Listening fixture profile Learn was rejected\");\n\n    return { clean, noisy, processAligned (engine, noisy) };\n'''
new = '''    auto engine = std::make_unique<smartdenoise::SmartDenoiseEngine>();\n    engine->prepare (sampleRate, blockSize, channels);\n    engine->setSilenceAmount (0.0f);\n    engine->setReductionDb (reductionDb);\n    engine->setPreserve (preserve);\n    learnProfile (*engine, profileNoise);\n\n    if (! engine->hasProfile())\n        throw std::runtime_error (\"Listening fixture profile Learn was rejected\");\n\n    return { clean, noisy, processAligned (*engine, noisy) };\n'''

if old not in text:
    raise RuntimeError("runFixture stack-engine block not found")

text = text.replace(old, new, 1)
path.write_text(text, encoding="utf-8", newline="\n")

updated = path.read_text(encoding="utf-8")
assert "std::make_unique<smartdenoise::SmartDenoiseEngine>()" in updated
assert "smartdenoise::SmartDenoiseEngine engine;" not in updated
assert "#include <memory>" in updated
