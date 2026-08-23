#include "PluginEditor.h"

#include <algorithm>
#include <cmath>

namespace
{
namespace ui
{
const auto backgroundTop = juce::Colour::fromRGB (6, 9, 16);
const auto backgroundBottom = juce::Colour::fromRGB (11, 17, 29);
const auto surface = juce::Colour::fromRGB (12, 17, 26);
const auto surfaceRaised = juce::Colour::fromRGB (16, 22, 34);
const auto surfaceSoft = juce::Colour::fromRGB (22, 29, 43);
const auto surfaceBright = juce::Colour::fromRGB (29, 37, 54);
const auto border = juce::Colour::fromRGB (47, 57, 78);
const auto borderSoft = juce::Colour::fromRGB (31, 40, 57);
const auto textPrimary = juce::Colour::fromRGB (246, 247, 251);
const auto textSecondary = juce::Colour::fromRGB (151, 160, 184);
const auto textMuted = juce::Colour::fromRGB (98, 108, 134);
const auto accentPurple = juce::Colour::fromRGB (166, 96, 255);
const auto accentBlue = juce::Colour::fromRGB (76, 130, 255);
const auto accentCyan = juce::Colour::fromRGB (80, 204, 255);
const auto warning = juce::Colour::fromRGB (255, 178, 82);

constexpr float panelRadius = 12.0f;
constexpr float controlRadius = 10.0f;

juce::ColourGradient accentGradient (juce::Rectangle<float> area)
{
    return juce::ColourGradient (
        accentPurple,
        area.getTopLeft(),
        accentBlue,
        area.getBottomRight(),
        false);
}

juce::ColourGradient panelGradient (juce::Rectangle<float> area, bool raised)
{
    const auto top = raised
        ? juce::Colour::fromRGB (20, 27, 41)
        : juce::Colour::fromRGB (14, 20, 31);

    const auto bottom = raised
        ? juce::Colour::fromRGB (12, 17, 27)
        : juce::Colour::fromRGB (9, 14, 23);

    return juce::ColourGradient (
        top,
        area.getTopLeft(),
        bottom,
        area.getBottomLeft(),
        false);
}

void drawPanel (
    juce::Graphics& g,
    juce::Rectangle<float> area,
    bool raised)
{
    juce::DropShadow (
        juce::Colours::black.withAlpha (0.34f),
        raised ? 12 : 8,
        { 0, raised ? 4 : 2 })
        .drawForRectangle (g, area.toNearestInt());

    g.setGradientFill (panelGradient (area, raised));
    g.fillRoundedRectangle (area, panelRadius);

    g.setColour (
        juce::Colours::white.withAlpha (
            raised ? 0.045f : 0.028f));
    g.drawRoundedRectangle (
        area.reduced (1.0f),
        panelRadius - 1.0f,
        1.0f);

    g.setColour (
        border.withAlpha (
            raised ? 0.74f : 0.58f));
    g.drawRoundedRectangle (
        area,
        panelRadius,
        1.0f);
}

void drawStepHeader (
    juce::Graphics& g,
    juce::Rectangle<int> area,
    int number,
    const juce::String& text)
{
    auto badge = area.removeFromLeft (28).toFloat().reduced (3.0f);

    g.setGradientFill (panelGradient (badge, true));
    g.fillEllipse (badge);
    g.setColour (border.brighter (0.28f));
    g.drawEllipse (badge, 1.0f);

    g.setColour (textPrimary);
    g.setFont (juce::FontOptions (11.0f));
    g.drawText (
        juce::String (number),
        badge.toNearestInt(),
        juce::Justification::centred);

    g.setColour (accentPurple.brighter (0.22f));
    g.setFont (juce::FontOptions (12.8f));
    g.drawText (
        text,
        area,
        juce::Justification::centredLeft);
}

void drawHeadphoneIcon (
    juce::Graphics& g,
    juce::Rectangle<float> area,
    juce::Colour colour)
{
    auto icon = area.withSizeKeepingCentre (
        juce::jmin (area.getWidth(), 28.0f),
        juce::jmin (area.getHeight(), 28.0f));

    const auto centre = icon.getCentre();
    const float radius = icon.getWidth() * 0.34f;

    juce::Path arc;
    arc.addCentredArc (
        centre.x,
        centre.y + 2.0f,
        radius,
        radius,
        0.0f,
        juce::MathConstants<float>::pi * 1.15f,
        juce::MathConstants<float>::pi * 1.85f,
        true);

    g.setColour (colour);
    g.strokePath (arc, juce::PathStrokeType (2.0f));

    g.fillRoundedRectangle (
        icon.getX() + 3.0f,
        centre.y + 2.0f,
        4.0f,
        10.0f,
        2.0f);

    g.fillRoundedRectangle (
        icon.getRight() - 7.0f,
        centre.y + 2.0f,
        4.0f,
        10.0f,
        2.0f);
}

void drawBypassIcon (
    juce::Graphics& g,
    juce::Rectangle<float> area,
    juce::Colour colour)
{
    auto icon = area.withSizeKeepingCentre (24.0f, 24.0f);

    g.setColour (colour);
    g.drawEllipse (icon.reduced (3.5f), 1.8f);
    g.drawLine (
        icon.getX() + 5.0f,
        icon.getBottom() - 5.0f,
        icon.getRight() - 5.0f,
        icon.getY() + 5.0f,
        1.8f);
}

void drawWaveformIcon (
    juce::Graphics& g,
    juce::Rectangle<float> area,
    juce::Colour colour)
{
    auto icon = area.withSizeKeepingCentre (36.0f, 28.0f);
    const auto cy = icon.getCentreY();

    const std::array<float, 9> heights {
        6.0f, 13.0f, 20.0f, 10.0f, 25.0f,
        16.0f, 22.0f, 11.0f, 7.0f
    };

    const float step = icon.getWidth()
        / static_cast<float> (heights.size());

    g.setColour (colour);

    for (size_t i = 0; i < heights.size(); ++i)
    {
        const float x =
            icon.getX()
            + step * (static_cast<float> (i) + 0.5f);

        const float h = heights[i];
        g.drawLine (
            x,
            cy - h * 0.5f,
            x,
            cy + h * 0.5f,
            1.7f);
    }
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
            accentPurple.withAlpha (0.30f));
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
            diameter * (primary ? 0.425f : 0.39f);

