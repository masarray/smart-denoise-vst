#include "PluginEditor.h"

#include <algorithm>
#include <cmath>

namespace
{
const auto backgroundTop = juce::Colour::fromRGB (8, 11, 18);
const auto backgroundBottom = juce::Colour::fromRGB (13, 20, 34);
const auto surface = juce::Colour::fromRGB (15, 20, 29);
const auto surfaceRaised = juce::Colour::fromRGB (19, 25, 36);
const auto surfaceSoft = juce::Colour::fromRGB (24, 30, 43);
const auto border = juce::Colour::fromRGB (43, 50, 68);
const auto textPrimary = juce::Colour::fromRGB (242, 244, 250);
const auto textSecondary = juce::Colour::fromRGB (145, 153, 175);
const auto accentPurple = juce::Colour::fromRGB (151, 91, 255);
const auto accentBlue = juce::Colour::fromRGB (67, 137, 255);
const auto accentCyan = juce::Colour::fromRGB (79, 198, 255);
const auto warning = juce::Colour::fromRGB (255, 177, 74);

juce::ColourGradient accentGradient (juce::Rectangle<float> area)
{
    return juce::ColourGradient (
        accentPurple,
        area.getTopLeft(),
        accentBlue,
        area.getBottomRight(),
        false);
}

void drawStepHeader (
    juce::Graphics& g,
    juce::Rectangle<int> area,
    int number,
    const juce::String& text)
{
    auto badge = area.removeFromLeft (26).toFloat().reduced (3.0f);

    g.setColour (surfaceSoft);
    g.fillEllipse (badge);
    g.setColour (border.brighter (0.35f));
    g.drawEllipse (badge, 1.0f);

    g.setColour (textPrimary);
    g.setFont (juce::FontOptions (11.5f));
    g.drawText (
        juce::String (number),
        badge.toNearestInt(),
        juce::Justification::centred);

    g.setColour (accentPurple.brighter (0.25f));
    g.setFont (juce::FontOptions (12.5f));
    g.drawText (
        text,
        area,
        juce::Justification::centredLeft);
}

class ConceptCLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    ConceptCLookAndFeel()
    {
        setColour (
            juce::ComboBox::textColourId,
            textPrimary);
        setColour (
            juce::ComboBox::backgroundColourId,
            surfaceSoft);
        setColour (
            juce::ComboBox::outlineColourId,
            border);
        setColour (
            juce::PopupMenu::backgroundColourId,
            surfaceRaised);
        setColour (
            juce::PopupMenu::textColourId,
            textPrimary);
        setColour (
            juce::PopupMenu::highlightedBackgroundColourId,
            accentPurple.withAlpha (0.35f));
    }

    void drawRotarySlider (
        juce::Graphics& g,
        int x,
        int y,
        int width,
        int height,
        float sliderPos,
        float startAngle,
        float endAngle,
        juce::Slider& slider) override
    {
        auto bounds =
            juce::Rectangle<float> (
                static_cast<float> (x),
                static_cast<float> (y),
                static_cast<float> (width),
                static_cast<float> (height))
                .reduced (5.0f);

        const bool primary =
            slider.getName() == "primary";

        const float diameter =
            juce::jmin (
                bounds.getWidth(),
                bounds.getHeight());

        const float radius =
            diameter * (primary ? 0.43f : 0.40f);

        const auto centre = bounds.getCentre();
        const float stroke = primary ? 10.0f : 6.0f;

        juce::Path baseArc;
        baseArc.addCentredArc (
            centre.x,
            centre.y,
            radius,
            radius,
            0.0f,
            startAngle,
            endAngle,
            true);

        g.setColour (juce::Colour::fromRGB (47, 53, 69));
        g.strokePath (
            baseArc,
            juce::PathStrokeType (stroke));

        const float valueAngle =
            startAngle
            + sliderPos * (endAngle - startAngle);

        juce::Path valueArc;
        valueArc.addCentredArc (
            centre.x,
            centre.y,
            radius,
            radius,
            0.0f,
            startAngle,
            valueAngle,
            true);

        auto gradientArea =
            bounds.withSizeKeepingCentre (
                radius * 2.1f,
                radius * 2.1f);

        g.setGradientFill (
            accentGradient (gradientArea));
        g.strokePath (
            valueArc,
            juce::PathStrokeType (stroke));

        const float knobRadius =
            radius - (primary ? 17.0f : 11.0f);

        auto knob =
            juce::Rectangle<float> (
                knobRadius * 2.0f,
                knobRadius * 2.0f)
                .withCentre (centre);

        juce::ColourGradient knobGradient (
            juce::Colour::fromRGB (34, 40, 55),
            knob.getTopLeft(),
            juce::Colour::fromRGB (11, 14, 21),
            knob.getBottomRight(),
            false);

        g.setGradientFill (knobGradient);
        g.fillEllipse (knob);
        g.setColour (border.withAlpha (0.9f));
        g.drawEllipse (knob, 1.0f);

        const auto marker =
            centre.getPointOnCircumference (
                radius,
                valueAngle);

        g.setColour (textPrimary);
        g.fillEllipse (
            juce::Rectangle<float> (
                primary ? 10.0f : 7.0f,
                primary ? 10.0f : 7.0f)
                .withCentre (marker));

        g.setColour (textPrimary);
        g.setFont (
            juce::FontOptions (
                primary ? 28.0f : 16.0f));

        g.drawFittedText (
            slider.getTextFromValue (
                slider.getValue()),
            knob.toNearestInt().reduced (
                primary ? 22 : 9),
            juce::Justification::centred,
            1);
    }

    void drawLinearSlider (
        juce::Graphics& g,
        int x,
        int y,
        int width,
        int height,
        float sliderPos,
        float,
        float,
        juce::Slider::SliderStyle,
        juce::Slider&) override
    {
        auto area =
            juce::Rectangle<float> (
                static_cast<float> (x),
                static_cast<float> (y),
                static_cast<float> (width),
                static_cast<float> (height));

        const float cy = area.getCentreY();
        const float left = area.getX() + 5.0f;
        const float right = area.getRight() - 5.0f;

        g.setColour (juce::Colour::fromRGB (48, 55, 70));
        g.fillRoundedRectangle (
            juce::Rectangle<float> (
                left,
                cy - 2.0f,
                right - left,
                4.0f),
            2.0f);

        auto active =
            juce::Rectangle<float> (
                left,
                cy - 2.0f,
                juce::jmax (0.0f, sliderPos - left),
                4.0f);

        g.setGradientFill (
            accentGradient (area));
        g.fillRoundedRectangle (active, 2.0f);

        g.setColour (textPrimary);
        g.fillEllipse (
            juce::Rectangle<float> (10.0f, 10.0f)
                .withCentre ({ sliderPos, cy }));
    }

