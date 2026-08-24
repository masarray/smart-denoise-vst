from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
editor = ROOT / 'Source/Plugin/PluginEditor.cpp'
text = editor.read_text(encoding='utf-8')

start = text.index('void CleanLookAndFeel::drawRotarySlider')
end = text.index('\nvoid CleanLookAndFeel::drawLinearSlider', start)

new_func = '''void CleanLookAndFeel::drawRotarySlider (
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

    g.setColour (juce::Colours::black.withAlpha (0.34f));
    g.strokePath (
        baseArc,
        juce::PathStrokeType (
            stroke + 5.0f,
            juce::PathStrokeType::curved,
            juce::PathStrokeType::rounded));

    g.setColour (
        juce::Colour::fromRGB (45, 51, 67));
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

    g.setColour (
        ui::accentBlue.withAlpha (
            primary ? 0.045f : 0.035f));
    g.strokePath (
        activeArc,
        juce::PathStrokeType (
            stroke + (primary ? 8.0f : 4.0f),
            juce::PathStrokeType::curved,
            juce::PathStrokeType::rounded));

    g.setColour (
        ui::accentPurple.withAlpha (
            primary ? 0.12f : 0.09f));
    g.strokePath (
        activeArc,
        juce::PathStrokeType (
            stroke + (primary ? 4.0f : 2.0f),
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

    auto recess =
        knob.expanded (primary ? 10.5f : 6.0f);

    juce::ColourGradient recessGradient (
        juce::Colour::fromRGB (10, 12, 17),
        recess.getTopLeft(),
        juce::Colour::fromRGB (26, 29, 38),
        recess.getBottomRight(),
        false);

    g.setGradientFill (recessGradient);
    g.fillEllipse (recess);

    g.setColour (juce::Colours::black.withAlpha (0.68f));
    g.drawEllipse (
        recess.reduced (0.8f),
        primary ? 2.2f : 1.6f);

    g.setColour (juce::Colours::white.withAlpha (0.04f));
    g.drawEllipse (
        recess.reduced (primary ? 4.0f : 2.6f),
        primary ? 1.0f : 0.8f);

    auto bezel =
        knob.expanded (primary ? 3.2f : 2.1f);

    juce::ColourGradient bezelGradient (
        juce::Colour::fromRGB (78, 83, 97),
        bezel.getX() + bezel.getWidth() * 0.22f,
        bezel.getY() + bezel.getHeight() * 0.20f,
        juce::Colour::fromRGB (18, 21, 28),
        bezel.getRight() - bezel.getWidth() * 0.18f,
        bezel.getBottom() - bezel.getHeight() * 0.14f,
        false);

    bezelGradient.addColour (
        0.30,
        juce::Colour::fromRGB (52, 58, 71));
    bezelGradient.addColour (
        0.68,
        juce::Colour::fromRGB (27, 31, 40));

    g.setGradientFill (bezelGradient);
    g.fillEllipse (bezel);

    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.drawEllipse (
        bezel.reduced (primary ? 1.2f : 0.9f),
        primary ? 1.9f : 1.35f);

    auto faceRim =
        knob.expanded (primary ? 0.9f : 0.65f);

    g.setColour (juce::Colour::fromRGB (23, 26, 34));
    g.fillEllipse (faceRim);

    juce::ColourGradient knobGradient (
        juce::Colour::fromRGB (36, 38, 45),
        knob.getX() + knob.getWidth() * 0.30f,
        knob.getY() + knob.getHeight() * 0.24f,
        juce::Colour::fromRGB (17, 19, 24),
        knob.getRight() - knob.getWidth() * 0.18f,
        knob.getBottom() - knob.getHeight() * 0.12f,
        false);

    knobGradient.addColour (
        0.54,
        juce::Colour::fromRGB (27, 29, 35));

    g.setGradientFill (knobGradient);
    g.fillEllipse (knob);

    {
        juce::Graphics::ScopedSaveState state (g);
        juce::Path faceClip;
        faceClip.addEllipse (knob.reduced (primary ? 1.6f : 1.2f));
        g.reduceClipRegion (faceClip);

        constexpr int spokeCount = 92;
        for (int spoke = 0; spoke < spokeCount; ++spoke)
        {
            const float angle =
                juce::MathConstants<float>::twoPi
                * static_cast<float> (spoke)
                / static_cast<float> (spokeCount);

            const auto end =
                centre.getPointOnCircumference (
                    knobRadius,
                    angle);

            g.setColour (
                juce::Colours::white.withAlpha (
                    (spoke % 2 == 0) ? 0.024f : 0.012f));
            g.drawLine (
                centre.x,
                centre.y,
                end.x,
                end.y,
                0.75f);
        }
    }

    auto sheen =
        juce::Rectangle<float> (
            knob.getWidth() * 0.74f,
            knob.getHeight() * 0.24f)
            .withCentre (
                { knob.getCentreX() - knob.getWidth() * 0.07f,
                  knob.getY() + knob.getHeight() * 0.26f });

    juce::ColourGradient sheenGradient (
        juce::Colours::white.withAlpha (0.13f),
        sheen.getCentreX(),
        sheen.getY(),
        juce::Colours::white.withAlpha (0.0f),
        sheen.getCentreX(),
        sheen.getBottom(),
        false);

    g.setGradientFill (sheenGradient);
    g.fillEllipse (sheen);

    g.setColour (juce::Colours::white.withAlpha (0.085f));
    g.drawEllipse (
        knob.reduced (1.0f),
        primary ? 1.15f : 0.9f);

    g.setColour (juce::Colours::black.withAlpha (0.62f));
    g.drawEllipse (
        knob.reduced (primary ? 6.0f : 4.2f),
        primary ? 1.5f : 1.05f);

    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.fillEllipse (
        juce::Rectangle<float> (
            primary ? 5.0f : 3.2f,
            primary ? 5.0f : 3.2f)
            .withCentre (
                { centre.x,
                  centre.y + (primary ? 1.4f : 0.9f) }));

    const auto marker =
        centre.getPointOnCircumference (
            radius,
            valueAngle);

    const float markerDiameter =
        primary ? 10.0f : 7.0f;

    g.setColour (juce::Colours::black.withAlpha (0.42f));
    g.fillEllipse (
        juce::Rectangle<float> (
            markerDiameter + 2.8f,
            markerDiameter + 2.8f)
            .withCentre (
                { marker.x + 0.9f,
                  marker.y + 1.0f }));

    g.setColour (
        ui::accentBlue.withAlpha (0.44f));
    g.fillEllipse (
        juce::Rectangle<float> (
            markerDiameter + 1.4f,
            markerDiameter + 1.4f)
            .withCentre (marker));

    g.setColour (ui::textPrimary);
    g.fillEllipse (
        juce::Rectangle<float> (
            markerDiameter,
            markerDiameter)
            .withCentre (marker));

    g.setColour (juce::Colours::white.withAlpha (0.48f));
    g.fillEllipse (
        juce::Rectangle<float> (
            markerDiameter * 0.34f,
            markerDiameter * 0.34f)
            .withCentre (
                { marker.x - markerDiameter * 0.14f,
                  marker.y - markerDiameter * 0.16f }));

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

    g.setColour (ui::textPrimary);
    g.setFont (
        juce::FontOptions (
            primary ? 35.0f : 15.5f));

    g.drawFittedText (
        valueText,
        knob.toNearestInt().reduced (
            primary ? 24 : 10),
        juce::Justification::centred,
        1);
}
'''

