from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
p=ROOT/'Source/Plugin/PluginEditor.cpp'
s=p.read_text(encoding='utf-8')

# Learn cursor
old='''    setClickingTogglesState (false);\n    setWantsKeyboardFocus (false);\n}'''
new='''    setClickingTogglesState (false);\n    setWantsKeyboardFocus (false);\n    setMouseCursor (juce::MouseCursor::PointingHandCursor);\n}'''
if old not in s: raise RuntimeError('learn ctor anchor')
s=s.replace(old,new,1)

# Learn 3D face: insert shadow/rim before existing face gradient.
anchor='''    juce::ColourGradient face (\n        ui::panelRaised.brighter (\n            isMouseOverButton ? 0.08f : 0.03f),\n        faceCircle.getTopLeft(),\n        ui::panelDeep,\n        faceCircle.getBottomRight(),\n        false);\n\n    g.setGradientFill (face);\n    g.fillEllipse (faceCircle);\n\n    g.setColour (ui::borderSoft);\n    g.drawEllipse (faceCircle, 1.0f);\n'''
replace='''    // P4.6 raised clickable face: shadow + hardware rim + pressed depth.\n    auto buttonShadow = faceCircle.translated (0.0f, isButtonDown ? 2.0f : 4.0f).expanded (2.4f);\n    g.setColour (juce::Colours::black.withAlpha (0.50f));\n    g.fillEllipse (buttonShadow);\n\n    auto buttonRim = faceCircle.expanded (3.0f);\n    juce::ColourGradient buttonRimGradient (\n        juce::Colour::fromRGB (66, 71, 86),\n        buttonRim.getX(), buttonRim.getY(),\n        juce::Colour::fromRGB (14, 17, 24),\n        buttonRim.getRight(), buttonRim.getBottom(),\n        false);\n    buttonRimGradient.addColour (0.44, juce::Colour::fromRGB (36, 41, 53));\n    g.setGradientFill (buttonRimGradient);\n    g.fillEllipse (buttonRim);\n    g.setColour (juce::Colours::black.withAlpha (0.72f));\n    g.drawEllipse (buttonRim, 1.5f);\n\n    juce::ColourGradient face (\n        ui::panelRaised.brighter (\n            isMouseOverButton ? 0.12f : 0.045f),\n        faceCircle.getTopLeft(),\n        ui::panelDeep.darker (isButtonDown ? 0.08f : 0.0f),\n        faceCircle.getBottomRight(),\n        false);\n\n    g.setGradientFill (face);\n    g.fillEllipse (faceCircle);\n\n    // Edge-only highlight/shadow makes the button look raised without becoming spherical.\n    juce::Path learnTopEdge;\n    learnTopEdge.addCentredArc (\n        centre.x, centre.y,\n        faceCircle.getWidth() * 0.5f - 1.5f,\n        faceCircle.getHeight() * 0.5f - 1.5f,\n        0.0f,\n        1.12f * juce::MathConstants<float>::pi,\n        1.88f * juce::MathConstants<float>::pi,\n        true);\n    g.setColour (juce::Colours::white.withAlpha (isMouseOverButton ? 0.18f : 0.11f));\n    g.strokePath (learnTopEdge, juce::PathStrokeType (1.2f));\n\n    juce::Path learnBottomEdge;\n    learnBottomEdge.addCentredArc (\n        centre.x, centre.y,\n        faceCircle.getWidth() * 0.5f - 1.5f,\n        faceCircle.getHeight() * 0.5f - 1.5f,\n        0.0f,\n        0.12f * juce::MathConstants<float>::pi,\n        0.88f * juce::MathConstants<float>::pi,\n        true);\n    g.setColour (juce::Colours::black.withAlpha (0.62f));\n    g.strokePath (learnBottomEdge, juce::PathStrokeType (1.6f));\n'''
if anchor not in s: raise RuntimeError('learn face anchor')
s=s.replace(anchor,replace,1)