    void drawButtonBackground (
        juce::Graphics& g,
        juce::Button& button,
        const juce::Colour&,
        bool isHighlighted,
        bool isDown) override
    {
        auto area =
            button.getLocalBounds().toFloat().reduced (0.5f);

        const bool primary =
            button.getName() == "learn";
        const bool top =
            button.getName() == "top";
        const bool toggled =
            button.getToggleState();

        if (primary)
        {
            g.setGradientFill (
                accentGradient (area));
            g.fillRoundedRectangle (area, 13.0f);

            if (isHighlighted || isDown)
            {
                g.setColour (
                    juce::Colours::white.withAlpha (
                        isDown ? 0.10f : 0.055f));
                g.fillRoundedRectangle (area, 13.0f);
            }

            g.setColour (
                accentPurple.brighter (0.45f)
                    .withAlpha (0.7f));
            g.drawRoundedRectangle (
                area,
                13.0f,
                1.0f);
            return;
        }

        if (top)
        {
            g.setColour (
                isHighlighted
                ? surfaceSoft.brighter (0.08f)
                : juce::Colours::transparentBlack);
            g.fillRoundedRectangle (area, 8.0f);
            return;
        }

        g.setColour (
            toggled
            ? accentPurple.withAlpha (0.22f)
            : surfaceRaised);
        g.fillRoundedRectangle (area, 10.0f);

        g.setColour (
            toggled
            ? accentBlue.withAlpha (0.85f)
            : border.withAlpha (
                isHighlighted ? 1.0f : 0.8f));
        g.drawRoundedRectangle (
            area,
            10.0f,
            toggled ? 1.4f : 1.0f);
    }

    void drawButtonText (
        juce::Graphics& g,
        juce::TextButton& button,
        bool,
        bool) override
    {
        const bool primary =
            button.getName() == "learn";
        const bool top =
            button.getName() == "top";

        g.setColour (
            button.isEnabled()
            ? textPrimary
            : textSecondary.withAlpha (0.45f));

        g.setFont (
            juce::FontOptions (
                primary ? 17.0f : (top ? 10.5f : 12.5f)));

        g.drawFittedText (
            button.getButtonText(),
            button.getLocalBounds().reduced (7, 4),
            juce::Justification::centred,
            primary ? 2 : 1);
    }

    void drawComboBox (
        juce::Graphics& g,
        int width,
        int height,
        bool,
        int,
        int,
        int,
        int,
        juce::ComboBox&) override
    {
        auto area =
            juce::Rectangle<float> (
                0.5f,
                0.5f,
                static_cast<float> (width) - 1.0f,
                static_cast<float> (height) - 1.0f);

        g.setColour (surfaceRaised);
        g.fillRoundedRectangle (area, 8.0f);
        g.setColour (border);
        g.drawRoundedRectangle (area, 8.0f, 1.0f);

        juce::Path arrow;
        const float cx = width - 14.0f;
        const float cy = height * 0.5f;
        arrow.startNewSubPath (cx - 4.0f, cy - 2.0f);
        arrow.lineTo (cx, cy + 2.0f);
        arrow.lineTo (cx + 4.0f, cy - 2.0f);

        g.setColour (textSecondary);
        g.strokePath (
            arrow,
            juce::PathStrokeType (1.4f));
    }

    void positionComboBoxText (
        juce::ComboBox& box,
        juce::Label& label) override
    {
        label.setBounds (
            10,
            0,
            box.getWidth() - 30,
            box.getHeight());
        label.setFont (juce::FontOptions (11.5f));
        label.setColour (
            juce::Label::textColourId,
            textPrimary);
    }
};
}