        const auto centre = bounds.getCentre();
        const float stroke = primary ? 12.0f : 7.0f;

        if (primary)
        {
            constexpr int tickCount = 25;

            for (int i = 0; i < tickCount; ++i)
            {
                const float t =
                    static_cast<float> (i)
                    / static_cast<float> (tickCount - 1);

                const float angle =
                    startAngle
                    + t * (endAngle - startAngle);

                const auto p1 =
                    centre.getPointOnCircumference (
                        radius + 14.0f,
                        angle);

                const auto p2 =
                    centre.getPointOnCircumference (
                        radius + (i % 4 == 0 ? 19.0f : 17.0f),
                        angle);

                g.setColour (
                    textMuted.withAlpha (
                        i % 4 == 0 ? 0.52f : 0.28f));

                g.drawLine (
                    p1.x,
                    p1.y,
                    p2.x,
                    p2.y,
                    i % 4 == 0 ? 1.2f : 0.8f);
            }
        }

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

        g.setColour (
            juce::Colour::fromRGB (43, 50, 67));
        g.strokePath (
            baseArc,
            juce::PathStrokeType (
                stroke,
                juce::PathStrokeType::curved,
                juce::PathStrokeType::rounded));

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
                radius * 2.18f,
                radius * 2.18f);

        g.setColour (
            accentPurple.withAlpha (
                primary ? 0.14f : 0.10f));
        g.strokePath (
            valueArc,
            juce::PathStrokeType (
                stroke + (primary ? 8.0f : 5.0f),
                juce::PathStrokeType::curved,
                juce::PathStrokeType::rounded));

        g.setGradientFill (
            accentGradient (gradientArea));
        g.strokePath (
            valueArc,
            juce::PathStrokeType (
                stroke,
                juce::PathStrokeType::curved,
                juce::PathStrokeType::rounded));

        const float knobRadius =
            radius - (primary ? 20.0f : 13.0f);

        auto knob =
            juce::Rectangle<float> (
                knobRadius * 2.0f,
                knobRadius * 2.0f)
                .withCentre (centre);

        juce::DropShadow (
            juce::Colours::black.withAlpha (0.56f),
            primary ? 12 : 8,
            { 0, primary ? 5 : 3 })
            .drawForRectangle (
                g,
                knob.toNearestInt());

        juce::ColourGradient knobGradient (
            juce::Colour::fromRGB (31, 38, 55),
            centre.x - knobRadius * 0.45f,
            centre.y - knobRadius * 0.48f,
            juce::Colour::fromRGB (8, 12, 20),
            centre.x + knobRadius * 0.60f,
            centre.y + knobRadius * 0.70f,
            true);

        g.setGradientFill (knobGradient);
        g.fillEllipse (knob);

        g.setColour (
            juce::Colours::white.withAlpha (0.055f));
        g.drawEllipse (
            knob.reduced (1.0f),
            1.0f);

        g.setColour (
            border.withAlpha (0.86f));
        g.drawEllipse (knob, 1.0f);

        const auto marker =
            centre.getPointOnCircumference (
                radius,
                valueAngle);

        g.setColour (
            juce::Colours::black.withAlpha (0.30f));
        g.fillEllipse (
            juce::Rectangle<float> (
                primary ? 13.0f : 9.0f,
                primary ? 13.0f : 9.0f)
                .withCentre (
                    marker.translated (0.0f, 1.5f)));

        g.setColour (textPrimary);
        g.fillEllipse (
            juce::Rectangle<float> (
                primary ? 10.0f : 7.0f,
                primary ? 10.0f : 7.0f)
                .withCentre (marker));

        juce::String valueText;

        if (primary)
        {
            const auto percent =
                juce::roundToInt (
                    sliderPos * 100.0f);
            valueText =
                juce::String (percent) + "%";
        }
        else
        {
            const auto percent =
                juce::roundToInt (
                    juce::jlimit (
                        0.0,
                        1.0,
                        slider.getValue())
                    * 100.0);
            valueText =
                juce::String (percent) + "%";
        }

        g.setColour (textPrimary);
        g.setFont (
            juce::FontOptions (
                primary ? 38.0f : 18.0f));

        g.drawFittedText (
            valueText,
            knob.toNearestInt().reduced (
                primary ? 23 : 10),
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

        g.setColour (
            juce::Colour::fromRGB (42, 50, 66));
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
                juce::jmax (
                    0.0f,
                    sliderPos - left),
                4.0f);

        g.setGradientFill (
            accentGradient (area));
        g.fillRoundedRectangle (active, 2.0f);

        g.setColour (
            accentPurple.withAlpha (0.18f));
        g.fillEllipse (
            juce::Rectangle<float> (17.0f, 17.0f)
                .withCentre ({ sliderPos, cy }));

