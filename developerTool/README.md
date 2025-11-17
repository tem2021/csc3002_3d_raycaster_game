# Developer Tools - Usage Guide

## Setup

Install dependencies:
```bash
pip install pygame Pillow numpy
```

Launch tools:
```bash
python3 map_editor.py      # Map editor (design game levels)
python3 texture_editor.py  # Texture editor (create wall textures)
```

---

## Texture Editor

Create 64×64 pixel art textures for walls.

### Drawing Tools
- **Point** (`P`): Draw individual pixels
- **Line** (`L`): Draw straight lines
- **Rectangle** (`R`): Draw rectangles (press `R` again to toggle Fill/Border)
- **Ellipse** (`E`): Draw ellipses (press `E` again to toggle Fill/Border)
- **Mode** (`M`): Toggle Draw/Erase mode
- **Shift + Drag**: Constrain lines/shapes to straight or square

### Color Selection (Right Panel)
- **Color Palette**: 192 pre-defined colors in gradient rows
- **RGB Sliders**: Drag to adjust R/G/B channels (0-255)
- **RGB Input**: Click numbers to type exact values
- **Eyedropper** (`I` or `Ctrl+K`): Pick colors from canvas

### Keyboard Shortcuts
- `Ctrl+N`: Create new texture
- `Ctrl+L`: Load existing texture (.h file)
- `Ctrl+I`: Import PNG image (auto-scales to 64×64)
- `Ctrl+E`: Export (saves both .h and .png files)
- `Ctrl+Z` or `U`: Undo
- `Ctrl+Y` or `Ctrl+R`: Redo
- `H`: Show help
- `Q`: Quit

### Workflow
1. **New texture**: `Ctrl+N` → Enter name → Draw → `Ctrl+E` to export
2. **Import PNG**: `Ctrl+I` → Select image → Edit → `Ctrl+E` to export
3. **Edit existing**: `Ctrl+L` → Select texture → Edit → `Ctrl+E` to save

Exported files go to `include/data/textures/` as both `.h` (C++ header) and `.png` (preview).

---

## Map Editor

Create game levels with walls and player spawn points. Supports texture mapping.

### Drawing Tools
- **Point** (`P`): Draw/erase individual cells
- **Line** (`L`): Draw straight lines
- **Rectangle** (`R`): Draw rectangles (press `R` again to toggle Fill/Border)
- **Ellipse** (`E`): Draw ellipses (press `E` again to toggle Fill/Border)
- **Mode** (`M`): Toggle Draw/Erase mode
- **Shift + Drag**: Constrain to straight lines/squares

### Map Controls
- **Left Click**: Draw/Erase walls (uses selected texture)
- **Right Click**: Set player spawn point (green cell)
- **Texture Panel** (Right Side): Select which wall texture to paint

### Keyboard Shortcuts
- `Ctrl+N`: Create new map
- `Ctrl+L`: Load existing map
- `Ctrl+E`: Export map to .h file
- `Ctrl+Z` or `U`: Undo
- `Ctrl+Y` or `Ctrl+R`: Redo
- `H`: Show help
- `Q`: Quit

### Workflow
1. **New map**: `Ctrl+N` → Enter name and dimensions
2. **Select texture**: Click texture in right panel
3. **Draw walls**: Left-click to paint with selected texture
4. **Set spawn**: Right-click to place player start position
5. **Export**: `Ctrl+E` → Saves to `include/data/maps/`

Map displays texture IDs in each cell. Empty cells = 0.

---

## Texture Configuration

**File**: `include/core/texture_mapping.txt`

Maps texture IDs to files. Both Python tools and C++ code read this file.

Format: `ID:filename:display_name`

Example:
```
0:null:Empty
1:brick.h:Brick
2:wood.h:Wood
100:custom.h:Custom
```

- ID 0 = empty space (no wall)
- IDs 1-255 = wall textures
- Add new textures by assigning next available ID

---

## Using Assets in C++

### Load Map
```cpp
#include "data/maps/level1.h"

int wallType = MapData::LEVEL1_MAP[y][x];  // 0 = empty, 1-255 = texture ID
int spawnX = MapData::LEVEL1_PLAYER_X;
int spawnY = MapData::LEVEL1_PLAYER_Y;
```

### Load Texture
```cpp
#include "data/textures/brick.h"

unsigned char r = BRICK_DATA[y][x][0];  // Red channel
unsigned char g = BRICK_DATA[y][x][1];  // Green channel
unsigned char b = BRICK_DATA[y][x][2];  // Blue channel
```

### Render Wall with Texture
```cpp
int textureID = MapData::LEVEL1_MAP[y][x];
if (textureID > 0) {
    // Load texture based on texture_mapping.txt
    // Use TEXTURE_DATA[y][x] to get RGB values
}
```

---

## File Structure
```
developerTool/
├── map_editor.py              # Map editor tool
├── texture_editor.py          # Texture editor tool
└── README.md                  # This file

include/
├── core/
│   └── texture_mapping.txt    # Texture ID configuration
└── data/
    ├── maps/                  # Exported map files (.h)
    │   └── level1.h
    └── textures/              # Exported texture files (.h + .png)
        ├── brick.h
        ├── brick.png
        └── ...
```

---

## Tips
- Both editors support unlimited undo/redo
- PNG imports are auto-centered on white background
- Map editor shows texture IDs in cells for clarity
- Border cells in maps are always walls (cannot be edited)
- All file dialogs use pygame (no external windows)
