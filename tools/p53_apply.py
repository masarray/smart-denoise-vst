from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

processor = ROOT / "Source/Plugin/PluginProcessor.cpp"
text = processor.read_text(encoding="utf-8")
old = '#include "PluginProcessor.h"\n#include "PluginEditor.h"\n'
new = '#include "PluginProcessor.h"\n#ifndef SMART_DENOISE_HEADLESS_PROCESSOR_TEST\n#include "PluginEditor.h"\n#endif\n'
if old not in text:
    raise RuntimeError("processor include anchor missing")
text = text.replace(old, new, 1)

old = '''juce::File SmartDenoiseAudioProcessor::getCapturedPresetDirectory() const\n{\n    return juce::File::getSpecialLocation (\n               juce::File::userApplicationDataDirectory)\n        .getChildFile ("Masarray")\n        .getChildFile ("Smart Denoise")\n        .getChildFile ("Captured Profiles");\n}\n'''
new = '''juce::File SmartDenoiseAudioProcessor::getCapturedPresetDirectory() const\n{\n    const auto testOverride = juce::SystemStats::getEnvironmentVariable (\n        "SMART_DENOISE_PROFILE_BANK_DIR", {});\n\n    if (testOverride.isNotEmpty())\n        return juce::File (testOverride);\n\n    return juce::File::getSpecialLocation (\n               juce::File::userApplicationDataDirectory)\n        .getChildFile ("Masarray")\n        .getChildFile ("Smart Denoise")\n        .getChildFile ("Captured Profiles");\n}\n'''
if old not in text:
    raise RuntimeError("profile directory anchor missing")
text = text.replace(old, new, 1)

old = '''juce::AudioProcessorEditor*\nSmartDenoiseAudioProcessor::createEditor()\n{\n    return new SmartDenoiseAudioProcessorEditor (*this);\n}\n'''
new = '''juce::AudioProcessorEditor*\nSmartDenoiseAudioProcessor::createEditor()\n{\n#ifdef SMART_DENOISE_HEADLESS_PROCESSOR_TEST\n    return nullptr;\n#else\n    return new SmartDenoiseAudioProcessorEditor (*this);\n#endif\n}\n'''
if old not in text:
    raise RuntimeError("createEditor anchor missing")
text = text.replace(old, new, 1)
processor.write_text(text, encoding="utf-8", newline="\n")

cmake = ROOT / "CMakeLists.txt"
text = cmake.read_text(encoding="utf-8")
anchor = '''    add_test(\n        NAME SmartDenoise.Engine\n        COMMAND SmartDenoiseTests\n    )\nendif()\n'''
replacement = '''    add_test(\n        NAME SmartDenoise.Engine\n        COMMAND SmartDenoiseTests\n    )\n\n    juce_add_console_app(SmartDenoiseProductTests\n        PRODUCT_NAME "Smart Denoise Product Tests"\n    )\n\n    target_sources(SmartDenoiseProductTests PRIVATE\n        ${SMART_DENOISE_ENGINE_SOURCES}\n        Source/Plugin/PluginProcessor.cpp\n        tests/SmartDenoiseProductTests.cpp\n    )\n\n    target_include_directories(SmartDenoiseProductTests PRIVATE Source)\n\n    target_compile_definitions(SmartDenoiseProductTests PRIVATE\n        JUCE_WEB_BROWSER=0\n        JUCE_USE_CURL=0\n        SMART_DENOISE_HEADLESS_PROCESSOR_TEST=1\n        "JucePlugin_Name=\\\"Smart Denoise\\\""\n    )\n\n    target_link_libraries(SmartDenoiseProductTests PRIVATE\n        juce::juce_audio_processors\n        PUBLIC\n        juce::juce_recommended_config_flags\n        juce::juce_recommended_warning_flags\n    )\n\n    add_test(\n        NAME SmartDenoise.ProductWorkflow\n        COMMAND SmartDenoiseProductTests\n    )\n\n    juce_add_console_app(SmartDenoiseListeningHarness\n        PRODUCT_NAME "Smart Denoise Listening Harness"\n    )\n\n    target_sources(SmartDenoiseListeningHarness PRIVATE\n        ${SMART_DENOISE_ENGINE_SOURCES}\n        tests/SmartDenoiseListeningHarness.cpp\n    )\n\n    target_include_directories(SmartDenoiseListeningHarness PRIVATE Source)\n\n    target_compile_definitions(SmartDenoiseListeningHarness PRIVATE\n        JUCE_WEB_BROWSER=0\n        JUCE_USE_CURL=0\n    )\n\n    target_link_libraries(SmartDenoiseListeningHarness PRIVATE\n        juce::juce_audio_basics\n        juce::juce_core\n        PUBLIC\n        juce::juce_recommended_config_flags\n        juce::juce_recommended_warning_flags\n    )\nendif()\n'''
if anchor not in text:
    raise RuntimeError("CMake test anchor missing")
text = text.replace(anchor, replacement, 1)
cmake.write_text(text, encoding="utf-8", newline="\n")

changelog = ROOT / "CHANGELOG.md"
text = changelog.read_text(encoding="utf-8")
anchor = '- P5.1 gives the Advanced drawer collision-safe vertical space and compresses DSP ceiling copy into one line so all real diagnostics remain visible without overlapping.\n'
insert = anchor + '- P5.3 adds native real-processor workflow validation (Empty → Capturing → Active → Saved → Restored), isolated Profile Bank persistence tests, incompatible-quality/rejected-relearn safety checks, and a deterministic WAV listening pack for hiss, hum, fan, speech-like program, transients, plucks and reverb tails.\n'
if anchor not in text:
    raise RuntimeError("changelog anchor missing")
if 'P5.3 adds native real-processor workflow validation' not in text:
    text = text.replace(anchor, insert, 1)
changelog.write_text(text, encoding="utf-8", newline="\n")

assert 'SMART_DENOISE_PROFILE_BANK_DIR' in processor.read_text(encoding='utf-8')
assert 'SmartDenoise.ProductWorkflow' in cmake.read_text(encoding='utf-8')
assert 'SmartDenoiseListeningHarness' in cmake.read_text(encoding='utf-8')