        g.setColour (textPrimary);
        g.fillEllipse (
            juce::Rectangle<float> (9.0f, 9.0f)
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
            button.getLocalBounds()
                .toFloat()
                .reduced (0.5f);

        const auto name = button.getName();
        const bool primary = name == "learn";
        const bool top = name == "top";
        const bool toggled =
            button.getToggleState();

        if (primary)
        {
            juce::DropShadow (
                accentPurple.withAlpha (
                    isHighlighted ? 0.24f : 0.14f),
                isHighlighted ? 15 : 11,
                { 0, 1 })
                .drawForRectangle (
                    g,
                    area.toNearestInt());

            juce::ColourGradient fill (
                juce::Colour::fromRGB (28, 28, 47),
                area.getTopLeft(),
                juce::Colour::fromRGB (13, 19, 34),
                area.getBottomRight(),
                false);

            g.setGradientFill (fill);
            g.fillRoundedRectangle (
                area,
                14.0f);

            g.setColour (
                accentPurple.withAlpha (0.20f));
            g.fillRoundedRectangle (
                area.reduced (1.0f),
                13.0f);

            g.setGradientFill (
                accentGradient (area));
            juce::Path borderPath;
            borderPath.addRoundedRectangle (
                area.reduced (0.5f),
                14.0f);
            g.strokePath (
                borderPath,
                juce::PathStrokeType (
                    isDown ? 2.0f : 1.4f));

            if (isHighlighted || isDown)
            {
                g.setColour (
                    juce::Colours::white.withAlpha (
                        isDown ? 0.08f : 0.035f));
                g.fillRoundedRectangle (
                    area.reduced (2.0f),
                    12.0f);
            }

            return;
        }

        if (top)
        {
            if (isHighlighted)
            {
                g.setColour (
                    surfaceSoft.withAlpha (0.72f));
                g.fillRoundedRectangle (
                    area,
                    8.0f);
            }

            return;
        }

        g.setGradientFill (
            panelGradient (area, true));
        g.fillRoundedRectangle (
            area,
            controlRadius);

        g.setColour (
            toggled
            ? accentPurple.withAlpha (0.12f)
            : juce::Colours::transparentBlack);
        g.fillRoundedRectangle (
            area.reduced (1.0f),
            controlRadius - 1.0f);

        g.setColour (
            toggled
            ? accentBlue.withAlpha (0.82f)
            : border.withAlpha (
                isHighlighted ? 0.95f : 0.70f));

        g.drawRoundedRectangle (
            area,
            controlRadius,
            toggled ? 1.4f : 1.0f);
    }

    void drawButtonText (
        juce::Graphics& g,
        juce::TextButton& button,
        bool,
        bool) override
    {
        const auto name = button.getName();
        const bool primary = name == "learn";
        const bool top = name == "top";

        const auto colour =
            button.isEnabled()
            ? textPrimary
            : textSecondary.withAlpha (0.45f);

        if (primary)
        {
            auto area =
                button.getLocalBounds()
                    .toFloat()
                    .reduced (10.0f);

            drawWaveformIcon (
                g,
                area.removeFromTop (42.0f),
                accentPurple.brighter (0.34f));

            g.setColour (textPrimary);
            g.setFont (juce::FontOptions (16.0f));
            g.drawFittedText (
                button.getButtonText(),
                area.toNearestInt().reduced (2, 2),
                juce::Justification::centred,
                2);
            return;
        }

        if (name == "monitor")
        {
            auto area =
                button.getLocalBounds()
                    .toFloat()
                    .reduced (8.0f);

            drawHeadphoneIcon (
                g,
                area.removeFromTop (31.0f),
                button.getToggleState()
                    ? accentCyan
                    : textPrimary);

            g.setColour (colour);
            g.setFont (juce::FontOptions (11.2f));
            g.drawFittedText (
                button.getButtonText(),
                area.toNearestInt(),
                juce::Justification::centred,
                1);
            return;
        }

        if (name == "bypassAction")
        {
            auto area =
                button.getLocalBounds()
                    .toFloat()
                    .reduced (8.0f);

            drawBypassIcon (
                g,
                area.removeFromTop (29.0f),
                button.getToggleState()
                    ? warning
                    : textPrimary);

            g.setColour (colour);
            g.setFont (juce::FontOptions (11.2f));
            g.drawFittedText (
                button.getButtonText(),
                area.toNearestInt(),
                juce::Justification::centred,
                1);
            return;
        }

        g.setColour (colour);
        g.setFont (
            juce::FontOptions (
                top ? 10.2f : 11.4f));

        g.drawFittedText (
            button.getButtonText(),
            button.getLocalBounds().reduced (7, 4),
            juce::Justification::centred,
            1);
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

        g.setGradientFill (
            panelGradient (area, true));
        g.fillRoundedRectangle (
            area,
            9.0f);

        g.setColour (
            juce::Colours::white.withAlpha (0.035f));
        g.drawRoundedRectangle (
            area.reduced (1.0f),
            8.0f,
            1.0f);

        g.setColour (border.withAlpha (0.82f));
        g.drawRoundedRectangle (
            area,
            9.0f,
            1.0f);

        juce::Path arrow;
        const float cx =
            static_cast<float> (width) - 15.0f;
        const float cy =
            static_cast<float> (height) * 0.5f;

        arrow.startNewSubPath (
            cx - 4.0f,
            cy - 2.0f);
        arrow.lineTo (
            cx,
            cy + 2.0f);
        arrow.lineTo (
            cx + 4.0f,
            cy - 2.0f);

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
            12,
            0,
            box.getWidth() - 32,
            box.getHeight());

        label.setFont (
            juce::FontOptions (11.0f));

        label.setColour (
            juce::Label::textColourId,
            textPrimary);
    }
};
} // namespace ui
} // namespace

