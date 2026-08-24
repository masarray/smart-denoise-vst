#include "PluginEditor.h"

#include <algorithm>
#include <cmath>

namespace
{
namespace ui
{
const auto backgroundTop = juce::Colour::fromRGB (7, 10, 16);
const auto backgroundBottom = juce::Colour::fromRGB (10, 16, 27);
const auto panel = juce::Colour::fromRGB (14, 19, 29);
const auto panelRaised = juce::Colour::fromRGB (18, 24, 36);
const auto panelDeep = juce::Colour::fromRGB (10, 14, 22);
const auto border = juce::Colour::fromRGB (39, 48, 66);
const auto borderSoft = juce::Colour::fromRGB (27, 34, 48);
const auto textPrimary = juce::Colour::fromRGB (243, 245, 250);
const auto textSecondary = juce::Colour::fromRGB (143, 153, 177);
const auto textMuted = juce::Colour::fromRGB (96, 108, 134);
const auto accentPurple = juce::Colour::fromRGB (159, 92, 255);
const auto accentBlue = juce::Colour::fromRGB (68, 132, 255);
const auto accentCyan = juce::Colour::fromRGB (80, 197, 255);

juce::ColourGradient accentGradient (juce::Rectangle<float> area)
{
    return juce::ColourGradient (
        accentPurple,
        area.getTopLeft(),
        accentBlue,
        area.getBottomRight(),
        false);
}

juce::ColourGradient panelGradient (
    juce::Rectangle<float> area,
    bool raised)
{
    return juce::ColourGradient (
        raised ? panelRaised.brighter (0.05f) : panel.brighter (0.025f),
        area.getTopLeft(),
        raised ? panelRaised.darker (0.25f) : panel.darker (0.24f),
        area.getBottomRight(),
        false);
}

float clamp01 (float value) noexcept
{
    return juce::jlimit (0.0f, 1.0f, value);
}

void drawWaveformIcon (
    juce::Graphics& g,
    juce::Rectangle<float> area,
    juce::Colour colour,
    float stroke)
{
    const std::array<float, 13> heights {
        0.15f, 0.34f, 0.66f, 0.40f, 0.88f, 0.54f, 1.0f,
        0.58f, 0.82f, 0.38f, 0.66f, 0.30f, 0.15f
    };

    g.setColour (colour);

    const float step =
        area.getWidth()
        / static_cast<float> (heights.size());

    for (size_t i = 0; i < heights.size(); ++i)
    {
        const float x =
            area.getX()
            + (static_cast<float> (i) + 0.5f) * step;

        const float half =
            0.5f
            * area.getHeight()
            * heights[i];

        g.drawLine (
            x,
            area.getCentreY() - half,
            x,
            area.getCentreY() + half,
            stroke);
    }
}

void drawHeadphones (
    juce::Graphics& g,
    juce::Rectangle<float> area,
    juce::Colour colour)
{
    auto bounds = area.reduced (3.0f, 2.0f);

    const float leftX = bounds.getX() + bounds.getWidth() * 0.22f;
    const float rightX = bounds.getRight() - bounds.getWidth() * 0.22f;
    const float topY = bounds.getY() + bounds.getHeight() * 0.08f;
    const float cupTop = bounds.getY() + bounds.getHeight() * 0.51f;
    const float cupW = bounds.getWidth() * 0.19f;
    const float cupH = bounds.getHeight() * 0.38f;

    // One continuous, unmistakable headphone headband silhouette.
    juce::Path headband;
    headband.startNewSubPath (leftX, cupTop + 2.0f);
    headband.cubicTo (
        leftX,
        topY,
        rightX,
        topY,
        rightX,
        cupTop + 2.0f);

    g.setColour (colour);
    g.strokePath (
        headband,
        juce::PathStrokeType (
            2.6f,
            juce::PathStrokeType::curved,
            juce::PathStrokeType::rounded));

    // Short stems make the headband read cleanly into the two ear cups.
    g.drawLine (
        leftX,
        cupTop - 1.0f,
        leftX,
        cupTop + 4.0f,
        2.4f);
    g.drawLine (
        rightX,
        cupTop - 1.0f,
        rightX,
        cupTop + 4.0f,
        2.4f);

    auto leftCup = juce::Rectangle<float> (
        cupW,
        cupH)
        .withCentre ({ leftX, cupTop + cupH * 0.45f });

    auto rightCup = juce::Rectangle<float> (
        cupW,
        cupH)
        .withCentre ({ rightX, cupTop + cupH * 0.45f });

    g.fillRoundedRectangle (leftCup, cupW * 0.46f);
    g.fillRoundedRectangle (rightCup, cupW * 0.46f);

    // Inner pads preserve the familiar over-ear headphone negative space.
    g.setColour (ui::panelDeep.withAlpha (0.92f));
    g.fillRoundedRectangle (
        leftCup.reduced (cupW * 0.34f, cupH * 0.20f),
        cupW * 0.28f);
    g.fillRoundedRectangle (
        rightCup.reduced (cupW * 0.34f, cupH * 0.20f),
        cupW * 0.28f);
}

void drawBypass (
    juce::Graphics& g,
    juce::Rectangle<float> area,
    juce::Colour colour)
{
    g.setColour (colour);
    auto circle = area.reduced (6.0f);
    g.drawEllipse (circle, 2.0f);
    g.drawLine (
        circle.getX() + 3.0f,
        circle.getBottom() - 3.0f,
        circle.getRight() - 3.0f,
        circle.getY() + 3.0f,
        2.0f);
}

void drawChevron (
    juce::Graphics& g,
    juce::Point<float> centre,
    bool up)
{
    juce::Path path;
    const float direction = up ? -1.0f : 1.0f;

    path.startNewSubPath (
        centre.x - 4.0f,
        centre.y - 2.0f * direction);
    path.lineTo (
        centre.x,
        centre.y + 2.0f * direction);
    path.lineTo (
        centre.x + 4.0f,
        centre.y - 2.0f * direction);

    g.setColour (textSecondary);
    g.strokePath (
        path,
        juce::PathStrokeType (
            1.4f,
            juce::PathStrokeType::curved,
            juce::PathStrokeType::rounded));
}
} // namespace ui
} // namespace

