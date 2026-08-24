from pathlib import Path
p=Path('Source/Plugin/PluginEditor.cpp')
s=p.read_text(encoding='utf-8')
old='''        // P4.6: no translucent sheen patch is painted over the machined face.\n    // Very thin face edge, not a 3D bevel.\n'''
new='''        // P4.6: no translucent sheen patch is painted over the machined face.\n    }\n\n    // Very thin face edge, not a 3D bevel.\n'''
if old not in s:
    raise RuntimeError('brace anchor not found')
p.write_text(s.replace(old,new,1),encoding='utf-8',newline='\n')
