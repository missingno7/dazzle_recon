"""List every declared POD application global for the full-state test oracle."""
from pathlib import Path
import re
import sys

header, output = map(Path, sys.argv[1:])
source = re.sub(r"/\*.*?\*/", "", header.read_text(), flags=re.S)
names = []
for declaration in re.findall(r"extern\s+([^;]+);", source):
    if "(" in declaration or "*" in declaration:
        continue
    match = re.fullmatch(r"(?:volatile\s+)?(?:u?int(?:8|16|32|64)_t|float|double|char|Point16|"
                         r"ConeDirections|StarTracePoint|TrigPair|PaletteMapSpec|transition_particle_t|"
                         r"struct text_info|struct file_field|struct dos_find_block|enum video_backend_kind)\s+(.+)",
                         declaration, re.S)
    if match:
        for item in match[1].split(","):
            name = re.match(r"\s*(\w+)", item)[1]
            if name not in names:
                names.append(name)
output.write_text("/* Generated from native/include/dazzle.h; test oracle only. */\n" +
                  "".join(f"FIELD({name});\n" for name in names))
print(f"Native state oracle: {len(names)} POD globals")