namespace smartdenoiseui
{
LearnCircleButton::LearnCircleButton()
    : juce::Button ("Learn Noise")
{
    setClickingTogglesState (false);
    setWantsKeyboardFocus (false);
}

void LearnCircleButton::setLearnState (
    bool learningIn,
    float progress01,
    bool profileReadyIn,
    bool rejectedIn)
{
    learning = learningIn;
    progress = ui::clamp01 (progress01);
    profileReady = profileReadyIn;
    rejected = rejectedIn;
    repaint();
}

void LearnCircleButton::paintButton (
    juce::Graphics& g,
    bool isMouseOverButton,
    bool isButtonDown)
{
    auto area =
        getLocalBounds().toFloat().reduced (5.0f);

    const float side =
        juce::jmin (area.getWidth(), area.getHeight());

    auto circle =
        juce::Rectangle<float> (side, side)
            .withCentre (area.getCentre());

    // P4.2: the Learn control is deliberately a true 360-degree circle.
    // The outer ring is the progress authority, not a rotary-knob arc.
    auto ringBounds = circle.reduced (4.0f);
    auto faceCircle = circle.reduced (15.0f);
    const auto centre = circle.getCentre();
    const float startAngle =
        -0.5f * juce::MathConstants<float>::pi;

    if (learning || isMouseOverButton)
    {
        for (int i = 5; i >= 1; --i)
        {
            g.setColour (
                ui::accentPurple.withAlpha (
                    0.018f * static_cast<float> (6 - i)));
            g.drawEllipse (
                ringBounds.expanded (
                    static_cast<float> (i) * 2.1f),
                1.4f + static_cast<float> (i) * 1.25f);
        }
    }

    juce::ColourGradient face (
        ui::panelRaised.brighter (
            isMouseOverButton ? 0.08f : 0.03f),
        faceCircle.getTopLeft(),
        ui::panelDeep,
        faceCircle.getBottomRight(),
        false);

    g.setGradientFill (face);
    g.fillEllipse (faceCircle);

    g.setColour (ui::borderSoft);
    g.drawEllipse (faceCircle, 1.0f);

    // Full 360-degree idle track. This remains visible before learning.
    g.setColour (juce::Colour::fromRGB (45, 52, 70));
    g.drawEllipse (ringBounds, 7.0f);

    float ringProgress = 0.0f;
    if (learning)
        ringProgress = progress;
    else if (profileReady)
        ringProgress = 1.0f;

    if (ringProgress > 0.001f)
    {
        if (ringProgress >= 0.999f)
        {
            // drawEllipse guarantees a visually closed ring at 100%.
            g.setColour (ui::accentPurple.withAlpha (0.16f));
            g.drawEllipse (ringBounds, 17.0f);
            g.setGradientFill (ui::accentGradient (ringBounds));
            g.drawEllipse (ringBounds, 7.5f);
        }
        else
        {
            juce::Path progressArc;
            progressArc.addCentredArc (
                centre.x,
                centre.y,
                ringBounds.getWidth() * 0.5f,
                ringBounds.getHeight() * 0.5f,
                0.0f,
                startAngle,
                startAngle
                    + ringProgress
                      * juce::MathConstants<float>::twoPi,
                true);

            g.setColour (ui::accentPurple.withAlpha (0.16f));
            g.strokePath (
                progressArc,
                juce::PathStrokeType (
                    17.0f,
                    juce::PathStrokeType::curved,
                    juce::PathStrokeType::rounded));

            g.setGradientFill (ui::accentGradient (ringBounds));
            g.strokePath (
                progressArc,
                juce::PathStrokeType (
                    7.5f,
                    juce::PathStrokeType::curved,
                    juce::PathStrokeType::rounded));
        }
    }

    if (isButtonDown)
    {
        g.setColour (juce::Colours::white.withAlpha (0.04f));
        g.fillEllipse (faceCircle.reduced (3.0f));
    }

    auto iconArea =
        juce::Rectangle<float> (
            42.0f,
            28.0f)
            .withCentre (
                { centre.x,
                  centre.y - 28.0f });

    ui::drawWaveformIcon (
        g,
        iconArea,
        learning
            ? ui::accentCyan
            : ui::accentPurple.brighter (0.28f),
        1.6f);

    juce::String mainText = "Learn Noise";
    juce::String subText = "3 seconds";

    if (learning)
    {
        mainText =
            juce::String (
                juce::roundToInt (
                    progress * 100.0f))
            + "%";
        subText = "Capturing noise profile";
    }
    else if (profileReady)
    {
        mainText = "Profile Ready";
        subText = "Click to re-learn";
    }
    else if (rejected)
    {
        mainText = "Try Again";
        subText = "Capture noise only";
    }

    g.setColour (ui::textPrimary);
    g.setFont (
        juce::FontOptions (
            learning ? 23.0f : 15.0f));

    g.drawFittedText (
        mainText,
        juce::Rectangle<int> (
            static_cast<int> (faceCircle.getX() + 12.0f),
            static_cast<int> (centre.y - 5.0f),
            static_cast<int> (faceCircle.getWidth() - 24.0f),
            30),
        juce::Justification::centred,
        1);

    g.setColour (ui::textSecondary);
    g.setFont (juce::FontOptions (9.5f));

    g.drawFittedText (
        subText,
        juce::Rectangle<int> (
            static_cast<int> (faceCircle.getX() + 8.0f),
            static_cast<int> (centre.y + 25.0f),
            static_cast<int> (faceCircle.getWidth() - 16.0f),
            19),
        juce::Justification::centred,
        1);
}

MonitorButton::MonitorButton (
    const juce::String& text,
    Icon iconIn)
    : juce::Button (text),
      icon (iconIn)
{
    setClickingTogglesState (true);
    setWantsKeyboardFocus (false);
}

void MonitorButton::paintButton (
    juce::Graphics& g,
    bool isMouseOverButton,
    bool isButtonDown)
{
    auto area =
        getLocalBounds().toFloat().reduced (0.5f);

    const bool active = getToggleState();

    juce::ColourGradient fill (
        active
            ? ui::accentPurple.withAlpha (0.18f)
            : ui::panelRaised,
        area.getTopLeft(),
        ui::panelDeep,
        area.getBottomRight(),
        false);

    g.setGradientFill (fill);
    g.fillRoundedRectangle (area, 11.0f);

    g.setColour (
        active
            ? ui::accentBlue.withAlpha (0.78f)
            : ui::border.withAlpha (
                isMouseOverButton ? 0.95f : 0.72f));

    g.drawRoundedRectangle (
        area,
        11.0f,
        active ? 1.4f : 1.0f);

    if (isButtonDown)
    {
        g.setColour (
            juce::Colours::white.withAlpha (0.04f));
        g.fillRoundedRectangle (
            area.reduced (2.0f),
            9.0f);
    }

    auto iconArea =
        juce::Rectangle<float> (
            38.0f,
            34.0f)
            .withCentre (
                { area.getCentreX(),
                  area.getY() + 26.0f });

    const auto iconColour =
        active
            ? ui::accentCyan
            : ui::textPrimary;

    if (icon == Icon::headphones)
        ui::drawHeadphones (
            g, iconArea, iconColour);
    else
        ui::drawBypass (
            g, iconArea, iconColour);

    g.setColour (
        active
            ? ui::textPrimary
            : ui::textSecondary.brighter (0.15f));
    g.setFont (juce::FontOptions (10.2f));

    g.drawFittedText (
        getName(),
        juce::Rectangle<int> (
            8,
            getHeight() - 29,
            getWidth() - 16,
            19),
        juce::Justification::centred,
        1);
}

CleanLookAndFeel::CleanLookAndFeel()
{
    setColour (
        juce::ComboBox::textColourId,
        ui::textPrimary);
    setColour (
        juce::ComboBox::backgroundColourId,
        ui::panelRaised);
    setColour (
        juce::ComboBox::outlineColourId,
        ui::border);
    setColour (
        juce::PopupMenu::backgroundColourId,
        ui::panelRaised);
    setColour (
        juce::PopupMenu::textColourId,
        ui::textPrimary);
    setColour (
        juce::PopupMenu::highlightedBackgroundColourId,
        ui::accentPurple.withAlpha (0.26f));
}

void CleanLookAndFeel::drawRotarySlider (
    juce::Graphics& g,
    int x,
    int y,
    int width,
    int height,
    float sliderPos,
    float startAngle,
    float endAngle,
    juce::Slider& slider)
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
        diameter
        * (primary ? 0.43f : 0.39f);

