#pragma once

#include "PluginProcessor.h"

#include <juce_gui_basics/juce_gui_basics.h>

class SmartDenoiseAudioProcessorEditor final
    : public juce::AudioProcessorEditor,
      private juce::Timer
{
public:
    explicit SmartDenoiseAudioProcessorEditor (
        SmartDenoiseAudioProcessor&);
    ~SmartDenoiseAudioProcessorEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void configureKnob (
        juce::Slider& slider,
        const juce::String& suffix);

    SmartDenoiseAudioProcessor& processor;

    juce::Label title;
    juce::Label status;

    juce::ToggleButton enabled { "Denoise" };
    juce::ToggleButton hearRemoved { "Hear Removed" };

    juce::TextButton learn { "Learn Noise (3 s)" };
    juce::ComboBox quality;

    juce::Slider reduction;
    juce::Slider preserve;
    juce::Slider silence;
    juce::Slider profileOffset;

    juce::Label reductionLabel;
    juce::Label preserveLabel;
    juce::Label silenceLabel;
    juce::Label offsetLabel;

    using SliderAttachment =
        juce::AudioProcessorValueTreeState::
            SliderAttachment;
    using ButtonAttachment =
        juce::AudioProcessorValueTreeState::
            ButtonAttachment;
    using ComboAttachment =
        juce::AudioProcessorValueTreeState::
            ComboBoxAttachment;

    std::unique_ptr<SliderAttachment>
        reductionAttachment;
    std::unique_ptr<SliderAttachment>
        preserveAttachment;
    std::unique_ptr<SliderAttachment>
        silenceAttachment;
    std::unique_ptr<SliderAttachment>
        offsetAttachment;

    std::unique_ptr<ButtonAttachment>
        enabledAttachment;
    std::unique_ptr<ButtonAttachment>
        removedAttachment;

    std::unique_ptr<ComboAttachment>
        qualityAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (
        SmartDenoiseAudioProcessorEditor)
};
