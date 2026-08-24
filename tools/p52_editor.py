from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
def read(p): return (ROOT/p).read_text(encoding='utf-8')
def write(p,s): (ROOT/p).write_text(s,encoding='utf-8',newline='\n')
def rep(s,a,b,label):
    if a not in s: raise RuntimeError(f'missing anchor: {label}')
    return s.replace(a,b,1)

h_path='Source/Plugin/PluginEditor.h'
h=read(h_path)
h=rep(h,'    void syncBypassButton();\n\n    void drawHeader','    void syncBypassButton();\n    void refreshProfileBank (const juce::String& selectName = {});\n\n    void drawHeader','bank method')
h=rep(h,'    void drawCaptureSection (juce::Graphics&);\n    void drawCleanSection','    void drawCaptureSection (juce::Graphics&);\n    void drawProfileFingerprint (juce::Graphics&);\n    void drawCleanSection','fingerprint method')
h=rep(h,'    juce::Label title;\n    juce::Label profileName;\n    juce::Label profileStatus;\n','    juce::Label title;\n    juce::Label profileName;\n    juce::Label profileStatus;\n    juce::Label profileBankCaption;\n    juce::ComboBox profileBank;\n','bank fields')
h=rep(h,'    bool advancedDrawerVisible = false;\n','    bool advancedDrawerVisible = false;\n    bool wasLearning = false;\n    int lastProfileBankQuality = -1;\n','workflow state')
write(h_path,h)

p='Source/Plugin/PluginEditor.cpp'
s=read(p)

s=rep(s,'    g.setColour (colour);\n    auto circle = area.reduced (6.0f);\n    g.drawEllipse (circle, 2.0f);\n','    g.setColour (colour);\n    auto available = area.reduced (5.0f);\n    const float diameter = juce::jmin (available.getWidth(), available.getHeight());\n    auto circle = juce::Rectangle<float> (diameter, diameter)\n        .withCentre (area.getCentre());\n    g.drawEllipse (circle, 2.0f);\n','bypass square geometry')

s=rep(s,'    else if (profileReady)\n    {\n        mainText = "Profile Ready";\n        subText = "Click to re-learn";\n    }\n    else if (rejected)\n    {\n        mainText = "Try Again";\n        subText = "Capture noise only";\n    }\n','    else if (profileReady)\n    {\n        mainText = "Profile Active";\n        subText = "Click to update";\n    }\n    else if (rejected)\n    {\n        mainText = "Learn Noise";\n        subText = "Choose noise-only moment";\n    }\n','learn language')

s=rep(s,'    profileStatus.setJustificationType (\n        juce::Justification::centredLeft);\n\n    quality.addItem (\n','    profileStatus.setJustificationType (\n        juce::Justification::centredLeft);\n\n    profileBankCaption.setText (\n        "CAPTURED PROFILE BANK",\n        juce::dontSendNotification);\n    profileBankCaption.setFont (juce::FontOptions (8.4f));\n    profileBankCaption.setColour (\n        juce::Label::textColourId,\n        ui::textMuted);\n    profileBankCaption.setJustificationType (\n        juce::Justification::centredRight);\n    profileBank.setTextWhenNothingSelected (\n        "No saved captures");\n\n    quality.addItem (\n','bank setup')

s=rep(s,'    const std::array<juce::Component*, 16> components {\n        &title,\n        &profileName,\n        &profileStatus,\n','    const std::array<juce::Component*, 17> components {\n        &title,\n        &profileStatus,\n        &profileBankCaption,\n        &profileBank,\n','visible components')

s=rep(s,'    quality.setTooltip (\n        "Live 1024 uses lower latency; Clean 2048 uses higher spectral resolution.");\n    profileOffset.setTooltip (\n','    quality.setTooltip (\n        "Live 1024 uses lower latency; Clean 2048 uses higher spectral resolution.");\n    profileBank.setTooltip (\n        "Captured-profile bank. Each entry restores the frozen noise profile plus Reduction, Preserve, Silence and Profile Offset captured with it.");\n    profileOffset.setTooltip (\n','bank tooltip')

