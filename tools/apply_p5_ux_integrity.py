from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CPP = ROOT / 'Source/Plugin/PluginEditor.cpp'
H = ROOT / 'Source/Plugin/PluginEditor.h'
P4 = ROOT / 'qa/verify_p4_visual_fidelity.py'
CI = ROOT / '.github/workflows/source-audit.yml'
REL = ROOT / '.github/workflows/release.yml'
CHANGELOG = ROOT / 'CHANGELOG.md'


def replace_once(text, old, new, name):
    if old not in text:
        raise RuntimeError(f'missing anchor: {name}')
    return text.replace(old, new, 1)

h = H.read_text(encoding='utf-8')
h = replace_once(h, '''    juce::TextButton abButton { "A / B" };\n    juce::TextButton undoButton { "UNDO" };\n    juce::TextButton redoButton { "REDO" };\n    juce::TextButton helpButton { "?" };\n\n''', '', 'fake header buttons')
h = replace_once(h, '''    std::array<float, 112> inputHistory {};\n    std::array<float, 112> outputHistory {};\n\n    float displayedInputDb = -72.0f;\n''', '''    std::array<float, 112> reductionHistory {};\n    juce::TooltipWindow tooltipWindow { nullptr, 450 };\n\n    float displayedInputDb = -72.0f;\n''', 'history arrays')
H.write_text(h, encoding='utf-8', newline='\n')

s = CPP.read_text(encoding='utf-8')
tick_start = s.index('    if (primary)\n    {\n        constexpr int tickCount = 25;')
tick_end = s.index('    juce::Path baseArc;', tick_start)
s = s[:tick_start] + s[tick_end:]
s = replace_once(s, '''    abButton.setName ("top");\n    undoButton.setName ("top");\n    redoButton.setName ("top");\n    helpButton.setName ("top");\n    advanced.setName ("footer");\n''', '''    advanced.setName ("footer");\n''', 'fake header names')
s = replace_once(s, '''    abButton.setInterceptsMouseClicks (\n        false, false);\n    undoButton.setInterceptsMouseClicks (\n        false, false);\n    redoButton.setInterceptsMouseClicks (\n        false, false);\n\n''', '', 'fake header click interception')
s = replace_once(s, 'const std::array<juce::Component*, 20> components {', 'const std::array<juce::Component*, 16> components {', 'component count')
for fake in ['        &abButton,\n', '        &undoButton,\n', '        &redoButton,\n', '        &helpButton,\n']:
    s = replace_once(s, fake, '', f'component {fake.strip()}')

anchor = '''    qualityAttachment =\n        std::make_unique<ComboAttachment> (\n            state,\n            "quality",\n            quality);\n\n'''
insert = anchor + '''    reduction.setDoubleClickReturnValue (true, 8.0);\n    preserve.setDoubleClickReturnValue (true, 0.75);\n    silence.setDoubleClickReturnValue (true, 0.55);\n    profileOffset.setDoubleClickReturnValue (true, 1.5);\n\n    learn.setTooltip (\n        "Capture 3 seconds of noise only. The learned profile stays frozen until you re-learn it.");\n    reduction.setTooltip (\n        "Noise reduction strength. Double-click to reset to 8 dB.");\n    preserve.setTooltip (\n        "Protects speech harmonics, consonants and transient detail. Double-click to reset.");\n    silence.setTooltip (\n        "Controls quiet-region clean-up after spectral denoising. Double-click to reset.");\n    hearRemoved.setTooltip (\n        "Monitor only what Smart Denoise is removing.");\n    bypass.setTooltip (\n        "Bypass Smart Denoise without changing the learned noise profile.");\n    advanced.setTooltip (\n        "Open the compact expert controls and profile diagnostics.");\n    quality.setTooltip (\n        "Live 1024 uses lower latency; Clean 2048 uses higher spectral resolution.");\n    profileOffset.setTooltip (\n        "Fine-tunes the learned profile threshold. Double-click to reset to +1.5 dB.");\n\n'''
s = replace_once(s, anchor, insert, 'attachments/tooltips')
help_start = s.index('    helpButton.onClick =')
help_end = s.index('    bypass.onClick =', help_start)
s = s[:help_start] + s[help_end:]
s = replace_once(s, '''    setSize (\n        940,\n        shouldShow ? 700 : 540);\n''', '''    setSize (\n        940,\n        shouldShow ? 660 : 540);\n''', 'advanced height')

