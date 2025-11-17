# Developer Tools - Usage Guide

## Setup

Install dependencies:
```bash
pip install pygame Pillow numpy
```

Launch tools:
```bash
python3 map_editor.py      # Map editor
python3 texture_editor.py  # Texture editor
```

---

## Map Editor

### Controls
- **Left Click**: Draw/Erase walls
- **Right Click**: Set player spawn point (green cell)
- **Shift + Drag**: Constrain to straight lines/squares

### Keyboard Shortcuts
- `M`: Toggle Draw/Erase mode
- `P`: Point tool
- `L`: Line tool
- `R`: Rectangle tool (click again to toggle Fill/Border)
- `E`: Ellipse tool (click again to toggle Fill/Border)
- `Ctrl+N`: Create new map
- `Ctrl+L`: Load map
- `Ctrl+E`: Export map
- `Ctrl+Z` or `U`: Undo
- `Ctrl+Y`: Redo
- `H`: Help
- `Q`: Quit

### Workflow
1. Create new map with `Ctrl+N`
2. Draw walls using tools
3. Right-click to set player spawn
4. Export with `Ctrl+E` → saves to `include/data/maps/`

---

## Texture Editor

### Color Selection (Right Panel)
- **Color Palette**: 192 colors in 16 rows × 12 columns
  - 15 hue rows with light-to-dark gradients
  - 1 grayscale row (white to black)
- **RGB Sliders**: Drag to adjust R/G/B channels
- **RGB Input**: Type exact values (0-255)
- **Eyedropper**: Pick colors from canvas

### Controls
- **Left Click**: Draw with selected tool
- **Shift + Drag**: Constrain lines/shapes

### Keyboard Shortcuts
- `M`: Toggle Draw/Erase mode
- `P`: Point tool
- `L`: Line tool
- `R`: Rectangle tool (click again to toggle Fill/Border)
- `E`: Ellipse tool (click again to toggle Fill/Border)
- `I`: Eyedropper (pick color from canvas)
- `Ctrl+N`: Create new texture
- `Ctrl+L`: Load texture
- `Ctrl+I`: Import PNG (custom file browser)
- `Ctrl+E`: Export as .h (C++ header)
- `Ctrl+P`: Export as PNG
- `Ctrl+Z` or `U`: Undo
- `Ctrl+Y`: Redo
- `H`: Help
- `Q`: Quit

### Workflow

**Create New Texture:**
1. `Ctrl+N` → Enter name
2. Select color and draw
3. `Ctrl+E` to export → saves to `include/data/textures/`

**Import PNG:**
1. `Ctrl+I` → Browse files (pygame dialog)
2. Select PNG file
3. Auto-scales to 64×64, centered with white background
4. Edit and export with `Ctrl+E`

**Edit Existing:**
1. `Ctrl+L` → Select texture
2. Edit with tools
3. `Ctrl+E` to save

---

## Using Assets in C++

### Maps
```cpp
#include "data/maps/level1.h"

bool isWall = MapData::LEVEL1_MAP[y][x];
int spawnX = MapData::LEVEL1_PLAYER_X;
int spawnY = MapData::LEVEL1_PLAYER_Y;
```

### Textures
```cpp
#include "data/textures/brick.h"

unsigned char r = BRICK_DATA[y][x][0];  // Red
unsigned char g = BRICK_DATA[y][x][1];  // Green
unsigned char b = BRICK_DATA[y][x][2];  // Blue
```

---

## File Structure
```
developerTool/
├── map_editor.py              # Map editor
├── texture_editor.py          # Texture editor
└── README.md                  # This file

include/data/
├── maps/                      # Map files (.h)
│   ├── level1.h
│   └── ...
└── textures/                  # Texture files (.h, .png)
    ├── brick.h
    ├── brick.png
    └── ...
```

---

## Tips
- **Map Editor**: Set player spawn before exporting; border cells cannot be edited
- **Texture Editor**: Import PNG auto-fills white background for non-square images; press `I` to quickly pick colors
- Both editors support Undo/Redo (up to 100 operations)
- All dialogs use pygame (no external windows)
- Export frequently to avoid losing work