SmartDenoiseAudioProcessorEditor::
SmartDenoiseAudioProcessorEditor (
    SmartDenoiseAudioProcessor& p)
    : juce::AudioProcessorEditor (&p),
      processor (p),
      conceptLookAndFeel (
          std::make_unique<ConceptCLookAndFeel>())
{
    setOpaque (true);
    setLookAndFeel (conceptLookAndFeel.get());
    setSize (940, 540);

    title.setText (
        "Smart Denoise",
        juce::dontSendNotification);
    title.setFont (juce::FontOptions (20.0f));
    title.setColour (
        juce::Label::textColourId,
        textPrimary);
    title.setJustificationType (
        juce::Justification::centredLeft);

    profileName.setText (
        "Studio Noise",
        juce::dontSendNotification);
    profileName.setFont (juce::FontOptions (11.5f));
    profileName.setColour (
        juce::Label::textColourId,
        textPrimary);
    profileName.setJustificationType (
        juce::Justification::centredLeft);

    profileStatus.setText (
        "No profile - learn room noise",
        juce::dontSendNotification);
    profileStatus.setFont (juce::FontOptions (10.5f));
    profileStatus.setColour (
        juce::Label::textColourId,
        textSecondary);
    profileStatus.setJustificationType (
        juce::Justification::centredLeft);

    quality.addItem (
        "Balanced - Live 1024", 1);
    quality.addItem (
        "Maximum - Clean 2048", 2);
    quality.setTextWhenNothingSelected (
        "Balanced - Live 1024");

    configureRotary (reduction, true);
    configureRotary (preserve, false);
    configureRotary (silence, false);

    reduction.setName ("primary");
    preserve.setName ("secondary");
    silence.setName ("secondary");

    reduction.textFromValueFunction =
        [] (double value)
        {
            const auto percent =
                juce::roundToInt (
                    100.0
                    * value
                    / smartdenoise::SmartDenoiseEngine::
                        maxReductionDb);
            return juce::String (percent) + "%";
        };

    preserve.textFromValueFunction =
        [] (double value)
        {
            return juce::String (
                juce::roundToInt (value * 100.0))
                + "%";
        };

    silence.textFromValueFunction =
        [] (double value)
        {
            return juce::String (
                juce::roundToInt (value * 100.0))
                + "%";
        };

    profileOffset.setSliderStyle (
        juce::Slider::LinearHorizontal);
    profileOffset.setTextBoxStyle (
        juce::Slider::TextBoxRight,
        false,
        76,
        22);
    profileOffset.setTextValueSuffix (" dB");
    profileOffset.setColour (
        juce::Slider::textBoxTextColourId,
        textPrimary);
    profileOffset.setColour (
        juce::Slider::textBoxBackgroundColourId,
        surfaceRaised);
    profileOffset.setColour (
        juce::Slider::textBoxOutlineColourId,
        border);

    reductionLabel.setText (
        "Reduce Noise",
        juce::dontSendNotification);
    preserveLabel.setText (
        "Preserve Detail",
        juce::dontSendNotification);
    silenceLabel.setText (
        "Silence Clean-up",
        juce::dontSendNotification);
    profileOffsetLabel.setText (
        "Profile Offset",
        juce::dontSendNotification);

    for (auto* label : {
             &reductionLabel,
             &preserveLabel,
             &silenceLabel,
             &profileOffsetLabel })
    {
        label->setFont (juce::FontOptions (11.5f));
        label->setColour (
            juce::Label::textColourId,
            textPrimary);
        label->setJustificationType (
            juce::Justification::centred);
    }

    abButton.setName ("top");
    undoButton.setName ("top");
    redoButton.setName ("top");
    helpButton.setName ("top");
    learn.setName ("learn");

    abButton.setInterceptsMouseClicks (false, false);
    undoButton.setInterceptsMouseClicks (false, false);
    redoButton.setInterceptsMouseClicks (false, false);

    hearRemoved.setClickingTogglesState (true);
    bypass.setClickingTogglesState (true);

    learnPopupTitle.setText (
        "Learn Noise",
        juce::dontSendNotification);
    learnPopupTitle.setFont (juce::FontOptions (16.0f));
    learnPopupTitle.setColour (
        juce::Label::textColourId,
        textPrimary);

    learnPopupInstruction.setText (
        "Keep only room / system noise playing.\nSmart Denoise captures a frozen 3-second profile.",
        juce::dontSendNotification);
    learnPopupInstruction.setFont (
        juce::FontOptions (10.5f));
    learnPopupInstruction.setColour (
        juce::Label::textColourId,
        textSecondary);
    learnPopupInstruction.setJustificationType (
        juce::Justification::topLeft);

    learnPopupStatus.setFont (
        juce::FontOptions (11.0f));
    learnPopupStatus.setColour (
        juce::Label::textColourId,
        accentCyan);
    learnPopupStatus.setJustificationType (
        juce::Justification::centredLeft);

    advancedTitle.setText (
        "Advanced",
        juce::dontSendNotification);
    advancedTitle.setFont (
        juce::FontOptions (15.0f));
    advancedTitle.setColour (
        juce::Label::textColourId,
        textPrimary);

    advancedStatus.setFont (
        juce::FontOptions (10.5f));
    advancedStatus.setColour (
        juce::Label::textColourId,
        textSecondary);
    advancedStatus.setJustificationType (
        juce::Justification::centredLeft);

    addAndMakeVisible (title);
    addAndMakeVisible (profileStatus);
    addAndMakeVisible (profileName);

    addAndMakeVisible (abButton);
    addAndMakeVisible (undoButton);
    addAndMakeVisible (redoButton);
    addAndMakeVisible (helpButton);

    addAndMakeVisible (learn);
    addAndMakeVisible (hearRemoved);
    addAndMakeVisible (bypass);
    addAndMakeVisible (advanced);
    addAndMakeVisible (quality);

    addAndMakeVisible (reduction);
    addAndMakeVisible (preserve);
    addAndMakeVisible (silence);
    addAndMakeVisible (profileOffset);

    addAndMakeVisible (reductionLabel);
    addAndMakeVisible (preserveLabel);
    addAndMakeVisible (silenceLabel);
    addAndMakeVisible (profileOffsetLabel);

    addAndMakeVisible (learnPopupTitle);
    addAndMakeVisible (learnPopupInstruction);
    addAndMakeVisible (learnPopupStatus);
    addAndMakeVisible (learnPopupClose);

    addAndMakeVisible (advancedTitle);
    addAndMakeVisible (advancedStatus);
    addAndMakeVisible (advancedClose);

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
            state, "thresholdOffset", profileOffset);

    removedAttachment =
        std::make_unique<ButtonAttachment> (
            state, "hearRemoved", hearRemoved);

    qualityAttachment =
        std::make_unique<ComboAttachment> (
            state, "quality", quality);

    learn.onClick =
        [this]
        {
            if (advancedDrawerVisible)
                showAdvancedDrawer (false);

            processor.startNoiseLearn();
            showLearnPopup (true);
        };

    learnPopupClose.onClick =
        [this]
        {
            showLearnPopup (false);
        };

    advanced.onClick =
        [this]
        {
            showAdvancedDrawer (
                ! advancedDrawerVisible);
        };

    advancedClose.onClick =
        [this]
        {
            showAdvancedDrawer (false);
        };

    helpButton.onClick =
        [this]
        {
            showAdvancedDrawer (true);
        };

    bypass.onClick =
        [this]
        {
            setBypassed (
                bypass.getToggleState());
        };

    showLearnPopup (false);
    showAdvancedDrawer (false);
    syncBypassButton();

    startTimerHz (24);
}