header_start = s.index('void SmartDenoiseAudioProcessorEditor::\ndrawHeader')
header_end = s.index('void SmartDenoiseAudioProcessorEditor::\ndrawCaptureSection', header_start)
new_header = '''void SmartDenoiseAudioProcessorEditor::\ndrawHeader (\n    juce::Graphics& g)\n{\n    drawPanel (g, headerBounds, true);\n\n    auto logoArea = juce::Rectangle<float> (\n        static_cast<float> (headerBounds.getX() + 15),\n        static_cast<float> (headerBounds.getY() + 11),\n        28.0f, 30.0f);\n\n    ui::drawWaveformIcon (\n        g, logoArea, ui::accentPurple.brighter (0.22f), 1.7f);\n}\n\n'''
s = s[:header_start] + new_header + s[header_end:]

chevron_start = s.index('    ui::drawChevron (\n        g,\n        { profilePill.getRight() - 12.0f,')
chevron_end = s.index('\n\n    g.setColour (ui::textSecondary);', chevron_start)
s = s[:chevron_start] + s[chevron_end:]

clean_start = s.index('void SmartDenoiseAudioProcessorEditor::\ndrawCleanSection')
clean_end = s.index('void SmartDenoiseAudioProcessorEditor::\ndrawCheckSection', clean_start)
new_clean = '''void SmartDenoiseAudioProcessorEditor::\ndrawCleanSection (\n    juce::Graphics& g)\n{\n    drawPanel (g, cleanBounds, true);\n    drawStepHeader (\n        g,\n        juce::Rectangle<int> (cleanBounds.getX() + 14, cleanBounds.getY() + 12, cleanBounds.getWidth() - 28, 23),\n        2, "CLEAN");\n\n    g.setColour (ui::borderSoft.withAlpha (0.62f));\n    g.drawVerticalLine (\n        cleanBounds.getX() + 302,\n        static_cast<float> (cleanBounds.getY() + 52),\n        static_cast<float> (cleanBounds.getBottom() - 20));\n\n    const float normalizedReduction = ui::clamp01 (\n        static_cast<float> (reduction.getValue() / smartdenoise::SmartDenoiseEngine::maxReductionDb));\n\n    juce::String character = "Gentle";\n    if (normalizedReduction > 0.68f) character = "Strong";\n    else if (normalizedReduction > 0.30f) character = "Moderate";\n\n    g.setColour (ui::textSecondary);\n    g.setFont (juce::FontOptions (9.4f));\n    g.drawText (\n        character, reduction.getX(), reduction.getBottom() - 22, reduction.getWidth(), 17,\n        juce::Justification::centred);\n}\n\n'''
s = s[:clean_start] + new_clean + s[clean_end:]
s = s.replace('        "DETAIL GUARD",\n        analysis.detailProtection);', '        "DETAIL GUARD  AUTO",\n        analysis.detailProtection);', 1)
s = s.replace('        "TAIL PROTECT",\n        analysis.tailProtection);', '        "TAIL PROTECT  AUTO",\n        analysis.tailProtection);', 1)

activity_start = s.index('void SmartDenoiseAudioProcessorEditor::\ndrawActivityStrip')
activity_end = s.index('void SmartDenoiseAudioProcessorEditor::\ndrawFooter', activity_start)
new_activity = '''void SmartDenoiseAudioProcessorEditor::\ndrawActivityStrip (\n    juce::Graphics& g)\n{\n    drawPanel (g, activityBounds, false);\n    auto graph = activityBounds.reduced (86, 12).toFloat();\n\n    g.setColour (ui::borderSoft.withAlpha (0.62f));\n    g.drawLine (graph.getX(), graph.getBottom() - 2.0f, graph.getRight(), graph.getBottom() - 2.0f, 1.0f);\n\n    juce::Path path;\n    for (size_t i = 0; i < reductionHistory.size(); ++i)\n    {\n        const float norm = ui::clamp01 (\n            reductionHistory[i] / smartdenoise::SmartDenoiseEngine::maxReductionDb);\n        const float px = graph.getX() + graph.getWidth() * static_cast<float> (i)\n            / static_cast<float> (reductionHistory.size() - 1);\n        const float py = graph.getBottom() - 3.0f - norm * (graph.getHeight() - 6.0f);\n        if (i == 0) path.startNewSubPath (px, py); else path.lineTo (px, py);\n    }\n\n    g.setColour (ui::accentPurple.withAlpha (0.075f));\n    g.strokePath (path, juce::PathStrokeType (4.0f));\n    g.setGradientFill (ui::accentGradient (graph));\n    g.strokePath (path, juce::PathStrokeType (1.35f));\n\n    g.setColour (ui::textMuted);\n    g.setFont (juce::FontOptions (8.8f));\n    g.drawText (\n        "Denoise activity", activityBounds.getX() + 14, activityBounds.getCentreY() - 9, 70, 18,\n        juce::Justification::centredLeft);\n    g.setColour (ui::textSecondary);\n    g.drawText (\n        juce::String (reductionHistory.back(), 1) + " dB",\n        activityBounds.getRight() - 70, activityBounds.getCentreY() - 9, 55, 18,\n        juce::Justification::centredRight);\n}\n\n'''
s = s[:activity_start] + new_activity + s[activity_end:]

