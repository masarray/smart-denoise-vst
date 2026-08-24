from pathlib import Path
p=Path('qa/verify_p4_visual_fidelity.py')
s=p.read_text(encoding='utf-8')
s=s.replace(
'check("P4.5 reference flat machined face", "constexpr int spokeCount = 144" in editor_cpp and "recessGradient" in editor_cpp and "faceGradient" in editor_cpp and "directionalSheen" in editor_cpp)\n',
'check("P4.6 flat machined face retained", "constexpr int spokeCount = 144" in editor_cpp and "recessGradient" in editor_cpp and "faceGradient" in editor_cpp and "edgeGradient" in editor_cpp)\n')
s=s.replace(
'check("P4.5 narrow graphite bezel", "auto bezel =" in editor_cpp and "face.expanded" in editor_cpp and "bezelGradient" not in editor_cpp)\n',
'check("P4.6 raised perimeter replaces flat bezel", "edgeOuter" in editor_cpp and "edgeInner" in editor_cpp and "bezelGradient" not in editor_cpp)\n')
p.write_text(s,encoding='utf-8',newline='\n')