text = text[:start] + new_func + text[end:]
editor.write_text(text, encoding='utf-8', newline='\n')

qa = ROOT / 'qa/verify_p4_visual_fidelity.py'
qa_text = qa.read_text(encoding='utf-8')
old = 'check("Knob inner shading", "knobGradient" in editor_cpp)\n'
new = old + 'check("P4.4 flat machined knob face", "constexpr int spokeCount = 92" in editor_cpp and "recessGradient" in editor_cpp and "bezelGradient" in editor_cpp and "sheenGradient" in editor_cpp)\n'
if old not in qa_text:
    raise RuntimeError('QA anchor not found')
qa_text = qa_text.replace(old, new, 1)
qa.write_text(qa_text, encoding='utf-8', newline='\n')

changelog = ROOT / 'CHANGELOG.md'
cl = changelog.read_text(encoding='utf-8')
anchor = '- Upgraded the primary Reduce Noise macro with a thicker gradient arc, glow pass, radial inner disc, visual ticks, white marker and explicit percentage readout.\n'
insert = anchor + '- P4.4 retunes the knob family away from a spherical dome and toward a flatter machined-disc look with a recessed well, narrower bezel, brushed radial face texture and restrained sheen closer to premium hardware knobs.\n'
if anchor in cl and 'P4.4 retunes the knob family away from a spherical dome' not in cl:
    cl = cl.replace(anchor, insert, 1)
changelog.write_text(cl, encoding='utf-8', newline='\n')

new_editor = editor.read_text(encoding='utf-8')
assert 'constexpr int spokeCount = 92' in new_editor
assert 'recessGradient' in new_editor
assert 'bezelGradient' in new_editor
assert 'sheenGradient' in new_editor
assert 'knobGradient' in new_editor
