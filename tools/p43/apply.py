from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[2]
editor = ROOT / "Source/Plugin/PluginEditor.cpp"
qa = ROOT / "qa/verify_p4_visual_fidelity.py"

text = editor.read_text(encoding="utf-8")

new_rotary = r'''void CleanLookAndFeel::drawRotarySlider (
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

void CleanLookAndFeel::drawLinearSlider ('''

text, count = re.subn(
    r"void CleanLookAndFeel::drawRotarySlider \(.*?\n\}\n\nvoid CleanLookAndFeel::drawLinearSlider \(",
    new_rotary,
    text,
    count=1,
    flags=re.S,
)
assert count == 1, f"drawRotarySlider replacement count={count}"

required = [
    "auto knobShadow =",
    "auto metalBezel =",
    "bezelGradient",
    "auto innerRim =",
    "juce::ColourGradient knobGradient",
    "auto specularGlow =",
    "specularGradient",
    "auto markerShadow =",
    "auto markerBezel =",
]
for needle in required:
    assert needle in text, needle

editor.write_text(text, encoding="utf-8", newline="\n")

qa_text = qa.read_text(encoding="utf-8")
needle = 'check("Knob inner shading", "knobGradient" in editor_cpp)\n'
replacement = needle + '''check("P4.3 physical knob shadow", "auto knobShadow =" in editor_cpp and "fillEllipse (knobShadow)" in editor_cpp)\ncheck("P4.3 metallic bezel", "auto metalBezel =" in editor_cpp and "bezelGradient" in editor_cpp)\ncheck("P4.3 inner bevel depth", "auto innerRim =" in editor_cpp and "rimGradient" in editor_cpp)\ncheck("P4.3 glossy specular reflection", "auto specularGlow =" in editor_cpp and "specularGradient" in editor_cpp)\ncheck("P4.3 dimensional marker", "auto markerShadow =" in editor_cpp and "auto markerBezel =" in editor_cpp)\n'''
assert needle in qa_text
qa_text = qa_text.replace(needle, replacement, 1)
qa_text = qa_text.replace(
    'print("SMART DENOISE P4.2 VISUAL CORRECTION CONTRACT")',
    'print("SMART DENOISE P4.3 PREMIUM 3D KNOB CONTRACT")',
)
qa.write_text(qa_text, encoding="utf-8", newline="\n")

print("P4.3 premium 3D glossy knob patch applied")