adv_start = s.index('void SmartDenoiseAudioProcessorEditor::\ndrawAdvancedDrawer')
adv_end = s.index('void SmartDenoiseAudioProcessorEditor::\ndrawTelemetry', adv_start)
new_adv = '''void SmartDenoiseAudioProcessorEditor::\ndrawAdvancedDrawer (\n    juce::Graphics& g)\n{\n    drawPanel (g, advancedBounds, true);\n    const auto& engine = processor.getEngine();\n    const auto qualityIndex = static_cast<int> (\n        processor.getParameters().getRawParameterValue ("quality")->load());\n\n    auto left = juce::Rectangle<int> (\n        advancedBounds.getX() + 18, advancedBounds.getY() + 43,\n        advancedBounds.getWidth() / 2 - 34, advancedBounds.getHeight() - 55);\n    auto right = juce::Rectangle<int> (\n        advancedBounds.getCentreX() + 18, advancedBounds.getY() + 43,\n        advancedBounds.getWidth() / 2 - 36, advancedBounds.getHeight() - 55);\n\n    g.setColour (ui::borderSoft.withAlpha (0.52f));\n    g.drawVerticalLine (\n        advancedBounds.getCentreX(),\n        static_cast<float> (advancedBounds.getY() + 42),\n        static_cast<float> (advancedBounds.getBottom() - 14));\n\n    g.setColour (ui::textSecondary);\n    g.setFont (juce::FontOptions (9.2f));\n    g.drawText ("PROCESSING", left.removeFromTop (18), juce::Justification::centredLeft);\n    left.removeFromTop (42);\n    g.setColour (ui::textMuted);\n    g.setFont (juce::FontOptions (8.5f));\n    g.drawText (\n        "Fine-tune the learned profile threshold. Double-click resets it.",\n        left.removeFromTop (18), juce::Justification::centredLeft);\n    g.drawText ("DSP CEILING", left.removeFromTop (16), juce::Justification::centredLeft);\n    g.setColour (ui::textSecondary);\n    g.setFont (juce::FontOptions (10.0f));\n    g.drawText ("24 dB max reduction", left.removeFromTop (18), juce::Justification::centredLeft);\n\n    g.setColour (ui::textSecondary);\n    g.setFont (juce::FontOptions (9.2f));\n    g.drawText ("PROFILE STATUS", right.removeFromTop (18), juce::Justification::centredLeft);\n    g.setColour (engine.hasProfile() ? ui::accentCyan : ui::textMuted);\n    g.setFont (juce::FontOptions (10.8f));\n    g.drawText (\n        engine.hasProfile() ? "FROZEN / LOCKED" : "NOT LEARNED",\n        right.removeFromTop (21), juce::Justification::centredLeft);\n\n    g.setColour (ui::textMuted);\n    g.setFont (juce::FontOptions (8.6f));\n    const juce::String profileQuality = engine.hasProfile()\n        ? juce::String (juce::roundToInt (engine.getProfileQuality() * 100.0f)) + "%" : "--";\n    g.drawText ("Profile quality   " + profileQuality, right.removeFromTop (18), juce::Justification::centredLeft);\n    g.drawText (\n        "Analysis quality  " + juce::String (qualityIndex == 1 ? "Clean 2048" : "Live 1024"),\n        right.removeFromTop (18), juce::Justification::centredLeft);\n    g.drawText (\n        "Latency           " + juce::String (engine.getLatencySamples()) + " samples",\n        right.removeFromTop (18), juce::Justification::centredLeft);\n}\n\n'''
s = s[:adv_start] + new_adv + s[adv_end:]

