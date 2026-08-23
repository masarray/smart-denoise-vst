#pragma once

#include "PluginProcessor.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <array>
#include <memory>

class SmartDenoiseAudioProcessorEditor final
    : public juce::AudioProcessorEditor,
      private juce::Timer
{
public:
    explicit SmartDenoiseAudioProcessorEditor (
        SmartDenoiseAudioProcessor&);
    ~SmartDenoiseAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    void configureRotary (
        juce::Slider& slider,
        bool primary);

    void showLearnPopup (bool shouldShow);
    void showAdvancedDrawer (bool shouldShow);
    void setBypassed (bool shouldBypass);
    void syncBypassButton();

    void drawHeader (juce::Graphics&);
    void drawCaptureSection (juce::Graphics&);
    void drawCleanSection (juce::Graphics&);
    void drawCheckSection (juce::Graphics&);
    void drawActivityStrip (juce::Graphics&);
    void drawFooter (juce::Graphics&);
    void drawLearnPopup (juce::Graphics&);
    void drawAdvancedDrawer (juce::Graphics&);
    void drawMeter (
        juce::Graphics&,
        juce::Rectangle<float> bounds,
        float valueDb,
        const juce::String& label);

    SmartDenoiseAudioProcessor& processor;

    std::unique_ptr<juce::LookAndFeel_V4> conceptLookAndFeel;

    juce::Label title;
    juce::Label profileStatus;
    juce::Label profileName;

    juce::TextButton abButton { "A / B" };
    juce::TextButton undoButton { "UNDO" };
    juce::TextButton redoButton { "REDO" };
    juce::TextButton helpButton { "?" };

    juce::TextButton learn { "Learn Noise\n3s" };
    juce::TextButton hearRemoved { "Hear Removed" };
    juce::TextButton bypass { "Bypass" };
    juce::TextButton advanced { "Advanced" };

    juce::ComboBox quality;

    juce::Slider reduction;
    juce::Slider preserve;
    juce::Slider silence;
    juce::Slider profileOffset;

    juce::Label reductionLabel;
    juce::Label preserveLabel;
    juce::Label silenceLabel;
    juce::Label profileOffsetLabel;

    juce::Label learnPopupTitle;
    juce::Label learnPopupInstruction;
    juce::Label learnPopupStatus;
    juce::TextButton learnPopupClose { "Hide" };

    juce::Label advancedTitle;
    juce::Label advancedStatus;
    juce::TextButton advancedClose { "Close" };

    using SliderAttachment =
        juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment =
        juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboAttachment =
        juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    std::unique_ptr<SliderAttachment> reductionAttachment;
    std::unique_ptr<SliderAttachment> preserveAttachment;
    std::unique_ptr<SliderAttachment> silenceAttachment;
    std::unique_ptr<SliderAttachment> offsetAttachment;

    std::unique_ptr<ButtonAttachment> removedAttachment;
    std::unique_ptr<ComboAttachment> qualityAttachment;

    juce::Rectangle<int> captureBounds;
    juce::Rectangle<int> cleanBounds;
    juce::Rectangle<int> checkBounds;
    juce::Rectangle<int> activityBounds;
    juce::Rectangle<int> footerBounds;
    juce::Rectangle<int> learnPopupBounds;
    juce::Rectangle<int> advancedBounds;

    std::array<float, 112> inputHistory {};
    std::array<float, 112> outputHistory {};

    float displayedInputDb = -72.0f;
    float displayedOutputDb = -72.0f;

    bool learnPopupVisible = false;
    bool advancedDrawerVisible = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (
        SmartDenoiseAudioProcessorEditor)
};