# Replace knob thin bezel with perimeter 3D edge.
anchor='''    // Narrow graphite bezel only: avoid wide chrome/3D rings.\n    auto bezel =\n        face.expanded (primary ? 1.15f : 0.75f);\n    g.setColour (juce::Colour::fromRGB (28, 30, 36));\n    g.fillEllipse (bezel);\n    g.setColour (juce::Colours::black.withAlpha (0.72f));\n    g.drawEllipse (\n        bezel,\n        primary ? 1.35f : 0.9f);\n'''
replace='''    // P4.6 perimeter-only 3D edge. The face stays flat; depth comes from the rim.\n    auto edgeOuter = face.expanded (primary ? 3.2f : 1.9f);\n    juce::ColourGradient edgeGradient (\n        juce::Colour::fromRGB (72, 76, 87),\n        edgeOuter.getX(), edgeOuter.getY(),\n        juce::Colour::fromRGB (9, 10, 14),\n        edgeOuter.getRight(), edgeOuter.getBottom(),\n        false);\n    edgeGradient.addColour (0.46, juce::Colour::fromRGB (34, 37, 45));\n    g.setGradientFill (edgeGradient);\n    g.fillEllipse (edgeOuter);\n\n    auto edgeInner = face.expanded (primary ? 1.15f : 0.72f);\n    g.setColour (juce::Colour::fromRGB (22, 24, 29));\n    g.fillEllipse (edgeInner);\n    g.setColour (juce::Colours::black.withAlpha (0.78f));\n    g.drawEllipse (edgeOuter, primary ? 1.5f : 0.95f);\n\n    juce::Path knobTopEdge;\n    knobTopEdge.addCentredArc (\n        centre.x, centre.y,\n        faceRadius + (primary ? 2.0f : 1.2f),\n        faceRadius + (primary ? 2.0f : 1.2f),\n        0.0f,\n        1.08f * juce::MathConstants<float>::pi,\n        1.92f * juce::MathConstants<float>::pi,\n        true);\n    g.setColour (juce::Colours::white.withAlpha (0.11f));\n    g.strokePath (knobTopEdge, juce::PathStrokeType (primary ? 1.2f : 0.75f));\n\n    juce::Path knobBottomEdge;\n    knobBottomEdge.addCentredArc (\n        centre.x, centre.y,\n        faceRadius + (primary ? 2.0f : 1.2f),\n        faceRadius + (primary ? 2.0f : 1.2f),\n        0.0f,\n        0.08f * juce::MathConstants<float>::pi,\n        0.92f * juce::MathConstants<float>::pi,\n        true);\n    g.setColour (juce::Colours::black.withAlpha (0.72f));\n    g.strokePath (knobBottomEdge, juce::PathStrokeType (primary ? 1.8f : 1.05f));\n'''
if anchor not in s: raise RuntimeError('knob bezel anchor')
s=s.replace(anchor,replace,1)

# Remove the translucent sheen-band block entirely.
start=s.index('        // A directional satin sheen only. No circular specular ellipse,')
end=s.index('    // Very thin face edge, not a 3D bevel.', start)
s=s[:start] + '''        // P4.6: no translucent sheen patch is painted over the machined face.\n''' + s[end:]

p.write_text(s,encoding='utf-8',newline='\n')

# QA
q=ROOT/'qa/verify_p4_visual_fidelity.py'
t=q.read_text(encoding='utf-8')
t=t.replace(
'check("P4.5 no sphere gloss or marker dot", "directionalSheen" in editor_cpp and "sheenBand" in editor_cpp and "markerDiameter" not in editor_cpp and "specularGlow" not in editor_cpp and "knobShadow" not in editor_cpp)\n',
'check("P4.6 no translucent knob overlay", "sheenBand" not in editor_cpp and "directionalSheen" not in editor_cpp and "specularGlow" not in editor_cpp)\ncheck("P4.6 perimeter-only 3D knob edge", "edgeOuter" in editor_cpp and "edgeGradient" in editor_cpp and "knobTopEdge" in editor_cpp and "knobBottomEdge" in editor_cpp)\ncheck("P4.6 Learn is visibly clickable", "buttonShadow" in editor_cpp and "buttonRim" in editor_cpp and "learnTopEdge" in editor_cpp and "PointingHandCursor" in editor_cpp)\n')
t=t.replace('SMART DENOISE P4.5 REFERENCE KNOB MATCH CONTRACT','SMART DENOISE P4.6 VISUAL MATERIAL FIX CONTRACT')
q.write_text(t,encoding='utf-8',newline='\n')

# Changelog
c=ROOT/'CHANGELOG.md'; z=c.read_text(encoding='utf-8')
a='- P4.5 matches the approved knob reference more aggressively: all remaining sphere/gloss cues and glossy marker dots are removed, the flat disc face is enlarged, the cavity/bezel are simplified, the active ring is crisper, and the face uses 144-line radial machining plus a directional satin sheen instead of dome-like specular highlights.\n'
e=a+'- P4.6 removes the translucent sheen-band artifact from every knob, moves 3D depth to the perimeter edge only, and makes Learn Noise visibly clickable with a raised circular rim, drop shadow, hover highlight and pressed-depth treatment.\n'
if a in z and 'P4.6 removes the translucent sheen-band artifact' not in z: z=z.replace(a,e,1)
c.write_text(z,encoding='utf-8',newline='\n')

f=p.read_text(encoding='utf-8')
assert 'sheenBand' not in f and 'directionalSheen' not in f
assert 'edgeOuter' in f and 'edgeGradient' in f and 'knobTopEdge' in f
assert 'buttonShadow' in f and 'buttonRim' in f and 'PointingHandCursor' in f