bs = s.index('    abButton.setBounds (')
be = s.index('    learn.setBounds (', bs)
s = s[:bs] + s[be:]
s = replace_once(s, '''        228,\n        37);\n''', '''        340,\n        37);\n''', 'title width')
s = replace_once(s, '''            captureBounds.getWidth() - 82,\n            26);\n''', '''            captureBounds.getWidth() - 64,\n            26);\n''', 'profile name width')
s = replace_once(s, '''        advancedBounds =\n            { 15, 538, 910, 146 };\n''', '''        advancedBounds =\n            { 15, 538, 910, 106 };\n''', 'advanced bounds')
s = replace_once(s, '''            advancedBounds.getY() + 69,\n            102,\n            23);\n\n        profileOffset.setBounds (\n            advancedBounds.getX() + 125,\n            advancedBounds.getY() + 67,\n            286,\n            27);\n''', '''            advancedBounds.getY() + 62,\n            102,\n            23);\n\n        profileOffset.setBounds (\n            advancedBounds.getX() + 125,\n            advancedBounds.getY() + 60,\n            286,\n            27);\n''', 'advanced profile offset geometry')

hist_start = s.index('    std::move (\n        inputHistory.begin() + 1,')
hist_end = s.index('    learn.setLearnState (', hist_start)
replacement = '''    auto& engine =\n        processor.getEngine();\n\n    const auto frameAnalysis = engine.getFrameAnalysis();\n\n    std::move (\n        reductionHistory.begin() + 1,\n        reductionHistory.end(),\n        reductionHistory.begin());\n\n    reductionHistory.back() = juce::jlimit (\n        0.0f,\n        smartdenoise::SmartDenoiseEngine::maxReductionDb,\n        frameAnalysis.spectralReductionDb);\n\n'''
s = s[:hist_start] + replacement + s[hist_end:]
CPP.write_text(s, encoding='utf-8', newline='\n')

p4 = P4.read_text(encoding='utf-8')
p4 = p4.replace('check("Primary macro has visual ticks", "tickCount = 25" in editor_cpp)\n', 'check("P5 removes noisy macro tick halo", "tickCount = 25" not in editor_cpp)\n')
p4 = p4.replace('check("Activity strip glow", "PathStrokeType (5.0f)" in editor_cpp and "inputHistory" in editor_h)\n', 'check("P5 real denoise activity strip", "reductionHistory" in editor_h and "spectralReductionDb" in editor_cpp and "Denoise activity" in editor_cpp)\n')
p4 = p4.replace('check("Activity center divider", "graph.getCentreX()" in editor_cpp)\n', 'check("P5 activity strip is denoise-specific", "Denoise activity" in editor_cpp)\n')
P4.write_text(p4, encoding='utf-8', newline='\n')