    const auto centre = bounds.getCentre();
    const float stroke =
        primary ? 11.0f : 6.5f;

    if (primary)
    {
        constexpr int tickCount = 25;
        for (int tick = 0; tick < tickCount; ++tick)
        {
            const float t =
                static_cast<float> (tick)
                / static_cast<float> (
                    tickCount - 1);

            const float angle =
                startAngle
                + t * (endAngle - startAngle);

            const auto outer =
                centre.getPointOnCircumference (
                    radius + 15.0f,
                    angle);

            const auto inner =
                centre.getPointOnCircumference (
                    radius + 10.0f,
                    angle);

            g.setColour (
                ui::border.withAlpha (0.50f));
            g.drawLine (
                inner.x,
                inner.y,
                outer.x,
                outer.y,
                1.0f);
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

    // Deep recessed track behind the illuminated value ring.
    g.setColour (juce::Colours::black.withAlpha (0.34f));
    g.strokePath (
        baseArc,
        juce::PathStrokeType (
            stroke + 5.0f,
            juce::PathStrokeType::curved,
            juce::PathStrokeType::rounded));

    g.setColour (
        juce::Colour::fromRGB (47, 55, 74));
    g.strokePath (
        baseArc,
        juce::PathStrokeType (
            stroke,
            juce::PathStrokeType::curved,
            juce::PathStrokeType::rounded));

    const float valueAngle =
        startAngle
        + sliderPos * (endAngle - startAngle);

    juce::Path activeArc;
    activeArc.addCentredArc (
        centre.x,
        centre.y,
        radius,
        radius,
        0.0f,
        startAngle,
        valueAngle,
        true);

    // Two-stage glow keeps the purple/blue illumination lively without
    // turning the control into a neon toy.
    g.setColour (
        ui::accentBlue.withAlpha (
            primary ? 0.065f : 0.050f));
    g.strokePath (
        activeArc,
        juce::PathStrokeType (
            stroke + (primary ? 14.0f : 8.0f),
            juce::PathStrokeType::curved,
            juce::PathStrokeType::rounded));

    g.setColour (
        ui::accentPurple.withAlpha (
            primary ? 0.14f : 0.10f));
    g.strokePath (
        activeArc,
        juce::PathStrokeType (
            stroke + (primary ? 8.0f : 4.0f),
            juce::PathStrokeType::curved,
            juce::PathStrokeType::rounded));

    g.setGradientFill (
        ui::accentGradient (
            bounds.withSizeKeepingCentre (
                radius * 2.2f,
                radius * 2.2f)));

    g.strokePath (
        activeArc,
        juce::PathStrokeType (
            stroke,
            juce::PathStrokeType::curved,
            juce::PathStrokeType::rounded));

    const float knobRadius =
        radius - (primary ? 18.0f : 11.0f);

    auto knob =
        juce::Rectangle<float> (
            knobRadius * 2.0f,
            knobRadius * 2.0f)
            .withCentre (centre);

    // P4.3: physical depth. The shadow is intentionally offset downward
    // while the bezel catches light from the upper-left.
    auto knobShadow =
        knob.expanded (primary ? 7.0f : 4.0f)
            .translated (0.0f, primary ? 6.0f : 3.5f);

    g.setColour (juce::Colours::black.withAlpha (0.48f));
    g.fillEllipse (knobShadow);

    auto metalBezel =
        knob.expanded (primary ? 7.5f : 4.5f);

    juce::ColourGradient bezelGradient (
        juce::Colour::fromRGB (108, 120, 148).withAlpha (0.86f),
        metalBezel.getX() + metalBezel.getWidth() * 0.22f,
        metalBezel.getY() + metalBezel.getHeight() * 0.12f,
        juce::Colour::fromRGB (22, 27, 39),
        metalBezel.getRight() - metalBezel.getWidth() * 0.12f,
        metalBezel.getBottom() - metalBezel.getHeight() * 0.08f,
        false);

    bezelGradient.addColour (
        0.34,
        juce::Colour::fromRGB (66, 76, 98));
    bezelGradient.addColour (
        0.72,
        juce::Colour::fromRGB (31, 37, 52));

    g.setGradientFill (bezelGradient);
    g.fillEllipse (metalBezel);

    g.setColour (juce::Colours::white.withAlpha (0.15f));
    g.drawEllipse (
        metalBezel.reduced (0.8f),
        primary ? 1.5f : 1.15f);

    g.setColour (juce::Colours::black.withAlpha (0.66f));
    g.drawEllipse (
        metalBezel.reduced (primary ? 3.0f : 2.0f),
        primary ? 2.4f : 1.7f);

    // Inner bevel separates the metal bezel from the glossy glass face.
    auto innerRim =
        knob.expanded (primary ? 2.5f : 1.8f);

    juce::ColourGradient rimGradient (
        juce::Colour::fromRGB (56, 66, 88),
        innerRim.getTopLeft(),
        juce::Colour::fromRGB (14, 18, 27),
        innerRim.getBottomRight(),
        false);

    g.setGradientFill (rimGradient);
    g.fillEllipse (innerRim);

    // Radial glass face: brighter upper-left focus falling into a deep
    // graphite lower-right, giving the knob a real convex volume.
    juce::ColourGradient knobGradient (
        ui::panelRaised.brighter (0.42f),
        knob.getX() + knob.getWidth() * 0.30f,
        knob.getY() + knob.getHeight() * 0.24f,
        ui::panelDeep.darker (0.45f),
        knob.getRight() - knob.getWidth() * 0.18f,
        knob.getBottom() - knob.getHeight() * 0.14f,
        true);

    knobGradient.addColour (
        0.42,
        juce::Colour::fromRGB (29, 37, 55));
    knobGradient.addColour (
        0.78,
        juce::Colour::fromRGB (12, 17, 27));

    g.setGradientFill (knobGradient);
    g.fillEllipse (knob);

    // Fine bevels around the glass create the machined / layered edge.
    g.setColour (juce::Colours::white.withAlpha (0.105f));
    g.drawEllipse (
        knob.reduced (1.6f),
        primary ? 1.65f : 1.25f);

    g.setColour (juce::Colours::black.withAlpha (0.58f));
    g.drawEllipse (
        knob.reduced (primary ? 5.0f : 3.5f),
        primary ? 2.0f : 1.45f);

    // Gloss reflection: a broad soft specular ellipse plus a small hot spot.
    auto specularGlow =
        juce::Rectangle<float> (
            knob.getWidth() * 0.58f,
            knob.getHeight() * 0.25f)
            .withCentre (
                { centre.x - knobRadius * 0.12f,
                  centre.y - knobRadius * 0.48f });

    juce::ColourGradient specularGradient (
        juce::Colours::white.withAlpha (primary ? 0.19f : 0.16f),
        specularGlow.getCentreX(),
        specularGlow.getY(),
        juce::Colours::white.withAlpha (0.0f),
        specularGlow.getCentreX(),
        specularGlow.getBottom(),
        false);

    g.setGradientFill (specularGradient);
    g.fillEllipse (specularGlow);

    g.setColour (juce::Colours::white.withAlpha (0.16f));
    g.fillEllipse (
        juce::Rectangle<float> (
            primary ? 7.0f : 4.5f,
            primary ? 4.0f : 3.0f)
            .withCentre (
                { centre.x - knobRadius * 0.34f,
                  centre.y - knobRadius * 0.48f }));

    const auto marker =
        centre.getPointOnCircumference (
            radius,
            valueAngle);

    const float markerSize =
        primary ? 10.0f : 7.0f;

    auto markerShadow =
        juce::Rectangle<float> (
            markerSize + 5.0f,
            markerSize + 5.0f)
            .withCentre (
                { marker.x + 1.2f,
                  marker.y + 1.8f });

    g.setColour (juce::Colours::black.withAlpha (0.48f));
    g.fillEllipse (markerShadow);

    auto markerBezel =
        juce::Rectangle<float> (
            markerSize + 3.0f,
            markerSize + 3.0f)
            .withCentre (marker);

    g.setColour (ui::accentBlue.withAlpha (0.82f));
    g.fillEllipse (markerBezel);

    g.setColour (ui::textPrimary);
    g.fillEllipse (
        juce::Rectangle<float> (
            markerSize,
            markerSize)
            .withCentre (marker));

    g.setColour (juce::Colours::white.withAlpha (0.60f));
    g.fillEllipse (
        juce::Rectangle<float> (
            primary ? 3.2f : 2.3f,
            primary ? 2.0f : 1.5f)
            .withCentre (
                { marker.x - markerSize * 0.16f,
                  marker.y - markerSize * 0.18f }));

    juce::String valueText;

    if (primary)
    {
        const auto percent =
            juce::roundToInt (
                100.0
                * slider.getValue()
                / smartdenoise::SmartDenoiseEngine::
                    maxReductionDb);

        valueText =
            juce::String (percent) + "%";
    }
    else
    {
        const auto percent =
            juce::roundToInt (
                slider.getValue() * 100.0);

        valueText =
            juce::String (percent) + "%";
    }

    const auto textArea =
        knob.toNearestInt().reduced (
            primary ? 24 : 10);

    // Tiny text shadow anchors the value inside the glossy face.
    g.setColour (juce::Colours::black.withAlpha (0.72f));
    g.setFont (
        juce::FontOptions (
            primary ? 35.0f : 15.5f));
    g.drawFittedText (
        valueText,
        textArea.translated (0, 1),
        juce::Justification::centred,
        1);

    g.setColour (ui::textPrimary);
    g.drawFittedText (
        valueText,
        textArea,
        juce::Justification::centred,
        1);
}

void CleanLookAndFeel::drawLinearSlider (
    juce::Graphics& g,
    int x,
    int y,
    int width,
    int height,
    float sliderPos,
    float,
    float,
    juce::Slider::SliderStyle,
    juce::Slider&)
{
    auto area =
        juce::Rectangle<float> (
            static_cast<float> (x),
            static_cast<float> (y),
            static_cast<float> (width),
            static_cast<float> (height));

    const float centreY =
        area.getCentreY();

    auto rail =
        juce::Rectangle<float> (
            area.getX() + 6.0f,
            centreY - 2.0f,
            area.getWidth() - 12.0f,
            4.0f);

    g.setColour (ui::borderSoft);
    g.fillRoundedRectangle (
        rail,
        2.0f);

    auto active = rail;
    active.setWidth (
        juce::jmax (
            0.0f,
            sliderPos - rail.getX()));

    g.setGradientFill (
        ui::accentGradient (rail));
    g.fillRoundedRectangle (
        active,
        2.0f);

    g.setColour (ui::textPrimary);
    g.fillEllipse (
        juce::Rectangle<float> (
            10.0f,
            10.0f)
            .withCentre (
                { sliderPos, centreY }));
}

void CleanLookAndFeel::drawButtonBackground (
    juce::Graphics& g,
    juce::Button& button,
    const juce::Colour&,
    bool isMouseOverButton,
    bool isButtonDown)
{
    auto area =
        button.getLocalBounds()
            .toFloat()
            .reduced (0.5f);

    const bool top =
        button.getName() == "top";

    const bool footer =
        button.getName() == "footer";

    if (top)
    {
        if (isMouseOverButton)
        {
            g.setColour (
                ui::panelRaised.withAlpha (0.75f));
            g.fillRoundedRectangle (
                area,
                7.0f);
        }
        return;
    }

    juce::ColourGradient fill (
        ui::panelRaised,
        area.getTopLeft(),
        ui::panelDeep,
        area.getBottomRight(),
        false);

    g.setGradientFill (fill);
    g.fillRoundedRectangle (
        area,
        footer ? 9.0f : 8.0f);

    g.setColour (
        ui::border.withAlpha (
            isMouseOverButton ? 0.95f : 0.72f));

    g.drawRoundedRectangle (
        area,
        footer ? 9.0f : 8.0f,
        1.0f);

    if (isButtonDown)
    {
        g.setColour (
            juce::Colours::white.withAlpha (0.04f));
        g.fillRoundedRectangle (
            area.reduced (2.0f),
            footer ? 7.0f : 6.0f);
    }
}

void CleanLookAndFeel::drawButtonText (
    juce::Graphics& g,
    juce::TextButton& button,
    bool,
    bool)
{
    const bool top =
        button.getName() == "top";

    g.setColour (
        button.isEnabled()
            ? ui::textPrimary
            : ui::textMuted);

    g.setFont (
        juce::FontOptions (
            top ? 9.8f : 10.5f));

    g.drawFittedText (
        button.getButtonText(),
        button.getLocalBounds().reduced (7, 3),
        juce::Justification::centred,
        1);
}

void CleanLookAndFeel::drawComboBox (
    juce::Graphics& g,
    int width,
    int height,
    bool,
    int,
    int,
    int,
    int,
    juce::ComboBox&)
{
    auto area =
        juce::Rectangle<float> (
            0.5f,
            0.5f,
            static_cast<float> (width) - 1.0f,
            static_cast<float> (height) - 1.0f);

    juce::ColourGradient fill (
        ui::panelRaised,
        area.getTopLeft(),
        ui::panelDeep,
        area.getBottomRight(),
        false);

    g.setGradientFill (fill);
    g.fillRoundedRectangle (
        area,
        9.0f);

    g.setColour (ui::border);
    g.drawRoundedRectangle (
        area,
        9.0f,
        1.0f);

    ui::drawChevron (
        g,
        { static_cast<float> (width - 14),
          static_cast<float> (height) * 0.5f },
        false);
}

void CleanLookAndFeel::positionComboBoxText (
    juce::ComboBox& box,
    juce::Label& label)
{
    label.setBounds (
        12,
        0,
        box.getWidth() - 34,
        box.getHeight());

    label.setFont (
        juce::FontOptions (10.5f));

    label.setColour (
        juce::Label::textColourId,
        ui::textPrimary);
}
} // namespace smartdenoiseui

SmartDenoiseAudioProcessorEditor::
SmartDenoiseAudioProcessorEditor (
    SmartDenoiseAudioProcessor& p)
    : juce::AudioProcessorEditor (&p),
      processor (p),
      cleanLookAndFeel (
          std::make_unique<
              smartdenoiseui::CleanLookAndFeel>())
{
    setOpaque (true);
    setLookAndFeel (
        cleanLookAndFeel.get());

    setSize (940, 540);

    title.setText (
        "Smart Denoise",
        juce::dontSendNotification);
    title.setFont (
        juce::FontOptions (20.5f));
    title.setColour (
        juce::Label::textColourId,
        ui::textPrimary);
    title.setJustificationType (
        juce::Justification::centredLeft);

    profileName.setText (
        "Noise Profile",
        juce::dontSendNotification);
    profileName.setFont (
        juce::FontOptions (10.5f));
    profileName.setColour (
        juce::Label::textColourId,
        ui::textPrimary);
    profileName.setJustificationType (
        juce::Justification::centredLeft);

    profileStatus.setText (
        "No profile - learn room noise",
        juce::dontSendNotification);
    profileStatus.setFont (
        juce::FontOptions (9.2f));
    profileStatus.setColour (
        juce::Label::textColourId,
        ui::textSecondary);
    profileStatus.setJustificationType (
        juce::Justification::centredLeft);

    quality.addItem (
        "Balanced - Live 1024",
        1);
    quality.addItem (
        "Maximum - Clean 2048",
        2);
    quality.setTextWhenNothingSelected (
        "Balanced - Live 1024");

    configureRotary (
        reduction,
        true);
    configureRotary (
        preserve,
        false);
    configureRotary (
        silence,
        false);

    reduction.setName ("primary");
    preserve.setName ("secondary");
    silence.setName ("secondary");

    profileOffset.setSliderStyle (
        juce::Slider::LinearHorizontal);
    profileOffset.setTextBoxStyle (
        juce::Slider::TextBoxRight,
        false,
        66,
        22);
    profileOffset.setTextValueSuffix (
        " dB");
    profileOffset.setColour (
        juce::Slider::textBoxTextColourId,
        ui::textPrimary);
    profileOffset.setColour (
        juce::Slider::textBoxBackgroundColourId,
        ui::panelDeep);
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

    for (auto* label : std::array<juce::Label*, 4> {
             &reductionLabel,
             &preserveLabel,
             &silenceLabel,
             &profileOffsetLabel })
    {
        label->setFont (
            juce::FontOptions (10.5f));
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
    advanced.setName ("footer");
    advancedClose.setName ("footer");

    abButton.setInterceptsMouseClicks (
        false, false);
    undoButton.setInterceptsMouseClicks (
        false, false);
    redoButton.setInterceptsMouseClicks (
        false, false);

    hearRemoved.setName (
        "Hear Removed");
    bypass.setName (
        "Bypass");

    advancedTitle.setText (
        "Advanced",
        juce::dontSendNotification);
    advancedTitle.setFont (
        juce::FontOptions (15.0f));
    advancedTitle.setColour (
        juce::Label::textColourId,
        ui::textPrimary);

    const std::array<juce::Component*, 20> components {
        &title,
        &profileName,
        &profileStatus,
        &abButton,
        &undoButton,
        &redoButton,
        &helpButton,
        &learn,
        &hearRemoved,
        &bypass,
        &advanced,
        &advancedClose,
        &quality,
        &reduction,
        &preserve,
        &silence,
        &profileOffset,
        &reductionLabel,
        &preserveLabel,
        &silenceLabel
    };

    for (auto* component : components)
        addAndMakeVisible (component);

    addAndMakeVisible (profileOffsetLabel);
    addAndMakeVisible (advancedTitle);

    auto& state =
        processor.getParameters();

    reductionAttachment =
        std::make_unique<SliderAttachment> (
            state,
            "reduction",
            reduction);

    preserveAttachment =
        std::make_unique<SliderAttachment> (
            state,
            "preserve",
            preserve);

    silenceAttachment =
        std::make_unique<SliderAttachment> (
            state,
            "silence",
            silence);

    offsetAttachment =
        std::make_unique<SliderAttachment> (
            state,
            "thresholdOffset",
            profileOffset);

    removedAttachment =
        std::make_unique<ButtonAttachment> (
            state,
            "hearRemoved",
            hearRemoved);

    qualityAttachment =
        std::make_unique<ComboAttachment> (
            state,
            "quality",
            quality);

    learn.onClick =
        [this]
        {
            if (! processor.getEngine().isLearning())
                processor.startNoiseLearn();
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

void SmartDenoiseAudioProcessorEditor::
configureRotary (
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

void SmartDenoiseAudioProcessorEditor::
showAdvancedDrawer (
    bool shouldShow)
{
    advancedDrawerVisible =
        shouldShow;

    profileOffset.setVisible (
        shouldShow);
    profileOffsetLabel.setVisible (
        shouldShow);
    advancedTitle.setVisible (
        shouldShow);
    advancedClose.setVisible (
        shouldShow);

    setSize (
        940,
        shouldShow ? 700 : 540);

    resized();
    repaint();
}

void SmartDenoiseAudioProcessorEditor::
setBypassed (
    bool shouldBypass)
{
    if (auto* parameter =
            processor.getParameters()
                .getParameter ("enabled"))
    {
        parameter->beginChangeGesture();
        parameter->setValueNotifyingHost (
            shouldBypass ? 0.0f : 1.0f);
        parameter->endChangeGesture();
    }
}

void SmartDenoiseAudioProcessorEditor::
syncBypassButton()
{
    if (const auto* enabledValue =
            processor.getParameters()
                .getRawParameterValue (
                    "enabled"))
    {
        bypass.setToggleState (
            enabledValue->load() < 0.5f,
            juce::dontSendNotification);
    }
}

void SmartDenoiseAudioProcessorEditor::drawPanel (
    juce::Graphics& g,
    juce::Rectangle<int> bounds,
    bool raised)
{
    auto area =
        bounds.toFloat();

    for (int i = 4; i >= 1; --i)
    {
        g.setColour (
            juce::Colours::black.withAlpha (
                0.035f
                * static_cast<float> (5 - i)));

        g.fillRoundedRectangle (
            area.translated (
                0.0f,
                static_cast<float> (i)),
            12.0f);
    }

    g.setGradientFill (
        ui::panelGradient (
            area,
            raised));
    g.fillRoundedRectangle (
        area,
        12.0f);

    g.setColour (
        ui::border.withAlpha (0.72f));
    g.drawRoundedRectangle (
        area,
        12.0f,
        1.0f);

    g.setColour (
        juce::Colours::white.withAlpha (0.022f));
    g.drawRoundedRectangle (
        area.reduced (1.0f),
        11.0f,
        1.0f);
}

void SmartDenoiseAudioProcessorEditor::
drawStepHeader (
    juce::Graphics& g,
    juce::Rectangle<int> area,
    int number,
    const juce::String& text)
{
    auto badge =
        area.removeFromLeft (25)
            .toFloat()
            .reduced (3.0f);

    g.setColour (ui::panelRaised);
    g.fillEllipse (badge);
    g.setColour (
        ui::border.brighter (0.25f));
    g.drawEllipse (
        badge,
        1.0f);

    g.setColour (ui::textPrimary);
    g.setFont (
        juce::FontOptions (10.5f));
    g.drawText (
        juce::String (number),
        badge.toNearestInt(),
        juce::Justification::centred);

    g.setColour (
        ui::accentPurple.brighter (0.22f));
    g.setFont (
        juce::FontOptions (10.8f));
    g.drawText (
        text,
        area,
        juce::Justification::centredLeft);
}

void SmartDenoiseAudioProcessorEditor::paint (
    juce::Graphics& g)
{
    juce::ColourGradient background (
        ui::backgroundTop,
        0.0f,
        0.0f,
        ui::backgroundBottom,
        0.0f,
        static_cast<float> (getHeight()),
        false);

    g.setGradientFill (background);
    g.fillAll();

    g.setColour (
        ui::border.withAlpha (0.70f));
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
}

void SmartDenoiseAudioProcessorEditor::
drawHeader (
    juce::Graphics& g)
{
    drawPanel (
        g,
        headerBounds,
        true);

    auto logoArea =
        juce::Rectangle<float> (
            static_cast<float> (
                headerBounds.getX() + 15),
            static_cast<float> (
                headerBounds.getY() + 11),
            28.0f,
            30.0f);

    ui::drawWaveformIcon (
        g,
        logoArea,
        ui::accentPurple.brighter (0.22f),
        1.7f);

    g.setColour (
        ui::borderSoft.withAlpha (0.78f));
    g.drawVerticalLine (
        headerBounds.getX() + 300,
        static_cast<float> (
            headerBounds.getY() + 11),
        static_cast<float> (
            headerBounds.getBottom() - 11));

    g.drawVerticalLine (
        headerBounds.getRight() - 230,
        static_cast<float> (
            headerBounds.getY() + 11),
        static_cast<float> (
            headerBounds.getBottom() - 11));
}

void SmartDenoiseAudioProcessorEditor::
drawCaptureSection (
    juce::Graphics& g)
{
    drawPanel (
        g,
        captureBounds,
        false);

    drawStepHeader (
        g,
        juce::Rectangle<int> (
            captureBounds.getX() + 14,
            captureBounds.getY() + 12,
            captureBounds.getWidth() - 28,
            23),
        1,
        "CAPTURE");

    auto profilePill =
        juce::Rectangle<float> (
            static_cast<float> (
                captureBounds.getX() + 20),
            static_cast<float> (
                captureBounds.getY() + 235),
            static_cast<float> (
                captureBounds.getWidth() - 40),
            28.0f);

    g.setColour (ui::panelDeep);
    g.fillRoundedRectangle (
        profilePill,
        14.0f);

    g.setColour (
        ui::border.withAlpha (0.76f));
    g.drawRoundedRectangle (
        profilePill,
        14.0f,
        1.0f);

    g.setColour (
        processor.getEngine().hasProfile()
            ? ui::accentCyan
            : ui::accentPurple);

    g.fillEllipse (
        profilePill.getX() + 11.0f,
        profilePill.getCentreY() - 3.5f,
        7.0f,
        7.0f);

    ui::drawChevron (
        g,
        { profilePill.getRight() - 12.0f,
          profilePill.getCentreY() },
        false);

    g.setColour (ui::textSecondary);
    g.setFont (
        juce::FontOptions (9.0f));
    g.drawText (
        "Profile Health",
        captureBounds.getX() + 20,
        captureBounds.getY() + 273,
        captureBounds.getWidth() - 40,
        17,
        juce::Justification::centredLeft);

    auto health =
        juce::Rectangle<float> (
            static_cast<float> (
                captureBounds.getX() + 20),
            static_cast<float> (
                captureBounds.getY() + 298),
            static_cast<float> (
                captureBounds.getWidth() - 68),
            5.0f);

    g.setColour (ui::borderSoft);
    g.fillRoundedRectangle (
        health,
        2.5f);

    const float qualityValue =
        processor.getEngine().hasProfile()
            ? ui::clamp01 (
                  processor.getEngine()
                      .getProfileQuality())
            : 0.0f;

    auto fill = health;
    fill.setWidth (
        health.getWidth()
        * qualityValue);

    g.setGradientFill (
        ui::accentGradient (health));
    g.fillRoundedRectangle (
        fill,
        2.5f);

    g.setColour (
        processor.getEngine().hasProfile()
            ? ui::accentCyan
            : ui::textMuted);

    g.setFont (
        juce::FontOptions (9.0f));

    g.drawText (
        processor.getEngine().hasProfile()
            ? juce::String (
                  juce::roundToInt (
                      qualityValue * 100.0f))
                  + "%"
            : "--",
        captureBounds.getRight() - 47,
        captureBounds.getY() + 290,
        29,
        20,
        juce::Justification::centredRight);
}

void SmartDenoiseAudioProcessorEditor::
drawCleanSection (
    juce::Graphics& g)
{
    drawPanel (
        g,
        cleanBounds,
        true);

    drawStepHeader (
        g,
        juce::Rectangle<int> (
            cleanBounds.getX() + 14,
            cleanBounds.getY() + 12,
            cleanBounds.getWidth() - 28,
            23),
        2,
        "CLEAN");

    g.setColour (
        ui::borderSoft.withAlpha (0.78f));
    g.drawVerticalLine (
        cleanBounds.getX() + 302,
        static_cast<float> (
            cleanBounds.getY() + 52),
        static_cast<float> (
            cleanBounds.getBottom() - 20));

    const float normalizedReduction =
        ui::clamp01 (
            static_cast<float> (
                reduction.getValue()
                / smartdenoise::SmartDenoiseEngine::
                    maxReductionDb));

    juce::String character = "Gentle";
    if (normalizedReduction > 0.68f)
        character = "Strong";
    else if (normalizedReduction > 0.30f)
        character = "Moderate";

    g.setColour (ui::textSecondary);
    g.setFont (
        juce::FontOptions (9.4f));
    g.drawText (
        character,
        reduction.getX(),
        reduction.getBottom() - 22,
        reduction.getWidth(),
        17,
        juce::Justification::centred);

    g.setColour (
        ui::textMuted);
    g.setFont (
        juce::FontOptions (8.6f));

    g.drawFittedText (
        "Protect voice presence",
        preserve.getX() - 5,
        preserve.getBottom() + 2,
        preserve.getWidth() + 10,
        16,
        juce::Justification::centred,
        1);

    g.drawFittedText (
        "Clean quiet regions",
        silence.getX() - 5,
        silence.getBottom() + 2,
        silence.getWidth() + 10,
        16,
        juce::Justification::centred,
        1);
}

void SmartDenoiseAudioProcessorEditor::
drawCheckSection (
    juce::Graphics& g)
{
    drawPanel (
        g,
        checkBounds,
        false);

    drawStepHeader (
        g,
        juce::Rectangle<int> (
            checkBounds.getX() + 14,
            checkBounds.getY() + 12,
            checkBounds.getWidth() - 28,
            23),
        3,
        "CHECK");

    drawMeter (
        g,
        juce::Rectangle<float> (
            static_cast<float> (
                checkBounds.getRight() - 57),
            static_cast<float> (
                checkBounds.getY() + 72),
            11.0f,
            142.0f),
        displayedInputDb,
        "IN");

    drawMeter (
        g,
        juce::Rectangle<float> (
            static_cast<float> (
                checkBounds.getRight() - 32),
            static_cast<float> (
                checkBounds.getY() + 72),
            11.0f,
            142.0f),
        displayedOutputDb,
        "OUT");

    const auto analysis =
        processor.getEngine()
            .getFrameAnalysis();

    drawTelemetry (
        g,
        juce::Rectangle<int> (
            checkBounds.getX() + 14,
            checkBounds.getBottom() - 77,
            checkBounds.getWidth() - 28,
            27),
        "DETAIL GUARD",
        analysis.detailProtection);

    drawTelemetry (
        g,
        juce::Rectangle<int> (
            checkBounds.getX() + 14,
            checkBounds.getBottom() - 43,
            checkBounds.getWidth() - 28,
            27),
        "TAIL PROTECT",
        analysis.tailProtection);
}

void SmartDenoiseAudioProcessorEditor::
drawActivityStrip (
    juce::Graphics& g)
{
    drawPanel (
        g,
        activityBounds,
        false);

    auto graph =
        activityBounds
            .reduced (52, 11)
            .toFloat();

    g.setColour (
        ui::borderSoft.withAlpha (0.72f));
    g.drawLine (
        graph.getX(),
        graph.getCentreY(),
        graph.getRight(),
        graph.getCentreY(),
        1.0f);

    g.drawLine (
        graph.getCentreX(),
        graph.getY() + 3.0f,
        graph.getCentreX(),
        graph.getBottom() - 3.0f,
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
                    ui::clamp01 (
                        (values[i] + 72.0f)
                        / 72.0f);

                const float x =
                    graph.getX()
                    + graph.getWidth()
                        * static_cast<float> (i)
                        / static_cast<float> (
                            values.size() - 1);

                const float excursion =
                    (norm - 0.5f)
                    * graph.getHeight()
                    * 0.78f;

                const float y =
                    graph.getCentreY()
                    - excursion;

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
        ui::accentPurple.withAlpha (0.08f));
    g.strokePath (
        inputPath,
        juce::PathStrokeType (5.0f));

    g.setColour (
        ui::accentBlue.withAlpha (0.08f));
    g.strokePath (
        outputPath,
        juce::PathStrokeType (5.0f));

    g.setColour (
        ui::accentPurple.withAlpha (0.82f));
    g.strokePath (
        inputPath,
        juce::PathStrokeType (1.2f));

    g.setColour (
        ui::accentBlue.brighter (0.12f));
    g.strokePath (
        outputPath,
        juce::PathStrokeType (1.25f));

    g.setColour (ui::textMuted);
    g.setFont (
        juce::FontOptions (8.8f));

    g.drawText (
        "Input",
        activityBounds.getX() + 14,
        activityBounds.getCentreY() - 9,
        34,
        18,
        juce::Justification::centredLeft);

    g.drawText (
        "Output",
        activityBounds.getRight() - 48,
        activityBounds.getCentreY() - 9,
        34,
        18,
        juce::Justification::centredRight);
}

void SmartDenoiseAudioProcessorEditor::
drawFooter (
    juce::Graphics& g)
{
    drawPanel (
        g,
        footerBounds,
        false);

    g.setColour (ui::textMuted);
    g.setFont (
        juce::FontOptions (8.5f));

    g.drawText (
        "QUALITY",
        quality.getX() - 49,
        quality.getY(),
        42,
        quality.getHeight(),
        juce::Justification::centredRight);

    g.drawText (
        "Smart Denoise  v0.4",
        footerBounds.getRight() - 134,
        footerBounds.getY(),
        118,
        footerBounds.getHeight(),
        juce::Justification::centredRight);
}

void SmartDenoiseAudioProcessorEditor::
drawAdvancedDrawer (
    juce::Graphics& g)
{
    drawPanel (
        g,
        advancedBounds,
        true);

    g.setColour (
        ui::borderSoft.withAlpha (0.72f));
    g.drawVerticalLine (
        advancedBounds.getCentreX(),
        static_cast<float> (
            advancedBounds.getY() + 48),
        static_cast<float> (
            advancedBounds.getBottom() - 18));

    const auto& engine =
        processor.getEngine();

    const auto analysis =
        engine.getFrameAnalysis();

    auto left =
        juce::Rectangle<int> (
            advancedBounds.getX() + 18,
            advancedBounds.getY() + 47,
            advancedBounds.getWidth() / 2 - 34,
            advancedBounds.getHeight() - 63);

    auto right =
        juce::Rectangle<int> (
            advancedBounds.getCentreX() + 18,
            advancedBounds.getY() + 47,
            advancedBounds.getWidth() / 2 - 36,
            advancedBounds.getHeight() - 63);

    g.setColour (ui::textSecondary);
    g.setFont (
        juce::FontOptions (9.2f));
    g.drawText (
        "PROCESSING",
        left.removeFromTop (18),
        juce::Justification::centredLeft);

    left.removeFromTop (43);

    g.setColour (ui::textMuted);
    g.setFont (
        juce::FontOptions (8.6f));
    g.drawText (
        "Max Reduction",
        left.removeFromTop (18),
        juce::Justification::centredLeft);

    g.setColour (ui::textPrimary);
    g.setFont (
        juce::FontOptions (10.2f));
    g.drawText (
        "24.0 dB",
        left.removeFromTop (18),
        juce::Justification::centredLeft);

    g.setColour (ui::textSecondary);
    g.setFont (
        juce::FontOptions (9.2f));
    g.drawText (
        "NOISE PROFILE",
        right.removeFromTop (18),
        juce::Justification::centredLeft);

    g.setColour (
        engine.hasProfile()
            ? ui::accentCyan
            : ui::textMuted);

    g.setFont (
        juce::FontOptions (11.0f));

    g.drawText (
        engine.hasProfile()
            ? "FROZEN / LOCKED"
            : "NOT LEARNED",
        right.removeFromTop (26),
        juce::Justification::centredLeft);

    g.setColour (ui::textMuted);
    g.setFont (
        juce::FontOptions (8.6f));

    g.drawText (
        "Explicit Learn stays frozen until re-learn.",
        right.removeFromTop (22),
        juce::Justification::centredLeft);

    auto detailArea =
        right.removeFromTop (30);
    drawTelemetry (
        g,
        detailArea,
        "DETAIL GUARD",
        analysis.detailProtection);

    auto tailArea =
        right.removeFromTop (30);
    drawTelemetry (
        g,
        tailArea,
        "TAIL PROTECT",
        analysis.tailProtection);

    g.setColour (ui::textMuted);
    g.setFont (
        juce::FontOptions (8.6f));
    g.drawText (
        "Latency  "
        + juce::String (
            engine.getLatencySamples())
        + " samples",
        right.removeFromTop (22),
        juce::Justification::centredLeft);
}

void SmartDenoiseAudioProcessorEditor::
drawTelemetry (
    juce::Graphics& g,
    juce::Rectangle<int> area,
    const juce::String& label,
    float value01)
{
    const float value =
        ui::clamp01 (value01);

    auto top =
        area.removeFromTop (14);

    g.setColour (ui::textMuted);
    g.setFont (
        juce::FontOptions (8.2f));
    g.drawText (
        label,
        top.removeFromLeft (
            top.getWidth() - 37),
        juce::Justification::centredLeft);

    g.setColour (
        value > 0.02f
            ? ui::accentCyan
            : ui::textMuted);

    g.drawText (
        juce::String (
            juce::roundToInt (
                value * 100.0f))
        + "%",
        top,
        juce::Justification::centredRight);

    auto bar =
        area.removeFromTop (4)
            .toFloat();

    g.setColour (ui::borderSoft);
    g.fillRoundedRectangle (
        bar,
        2.0f);

    auto fill = bar;
    fill.setWidth (
        bar.getWidth() * value);

    g.setGradientFill (
        ui::accentGradient (bar));
    g.fillRoundedRectangle (
        fill,
        2.0f);
}

void SmartDenoiseAudioProcessorEditor::
drawMeter (
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
        (clamped + 60.0f)
        / 60.0f;

    g.setColour (ui::textMuted);
    g.setFont (
        juce::FontOptions (8.2f));

    g.drawText (
        label,
        juce::Rectangle<int> (
            static_cast<int> (
                bounds.getX() - 9.0f),
            static_cast<int> (
                bounds.getY() - 21.0f),
            30,
            16),
        juce::Justification::centred);

    constexpr int segments = 20;
    const float gap = 2.0f;

    const float segmentHeight =
        (bounds.getHeight()
         - gap
             * static_cast<float> (
                 segments - 1))
        / static_cast<float> (
            segments);

    const int lit =
        juce::roundToInt (
            norm
            * static_cast<float> (
                segments));

    for (int i = 0;
         i < segments;
         ++i)
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

        g.setColour (
            i < lit
                ? ui::accentBlue
                    .interpolatedWith (
                        ui::accentPurple,
                        static_cast<float> (i)
                        / static_cast<float> (
                            segments - 1))
                : ui::borderSoft);

        g.fillRoundedRectangle (
            segment,
            1.5f);
    }

    const std::array<int, 5> tickValues {
        0, -12, -24, -36, -60
    };

    if (label == "IN")
    {
        for (const int tick : tickValues)
        {
            const float t =
                static_cast<float> (
                    tick + 60)
                / 60.0f;

            const float y =
                bounds.getBottom()
                - t * bounds.getHeight();

            g.setColour (
                ui::textMuted.withAlpha (0.75f));
            g.setFont (
                juce::FontOptions (6.8f));

            g.drawText (
                juce::String (tick),
                juce::Rectangle<int> (
                    static_cast<int> (
                        bounds.getX() - 24.0f),
                    static_cast<int> (
                        y - 6.0f),
                    19,
                    12),
                juce::Justification::centredRight);
        }
    }

    g.setColour (ui::textSecondary);
    g.setFont (
        juce::FontOptions (8.0f));

    g.drawText (
        juce::String (valueDb, 1),
        juce::Rectangle<int> (
            static_cast<int> (
                bounds.getX() - 13.0f),
            static_cast<int> (
                bounds.getBottom() + 5.0f),
            37,
            15),
        juce::Justification::centred);
}

void SmartDenoiseAudioProcessorEditor::resized()
{
    headerBounds =
        { 15, 11, 910, 54 };

    captureBounds =
        { 15, 77, 218, 330 };

    cleanBounds =
        { 245, 77, 456, 330 };

    checkBounds =
        { 713, 77, 212, 330 };

    activityBounds =
        { 15, 419, 910, 61 };

    footerBounds =
        { 15, 492, 910, 34 };

    title.setBounds (
        headerBounds.getX() + 51,
        headerBounds.getY() + 8,
        228,
        37);

    abButton.setBounds (
        headerBounds.getRight() - 219,
        headerBounds.getY() + 12,
        52,
        29);

    undoButton.setBounds (
        headerBounds.getRight() - 162,
        headerBounds.getY() + 12,
        44,
        29);

    redoButton.setBounds (
        headerBounds.getRight() - 115,
        headerBounds.getY() + 12,
        44,
        29);

    helpButton.setBounds (
        headerBounds.getRight() - 63,
        headerBounds.getY() + 12,
        30,
        29);

    learn.setBounds (
        captureBounds.getX() + 37,
        captureBounds.getY() + 50,
        144,
        144);

    profileName.setBounds (
        captureBounds.getX() + 45,
        captureBounds.getY() + 236,
        captureBounds.getWidth() - 82,
        26);

    profileStatus.setBounds (
        captureBounds.getX() + 20,
        captureBounds.getY() + 308,
        captureBounds.getWidth() - 40,
        18);

    reductionLabel.setBounds (
        cleanBounds.getX() + 25,
        cleanBounds.getY() + 51,
        260,
        20);

    reduction.setBounds (
        cleanBounds.getX() + 17,
        cleanBounds.getY() + 67,
        276,
        246);

    preserveLabel.setBounds (
        cleanBounds.getX() + 315,
        cleanBounds.getY() + 54,
        122,
        18);

    preserve.setBounds (
        cleanBounds.getX() + 320,
        cleanBounds.getY() + 73,
        112,
        106);

    silenceLabel.setBounds (
        cleanBounds.getX() + 315,
        cleanBounds.getY() + 190,
        122,
        18);

    silence.setBounds (
        cleanBounds.getX() + 320,
        cleanBounds.getY() + 209,
        112,
        103);

    hearRemoved.setBounds (
        checkBounds.getX() + 14,
        checkBounds.getY() + 61,
        111,
        77);

    bypass.setBounds (
        checkBounds.getX() + 14,
        checkBounds.getY() + 149,
        111,
        70);

    advanced.setBounds (
        footerBounds.getX() + 8,
        footerBounds.getY() + 4,
        108,
        26);

    quality.setBounds (
        footerBounds.getCentreX() - 60,
        footerBounds.getY() + 4,
        190,
        26);

    if (advancedDrawerVisible)
    {
        advancedBounds =
            { 15, 538, 910, 146 };

        advancedTitle.setBounds (
            advancedBounds.getX() + 18,
            advancedBounds.getY() + 9,
            140,
            27);

        advancedClose.setBounds (
            advancedBounds.getRight() - 82,
            advancedBounds.getY() + 9,
            66,
            25);

        profileOffsetLabel.setBounds (
            advancedBounds.getX() + 21,
            advancedBounds.getY() + 69,
            102,
            23);

        profileOffset.setBounds (
            advancedBounds.getX() + 125,
            advancedBounds.getY() + 67,
            286,
            27);
    }
}

void SmartDenoiseAudioProcessorEditor::
timerCallback()
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

    auto& engine =
        processor.getEngine();

    learn.setLearnState (
        engine.isLearning(),
        engine.getLearningProgress(),
        engine.hasProfile(),
        engine.wasLastLearnRejected());

    juce::String profileText;

    if (engine.isLearning())
    {
        profileText =
            "Learning  "
            + juce::String (
                juce::roundToInt (
                    engine.getLearningProgress()
                    * 100.0f))
            + "%";
    }
    else if (engine.wasLastLearnRejected())
    {
        profileText =
            engine.hasProfile()
                ? "Learn rejected - previous profile kept"
                : "Learn rejected - capture noise only";
    }
    else if (engine.hasProfile())
    {
        profileText =
            "Frozen profile  "
            + juce::String (
                juce::roundToInt (
                    engine.getProfileQuality()
                    * 100.0f))
            + "%";

        profileName.setText (
            "Frozen Noise Profile",
            juce::dontSendNotification);
    }
    else
    {
        profileText =
            "No profile - learn room noise";

        profileName.setText (
            "Noise Profile",
            juce::dontSendNotification);
    }

    profileStatus.setText (
        profileText,
        juce::dontSendNotification);

    syncBypassButton();
    repaint();
}