SmartDenoiseAudioProcessorEditor::
SmartDenoiseAudioProcessorEditor (
    SmartDenoiseAudioProcessor& p)
    : juce::AudioProcessorEditor (&p),
      processor (p),
      conceptLookAndFeel (
          std::make_unique<ui::ConceptCLookAndFeel>())
{
    setOpaque (true);
    setLookAndFeel (conceptLookAndFeel.get());
    setSize (940, 540);

    title.setText (
        "Smart Denoise",
        juce::dontSendNotification);
    title.setFont (
        juce::FontOptions (21.0f));
    title.setColour (
        juce::Label::textColourId,
        ui::textPrimary);
    title.setJustificationType (
        juce::Justification::centredLeft);

    profileName.setText (
        "Studio Noise",
        juce::dontSendNotification);
    profileName.setFont (
        juce::FontOptions (11.2f));
    profileName.setColour (
        juce::Label::textColourId,
        ui::textPrimary);
    profileName.setJustificationType (
        juce::Justification::centredLeft);

    profileStatus.setText (
        "No profile - learn room noise",
        juce::dontSendNotification);
    profileStatus.setFont (
        juce::FontOptions (10.2f));
    profileStatus.setColour (
        juce::Label::textColourId,
        ui::textSecondary);
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
        ui::textPrimary);
    profileOffset.setColour (
        juce::Slider::textBoxBackgroundColourId,
        ui::surfaceRaised);
    profileOffset.setColour (
        juce::Slider::textBoxOutlineColourId,
        ui::border);

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

    reductionLabel.setFont (
        juce::FontOptions (13.5f));
    preserveLabel.setFont (
        juce::FontOptions (11.8f));
    silenceLabel.setFont (
        juce::FontOptions (11.8f));
    profileOffsetLabel.setFont (
        juce::FontOptions (11.0f));

    for (auto* label : {
             &reductionLabel,
             &preserveLabel,
             &silenceLabel,
             &profileOffsetLabel })
    {
        label->setColour (
            juce::Label::textColourId,
            ui::textPrimary);
        label->setJustificationType (
            juce::Justification::centred);
    }

    abButton.setName ("top");
    undoButton.setName ("top");
    redoButton.setName ("top");
    helpButton.setName ("top");
    learn.setName ("learn");
    hearRemoved.setName ("monitor");
    bypass.setName ("bypassAction");
    advanced.setName ("advancedAction");

    abButton.setInterceptsMouseClicks (
        false, false);
    undoButton.setInterceptsMouseClicks (
        false, false);
    redoButton.setInterceptsMouseClicks (
        false, false);

    hearRemoved.setClickingTogglesState (true);
    bypass.setClickingTogglesState (true);

    learnPopupTitle.setText (
        "Learn Noise",
        juce::dontSendNotification);
    learnPopupTitle.setFont (
        juce::FontOptions (17.0f));
    learnPopupTitle.setColour (
        juce::Label::textColourId,
        ui::textPrimary);

    learnPopupInstruction.setText (
        "Find a section containing only room or system noise.\n"
        "Smart Denoise captures a frozen 3-second profile.",
        juce::dontSendNotification);
    learnPopupInstruction.setFont (
        juce::FontOptions (10.5f));
    learnPopupInstruction.setColour (
        juce::Label::textColourId,
        ui::textSecondary);
    learnPopupInstruction.setJustificationType (
        juce::Justification::topLeft);

    learnPopupStatus.setFont (
        juce::FontOptions (11.0f));
    learnPopupStatus.setColour (
        juce::Label::textColourId,
        ui::accentCyan);
    learnPopupStatus.setJustificationType (
        juce::Justification::centredLeft);

    advancedTitle.setText (
        "Advanced",
        juce::dontSendNotification);
    advancedTitle.setFont (
        juce::FontOptions (16.0f));
    advancedTitle.setColour (
        juce::Label::textColourId,
        ui::textPrimary);

    advancedStatus.setFont (
        juce::FontOptions (10.5f));
    advancedStatus.setColour (
        juce::Label::textColourId,
        ui::textSecondary);
    advancedStatus.setJustificationType (
        juce::Justification::centredLeft);

    const std::array<juce::Component*, 27> components {
        &title,
        &profileStatus,
        &profileName,
        &abButton,
        &undoButton,
        &redoButton,
        &helpButton,
        &learn,
        &hearRemoved,
        &bypass,
        &advanced,
        &quality,
        &reduction,
        &preserve,
        &silence,
        &profileOffset,
        &reductionLabel,
        &preserveLabel,
        &silenceLabel,
        &profileOffsetLabel,
        &learnPopupTitle,
        &learnPopupInstruction,
        &learnPopupStatus,
        &learnPopupClose,
        &advancedTitle,
        &advancedStatus,
        &advancedClose
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
        primary ? 280 : 190);
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
            processor.getParameters()
                .getRawParameterValue ("enabled"))
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
        ui::backgroundTop,
        0.0f,
        0.0f,
        ui::backgroundBottom,
        static_cast<float> (getWidth()),
        static_cast<float> (getHeight()),
        false);

    g.setGradientFill (background);
    g.fillAll();

    g.setColour (
        ui::accentBlue.withAlpha (0.025f));
    g.fillEllipse (
        static_cast<float> (getWidth()) * 0.45f,
        -170.0f,
        650.0f,
        420.0f);

    g.setColour (
        ui::border.withAlpha (0.78f));
    g.drawRoundedRectangle (
        getLocalBounds()
            .toFloat()
            .reduced (0.5f),
        13.0f,
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
        juce::Rectangle<float> (
            14.0f,
            10.0f,
            static_cast<float> (getWidth() - 28),
            56.0f);

    ui::drawPanel (g, header, true);

    auto logoArea =
        juce::Rectangle<float> (
            27.0f,
            23.0f,
            28.0f,
            26.0f);

    juce::Path waveform;
    waveform.startNewSubPath (
        logoArea.getX(),
        logoArea.getCentreY());
    waveform.lineTo (
        logoArea.getX() + 4.0f,
        logoArea.getCentreY());
    waveform.lineTo (
        logoArea.getX() + 8.0f,
        logoArea.getY() + 2.0f);
    waveform.lineTo (
        logoArea.getX() + 12.0f,
        logoArea.getBottom() - 2.0f);
    waveform.lineTo (
        logoArea.getX() + 17.0f,
        logoArea.getY() + 6.0f);
    waveform.lineTo (
        logoArea.getX() + 21.0f,
        logoArea.getCentreY());
    waveform.lineTo (
        logoArea.getRight(),
        logoArea.getCentreY());

    g.setColour (
        ui::accentPurple.withAlpha (0.16f));
    g.strokePath (
        waveform,
        juce::PathStrokeType (6.0f));

    g.setGradientFill (
        ui::accentGradient (logoArea));
    g.strokePath (
        waveform,
        juce::PathStrokeType (2.2f));

    g.setColour (
        ui::borderSoft.withAlpha (0.90f));
    g.drawLine (
        315.0f,
        22.0f,
        315.0f,
        54.0f,
        1.0f);

    g.drawLine (
        695.0f,
        22.0f,
        695.0f,
        54.0f,
        1.0f);
}