s=rep(s,'    advanced.onClick =\n        [this]\n','    profileBank.onChange =\n        [this]\n        {\n            const auto name = profileBank.getText();\n            if (name.isNotEmpty()\n                && ! processor.loadCapturedProfilePreset (name))\n            {\n                profileStatus.setText (\n                    "Profile bank entry requires the matching quality mode",\n                    juce::dontSendNotification);\n            }\n        };\n\n    advanced.onClick =\n        [this]\n','bank callback')

s=rep(s,'    showAdvancedDrawer (false);\n    syncBypassButton();\n\n    startTimerHz (24);\n','    showAdvancedDrawer (false);\n    syncBypassButton();\n    refreshProfileBank();\n    wasLearning = processor.getEngine().isLearning();\n    lastProfileBankQuality = static_cast<int> (\n        processor.getParameters().getRawParameterValue ("quality")->load());\n\n    startTimerHz (24);\n','bank initial state')

anchor='void SmartDenoiseAudioProcessorEditor::drawPanel (\n'
if anchor not in s: raise RuntimeError('drawPanel insertion anchor missing')
refresh=r'''void SmartDenoiseAudioProcessorEditor::refreshProfileBank (
    const juce::String& selectName)
{
    const auto names = processor.getCapturedProfilePresetNames();
    profileBank.clear (juce::dontSendNotification);

    for (int index = 0; index < names.size(); ++index)
        profileBank.addItem (names[index], index + 1);

    profileBank.setTextWhenNothingSelected (
        names.isEmpty() ? "No saved captures" : "Select captured profile");

    if (selectName.isNotEmpty())
    {
        const int selected = names.indexOf (selectName);
        if (selected >= 0)
            profileBank.setSelectedItemIndex (
                selected, juce::dontSendNotification);
    }
}

'''
s=s.replace(anchor,refresh+anchor,1)

# Replace passive pill/health area with a real fingerprint card.
section=s.index('void SmartDenoiseAudioProcessorEditor::\ndrawCaptureSection')
start=s.index('    auto profilePill =\n',section)
end_marker='}\n\nvoid SmartDenoiseAudioProcessorEditor::\ndrawCleanSection'
end=s.index(end_marker,start)
tail=r'''    drawProfileFingerprint (g);
}

void SmartDenoiseAudioProcessorEditor::drawProfileFingerprint (
    juce::Graphics& g)
{
    const auto& engine = processor.getEngine();
    const bool hasProfile = engine.hasProfile();

    auto card = juce::Rectangle<float> (
        static_cast<float> (captureBounds.getX() + 20),
        static_cast<float> (captureBounds.getY() + 229),
        static_cast<float> (captureBounds.getWidth() - 40),
        70.0f);

    g.setColour (ui::panelDeep);
    g.fillRoundedRectangle (card, 9.0f);
    g.setColour (ui::border.withAlpha (0.74f));
    g.drawRoundedRectangle (card, 9.0f, 1.0f);

    g.setColour (hasProfile ? ui::accentCyan : ui::textMuted);
    g.setFont (juce::FontOptions (8.3f));
    g.drawText (
        hasProfile ? "CAPTURED NOISE PROFILE" : "NOISE PROFILE  ·  EMPTY",
        card.getX() + 9.0f,
        card.getY() + 5.0f,
        card.getWidth() - 18.0f,
        14.0f,
        juce::Justification::centredLeft);

    auto graph = card.reduced (9.0f, 7.0f);
    graph.removeFromTop (15.0f);
    graph.removeFromBottom (2.0f);

    g.setColour (ui::borderSoft.withAlpha (0.80f));
    g.drawLine (
        graph.getX(), graph.getBottom(),
        graph.getRight(), graph.getBottom(),
        1.0f);

    if (! hasProfile)
    {
        g.setColour (ui::textMuted);
        g.setFont (juce::FontOptions (8.5f));
        g.drawText (
            "Capture noise to build the denoise fingerprint",
            graph.toNearestInt(),
            juce::Justification::centred);
        return;
    }

    const auto fingerprint = engine.getProfileDisplay();
    const float step = graph.getWidth()
        / static_cast<float> (fingerprint.size());

    for (size_t index = 0; index < fingerprint.size(); ++index)
    {
        const float value = ui::clamp01 (fingerprint[index]);
        const float barHeight = 2.0f + value * (graph.getHeight() - 3.0f);
        auto bar = juce::Rectangle<float> (
            graph.getX() + static_cast<float> (index) * step + 0.55f,
            graph.getBottom() - barHeight,
            juce::jmax (1.0f, step - 1.1f),
            barHeight);

        const float position = static_cast<float> (index)
            / static_cast<float> (fingerprint.size() - 1);
        g.setColour (
            ui::accentPurple.interpolatedWith (
                ui::accentCyan, position).withAlpha (0.78f));
        g.fillRoundedRectangle (bar, 1.0f);
    }

    g.setColour (ui::textSecondary);
    g.setFont (juce::FontOptions (7.8f));
    g.drawText (
        juce::String (juce::roundToInt (engine.getProfileQuality() * 100.0f))
            + "% quality  ·  frozen",
        static_cast<int> (card.getX() + 8.0f),
        static_cast<int> (card.getBottom() - 16.0f),
        static_cast<int> (card.getWidth() - 16.0f),
        12,
        juce::Justification::centredRight);
}

void SmartDenoiseAudioProcessorEditor::
drawCleanSection'''
s=s[:start]+tail+s[end+len('}\n\nvoid SmartDenoiseAudioProcessorEditor::\ndrawCleanSection'):]

