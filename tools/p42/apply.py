from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[2]
editor = ROOT / "Source/Plugin/PluginEditor.cpp"
qa = ROOT / "qa/verify_p4_visual_fidelity.py"

text = editor.read_text(encoding="utf-8")

new_headphones = r'''void drawHeadphones (
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

void drawBypass ('''

text, count = re.subn(
    r"void drawHeadphones \(.*?\n\}\n\nvoid drawBypass \(",
    new_headphones,
    text,
    count=1,
    flags=re.S,
)
assert count == 1, f"drawHeadphones replacement count={count}"

new_learn = r'''void LearnCircleButton::paintButton (
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

MonitorButton::MonitorButton ('''

text, count = re.subn(
    r"void LearnCircleButton::paintButton \(.*?\n\}\n\nMonitorButton::MonitorButton \(",
    new_learn,
    text,
    count=1,
    flags=re.S,
)
assert count == 1, f"LearnCircleButton replacement count={count}"

text = text.replace(
'''        juce::Rectangle<float> (
            34.0f,
            32.0f)
            .withCentre (
                { area.getCentreX(),
                  area.getY() + 25.0f });''',
'''        juce::Rectangle<float> (
            38.0f,
            34.0f)
            .withCentre (
                { area.getCentreX(),
                  area.getY() + 26.0f });''',
1,
)

assert "start + ringProgress * (end - start)" not in text
assert "g.drawEllipse (ringBounds, 7.0f);" in text
assert "juce::MathConstants<float>::twoPi" in text
assert "headband.cubicTo" in text
assert "leftCup.reduced" in text

editor.write_text(text, encoding="utf-8", newline="\n")

qa_text = qa.read_text(encoding="utf-8")
needle = 'check("Learn progress ring", "ringProgress = progress" in editor_cpp and "progressArc.addCentredArc" in editor_cpp)\n'
replacement = needle + '''check("P4.2 Learn uses full 360-degree idle track", "g.drawEllipse (ringBounds, 7.0f);" in editor_cpp)\ncheck("P4.2 Learn progress spans full two-pi", "juce::MathConstants<float>::twoPi" in editor_cpp and "ringProgress >= 0.999f" in editor_cpp)\ncheck("P4.2 headphone has continuous headband silhouette", "headband.cubicTo" in editor_cpp and "leftCup" in editor_cpp and "rightCup" in editor_cpp)\n'''
assert needle in qa_text
qa_text = qa_text.replace(needle, replacement, 1)
qa_text = qa_text.replace(
    'print("SMART DENOISE P4.1 CLEAN FOUNDATION CONTRACT")',
    'print("SMART DENOISE P4.2 VISUAL CORRECTION CONTRACT")',
)
qa.write_text(qa_text, encoding="utf-8", newline="\n")

print("P4.2 source patch applied")