void SmartDenoiseAudioProcessorEditor::drawCaptureSection (
    juce::Graphics& g)
{
    ui::drawPanel (
        g,
        captureBounds.toFloat(),
        false);

    ui::drawStepHeader (
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

    auto halo =
        learn.getBounds()
            .toFloat()
            .expanded (4.0f);

    g.setColour (
        ui::accentPurple.withAlpha (0.045f));
    g.fillRoundedRectangle (
        halo,
        17.0f);

    auto profilePill =
        juce::Rectangle<float> (
            static_cast<float> (
                captureBounds.getX() + 18),
            static_cast<float> (
                captureBounds.getY() + 199),
            static_cast<float> (
                captureBounds.getWidth() - 36),
            30.0f);

    g.setGradientFill (
        ui::panelGradient (
            profilePill,
            true));
    g.fillRoundedRectangle (
        profilePill,
        15.0f);

    g.setColour (
        ui::border.withAlpha (0.72f));
    g.drawRoundedRectangle (
        profilePill,
        15.0f,
        1.0f);

    g.setColour (ui::accentPurple);
    g.fillEllipse (
        profilePill.getX() + 10.0f,
        profilePill.getCentreY() - 4.0f,
        8.0f,
        8.0f);

    juce::Path caret;
    caret.startNewSubPath (
        profilePill.getRight() - 17.0f,
        profilePill.getCentreY() - 2.0f);
    caret.lineTo (
        profilePill.getRight() - 13.0f,
        profilePill.getCentreY() + 2.0f);
    caret.lineTo (
        profilePill.getRight() - 9.0f,
        profilePill.getCentreY() - 2.0f);

    g.setColour (ui::textSecondary);
    g.strokePath (
        caret,
        juce::PathStrokeType (1.2f));

    g.setColour (ui::textSecondary);
    g.setFont (juce::FontOptions (10.0f));
    g.drawText (
        "Profile Health",
        captureBounds.getX() + 18,
        captureBounds.getY() + 239,
        captureBounds.getWidth() - 36,
        18,
        juce::Justification::centredLeft);

    auto health =
        juce::Rectangle<float> (
            static_cast<float> (
                captureBounds.getX() + 18),
            static_cast<float> (
                captureBounds.getY() + 264),
            static_cast<float> (
                captureBounds.getWidth() - 72),
            6.0f);

    g.setColour (
        juce::Colour::fromRGB (41, 48, 64));
    g.fillRoundedRectangle (
        health,
        3.0f);

    const float qualityValue =
        processor.getEngine().hasProfile()
        ? juce::jlimit (
              0.0f,
              1.0f,
              processor.getEngine()
                  .getProfileQuality())
        : 0.0f;

    auto qualityFill = health;
    qualityFill.setWidth (
        health.getWidth() * qualityValue);

    g.setGradientFill (
        ui::accentGradient (health));
    g.fillRoundedRectangle (
        qualityFill,
        3.0f);

    g.setColour (
        processor.getEngine().hasProfile()
        ? ui::accentCyan
        : ui::textMuted);
    g.setFont (
        juce::FontOptions (10.0f));

    g.drawText (
        processor.getEngine().hasProfile()
        ? (qualityValue > 0.72f
               ? "Good"
               : (qualityValue > 0.45f
                      ? "Fair"
                      : "Low"))
        : "--",
        captureBounds.getRight() - 58,
        captureBounds.getY() + 254,
        42,
        22,
        juce::Justification::centredRight);

    if (processor.getEngine().hasProfile())
    {
        auto frozen =
            juce::Rectangle<float> (
                static_cast<float> (
                    captureBounds.getX() + 18),
                static_cast<float> (
                    captureBounds.getBottom() - 49),
                static_cast<float> (
                    captureBounds.getWidth() - 36),
                28.0f);

        g.setColour (
            ui::accentBlue.withAlpha (0.07f));
        g.fillRoundedRectangle (
            frozen,
            8.0f);

        g.setColour (
            ui::accentBlue.withAlpha (0.40f));
        g.drawRoundedRectangle (
            frozen,
            8.0f,
            1.0f);

        g.setColour (ui::accentCyan);
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
    ui::drawPanel (
        g,
        cleanBounds.toFloat(),
        true);

    ui::drawStepHeader (
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

    auto macroGlow =
        reduction.getBounds()
            .toFloat()
            .reduced (42.0f);

    g.setColour (
        ui::accentPurple.withAlpha (0.025f));
    g.fillEllipse (macroGlow);

    g.setColour (ui::textSecondary);
    g.setFont (juce::FontOptions (10.5f));
    g.drawText (
        character,
        reduction.getX(),
        reduction.getBottom() - 22,
        reduction.getWidth(),
        18,
        juce::Justification::centred);

    g.setColour (
        ui::borderSoft.withAlpha (0.90f));
    g.drawVerticalLine (
        preserve.getX() - 15,
        static_cast<float> (
            cleanBounds.getY() + 58),
        static_cast<float> (
            cleanBounds.getBottom() - 24));

    g.setColour (
        ui::textMuted.withAlpha (0.92f));
    g.setFont (juce::FontOptions (9.0f));

    g.drawText (
        "Protect presence + consonants",
        preserve.getX() - 15,
        preserve.getBottom() - 1,
        preserve.getWidth() + 30,
        16,
        juce::Justification::centred);

    g.drawText (
        "Quiet-region clean-up",
        silence.getX() - 15,
        silence.getBottom() - 1,
        silence.getWidth() + 30,
        16,
        juce::Justification::centred);
}

void SmartDenoiseAudioProcessorEditor::drawCheckSection (
    juce::Graphics& g)
{
    ui::drawPanel (
        g,
        checkBounds.toFloat(),
        false);

    ui::drawStepHeader (
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
                checkBounds.getY() + 82),
            14.0f,
            174.0f),
        displayedInputDb,
        "IN");

    drawMeter (
        g,
        juce::Rectangle<float> (
            static_cast<float> (
                checkBounds.getRight() - 33),
            static_cast<float> (
                checkBounds.getY() + 82),
            14.0f,
            174.0f),
        displayedOutputDb,
        "OUT");

    const auto analysis =
        processor.getEngine().getFrameAnalysis();

    g.setColour (
        ui::borderSoft.withAlpha (0.88f));
    g.drawHorizontalLine (
        checkBounds.getBottom() - 80,
        static_cast<float> (
            checkBounds.getX() + 16),
        static_cast<float> (
            checkBounds.getRight() - 16));

    auto drawTelemetry =
        [&] (int y,
             const juce::String& label,
             float value,
             juce::Colour colour)
        {
            const auto x =
                checkBounds.getX() + 16;
            const auto w =
                checkBounds.getWidth() - 32;

            g.setColour (ui::textSecondary);
            g.setFont (juce::FontOptions (9.2f));
            g.drawText (
                label,
                x,
                y,
                108,
                15,
                juce::Justification::centredLeft);

            g.setColour (colour);
            g.drawText (
                juce::String (
                    juce::roundToInt (
                        value * 100.0f))
                    + "%",
                x + 112,
                y,
                w - 112,
                15,
                juce::Justification::centredRight);

            auto bar =
                juce::Rectangle<float> (
                    static_cast<float> (x),
                    static_cast<float> (y + 18),
                    static_cast<float> (w),
                    3.0f);

            g.setColour (
                juce::Colour::fromRGB (38, 46, 61));
            g.fillRoundedRectangle (
                bar,
                1.5f);

            auto fill = bar;
            fill.setWidth (
                bar.getWidth()
                * juce::jlimit (
                    0.0f,
                    1.0f,
                    value));

            g.setColour (colour.withAlpha (0.85f));
            g.fillRoundedRectangle (
                fill,
                1.5f);
        };

    drawTelemetry (
        checkBounds.getBottom() - 68,
        "P3 DETAIL GUARD",
        analysis.detailProtection,
        ui::accentCyan);

    drawTelemetry (
        checkBounds.getBottom() - 39,
        "TAIL PROTECT",
        analysis.tailProtection,
        ui::accentPurple.brighter (0.22f));
}

void SmartDenoiseAudioProcessorEditor::drawActivityStrip (
    juce::Graphics& g)
{
    ui::drawPanel (
        g,
        activityBounds.toFloat(),
        false);

    auto graph =
        activityBounds.reduced (52, 10).toFloat();

    const float cy = graph.getCentreY();

    g.setColour (
        ui::borderSoft.withAlpha (0.90f));
    g.drawLine (
        graph.getX(),
        cy,
        graph.getRight(),
        cy,
        1.0f);

    g.setColour (
        ui::borderSoft.withAlpha (0.65f));
    g.drawLine (
        graph.getCentreX(),
        graph.getY(),
        graph.getCentreX(),
        graph.getBottom(),
        1.0f);

    auto makePath =
        [&] (const std::array<float, 112>& values,
             float phase)
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

                const float oscillation =
                    std::sin (
                        static_cast<float> (i)
                            * 0.62f
                        + phase);

                const float y =
                    cy
                    - oscillation
                        * norm
                        * graph.getHeight()
                        * 0.32f;

                if (i == 0)
                    path.startNewSubPath (x, y);
                else
                    path.lineTo (x, y);
            }

            return path;
        };

    const auto inputPath =
        makePath (
            inputHistory,
            0.0f);

    const auto outputPath =
        makePath (
            outputHistory,
            0.85f);

    g.setColour (
        ui::accentPurple.withAlpha (0.11f));
    g.strokePath (
        inputPath,
        juce::PathStrokeType (5.0f));

    g.setColour (
        ui::accentBlue.withAlpha (0.11f));
    g.strokePath (
        outputPath,
        juce::PathStrokeType (5.0f));

    g.setColour (
        ui::accentPurple.withAlpha (0.80f));
    g.strokePath (
        inputPath,
        juce::PathStrokeType (1.2f));

    g.setColour (
        ui::accentBlue.brighter (0.14f));
    g.strokePath (
        outputPath,
        juce::PathStrokeType (1.35f));

    g.setColour (ui::textSecondary);
    g.setFont (juce::FontOptions (9.3f));

    g.drawText (
        "Input",
        activityBounds.getX() + 14,
        activityBounds.getCentreY() - 10,
        38,
        20,
        juce::Justification::centredLeft);

    g.drawText (
        "Output",
        activityBounds.getRight() - 51,
        activityBounds.getCentreY() - 10,
        39,
        20,
        juce::Justification::centredRight);
}