SmartDenoiseAudioProcessorEditor::
~SmartDenoiseAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void SmartDenoiseAudioProcessorEditor::configureRotary (
    juce::Slider& slider,
    bool primary)
{
    slider.setSliderStyle (
        juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (
        juce::Slider::NoTextBox,
        false,
        0,
        0);
    slider.setMouseDragSensitivity (
        primary ? 240 : 180);
}

void SmartDenoiseAudioProcessorEditor::showLearnPopup (
    bool shouldShow)
{
    learnPopupVisible = shouldShow;

    const bool mainVisible = ! shouldShow;

    profileName.setVisible (mainVisible);
    profileStatus.setVisible (mainVisible);
    learn.setVisible (mainVisible);
    reduction.setVisible (mainVisible);
    preserve.setVisible (mainVisible);
    silence.setVisible (mainVisible);
    reductionLabel.setVisible (mainVisible);
    preserveLabel.setVisible (mainVisible);
    silenceLabel.setVisible (mainVisible);
    hearRemoved.setVisible (mainVisible);
    bypass.setVisible (mainVisible);
    advanced.setVisible (mainVisible);
    quality.setVisible (mainVisible);

    learnPopupTitle.setVisible (shouldShow);
    learnPopupInstruction.setVisible (shouldShow);
    learnPopupStatus.setVisible (shouldShow);
    learnPopupClose.setVisible (shouldShow);

    resized();
    repaint();
}

void SmartDenoiseAudioProcessorEditor::showAdvancedDrawer (
    bool shouldShow)
{
    if (shouldShow && learnPopupVisible)
        showLearnPopup (false);

    advancedDrawerVisible = shouldShow;

    profileOffset.setVisible (shouldShow);
    profileOffsetLabel.setVisible (shouldShow);
    advancedTitle.setVisible (shouldShow);
    advancedStatus.setVisible (shouldShow);
    advancedClose.setVisible (shouldShow);

    setSize (
        940,
        shouldShow ? 700 : 540);

    resized();
    repaint();
}

void SmartDenoiseAudioProcessorEditor::setBypassed (
    bool shouldBypass)
{
    if (auto* parameter =
            processor.getParameters().getParameter (
                "enabled"))
    {
        parameter->beginChangeGesture();
        parameter->setValueNotifyingHost (
            shouldBypass ? 0.0f : 1.0f);
        parameter->endChangeGesture();
    }
}

void SmartDenoiseAudioProcessorEditor::syncBypassButton()
{
    if (const auto* enabledValue =
            processor.getParameters().getRawParameterValue (
                "enabled"))
    {
        bypass.setToggleState (
            enabledValue->load() < 0.5f,
            juce::dontSendNotification);
    }
}

void SmartDenoiseAudioProcessorEditor::paint (
    juce::Graphics& g)
{
    juce::ColourGradient background (
        backgroundTop,
        0.0f,
        0.0f,
        backgroundBottom,
        0.0f,
        static_cast<float> (getHeight()),
        false);

    g.setGradientFill (background);
    g.fillAll();

    g.setColour (border.withAlpha (0.8f));
    g.drawRoundedRectangle (
        getLocalBounds()
            .toFloat()
            .reduced (0.5f),
        12.0f,
        1.0f);

    drawHeader (g);
    drawCaptureSection (g);
    drawCleanSection (g);
    drawCheckSection (g);
    drawActivityStrip (g);
    drawFooter (g);

    if (advancedDrawerVisible)
        drawAdvancedDrawer (g);

    if (learnPopupVisible)
        drawLearnPopup (g);
}

void SmartDenoiseAudioProcessorEditor::drawHeader (
    juce::Graphics& g)
{
    auto header =
        juce::Rectangle<int> (
            14, 10, getWidth() - 28, 46);

    g.setColour (surface.withAlpha (0.96f));
    g.fillRoundedRectangle (
        header.toFloat(),
        10.0f);

    g.setColour (border.withAlpha (0.75f));
    g.drawRoundedRectangle (
        header.toFloat(),
        10.0f,
        1.0f);

    auto logoArea =
        juce::Rectangle<float> (
            27.0f,
            22.0f,
            24.0f,
            22.0f);

    juce::Path waveform;
    waveform.startNewSubPath (
        logoArea.getX(),
        logoArea.getCentreY());
    waveform.lineTo (
        logoArea.getX() + 4.0f,
        logoArea.getCentreY());
    waveform.lineTo (
        logoArea.getX() + 7.0f,
        logoArea.getY() + 3.0f);
    waveform.lineTo (
        logoArea.getX() + 11.0f,
        logoArea.getBottom() - 3.0f);
    waveform.lineTo (
        logoArea.getX() + 15.0f,
        logoArea.getY() + 6.0f);
    waveform.lineTo (
        logoArea.getX() + 18.0f,
        logoArea.getCentreY());
    waveform.lineTo (
        logoArea.getRight(),
        logoArea.getCentreY());

    g.setGradientFill (
        accentGradient (logoArea));
    g.strokePath (
        waveform,
        juce::PathStrokeType (2.2f));

    g.setColour (border.withAlpha (0.8f));
    g.drawLine (
        14.0f,
        56.0f,
        static_cast<float> (getWidth() - 14),
        56.0f,
        1.0f);
}

void SmartDenoiseAudioProcessorEditor::drawCaptureSection (
    juce::Graphics& g)
{
    g.setColour (surface.withAlpha (0.88f));
    g.fillRoundedRectangle (
        captureBounds.toFloat(),
        10.0f);

    g.setColour (border.withAlpha (0.7f));
    g.drawRoundedRectangle (
        captureBounds.toFloat(),
        10.0f,
        1.0f);

    drawStepHeader (
        g,
        juce::Rectangle<int> (
            captureBounds.getX() + 14,
            captureBounds.getY() + 12,
            captureBounds.getWidth() - 28,
            24),
        1,
        "CAPTURE");

    if (learnPopupVisible)
        return;

    auto profilePill =
        juce::Rectangle<float> (
            static_cast<float> (
                captureBounds.getX() + 18),
            static_cast<float> (
                captureBounds.getY() + 190),
            static_cast<float> (
                captureBounds.getWidth() - 36),
            30.0f);

    g.setColour (surfaceRaised);
    g.fillRoundedRectangle (profilePill, 15.0f);
    g.setColour (border);
    g.drawRoundedRectangle (
        profilePill,
        15.0f,
        1.0f);

    g.setColour (accentPurple);
    g.fillEllipse (
        profilePill.getX() + 10.0f,
        profilePill.getCentreY() - 4.0f,
        8.0f,
        8.0f);

    g.setColour (textSecondary);
    g.setFont (juce::FontOptions (10.0f));
    g.drawText (
        "Profile Health",
        captureBounds.getX() + 18,
        captureBounds.getY() + 228,
        captureBounds.getWidth() - 36,
        18,
        juce::Justification::centredLeft);

    auto health =
        juce::Rectangle<float> (
            static_cast<float> (
                captureBounds.getX() + 18),
            static_cast<float> (
                captureBounds.getY() + 254),
            static_cast<float> (
                captureBounds.getWidth() - 78),
            5.0f);

    g.setColour (juce::Colour::fromRGB (45, 51, 65));
    g.fillRoundedRectangle (health, 2.5f);

    const float qualityValue =
        processor.getEngine().hasProfile()
        ? juce::jlimit (
              0.0f,
              1.0f,
              processor.getEngine().getProfileQuality())
        : 0.0f;

    auto qualityFill = health;
    qualityFill.setWidth (
        health.getWidth() * qualityValue);

    g.setGradientFill (
        accentGradient (health));
    g.fillRoundedRectangle (
        qualityFill,
        2.5f);

    g.setColour (
        processor.getEngine().hasProfile()
        ? accentCyan
        : textSecondary);
    g.setFont (juce::FontOptions (10.0f));
    g.drawText (
        processor.getEngine().hasProfile()
        ? juce::String (
              juce::roundToInt (
                  qualityValue * 100.0f))
              + "%"
        : "--",
        captureBounds.getRight() - 55,
        captureBounds.getY() + 246,
        38,
        20,
        juce::Justification::centredRight);

    if (processor.getEngine().hasProfile())
    {
        auto frozen =
            juce::Rectangle<float> (
                static_cast<float> (
                    captureBounds.getX() + 18),
                static_cast<float> (
                    captureBounds.getBottom() - 50),
                static_cast<float> (
                    captureBounds.getWidth() - 36),
                28.0f);

        g.setColour (
            accentBlue.withAlpha (0.09f));
        g.fillRoundedRectangle (frozen, 8.0f);
        g.setColour (
            accentBlue.withAlpha (0.42f));
        g.drawRoundedRectangle (
            frozen,
            8.0f,
            1.0f);
        g.setColour (accentCyan);
        g.setFont (juce::FontOptions (10.0f));
        g.drawText (
            "FROZEN PROFILE",
            frozen.toNearestInt(),
            juce::Justification::centred);
    }
}

void SmartDenoiseAudioProcessorEditor::drawCleanSection (
    juce::Graphics& g)
{
    g.setColour (surfaceRaised.withAlpha (0.72f));
    g.fillRoundedRectangle (
        cleanBounds.toFloat(),
        10.0f);

    g.setColour (border.withAlpha (0.62f));
    g.drawRoundedRectangle (
        cleanBounds.toFloat(),
        10.0f,
        1.0f);

    drawStepHeader (
        g,
        juce::Rectangle<int> (
            cleanBounds.getX() + 14,
            cleanBounds.getY() + 12,
            cleanBounds.getWidth() - 28,
            24),
        2,
        "CLEAN");

    if (learnPopupVisible)
        return;

    const float normalizedReduction =
        juce::jlimit (
            0.0f,
            1.0f,
            static_cast<float> (
                reduction.getValue()
                / smartdenoise::SmartDenoiseEngine::
                    maxReductionDb));

    juce::String character = "Gentle";
    if (normalizedReduction > 0.68f)
        character = "Strong";
    else if (normalizedReduction > 0.30f)
        character = "Moderate";

    g.setColour (textSecondary);
    g.setFont (juce::FontOptions (10.5f));
    g.drawText (
        character,
        reduction.getX(),
        reduction.getBottom() - 28,
        reduction.getWidth(),
        18,
        juce::Justification::centred);

    g.setColour (border.withAlpha (0.55f));
    g.drawVerticalLine (
        preserve.getX() - 14,
        static_cast<float> (
            cleanBounds.getY() + 58),
        static_cast<float> (
            cleanBounds.getBottom() - 22));

    g.setColour (textSecondary.withAlpha (0.82f));
    g.setFont (juce::FontOptions (9.5f));
    g.drawText (
        "Protect presence and consonants",
        preserve.getX() - 8,
        preserve.getBottom() - 4,
        preserve.getWidth() + 16,
        15,
        juce::Justification::centred);

    g.drawText (
        "Quiet-region clean-up",
        silence.getX() - 8,
        silence.getBottom() - 4,
        silence.getWidth() + 16,
        15,
        juce::Justification::centred);
}

void SmartDenoiseAudioProcessorEditor::drawCheckSection (
    juce::Graphics& g)
{
    g.setColour (surface.withAlpha (0.90f));
    g.fillRoundedRectangle (
        checkBounds.toFloat(),
        10.0f);

    g.setColour (border.withAlpha (0.70f));
    g.drawRoundedRectangle (
        checkBounds.toFloat(),
        10.0f,
        1.0f);

    drawStepHeader (
        g,
        juce::Rectangle<int> (
            checkBounds.getX() + 14,
            checkBounds.getY() + 12,
            checkBounds.getWidth() - 28,
            24),
        3,
        "CHECK");

    if (learnPopupVisible)
        return;

    drawMeter (
        g,
        juce::Rectangle<float> (
            static_cast<float> (
                checkBounds.getRight() - 60),
            static_cast<float> (
                checkBounds.getY() + 83),
            13.0f,
            168.0f),
        displayedInputDb,
        "IN");

    drawMeter (
        g,
        juce::Rectangle<float> (
            static_cast<float> (
                checkBounds.getRight() - 34),
            static_cast<float> (
                checkBounds.getY() + 83),
            13.0f,
            168.0f),
        displayedOutputDb,
        "OUT");

    const auto analysis =
        processor.getEngine().getFrameAnalysis();

    g.setColour (border.withAlpha (0.55f));
    g.drawHorizontalLine (
        checkBounds.getBottom() - 77,
        static_cast<float> (
            checkBounds.getX() + 16),
        static_cast<float> (
            checkBounds.getRight() - 16));

    g.setColour (textSecondary);
    g.setFont (juce::FontOptions (9.3f));
    g.drawText (
        "P3 DETAIL GUARD",
        checkBounds.getX() + 16,
        checkBounds.getBottom() - 65,
        112,
        16,
        juce::Justification::centredLeft);

    g.setColour (accentCyan);
    g.drawText (
        juce::String (
            juce::roundToInt (
                analysis.detailProtection * 100.0f))
            + "%",
        checkBounds.getRight() - 58,
        checkBounds.getBottom() - 65,
        42,
        16,
        juce::Justification::centredRight);

    g.setColour (textSecondary);
    g.drawText (
        "TAIL PROTECT",
        checkBounds.getX() + 16,
        checkBounds.getBottom() - 43,
        100,
        16,
        juce::Justification::centredLeft);

    g.setColour (accentPurple.brighter (0.25f));
    g.drawText (
        juce::String (
            juce::roundToInt (
                analysis.tailProtection * 100.0f))
            + "%",
        checkBounds.getRight() - 58,
        checkBounds.getBottom() - 43,
        42,
        16,
        juce::Justification::centredRight);
}

void SmartDenoiseAudioProcessorEditor::drawActivityStrip (
    juce::Graphics& g)
{
    g.setColour (surface.withAlpha (0.94f));
    g.fillRoundedRectangle (
        activityBounds.toFloat(),
        8.0f);

    g.setColour (border.withAlpha (0.65f));
    g.drawRoundedRectangle (
        activityBounds.toFloat(),
        8.0f,
        1.0f);

    auto graph =
        activityBounds.reduced (50, 10).toFloat();
    graph.removeFromTop (4.0f);

    g.setColour (border.withAlpha (0.35f));
    g.drawLine (
        graph.getX(),
        graph.getCentreY(),
        graph.getRight(),
        graph.getCentreY(),
        1.0f);

    auto makePath =
        [&] (const std::array<float, 112>& values)
        {
            juce::Path path;
            for (size_t i = 0;
                 i < values.size();
                 ++i)
            {
                const float norm =
                    juce::jlimit (
                        0.0f,
                        1.0f,
                        (values[i] + 72.0f)
                            / 72.0f);

                const float x =
                    graph.getX()
                    + graph.getWidth()
                        * static_cast<float> (i)
                        / static_cast<float> (
                            values.size() - 1);

                const float y =
                    graph.getBottom()
                    - norm * graph.getHeight();

                if (i == 0)
                    path.startNewSubPath (x, y);
                else
                    path.lineTo (x, y);
            }
            return path;
        };

    const auto inputPath =
        makePath (inputHistory);
    const auto outputPath =
        makePath (outputHistory);

    g.setColour (
        accentPurple.withAlpha (0.74f));
    g.strokePath (
        inputPath,
        juce::PathStrokeType (1.25f));

    g.setColour (
        accentBlue.brighter (0.15f));
    g.strokePath (
        outputPath,
        juce::PathStrokeType (1.35f));

    g.setColour (textSecondary);
    g.setFont (juce::FontOptions (9.5f));
    g.drawText (
        "Input",
        activityBounds.getX() + 14,
        activityBounds.getCentreY() - 10,
        35,
        20,
        juce::Justification::centredLeft);
    g.drawText (
        "Output",
        activityBounds.getRight() - 47,
        activityBounds.getCentreY() - 10,
        35,
        20,
        juce::Justification::centredRight);
}

void SmartDenoiseAudioProcessorEditor::drawFooter (
    juce::Graphics& g)
{
    g.setColour (surface.withAlpha (0.96f));
    g.fillRoundedRectangle (
        footerBounds.toFloat(),
        8.0f);

    g.setColour (border.withAlpha (0.65f));
    g.drawRoundedRectangle (
        footerBounds.toFloat(),
        8.0f,
        1.0f);

    if (learnPopupVisible)
        return;

    g.setColour (textSecondary);
    g.setFont (juce::FontOptions (9.5f));
    g.drawText (
        "Quality",
        quality.getX() - 52,
        quality.getY(),
        46,
        quality.getHeight(),
        juce::Justification::centredRight);

    g.setColour (textSecondary.withAlpha (0.78f));
    g.setFont (juce::FontOptions (9.2f));
    g.drawText (
        "Smart Denoise  v0.3",
        footerBounds.getRight() - 135,
        footerBounds.getY(),
        120,
        footerBounds.getHeight(),
        juce::Justification::centredRight);
}

void SmartDenoiseAudioProcessorEditor::drawLearnPopup (
    juce::Graphics& g)
{
    auto dim =
        juce::Rectangle<int> (
            14,
            57,
            getWidth() - 28,
            472);

    g.setColour (
        juce::Colours::black.withAlpha (0.72f));
    g.fillRoundedRectangle (
        dim.toFloat(),
        10.0f);

    g.setColour (surfaceRaised);
    g.fillRoundedRectangle (
        learnPopupBounds.toFloat(),
        14.0f);

    g.setColour (
        accentPurple.withAlpha (0.62f));
    g.drawRoundedRectangle (
        learnPopupBounds.toFloat(),
        14.0f,
        1.2f);

    const auto& engine =
        processor.getEngine();

    const float progress =
        juce::jlimit (
            0.0f,
            1.0f,
            engine.getLearningProgress());

    auto circle =
        juce::Rectangle<float> (
            static_cast<float> (
                learnPopupBounds.getX() + 27),
            static_cast<float> (
                learnPopupBounds.getY() + 82),
            104.0f,
            104.0f);

    const auto centre = circle.getCentre();
    const float radius = 45.0f;
    const float start =
        juce::MathConstants<float>::pi * 1.25f;
    const float end =
        juce::MathConstants<float>::pi * 2.75f;

    juce::Path baseArc;
    baseArc.addCentredArc (
        centre.x,
        centre.y,
        radius,
        radius,
        0.0f,
        start,
        end,
        true);

    g.setColour (juce::Colour::fromRGB (47, 53, 69));
    g.strokePath (
        baseArc,
        juce::PathStrokeType (7.0f));

    juce::Path progressArc;
    progressArc.addCentredArc (
        centre.x,
        centre.y,
        radius,
        radius,
        0.0f,
        start,
        start + progress * (end - start),
        true);

    g.setGradientFill (
        accentGradient (circle));
    g.strokePath (
        progressArc,
        juce::PathStrokeType (7.0f));

    g.setColour (textPrimary);
    g.setFont (juce::FontOptions (29.0f));

    juce::String centreText = "3";
    if (engine.isLearning())
    {
        centreText =
            juce::String (
                juce::roundToInt (
                    100.0f * progress))
            + "%";
    }
    else if (engine.hasProfile())
    {
        centreText = "OK";
    }

    g.drawText (
        centreText,
        circle.toNearestInt(),
        juce::Justification::centred);

    g.setColour (textSecondary);
    g.setFont (juce::FontOptions (9.5f));
    g.drawText (
        engine.isLearning()
        ? "capturing"
        : "seconds",
        circle.toNearestInt().translated (0, 31),
        juce::Justification::centred);
}

void SmartDenoiseAudioProcessorEditor::drawAdvancedDrawer (
    juce::Graphics& g)
{
    g.setColour (surface.withAlpha (0.97f));
    g.fillRoundedRectangle (
        advancedBounds.toFloat(),
        10.0f);

    g.setColour (border.withAlpha (0.85f));
    g.drawRoundedRectangle (
        advancedBounds.toFloat(),
        10.0f,
        1.0f);

    auto tabs =
        juce::Rectangle<int> (
            advancedBounds.getX() + 12,
            advancedBounds.getY() + 42,
            126,
            advancedBounds.getHeight() - 55);

    g.setColour (surfaceRaised);
    g.fillRoundedRectangle (
        tabs.toFloat(),
        8.0f);

    const std::array<juce::String, 4> names {
        "Processing",
        "Noise Profile",
        "P3 Guard",
        "Output"
    };

    for (int i = 0; i < 4; ++i)
    {
        auto row =
            juce::Rectangle<int> (
                tabs.getX() + 6,
                tabs.getY() + 6 + i * 27,
                tabs.getWidth() - 12,
                23);

        if (i == 0)
        {
            g.setColour (
                accentPurple.withAlpha (0.20f));
            g.fillRoundedRectangle (
                row.toFloat(),
                6.0f);
        }

        g.setColour (
            i == 0
            ? textPrimary
            : textSecondary);
        g.setFont (juce::FontOptions (10.5f));
        g.drawText (
            names[static_cast<size_t> (i)],
            row,
            juce::Justification::centredLeft);
    }

    auto content =
        advancedBounds.reduced (160, 42);

    g.setColour (textSecondary);
    g.setFont (juce::FontOptions (9.8f));
    g.drawText (
        "MAX REDUCTION",
        content.getX(),
        content.getY() + 7,
        120,
        18,
        juce::Justification::centredLeft);

    g.setColour (textPrimary);
    g.setFont (juce::FontOptions (11.0f));
    g.drawText (
        "24.0 dB",
        content.getX() + 122,
        content.getY() + 7,
        80,
        18,
        juce::Justification::centredLeft);

    g.setColour (textSecondary);
    g.setFont (juce::FontOptions (9.8f));
    g.drawText (
        "FROZEN PROFILE",
        content.getX() + 350,
        content.getY() + 7,
        115,
        18,
        juce::Justification::centredLeft);

    g.setColour (
        processor.getEngine().hasProfile()
        ? accentCyan
        : textSecondary);
    g.drawText (
        processor.getEngine().hasProfile()
        ? "LOCKED"
        : "NOT LEARNED",
        content.getX() + 468,
        content.getY() + 7,
        90,
        18,
        juce::Justification::centredLeft);

    const auto analysis =
        processor.getEngine().getFrameAnalysis();

    auto detailBar =
        juce::Rectangle<float> (
            static_cast<float> (
                content.getX() + 350),
            static_cast<float> (
                content.getY() + 55),
            180.0f,
            5.0f);

    g.setColour (juce::Colour::fromRGB (45, 51, 65));
    g.fillRoundedRectangle (detailBar, 2.5f);

    auto detailFill = detailBar;
    detailFill.setWidth (
        detailBar.getWidth()
        * juce::jlimit (
            0.0f,
            1.0f,
            analysis.detailProtection));

    g.setGradientFill (
        accentGradient (detailBar));
    g.fillRoundedRectangle (
        detailFill,
        2.5f);

    g.setColour (textSecondary);
    g.setFont (juce::FontOptions (9.5f));
    g.drawText (
        "P3 detail / tail protection is automatic and profile-safe.",
        content.getX() + 350,
        content.getY() + 66,
        300,
        20,
        juce::Justification::centredLeft);
}

void SmartDenoiseAudioProcessorEditor::drawMeter (
    juce::Graphics& g,
    juce::Rectangle<float> bounds,
    float valueDb,
    const juce::String& label)
{
    const float clamped =
        juce::jlimit (-60.0f, 0.0f, valueDb);
    const float norm =
        (clamped + 60.0f) / 60.0f;

    g.setColour (textSecondary);
    g.setFont (juce::FontOptions (9.5f));
    g.drawText (
        label,
        juce::Rectangle<int> (
            static_cast<int> (bounds.getX() - 8.0f),
            static_cast<int> (bounds.getY() - 25.0f),
            30,
            18),
        juce::Justification::centred);

    constexpr int segments = 18;
    const float gap = 2.0f;
    const float segmentHeight =
        (bounds.getHeight()
         - gap * static_cast<float> (segments - 1))
        / static_cast<float> (segments);

    const int lit =
        juce::roundToInt (
            norm * static_cast<float> (segments));

    for (int i = 0; i < segments; ++i)
    {
        const int fromBottom = i;
        const float y =
            bounds.getBottom()
            - segmentHeight
            - static_cast<float> (fromBottom)
                * (segmentHeight + gap);

        auto segment =
            juce::Rectangle<float> (
                bounds.getX(),
                y,
                bounds.getWidth(),
                segmentHeight);

        if (i < lit)
        {
            const float position =
                static_cast<float> (i)
                / static_cast<float> (segments - 1);

            g.setColour (
                accentBlue.interpolatedWith (
                    accentPurple,
                    position));
        }
        else
        {
            g.setColour (
                juce::Colour::fromRGB (35, 41, 54));
        }

        g.fillRoundedRectangle (
            segment,
            1.5f);
    }

    g.setColour (textSecondary);
    g.setFont (juce::FontOptions (8.6f));
    g.drawText (
        juce::String (valueDb, 1),
        juce::Rectangle<int> (
            static_cast<int> (bounds.getX() - 14.0f),
            static_cast<int> (bounds.getBottom() + 5.0f),
            42,
            16),
        juce::Justification::centred);
}

void SmartDenoiseAudioProcessorEditor::resized()
{
    title.setBounds (58, 15, 230, 36);

    abButton.setBounds (705, 17, 58, 30);
    undoButton.setBounds (768, 17, 48, 30);
    redoButton.setBounds (818, 17, 48, 30);
    helpButton.setBounds (872, 17, 34, 30);

    captureBounds = { 14, 65, 220, 338 };
    cleanBounds = { 242, 65, 468, 338 };
    checkBounds = { 718, 65, 208, 338 };
    activityBounds = { 14, 411, 912, 67 };
    footerBounds = { 14, 486, 912, 42 };

    learn.setBounds (
        captureBounds.getX() + 30,
        captureBounds.getY() + 53,
        captureBounds.getWidth() - 60,
        126);

    profileName.setBounds (
        captureBounds.getX() + 43,
        captureBounds.getY() + 191,
        captureBounds.getWidth() - 72,
        28);

    profileStatus.setBounds (
        captureBounds.getX() + 18,
        captureBounds.getY() + 269,
        captureBounds.getWidth() - 36,
        34);

    reductionLabel.setBounds (
        cleanBounds.getX() + 40,
        cleanBounds.getY() + 51,
        238,
        23);

    reduction.setBounds (
        cleanBounds.getX() + 24,
        cleanBounds.getY() + 68,
        270,
        244);

    preserveLabel.setBounds (
        cleanBounds.getX() + 315,
        cleanBounds.getY() + 57,
        130,
        20);

    preserve.setBounds (
        cleanBounds.getX() + 321,
        cleanBounds.getY() + 77,
        118,
        116);

    silenceLabel.setBounds (
        cleanBounds.getX() + 315,
        cleanBounds.getY() + 197,
        130,
        20);

    silence.setBounds (
        cleanBounds.getX() + 321,
        cleanBounds.getY() + 216,
        118,
        112);

    hearRemoved.setBounds (
        checkBounds.getX() + 15,
        checkBounds.getY() + 73,
        116,
        67);

    bypass.setBounds (
        checkBounds.getX() + 15,
        checkBounds.getY() + 151,
        116,
        58);

    advanced.setBounds (
        footerBounds.getX() + 8,
        footerBounds.getY() + 7,
        104,
        28);

    quality.setBounds (
        footerBounds.getCentreX() - 70,
        footerBounds.getY() + 7,
        185,
        28);

    learnPopupBounds =
        juce::Rectangle<int> (
            getWidth() / 2 - 175,
            135,
            350,
            265);

    learnPopupTitle.setBounds (
        learnPopupBounds.getX() + 18,
        learnPopupBounds.getY() + 12,
        learnPopupBounds.getWidth() - 36,
        28);

    learnPopupInstruction.setBounds (
        learnPopupBounds.getX() + 150,
        learnPopupBounds.getY() + 82,
        178,
        80);

    learnPopupStatus.setBounds (
        learnPopupBounds.getX() + 150,
        learnPopupBounds.getY() + 164,
        178,
        38);

    learnPopupClose.setBounds (
        learnPopupBounds.getRight() - 102,
        learnPopupBounds.getBottom() - 46,
        80,
        28);

    if (advancedDrawerVisible)
    {
        advancedBounds =
            juce::Rectangle<int> (
                14,
                542,
                912,
                145);

        advancedTitle.setBounds (
            advancedBounds.getX() + 14,
            advancedBounds.getY() + 8,
            120,
            26);

        advancedClose.setBounds (
            advancedBounds.getRight() - 84,
            advancedBounds.getY() + 8,
            66,
            24);

        profileOffsetLabel.setBounds (
            advancedBounds.getX() + 162,
            advancedBounds.getY() + 48,
            116,
            24);

        profileOffset.setBounds (
            advancedBounds.getX() + 276,
            advancedBounds.getY() + 46,
            235,
            28);

        advancedStatus.setBounds (
            advancedBounds.getX() + 162,
            advancedBounds.getY() + 93,
            advancedBounds.getWidth() - 188,
            28);
    }
}

void SmartDenoiseAudioProcessorEditor::timerCallback()
{
    const float currentInput =
        processor.getInputPeakDb();
    const float currentOutput =
        processor.getOutputPeakDb();

    displayedInputDb =
        currentInput > displayedInputDb
        ? currentInput
        : juce::jmax (
              -72.0f,
              displayedInputDb - 1.8f);

    displayedOutputDb =
        currentOutput > displayedOutputDb
        ? currentOutput
        : juce::jmax (
              -72.0f,
              displayedOutputDb - 1.8f);

    std::move (
        inputHistory.begin() + 1,
        inputHistory.end(),
        inputHistory.begin());
    std::move (
        outputHistory.begin() + 1,
        outputHistory.end(),
        outputHistory.begin());

    inputHistory.back() = displayedInputDb;
    outputHistory.back() = displayedOutputDb;

    auto& engine = processor.getEngine();

    juce::String profileText;

    if (engine.isLearning())
    {
        const int progress =
            juce::roundToInt (
                engine.getLearningProgress()
                * 100.0f);

        profileText =
            "Learning "
            + juce::String (progress)
            + "%";

        learn.setButtonText (
            "Learning...\n"
            + juce::String (progress)
            + "%");

        learnPopupStatus.setText (
            "Capturing profile... "
            + juce::String (progress)
            + "%",
            juce::dontSendNotification);
    }
    else if (engine.wasLastLearnRejected())
    {
        profileText =
            engine.hasProfile()
            ? "Learn rejected | previous profile kept"
            : "Learn rejected | retry with noise only";

        learn.setButtonText (
            "Re-learn Noise\n3s");

        learnPopupStatus.setText (
            profileText,
            juce::dontSendNotification);
    }
    else if (engine.hasProfile())
    {
        const int qualityPercent =
            juce::roundToInt (
                engine.getProfileQuality()
                * 100.0f);

        profileText =
            "Profile Health "
            + juce::String (qualityPercent)
            + "% | Frozen";

        profileName.setText (
            "Frozen Noise Profile",
            juce::dontSendNotification);

        learn.setButtonText (
            "Re-learn Noise\n3s");

        learnPopupStatus.setText (
            "Profile locked at "
            + juce::String (qualityPercent)
            + "% quality",
            juce::dontSendNotification);
    }
    else
    {
        profileText =
            "No profile - learn room noise";
        profileName.setText (
            "Studio Noise",
            juce::dontSendNotification);
        learn.setButtonText (
            "Learn Noise\n3s");
        learnPopupStatus.setText (
            "Waiting for a valid profile",
            juce::dontSendNotification);
    }

    profileStatus.setText (
        profileText,
        juce::dontSendNotification);

    const auto analysis =
        engine.getFrameAnalysis();

    advancedStatus.setText (
        "Detail Guard "
        + juce::String (
            juce::roundToInt (
                analysis.detailProtection * 100.0f))
        + "%   |   Tail Protect "
        + juce::String (
            juce::roundToInt (
                analysis.tailProtection * 100.0f))
        + "%   |   Latency "
        + juce::String (
            engine.getLatencySamples())
        + " samples",
        juce::dontSendNotification);

    syncBypassButton();
    repaint();
}