s=rep(s,'    title.setBounds (\n        headerBounds.getX() + 51,\n        headerBounds.getY() + 8,\n        340,\n        37);\n\n    learn.setBounds (\n','    title.setBounds (\n        headerBounds.getX() + 51,\n        headerBounds.getY() + 8,\n        330,\n        37);\n\n    profileBankCaption.setBounds (\n        headerBounds.getX() + 470,\n        headerBounds.getY() + 17,\n        116,\n        20);\n    profileBank.setBounds (\n        headerBounds.getX() + 596,\n        headerBounds.getY() + 11,\n        292,\n        32);\n\n    learn.setBounds (\n','header bank layout')

s=rep(s,'    profileName.setBounds (\n        captureBounds.getX() + 45,\n        captureBounds.getY() + 236,\n        captureBounds.getWidth() - 64,\n        26);\n\n    profileStatus.setBounds (\n        captureBounds.getX() + 20,\n        captureBounds.getY() + 308,\n        captureBounds.getWidth() - 40,\n        18);\n','    profileStatus.setBounds (\n        captureBounds.getX() + 20,\n        captureBounds.getY() + 304,\n        captureBounds.getWidth() - 40,\n        20);\n','capture layout')

s=rep(s,'    auto& engine =\n        processor.getEngine();\n\n    const auto frameAnalysis = engine.getFrameAnalysis();\n','    auto& engine =\n        processor.getEngine();\n\n    const bool learningNow = engine.isLearning();\n    if (wasLearning\n        && ! learningNow\n        && engine.hasProfile()\n        && ! engine.wasLastLearnRejected())\n    {\n        const auto savedName = processor.saveCapturedProfilePreset();\n        if (savedName.isNotEmpty())\n            refreshProfileBank (savedName);\n    }\n    wasLearning = learningNow;\n\n    const int qualityIndexNow = static_cast<int> (\n        processor.getParameters().getRawParameterValue ("quality")->load());\n    if (qualityIndexNow != lastProfileBankQuality)\n    {\n        lastProfileBankQuality = qualityIndexNow;\n        refreshProfileBank();\n    }\n\n    const auto frameAnalysis = engine.getFrameAnalysis();\n','learn transition autosave')

status_start=s.index('    juce::String profileText;\n',s.index('timerCallback'))
status_end=s.index('    profileStatus.setText (\n',status_start)
status=r'''    juce::String profileText;

    if (engine.isLearning())
    {
        profileText =
            "Building frozen profile  ·  "
            + juce::String (
                juce::roundToInt (engine.getLearningProgress() * 100.0f))
            + "%";
    }
    else if (engine.wasLastLearnRejected())
    {
        profileText = engine.hasProfile()
            ? "Denoise active  ·  previous profile kept"
            : "Waiting for a clean noise-only capture";
    }
    else if (engine.hasProfile())
    {
        profileText = "Denoise active  ·  frozen noise profile";
    }
    else
    {
        profileText = "Empty profile  ·  capture room noise to activate";
    }

'''
s=s[:status_start]+status+s[status_end:]
write(p,s)