void SmartDenoiseAudioProcessorEditor::drawFooter (
    juce::Graphics& g)
{
    ui::drawPanel (
        g,
        footerBounds.toFloat(),
        false);

    if (learnPopupVisible)
        return;

    g.setColour (ui::textMuted);
    g.setFont (juce::FontOptions (9.3f));

    g.drawText (
        "QUALITY",
        quality.getX() - 54,
        quality.getY(),
        48,
        quality.getHeight(),
        juce::Justification::centredRight);

    g.setColour (
        ui::textMuted.withAlpha (0.86f));
    g.setFont (juce::FontOptions (9.0f));

    g.drawText (
        "Smart Denoise  v0.4",
        footerBounds.getRight() - 136,
        footerBounds.getY(),
        122,
        footerBounds.getHeight(),
        juce::Justification::centredRight);
}

void SmartDenoiseAudioProcessorEditor::drawLearnPopup (
    juce::Graphics& g)
{
    auto dim =
        juce::Rectangle<int> (
            14,
            68,
            getWidth() - 28,
            418);

    g.setColour (
        juce::Colours::black.withAlpha (0.76f));
    g.fillRoundedRectangle (
        dim.toFloat(),
        12.0f);

    ui::drawPanel (
        g,
        learnPopupBounds.toFloat(),
        true);

    g.setColour (
        ui::accentPurple.withAlpha (0.55f));
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
                learnPopupBounds.getX() + 25),
            static_cast<float> (
                learnPopupBounds.getY() + 82),
            118.0f,
            118.0f);

    const auto centre = circle.getCentre();
    const float radius = 50.0f;
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

    g.setColour (
        juce::Colour::fromRGB (43, 50, 67));
    g.strokePath (
        baseArc,
        juce::PathStrokeType (
            8.0f,
            juce::PathStrokeType::curved,
            juce::PathStrokeType::rounded));

    juce::Path progressArc;
    progressArc.addCentredArc (
        centre.x,
        centre.y,
        radius,
        radius,
        0.0f,
        start,
        start
            + progress * (end - start),
        true);

    g.setColour (
        ui::accentPurple.withAlpha (0.14f));
    g.strokePath (
        progressArc,
        juce::PathStrokeType (
            14.0f,
            juce::PathStrokeType::curved,
            juce::PathStrokeType::rounded));

    g.setGradientFill (
        ui::accentGradient (circle));
    g.strokePath (
        progressArc,
        juce::PathStrokeType (
            8.0f,
            juce::PathStrokeType::curved,
            juce::PathStrokeType::rounded));

    g.setColour (ui::textPrimary);
    g.setFont (juce::FontOptions (31.0f));

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

    g.setColour (ui::textSecondary);
    g.setFont (juce::FontOptions (9.5f));
    g.drawText (
        engine.isLearning()
        ? "capturing"
        : "seconds",
        circle.toNearestInt()
            .translated (0, 34),
        juce::Justification::centred);

    auto mini =
        juce::Rectangle<float> (
            static_cast<float> (
                learnPopupBounds.getX() + 168),
            static_cast<float> (
                learnPopupBounds.getY() + 170),
            176.0f,
            32.0f);

    juce::Path trace;
    for (int i = 0; i < 36; ++i)
    {
        const float x =
            mini.getX()
            + mini.getWidth()
                * static_cast<float> (i)
                / 35.0f;

        const float y =
            mini.getCentreY()
            + std::sin (
                static_cast<float> (i)
                    * 1.37f)
                * (3.0f
                   + 4.0f
                       * std::sin (
                           static_cast<float> (i)
                               * 0.31f));

        if (i == 0)
            trace.startNewSubPath (x, y);
        else
            trace.lineTo (x, y);
    }

    g.setColour (
        ui::accentPurple.withAlpha (0.22f));
    g.strokePath (
        trace,
        juce::PathStrokeType (4.0f));

    g.setGradientFill (
        ui::accentGradient (mini));
    g.strokePath (
        trace,
        juce::PathStrokeType (1.1f));
}