p5 = ROOT / 'qa/verify_p5_ux_integrity.py'
p5.write_text('''from pathlib import Path\nROOT = Path(__file__).resolve().parents[1]\ncpp = (ROOT / "Source/Plugin/PluginEditor.cpp").read_text(encoding="utf-8")\nh = (ROOT / "Source/Plugin/PluginEditor.h").read_text(encoding="utf-8")\nprocessor = (ROOT / "Source/Plugin/PluginProcessor.cpp").read_text(encoding="utf-8")\nchecks=[]\ndef check(name, condition): checks.append((name, bool(condition)))\nactivity = cpp[cpp.index("drawActivityStrip"):cpp.index("drawFooter")]\nadvanced = cpp[cpp.index("drawAdvancedDrawer"):cpp.index("drawTelemetry")]\ncheck("No fake A/B control", "abButton" not in cpp and "abButton" not in h)\ncheck("No fake Undo control", "undoButton" not in cpp and "undoButton" not in h)\ncheck("No fake Redo control", "redoButton" not in cpp and "redoButton" not in h)\ncheck("No fake/miswired Help control", "helpButton" not in cpp and "helpButton" not in h)\ncheck("No disabled-click fake utilities", "setInterceptsMouseClicks" not in cpp)\ncheck("Profile capsule has no false dropdown affordance", "profilePill.getRight() - 12.0f" not in cpp)\ncheck("Real controls have contextual tooltips", all(x in cpp for x in ["learn.setTooltip", "reduction.setTooltip", "preserve.setTooltip", "silence.setTooltip", "hearRemoved.setTooltip", "quality.setTooltip", "profileOffset.setTooltip"]))\ncheck("Rotary controls support double-click reset", all(x in cpp for x in ["reduction.setDoubleClickReturnValue", "preserve.setDoubleClickReturnValue", "silence.setDoubleClickReturnValue"]))\ncheck("Advanced control supports double-click reset", "profileOffset.setDoubleClickReturnValue" in cpp)\ncheck("Macro tick halo removed", "tickCount = 25" not in cpp)\ncheck("Secondary helper-copy clutter removed", "Protect voice presence" not in cpp and "Clean quiet regions" not in cpp)\ncheck("Activity strip uses real spectral reduction", "spectralReductionDb" in cpp and "reductionHistory" in h and "Denoise activity" in activity)\ncheck("Activity strip no longer duplicates input/output history", "inputHistory" not in h and "outputHistory" not in h and '\"Input\"' not in activity and '\"Output\"' not in activity)\ncheck("Advanced has real control plus profile diagnostics", "Profile quality" in advanced and "Analysis quality" in advanced and "Latency" in advanced and "DSP CEILING" in advanced)\ncheck("Advanced removes duplicate P3 telemetry", "DETAIL GUARD" not in advanced and "TAIL PROTECT" not in advanced)\ncheck("Check telemetry explicitly automatic", '\"DETAIL GUARD  AUTO\"' in cpp and '\"TAIL PROTECT  AUTO\"' in cpp)\ncheck("Advanced drawer compact", "shouldShow ? 660 : 540" in cpp and "{ 15, 538, 910, 106 }" in cpp)\ncheck("Frozen profile authority retained", '\"FROZEN / LOCKED\"' in cpp)\ncheck("Parameter IDs retained", all(x in processor for x in ['\"reduction\"','\"preserve\"','\"silence\"','\"thresholdOffset\"','\"quality\"','\"hearRemoved\"','\"enabled\"']))\ncheck("No UI mutex", "mutex" not in h.lower() and "mutex" not in cpp.lower())\nfailed=[name for name,ok in checks if not ok]\nprint("SMART DENOISE P5 UX INTEGRITY CONTRACT")\nprint("======================================")\nfor name,ok in checks: print(f"[{'PASS' if ok else 'FAIL'}] {name}")\nprint(f"Checks: {len(checks)}  Passed: {len(checks)-len(failed)}  Failed: {len(failed)}")\nraise SystemExit(1 if failed else 0)\n''', encoding='utf-8', newline='\n')

ci = CI.read_text(encoding='utf-8')
ci_anchor = '''      - name: Validate P4 visual fidelity contract\n        run: python qa/verify_p4_visual_fidelity.py\n'''
ci = replace_once(ci, ci_anchor, ci_anchor + '''\n      - name: Validate P5 UX integrity contract\n        run: python qa/verify_p5_ux_integrity.py\n''', 'CI P5 step')
CI.write_text(ci, encoding='utf-8', newline='\n')

rel = REL.read_text(encoding='utf-8')
rel_anchor = '''          python qa/verify_p4_visual_fidelity.py\n          if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }\n'''
rel = replace_once(rel, rel_anchor, rel_anchor + '''          python qa/verify_p5_ux_integrity.py\n          if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }\n''', 'release P5 step')
REL.write_text(rel, encoding='utf-8', newline='\n')

cl = CHANGELOG.read_text(encoding='utf-8')
ch_anchor = '- P4.6 removes the translucent sheen-band artifact from every knob, moves 3D depth to the perimeter edge only, and makes Learn Noise visibly clickable with a raised circular rim, drop shadow, hover highlight and pressed-depth treatment.\n'
cl = replace_once(cl, ch_anchor, ch_anchor + '- P5 removes deceptive A/B/Undo/Redo/Help chrome, removes the false profile dropdown affordance, adds professional tooltips and double-click resets, replaces redundant input/output history with real spectral-reduction activity, and compresses Advanced into one real control plus concise profile diagnostics.\n', 'changelog P5')
CHANGELOG.write_text(cl, encoding='utf-8', newline='\n')

cpp = CPP.read_text(encoding='utf-8')
h = H.read_text(encoding='utf-8')
assert all(x not in cpp + h for x in ['abButton','undoButton','redoButton','helpButton'])
assert 'reductionHistory' in h and 'spectralReductionDb' in cpp
assert 'DETAIL GUARD  AUTO' in cpp and 'TAIL PROTECT  AUTO' in cpp
assert 'Profile quality' in cpp and 'Analysis quality' in cpp
assert 'qa/verify_p5_ux_integrity.py' in CI.read_text(encoding='utf-8')
