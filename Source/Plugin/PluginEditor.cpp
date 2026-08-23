#include "PluginEditor.h"

#include <array>

SmartDenoiseAudioProcessorEditor::
SmartDenoiseAudioProcessorEditor (
    SmartDenoiseAudioProcessor& p)
    : juce::AudioProcessorEditor (&p),
      processor (p)
{
    setSize (720, 300);

    title.setText (
        "SMART DENOISE",
        juce::dontSendNotification);
    title.setFont (
        juce::FontOptions (22.0f));
    title.setJustificationType (
        juce::Justification::centredLeft);

    status.setText (
        "LEARN NOISE REQUIRED",
        juce::dontSendNotification);
    status.setJustificationType (
        juce::Justification::centredLeft);

    quality.addItem (
        "Live - FFT 1024", 1);
    quality.addItem (
        "Clean - FFT 2048", 2);

    configureKnob (reduction, " dB");
    configureKnob (preserve, " %");
    configureKnob (silence, " %");
    configureKnob (profileOffset, " dB");

    preserve.textFromValueFunction =
        [] (double v)
        {
            return juce::String (
                juce::roundToInt (v * 100.0))
                + " %";
        };

    silence.textFromValueFunction =
        [] (double v)
        {
            return juce::String (
                juce::roundToInt (v * 100.0))
                + " %";
        };

    reductionLabel.setText (
        "Reduction",
        juce::dontSendNotification);
    preserveLabel.setText (
        "Preserve",
        juce::dontSendNotification);
    silenceLabel.setText (
        "Silence",
        juce::dontSendNotification);
    offsetLabel.setText (
        "Profile Offset",
        juce::dontSendNotification);

    const std::array<juce::Component*, 14> components {
        &title,
        &status,
        &enabled,
        &hearRemoved,
        &learn,
        &quality,
        &reduction,
        &preserve,
        &silence,
        &profileOffset,
        &reductionLabel,
        &preserveLabel,
        &silenceLabel,
        &offsetLabel
    };

    for (auto* component : components)
        addAndMakeVisible (component);

    auto& state = processor.getParameters();

    reductionAttachment =
        std::make_unique<SliderAttachment> (
            state, "reduction", reduction);

    preserveAttachment =
        std::make_unique<SliderAttachment> (
            state, "preserve", preserve);

    silenceAttachment =
        std::make_unique<SliderAttachment> (
            state, "silence", silence);

    offsetAttachment =
        std::make_unique<SliderAttachment> (
            state, "thresholdOffset",
            profileOffset);

    enabledAttachment =
        std::make_unique<ButtonAttachment> (
            state, "enabled", enabled);

    removedAttachment =
        std::make_unique<ButtonAttachment> (
            state, "hearRemoved",
            hearRemoved);

    qualityAttachment =
        std::make_unique<ComboAttachment> (
            state, "quality", quality);

    learn.onClick =
        [this]
        {
            processor.startNoiseLearn();
        };

    startTimerHz (10);
}

void SmartDenoiseAudioProcessorEditor::configureKnob (
    juce::Slider& slider,
    const juce::String& suffix)
{
    slider.setSliderStyle (
        juce::Slider::RotaryHorizontalVerticalDrag);

    slider.setTextBoxStyle (
        juce::Slider::TextBoxBelow,
        false,
        86,
        22);

    slider.setTextValueSuffix (suffix);
}

void SmartDenoiseAudioProcessorEditor::paint (
    juce::Graphics& g)
{
    g.fillAll (
        juce::Colour::fromRGB (
            20, 22, 27));

    g.setColour (
        juce::Colour::fromRGB (
            42, 46, 56));

    g.drawRoundedRectangle (
        getLocalBounds()
            .toFloat()
            .reduced (12.0f),
        10.0f,
        1.0f);
}

void SmartDenoiseAudioProcessorEditor::resized()
{
    auto area =
        getLocalBounds().reduced (22);

    auto top =
        area.removeFromTop (36);

    title.setBounds (
        top.removeFromLeft (220));

    enabled.setBounds (
        top.removeFromLeft (100));

    quality.setBounds (
        top.removeFromLeft (180)
            .reduced (4, 3));

    hearRemoved.setBounds (
        top.removeFromLeft (120));

    area.removeFromTop (8);

    auto statusRow =
        area.removeFromTop (34);

    status.setBounds (
        statusRow.removeFromLeft (420));

    learn.setBounds (
        statusRow.removeFromRight (180)
            .reduced (4, 2));

    area.removeFromTop (12);

    auto controls =
        area.removeFromTop (170);

    const int column =
        controls.getWidth() / 4;

    auto place =
        [&] (juce::Slider& slider,
             juce::Label& label)
        {
            auto cell =
                controls.removeFromLeft (column);

            label.setBounds (
                cell.removeFromTop (24));

            slider.setBounds (
                cell.reduced (6));
        };

    place (reduction, reductionLabel);
    place (preserve, preserveLabel);
    place (silence, silenceLabel);
    place (profileOffset, offsetLabel);
}

void SmartDenoiseAudioProcessorEditor::timerCallback()
{
    const auto& engine =
        processor.getEngine();

    juce::String text;

    if (engine.isLearning())
    {
        text =
            "LEARNING "
            + juce::String (
                juce::roundToInt (
                    engine.getLearningProgress()
                    * 100.0f))
            + "%";
    }
    else if (engine.wasLastLearnRejected())
    {
        text =
            engine.hasProfile()
            ? "LEARN REJECTED - PROFILE KEPT"
            : "LEARN REJECTED - RETRY";
    }
    else if (engine.hasProfile())
    {
        text =
            "PROFILE "
            + juce::String (
                juce::roundToInt (
                    engine.getProfileQuality()
                    * 100.0f))
            + "%  |  SPECTRAL GR "
            + juce::String (
                engine.getFrameAnalysis()
                    .spectralReductionDb,
                1)
            + " dB  |  SILENCE GR "
            + juce::String (
                engine.getExpanderGainReductionDb(),
                1)
            + " dB";
    }
    else
    {
        text = "LEARN NOISE REQUIRED";
    }

    status.setText (
        text,
        juce::dontSendNotification);
}
