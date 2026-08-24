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
            .reduced (4.5f);

    const bool primary =
        slider.getName() == "primary";

    const float diameter =
        juce::jmin (
            bounds.getWidth(),
            bounds.getHeight());

    const auto centre = bounds.getCentre();
    const float outerRadius =
        diameter * (primary ? 0.432f : 0.392f);
    const float trackStroke =
        primary ? 10.0f : 6.0f;

    if (primary)
    {
        constexpr int tickCount = 25;
        for (int tick = 0; tick < tickCount; ++tick)
        {
            const float t =
                static_cast<float> (tick)
                / static_cast<float> (tickCount - 1);
            const float angle =
                startAngle + t * (endAngle - startAngle);
            const auto outer =
                centre.getPointOnCircumference (
                    outerRadius + 14.0f,
                    angle);
            const auto inner =
                centre.getPointOnCircumference (
                    outerRadius + 9.5f,
                    angle);
            g.setColour (ui::border.withAlpha (0.44f));
            g.drawLine (
                inner.x, inner.y,
                outer.x, outer.y,
                1.0f);
        }
    }

    juce::Path baseArc;
    baseArc.addCentredArc (
        centre.x,
        centre.y,
        outerRadius,
        outerRadius,
        0.0f,
        startAngle,
        endAngle,
        true);

    g.setColour (juce::Colours::black.withAlpha (0.42f));
    g.strokePath (
        baseArc,
        juce::PathStrokeType (
            trackStroke + (primary ? 4.0f : 2.5f),
            juce::PathStrokeType::curved,
            juce::PathStrokeType::rounded));

    g.setColour (juce::Colour::fromRGB (46, 50, 60));
    g.strokePath (
        baseArc,
        juce::PathStrokeType (
            trackStroke,
            juce::PathStrokeType::curved,
            juce::PathStrokeType::rounded));

    const float valueAngle =
        startAngle + sliderPos * (endAngle - startAngle);

    juce::Path activeArc;
    activeArc.addCentredArc (
        centre.x,
        centre.y,
        outerRadius,
        outerRadius,
        0.0f,
        startAngle,
        valueAngle,
        true);

    // Reference direction: the purple-blue arc is crisp and hardware-like,
    // with restrained illumination rather than a broad synthetic glow.
    g.setColour (ui::accentPurple.withAlpha (0.085f));
    g.strokePath (
        activeArc,
        juce::PathStrokeType (
            trackStroke + (primary ? 2.8f : 1.6f),
            juce::PathStrokeType::curved,
            juce::PathStrokeType::rounded));

    juce::ColourGradient arcGradient (
        ui::accentPurple,
        centre.x - outerRadius,
        centre.y,
        ui::accentBlue,
        centre.x + outerRadius,
        centre.y,
        false);
    arcGradient.addColour (
        0.35,
        juce::Colour::fromRGB (214, 28, 244));
    g.setGradientFill (arcGradient);
    g.strokePath (
        activeArc,
        juce::PathStrokeType (
            trackStroke,
            juce::PathStrokeType::curved,
            juce::PathStrokeType::rounded));

    // Depth comes from a dark recessed circular cavity, not a bulging face.
    const float recessRadius =
        outerRadius - (primary ? 7.2f : 4.3f);
    auto recess =
        juce::Rectangle<float> (
            recessRadius * 2.0f,
            recessRadius * 2.0f)
            .withCentre (centre);

    juce::ColourGradient recessGradient (
        juce::Colour::fromRGB (5, 6, 9),
        centre.x,
        recess.getY(),
        juce::Colour::fromRGB (20, 21, 26),
        centre.x,
        recess.getBottom(),
        false);
    recessGradient.addColour (
        0.48,
        juce::Colour::fromRGB (8, 9, 12));
    g.setGradientFill (recessGradient);
    g.fillEllipse (recess);

    g.setColour (juce::Colours::black.withAlpha (0.82f));
    g.drawEllipse (
        recess.reduced (0.5f),
        primary ? 1.8f : 1.15f);

    // Thin restrained inner ring, similar to the reference housing lip.
    g.setColour (juce::Colours::white.withAlpha (0.030f));
    g.drawEllipse (
        recess.reduced (primary ? 3.0f : 1.8f),
        primary ? 0.85f : 0.55f);

    // Make the face large and visually dominant. The reference is a flat
    // machined disc sitting inside the cavity, not a small glass dome.
    const float faceRadius =
        outerRadius - (primary ? 12.2f : 7.0f);
    auto face =
        juce::Rectangle<float> (
            faceRadius * 2.0f,
            faceRadius * 2.0f)
            .withCentre (centre);

    // Narrow graphite bezel only: avoid wide chrome/3D rings.
    auto bezel =
        face.expanded (primary ? 1.15f : 0.75f);
    g.setColour (juce::Colour::fromRGB (28, 30, 36));
    g.fillEllipse (bezel);
    g.setColour (juce::Colours::black.withAlpha (0.72f));
    g.drawEllipse (
        bezel,
        primary ? 1.35f : 0.9f);

    // Mostly-flat face: linear directional shading, deliberately no radial
    // convex hotspot. This prevents the control from reading as a sphere.
    juce::ColourGradient faceGradient (
        juce::Colour::fromRGB (32, 34, 40),
        centre.x - faceRadius * 0.58f,
        centre.y - faceRadius * 0.50f,
        juce::Colour::fromRGB (17, 18, 22),
        centre.x + faceRadius * 0.70f,
        centre.y + faceRadius * 0.66f,
        false);
    faceGradient.addColour (
        0.52,
        juce::Colour::fromRGB (22, 23, 28));
    g.setGradientFill (faceGradient);
    g.fillEllipse (face);

    // Fine radial machining. Lots of low-alpha spokes give the subtle
    // sunburst texture seen on premium brushed/machined hardware knobs.
    {
        juce::Graphics::ScopedSaveState state (g);
        juce::Path faceClip;
        faceClip.addEllipse (
            face.reduced (primary ? 1.0f : 0.7f));
        g.reduceClipRegion (faceClip);

        constexpr int spokeCount = 144;
        for (int spoke = 0; spoke < spokeCount; ++spoke)
        {
            const float angle =
                juce::MathConstants<float>::twoPi
                * static_cast<float> (spoke)
                / static_cast<float> (spokeCount);
            const auto end =
                centre.getPointOnCircumference (
                    faceRadius,
                    angle);

            const float alpha =
                (spoke % 3 == 0) ? 0.028f : 0.012f;
            g.setColour (
                juce::Colours::white.withAlpha (alpha));
            g.drawLine (
                centre.x,
                centre.y,
                end.x,
                end.y,
                0.52f);
        }

        // A directional satin sheen only. No circular specular ellipse,
        // no glossy dome, no sphere highlight.
        juce::ColourGradient directionalSheen (
            juce::Colours::white.withAlpha (0.042f),
            centre.x - faceRadius * 0.70f,
            centre.y - faceRadius * 0.64f,
            juce::Colours::transparentWhite,
            centre.x + faceRadius * 0.34f,
            centre.y + faceRadius * 0.30f,
            false);
        g.setGradientFill (directionalSheen);

        juce::Path sheenBand;
        sheenBand.startNewSubPath (
            face.getX(),
            face.getY() + face.getHeight() * 0.20f);
        sheenBand.lineTo (
            face.getRight(),
            face.getY() + face.getHeight() * 0.40f);
        sheenBand.lineTo (
            face.getRight(),
            face.getY() + face.getHeight() * 0.53f);
        sheenBand.lineTo (
            face.getX(),
            face.getY() + face.getHeight() * 0.33f);
        sheenBand.closeSubPath();
        g.fillPath (sheenBand);
    }

    // Very thin face edge, not a 3D bevel.
    g.setColour (juce::Colours::white.withAlpha (0.045f));
    g.drawEllipse (
        face.reduced (primary ? 0.8f : 0.55f),
        primary ? 0.72f : 0.50f);

    juce::String valueText;
    if (primary)
    {
        const auto percent =
            juce::roundToInt (
                100.0
                * slider.getValue()
                / smartdenoise::SmartDenoiseEngine::maxReductionDb);
        valueText = juce::String (percent) + "%";
    }
    else
    {
        const auto percent =
            juce::roundToInt (
                slider.getValue() * 100.0);
        valueText = juce::String (percent) + "%";
    }

    g.setColour (ui::textPrimary);
    g.setFont (
        juce::FontOptions (
            primary ? 35.0f : 15.5f));
    g.drawFittedText (
        valueText,
        face.toNearestInt().reduced (
            primary ? 24 : 10),
        juce::Justification::centred,
        1);
}
'''

text = text[:start] + new_func + text[end:]
editor.write_text(text, encoding='utf-8', newline='\n')

qa = ROOT / 'qa/verify_p4_visual_fidelity.py'
qa_text = qa.read_text(encoding='utf-8')
replacements = {
    'check("Primary active arc glow", "stroke + (primary ? 8.0f : 4.0f)" in editor_cpp)\n': 'check("P4.5 restrained hardware arc illumination", "trackStroke + (primary ? 2.8f : 1.6f)" in editor_cpp)\n',
    'check("Knob inner shading", "knobGradient" in editor_cpp)\n': 'check("P4.5 flat directional face shading", "faceGradient" in editor_cpp and "Mostly-flat face" in editor_cpp)\n',
    'check("P4.4 flat machined knob face", "constexpr int spokeCount = 92" in editor_cpp and "recessGradient" in editor_cpp and "bezelGradient" in editor_cpp and "sheenGradient" in editor_cpp)\n': 'check("P4.5 reference flat machined face", "constexpr int spokeCount = 144" in editor_cpp and "recessGradient" in editor_cpp and "faceGradient" in editor_cpp and "directionalSheen" in editor_cpp)\n',
    'check("P4.4 recessed hardware well", "auto recess =" in editor_cpp and "recessGradient" in editor_cpp)\n': 'check("P4.5 clean recessed hardware well", "const float recessRadius =" in editor_cpp and "fillEllipse (recess)" in editor_cpp)\n',
    'check("P4.4 narrow machined bezel", "auto bezel =" in editor_cpp and "bezelGradient" in editor_cpp)\n': 'check("P4.5 narrow graphite bezel", "auto bezel =" in editor_cpp and "face.expanded" in editor_cpp and "bezelGradient" not in editor_cpp)\n',
    'check("P4.4 flat face rim", "auto faceRim =" in editor_cpp and "fillEllipse (faceRim)" in editor_cpp)\n': 'check("P4.5 dominant flat disc face", "const float faceRadius =" in editor_cpp and "fillEllipse (face)" in editor_cpp)\n',
    'check("P4.4 brushed radial texture", "constexpr int spokeCount = 92" in editor_cpp and "faceClip.addEllipse" in editor_cpp)\n': 'check("P4.5 fine radial machining", "constexpr int spokeCount = 144" in editor_cpp and "faceClip.addEllipse" in editor_cpp)\n',
    'check("P4.4 restrained top sheen", "auto sheen =" in editor_cpp and "sheenGradient" in editor_cpp)\n': 'check("P4.5 no sphere gloss or marker dot", "directionalSheen" in editor_cpp and "sheenBand" in editor_cpp and "markerDiameter" not in editor_cpp and "specularGlow" not in editor_cpp and "knobShadow" not in editor_cpp)\n'
}
for old, new in replacements.items():
    if old not in qa_text:
        raise RuntimeError(f'QA anchor missing: {old}')
    qa_text = qa_text.replace(old, new, 1)

qa_text = qa_text.replace(
    'print("SMART DENOISE P4.4 FLAT MACHINED KNOB CONTRACT")\n',
    'print("SMART DENOISE P4.5 REFERENCE KNOB MATCH CONTRACT")\n')
qa.write_text(qa_text, encoding='utf-8', newline='\n')

changelog = ROOT / 'CHANGELOG.md'
cl = changelog.read_text(encoding='utf-8')
anchor = '- P4.4 retunes the knob family away from a spherical dome and toward a flatter machined-disc look with a recessed well, narrower bezel, brushed radial face texture and restrained sheen closer to premium hardware knobs.\n'
insert = anchor + '- P4.5 matches the approved knob reference more aggressively: all remaining sphere/gloss cues and glossy marker dots are removed, the flat disc face is enlarged, the cavity/bezel are simplified, the active ring is crisper, and the face uses 144-line radial machining plus a directional satin sheen instead of dome-like specular highlights.\n'
if anchor in cl and 'P4.5 matches the approved knob reference more aggressively' not in cl:
    cl = cl.replace(anchor, insert, 1)
changelog.write_text(cl, encoding='utf-8', newline='\n')

new_editor = editor.read_text(encoding='utf-8')
assert 'constexpr int spokeCount = 144' in new_editor
assert 'directionalSheen' in new_editor
assert 'sheenBand' in new_editor
assert 'markerDiameter' not in new_editor
assert 'specularGlow' not in new_editor
assert 'knobShadow' not in new_editor
