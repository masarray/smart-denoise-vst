from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
cpp = ROOT / 'Source/Plugin/PluginEditor.cpp'
p5 = ROOT / 'qa/verify_p5_ux_integrity.py'
concept = ROOT / 'qa/verify_concept_c_ui.py'
changelog = ROOT / 'CHANGELOG.md'

s = cpp.read_text(encoding='utf-8')
old = '''    setSize (\n        940,\n        shouldShow ? 660 : 540);\n'''
new = '''    setSize (\n        940,\n        shouldShow ? 704 : 540);\n'''
if old not in s: raise RuntimeError('advanced window height anchor missing')
s = s.replace(old, new, 1)

old = '''        advancedBounds =\n            { 15, 538, 910, 106 };\n'''
new = '''        advancedBounds =\n            { 15, 538, 910, 150 };\n'''
if old not in s: raise RuntimeError('advanced bounds anchor missing')
s = s.replace(old, new, 1)

old = '''    g.drawText ("DSP CEILING", left.removeFromTop (16), juce::Justification::centredLeft);\n    g.setColour (ui::textSecondary);\n    g.setFont (juce::FontOptions (10.0f));\n    g.drawText ("24 dB max reduction", left.removeFromTop (18), juce::Justification::centredLeft);\n'''
new = '''    g.setColour (ui::textSecondary);\n    g.setFont (juce::FontOptions (9.4f));\n    g.drawText (\n        "DSP ceiling  ·  24 dB max reduction",\n        left.removeFromTop (17),\n        juce::Justification::centredLeft);\n'''
if old not in s: raise RuntimeError('DSP ceiling copy anchor missing')
s = s.replace(old, new, 1)
cpp.write_text(s, encoding='utf-8', newline='\n')

s = p5.read_text(encoding='utf-8')
s = s.replace('"shouldShow ? 660 : 540" in cpp and "{ 15, 538, 910, 106 }" in cpp', '"shouldShow ? 704 : 540" in cpp and "{ 15, 538, 910, 150 }" in cpp')
s = s.replace('check("Advanced drawer compact",', 'check("P5.1 Advanced drawer has collision-safe geometry",')
p5.write_text(s, encoding='utf-8', newline='\n')

s = concept.read_text(encoding='utf-8')
s = s.replace('"shouldShow ? 660 : 540"', '"shouldShow ? 704 : 540"')
concept.write_text(s, encoding='utf-8', newline='\n')

s = changelog.read_text(encoding='utf-8')
anchor = '- P5 removes deceptive A/B/Undo/Redo/Help chrome, removes the false profile dropdown affordance, adds professional tooltips and double-click resets, replaces redundant input/output history with real spectral-reduction activity, and compresses Advanced into one real control plus concise profile diagnostics.\n'
insert = anchor + '- P5.1 gives the Advanced drawer collision-safe vertical space and compresses DSP ceiling copy into one line so all real diagnostics remain visible without overlapping.\n'
if anchor in s and 'P5.1 gives the Advanced drawer collision-safe vertical space' not in s:
    s = s.replace(anchor, insert, 1)
changelog.write_text(s, encoding='utf-8', newline='\n')

final = cpp.read_text(encoding='utf-8')
assert 'shouldShow ? 704 : 540' in final
assert '{ 15, 538, 910, 150 }' in final
assert 'DSP ceiling  ·  24 dB max reduction' in final