void SmartDenoiseAudioProcessorEditor::drawAdvancedDrawer (
    juce::Graphics& g)
{
    ui::drawPanel (
        g,
        advancedBounds.toFloat(),
        true);

    auto tabs =
        juce::Rectangle<int> (
            advancedBounds.getX() + 12,
            advancedBounds.getY() + 42,
            126,
            advancedBounds.getHeight() - 55);

    g.setGradientFill (
        ui::panelGradient (
            tabs.toFloat(),
            false));
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
                ui::accentPurple.withAlpha (0.18f));
            g.fillRoundedRectangle (
                row.toFloat(),
                6.0f);

            g.setColour (
                ui::accentPurple.withAlpha (0.75f));
            g.fillRoundedRectangle (
                static_cast<float> (row.getX()),
                static_cast<float> (row.getY() + 4),
                2.0f,
                static_cast<float> (
                    row.getHeight() - 8),
                1.0f);
        }

        g.setColour (
            i == 0
            ? ui::textPrimary
            : ui::textSecondary);

        g.setFont (juce::FontOptions (10.4f));
        g.drawText (
            names[static_cast<size_t> (i)],
            row,
            juce::Justification::centredLeft);
    }

    auto content =
        advancedBounds.reduced (160, 42);

    g.setColour (ui::textSecondary);
    g.setFont (juce::FontOptions (9.5f));

    g.drawText (
        "MAX REDUCTION",
        content.getX(),
        content.getY() + 7,
        120,
        18,
        juce::Justification::centredLeft);

    g.setColour (ui::textPrimary);
    g.setFont (juce::FontOptions (11.0f));
    g.drawText (
        "24.0 dB",
        content.getX() + 122,
        content.getY() + 7,
        80,
        18,
        juce::Justification::centredLeft);

    g.setColour (ui::textSecondary);
    g.setFont (juce::FontOptions (9.5f));
    g.drawText (
        "FROZEN PROFILE",
        content.getX() + 350,
        content.getY() + 7,
        115,
        18,
        juce::Justification::centredLeft);

    g.setColour (
        processor.getEngine().hasProfile()
        ? ui::accentCyan
        : ui::textSecondary);

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

    g.setColour (
        juce::Colour::fromRGB (41, 48, 64));
    g.fillRoundedRectangle (
        detailBar,
        2.5f);

    auto detailFill = detailBar;
    detailFill.setWidth (
        detailBar.getWidth()
        * juce::jlimit (
            0.0f,
            1.0f,
            analysis.detailProtection));

    g.setGradientFill (
        ui::accentGradient (detailBar));
    g.fillRoundedRectangle (
        detailFill,
        2.5f);

    g.setColour (ui::textSecondary);
    g.setFont (juce::FontOptions (9.3f));
    g.drawText (
        "P3 detail / tail protection is automatic and profile-safe.",
        content.getX() + 350,
        content.getY() + 66,
        310,
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
        juce::jlimit (
            -60.0f,
            0.0f,
            valueDb);

    const float norm =
        (clamped + 60.0f) / 60.0f;

    g.setColour (ui::textSecondary);
    g.setFont (juce::FontOptions (9.0f));

    g.drawText (
        label,
        juce::Rectangle<int> (
            static_cast<int> (
                bounds.getX() - 8.0f),
            static_cast<int> (
                bounds.getY() - 24.0f),
            30,
            17),
        juce::Justification::centred);

    constexpr int segments = 20;
    const float gap = 2.0f;
    const float segmentHeight =
        (bounds.getHeight()
         - gap
             * static_cast<float> (
                 segments - 1))
        / static_cast<float> (segments);

    const int lit =
        juce::roundToInt (
            norm
            * static_cast<float> (segments));

    for (int i = 0; i < segments; ++i)
    {
        const float y =
            bounds.getBottom()
            - segmentHeight
            - static_cast<float> (i)
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
                / static_cast<float> (
                    segments - 1);

            auto colour =
                ui::accentBlue
                    .interpolatedWith (
                        ui::accentPurple,
                        position);

            g.setColour (
                colour.withAlpha (0.18f));
            g.fillRoundedRectangle (
                segment.expanded (2.0f, 1.0f),
                2.0f);

            g.setColour (colour);
        }
        else
        {
            g.setColour (
                juce::Colour::fromRGB (
                    31, 38, 51));
        }

        g.fillRoundedRectangle (
            segment,
            1.6f);
    }

    g.setColour (ui::textMuted);
    g.setFont (juce::FontOptions (7.6f));

    const std::array<int, 5> ticks {
        0, -12, -24, -36, -60
    };

    for (const int db : ticks)
    {
        const float p =
            static_cast<float> (-db) / 60.0f;

        const int ty =
            static_cast<int> (
                bounds.getY()
                + p * bounds.getHeight()
                - 6.0f);

        if (label == "IN")
        {
            g.drawText (
                juce::String (db),
                static_cast<int> (
                    bounds.getX() - 25.0f),
                ty,
                19,
                12,
                juce::Justification::centredRight);
        }
    }

    g.setColour (ui::textSecondary);
    g.setFont (juce::FontOptions (8.5f));

    g.drawText (
        juce::String (valueDb, 1),
        juce::Rectangle<int> (
            static_cast<int> (
                bounds.getX() - 14.0f),
            static_cast<int> (
                bounds.getBottom() + 5.0f),
            44,
            16),
        juce::Justification::centred);
}

