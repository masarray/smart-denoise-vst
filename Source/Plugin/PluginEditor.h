#pragma once

#include "PluginProcessor.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <array>
#include <memory>

namespace smartdenoiseui
{
class LearnCircleButton final : public juce::Button
{
public:
    LearnCircleButton();

    void setLearnState (bool learning,
                        float progress01,
                        bool profileReady,
                        bool rejected);

    void paintButton (juce::Graphics&,
                      bool isMouseOverButton,
                      bool isButtonDown) override;

private:
    bool learning = false;
    bool profileReady = false;
    bool rejected = false;
    float progress = 0.0f;
};

class MonitorButton final : public juce::Button
{
public:
    enum class Icon
    {
        headphones,
        bypass
    };

    MonitorButton (const juce::String& text, Icon icon);

    void paintButton (juce::Graphics&,
                      bool isMouseOverButton,
                      bool isButtonDown) override;

private:
    Icon icon;
};

class CleanLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    CleanLookAndFeel();

    void drawRotarySlider (juce::Graphics&,
                           int x,
                           int y,
                           int width,
                           int height,
                           float sliderPos,
                           float rotaryStartAngle,
                           float rotaryEndAngle,
                           juce::Slider&) override;

    void drawLinearSlider (juce::Graphics&,
                           int x,
                           int y,
                           int width,
                           int height,
                           float sliderPos,
                           float minSliderPos,
                           float maxSliderPos,
                           juce::Slider::SliderStyle,
                           juce::Slider&) override;

    void drawButtonBackground (juce::Graphics&,
                               juce::Button&,
                               const juce::Colour&,
                               bool isMouseOverButton,
                               bool isButtonDown) override;

    void drawButtonText (juce::Graphics&,
                         juce::TextButton&,
                         bool isMouseOverButton,
                         bool isButtonDown) override;

    void drawComboBox (juce::Graphics&,
                       int width,
                       int height,
                       bool isButtonDown,
                       int buttonX,
                       int buttonY,
                       int buttonW,
                       int buttonH,
                       juce::ComboBox&) override;

    void positionComboBoxText (juce::ComboBox&,
                               juce::Label&) override;
};
} // namespace smartdenoiseui

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

    void configureRotary (juce::Slider& slider, bool primary);
    void showAdvancedDrawer (bool shouldShow);
    void setBypassed (bool shouldBypass);
    void syncBypassButton();

    void drawHeader (juce::Graphics&);
    void drawCaptureSection (juce::Graphics&);
    void drawCleanSection (juce::Graphics&);
    void drawCheckSection (juce::Graphics&);
    void drawActivityStrip (juce::Graphics&);
    void drawFooter (juce::Graphics&);
    void drawAdvancedDrawer (juce::Graphics&);

    void drawPanel (juce::Graphics&,
                    juce::Rectangle<int> bounds,
                    bool raised = false);

    void drawStepHeader (juce::Graphics&,
                         juce::Rectangle<int> area,
                         int number,
                         const juce::String& text);

    void drawMeter (juce::Graphics&,
                    juce::Rectangle<float> bounds,
                    float valueDb,
                    const juce::String& label);

    void drawTelemetry (juce::Graphics&,
                        juce::Rectangle<int> area,
                        const juce::String& label,
                        float value01);

    SmartDenoiseAudioProcessor& processor;
    std::unique_ptr<smartdenoiseui::CleanLookAndFeel> cleanLookAndFeel;

    juce::Label title;
    juce::Label profileName;
    juce::Label profileStatus;

    juce::TextButton abButton { "A / B" };
    juce::TextButton undoButton { "UNDO" };
    juce::TextButton redoButton { "REDO" };
    juce::TextButton helpButton { "?" };

    smartdenoiseui::LearnCircleButton learn;
    smartdenoiseui::MonitorButton hearRemoved {
        "Hear Removed",
        smartdenoiseui::MonitorButton::Icon::headphones
    };
    smartdenoiseui::MonitorButton bypass {
        "Bypass",
        smartdenoiseui::MonitorButton::Icon::bypass
    };

    juce::TextButton advanced { "Advanced" };
    juce::TextButton advancedClose { "Close" };
    juce::ComboBox quality;

    juce::Slider reduction;
    juce::Slider preserve;
    juce::Slider silence;
    juce::Slider profileOffset;

    juce::Label reductionLabel;
    juce::Label preserveLabel;
    juce::Label silenceLabel;
    juce::Label profileOffsetLabel;

    juce::Label advancedTitle;

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

    juce::Rectangle<int> headerBounds;
    juce::Rectangle<int> captureBounds;
    juce::Rectangle<int> cleanBounds;
    juce::Rectangle<int> checkBounds;
    juce::Rectangle<int> activityBounds;
    juce::Rectangle<int> footerBounds;
    juce::Rectangle<int> advancedBounds;

    std::array<float, 112> inputHistory {};
    std::array<float, 112> outputHistory {};

    float displayedInputDb = -72.0f;
    float displayedOutputDb = -72.0f;

    bool advancedDrawerVisible = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (
        SmartDenoiseAudioProcessorEditor)
};