void SmartDenoiseAudioProcessorEditor::resized()
{
    title.setBounds (
        62, 18, 232, 34);

    abButton.setBounds (
        705, 22, 55, 28);
    undoButton.setBounds (
        764, 22, 48, 28);
    redoButton.setBounds (
        814, 22, 48, 28);
    helpButton.setBounds (
        869, 22, 36, 28);

    captureBounds =
        { 14, 76, 220, 330 };
    cleanBounds =
        { 242, 76, 468, 330 };
    checkBounds =
        { 718, 76, 208, 330 };

    activityBounds =
        { 14, 414, 912, 70 };
    footerBounds =
        { 14, 492, 912, 36 };

    learn.setBounds (
        captureBounds.getX() + 31,
        captureBounds.getY() + 55,
        captureBounds.getWidth() - 62,
        132);

    profileName.setBounds (
        captureBounds.getX() + 42,
        captureBounds.getY() + 200,
        captureBounds.getWidth() - 78,
        28);

    profileStatus.setBounds (
        captureBounds.getX() + 18,
        captureBounds.getY() + 277,
        captureBounds.getWidth() - 36,
        34);

    reductionLabel.setBounds (
        cleanBounds.getX() + 42,
        cleanBounds.getY() + 48,
        238,
        24);

    reduction.setBounds (
        cleanBounds.getX() + 18,
        cleanBounds.getY() + 67,
        282,
        238);

    preserveLabel.setBounds (
        cleanBounds.getX() + 312,
        cleanBounds.getY() + 53,
        136,
        20);

    preserve.setBounds (
        cleanBounds.getX() + 321,
        cleanBounds.getY() + 74,
        118,
        112);

    silenceLabel.setBounds (
        cleanBounds.getX() + 312,
        cleanBounds.getY() + 189,
        136,
        20);

    silence.setBounds (
        cleanBounds.getX() + 321,
        cleanBounds.getY() + 210,
        118,
        108);

    hearRemoved.setBounds (
        checkBounds.getX() + 14,
        checkBounds.getY() + 71,
        116,
        72);

    bypass.setBounds (
        checkBounds.getX() + 14,
        checkBounds.getY() + 154,
        116,
        68);

    advanced.setBounds (
        footerBounds.getX() + 8,
        footerBounds.getY() + 4,
        108,
        28);

    quality.setBounds (
        footerBounds.getCentreX() - 70,
        footerBounds.getY() + 4,
        188,
        28);

    learnPopupBounds =
        juce::Rectangle<int> (
            getWidth() / 2 - 190,
            128,
            380,
            282);

    learnPopupTitle.setBounds (
        learnPopupBounds.getX() + 18,
        learnPopupBounds.getY() + 12,
        learnPopupBounds.getWidth() - 36,
        28);

    learnPopupInstruction.setBounds (
        learnPopupBounds.getX() + 168,
        learnPopupBounds.getY() + 81,
        190,
        80);

    learnPopupStatus.setBounds (
        learnPopupBounds.getX() + 168,
        learnPopupBounds.getY() + 208,
        190,
        32);

    learnPopupClose.setBounds (
        learnPopupBounds.getRight() - 105,
        learnPopupBounds.getBottom() - 42,
        82,
        26);

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

    inputHistory.back() =
        displayedInputDb;
    outputHistory.back() =
        displayedOutputDb;

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
                analysis.detailProtection
                * 100.0f))
        + "%   |   Tail Protect "
        + juce::String (
            juce::roundToInt (
                analysis.tailProtection
                * 100.0f))
        + "%   |   Latency "
        + juce::String (
            engine.getLatencySamples())
        + " samples",
        juce::dontSendNotification);

    syncBypassButton();
    repaint();
}
