import pygame
import sys
import math
import re  
import copy
import os
from pathlib import Path

# The WIDTH and HEIGHT of the map are able to change here
MAP_W, MAP_H = 64, 64
CELL_SIZE = 12
TOOLBAR_W = 100
BUTTON_H = 26
WIDTH, HEIGHT = MAP_W * CELL_SIZE + TOOLBAR_W, MAP_H * CELL_SIZE

mapdata = [[1 if x == 0 or x == MAP_W-1 or y == 0 or y == MAP_H-1 else 0 for x in range(MAP_W)] for y in range(MAP_H)]
player_pos = None

undo_stack = []
redo_stack = []
show_help = False
current_map_file = None 
available_maps = []  
map_select_mode = False  
new_map_input_mode = False 
new_map_name = ""  

def scan_map_files(base_path="../include/data/maps"):
    """scan all map files under /include/data/maps"""
    maps = []
    try:
        maps_dir = Path(base_path)
        if maps_dir.exists():
            for file in maps_dir.glob("*.h"):
                maps.append(str(file))
        # if not, try relative path
        if not maps:
            alt_path = Path("include/data/maps")
            if alt_path.exists():
                for file in alt_path.glob("*.h"):
                    maps.append(str(file))
    except Exception as e:
        print(f"Error scanning map files: {e}")
    
    return sorted(maps) if maps else []

def read_map_from_file(filename):
    """parse the map file (level1.h)"""
    global mapdata, player_pos, MAP_W, MAP_H, current_map_file
    try:
        with open(filename, "r", encoding="utf-8") as f:
            content = f.read()
        
        # parse const & namespace
        # match constexpr int LEVEL1_WIDTH = 64;
        width_match = re.search(r'constexpr\s+int\s+\w+_WIDTH\s+=\s+(\d+)', content)
        height_match = re.search(r'constexpr\s+int\s+\w+_HEIGHT\s+=\s+(\d+)', content)
        init_x_match = re.search(r'constexpr\s+int\s+\w+_INIT_X\s+=\s+(\d+)', content)
        init_y_match = re.search(r'constexpr\s+int\s+\w+_INIT_Y\s+=\s+(\d+)', content)
        
        if width_match and height_match:
            MAP_W = int(width_match.group(1))
            MAP_H = int(height_match.group(1))
        
        # parse map data
        # match static int LEVEL1[LEVEL1_HEIGHT][LEVEL1_WIDTH] = { ... };
        map_array_match = re.search(r'static\s+int\s+\w+\[\w+_HEIGHT\]\[\w+_WIDTH\]\s+=\s+\{(.+?)\};', content, re.DOTALL)
        
        if map_array_match:
            map_content = map_array_match.group(1)
            rows = re.findall(r'\{([^\}]+)\}', map_content)
            mapdata = []
            for row in rows:
                mapdata.append([int(v) for v in row.split(',') if v.strip() != ''])
            
            # parse spawn point
            if init_x_match and init_y_match:
                player_pos = (int(init_x_match.group(1)), int(init_y_match.group(1)))
            else:
                player_pos = None
            
            current_map_file = filename
            print(f"Successfully loaded map from {filename}")
            print(f"Map size: {MAP_W}x{MAP_H}, Spawn: {player_pos}")
            return True
        else:
            print(f"Failed to parse map data from {filename}")
            return False
            
    except Exception as e:
        print(f"Error reading {filename}: {e}")
        return False

def create_new_map(map_name, width=64, height=64):
    """creat a new map"""
    global mapdata, player_pos, MAP_W, MAP_H, current_map_file
    
    # ensure map name is valid
    if not map_name or not map_name.replace('_', '').isalnum():
        print("Invalid map name. Use only letters, numbers, and underscores.")
        return False
    
    # ensure the existence of /include/data/maps
    maps_dir = Path("../include/data/maps")
    if not maps_dir.exists():
        maps_dir = Path("include/data/maps")
    
    try:
        maps_dir.mkdir(parents=True, exist_ok=True)
    except Exception as e:
        print(f"Error creating directory: {e}")
        return False
    
    # set the file location
    filename = maps_dir / f"{map_name}.h"
    
    # check whether the file exists
    if filename.exists():
        print(f"Map file {filename} already exists!")
        return False
    
    # set the map size
    MAP_W = width
    MAP_H = height
    
    # create new map (with wall around)
    mapdata = [[1 if x == 0 or x == MAP_W-1 or y == 0 or y == MAP_H-1 else 0 for x in range(MAP_W)] for y in range(MAP_H)]
    
    # set the default spawn point
    player_pos = (2, 2)
    
    # set the current map file
    current_map_file = str(filename)
    
    # save the new map immediately
    export_map()
    
    print(f"Created new map: {filename}")
    return True

def save_state():
    undo_stack.append((copy.deepcopy(mapdata), player_pos))
    if len(undo_stack) > 100:
        undo_stack.pop(0)
    redo_stack.clear()

def undo():
    global mapdata, player_pos
    if undo_stack:
        redo_stack.append((copy.deepcopy(mapdata), player_pos))
        last_map, last_player = undo_stack.pop()
        mapdata = copy.deepcopy(last_map)
        player_pos = last_player

def redo():
    global mapdata, player_pos
    if redo_stack:
        undo_stack.append((copy.deepcopy(mapdata), player_pos))
        next_map, next_player = redo_stack.pop()
        mapdata = copy.deepcopy(next_map)
        player_pos = next_player

pygame.init()
screen = pygame.display.set_mode((WIDTH, HEIGHT))
pygame.display.set_caption("Map Editor")

DRAW_MODE, ERASE_MODE = 0, 1
POINT_TOOL, LINE_TOOL, RECT_TOOL, ELLIPSE_TOOL = 0, 1, 2, 3
FILL_RECT, BORDER_RECT = 0, 1
mode = DRAW_MODE
tool = LINE_TOOL
rect_mode = BORDER_RECT

line_start = None
line_end = None
rect_start = None
rect_end = None
ellipse_start = None
ellipse_end = None

class Button:
    def __init__(self, name, rect, group, value):
        self.name = name
        self.rect = pygame.Rect(rect)
        self.group = group
        self.value = value
        self.active = False
        self.enabled = True
        self.flash = False

    def draw(self, surface):
        color = (180,220,180) if self.active else (200,200,210)
        if self.flash:
            color = (180,220,180)
        if not self.enabled:
            color = (200,200,200)
        pygame.draw.rect(surface, color, self.rect)
        pygame.draw.rect(surface, (115,115,130), self.rect, 2)
        font = pygame.font.SysFont(None, 16)
        text = font.render(self.name, True, (80,80,80) if self.enabled else (175,175,175))
        text_rect = text.get_rect(center=self.rect.center)
        surface.blit(text, text_rect)

    def handle_click(self):
        global mode, tool, rect_mode, show_help, map_select_mode
        if self.group == 'mode':
            mode = DRAW_MODE if mode == ERASE_MODE else ERASE_MODE
            self.name = "Mode(Draw)" if mode == DRAW_MODE else "Mode(Erase)"
            for b in mode_buttons: b.active = False
            self.active = True
        elif self.group == 'tool':
            if self.value == RECT_TOOL:
                if tool != RECT_TOOL:
                    tool = RECT_TOOL
                    for b in tool_buttons: b.active = (b.value == tool)
                    tool_buttons[2].name = "Rect(Border)" if rect_mode == BORDER_RECT else "Rect(Fill)"
                else:
                    rect_mode = FILL_RECT if rect_mode == BORDER_RECT else BORDER_RECT
                    tool_buttons[2].name = "Rect(Border)" if rect_mode == BORDER_RECT else "Rect(Fill)"
                for b in tool_buttons: b.active = (b.value == tool)
            elif self.value == ELLIPSE_TOOL:
                if tool != ELLIPSE_TOOL:
                    tool = ELLIPSE_TOOL
                    for b in tool_buttons: b.active = (b.value == tool)
                    tool_buttons[3].name = "Ellipse(Border)" if rect_mode == BORDER_RECT else "Ellipse(Fill)"
                else:
                    rect_mode = FILL_RECT if rect_mode == BORDER_RECT else BORDER_RECT
                    tool_buttons[3].name = "Ellipse(Border)" if rect_mode == BORDER_RECT else "Ellipse(Fill)"
                for b in tool_buttons: b.active = (b.value == tool)
            else:
                tool = self.value
                for b in tool_buttons: b.active = (b.value == tool)
                tool_buttons[2].name = "Rect(Border)" if rect_mode == BORDER_RECT else "Rect(Fill)"
                tool_buttons[3].name = "Ellipse(Border)" if rect_mode == BORDER_RECT else "Ellipse(Fill)"
        elif self.group == 'extra':
            if self.name == "Help":
                show_help = not show_help
                self.active = show_help
            elif self.name == "Load Map":
                map_select_mode = not map_select_mode
                self.active = map_select_mode
            elif self.name != "Redo":
                for b in extra_buttons: 
                    if b.name != "Help" and b.name != "Load Map":
                        b.active = False
                self.active = True

def export_map():
    """export the current map"""
    global current_map_file
    
    if not current_map_file:
        # if no current map, export the level1.h by default
        current_map_file = "../include/data/maps/level1.h"
        os.makedirs(os.path.dirname(current_map_file), exist_ok=True)
    
    # extract the level name from the filename
    filename = os.path.basename(current_map_file)
    level_name = os.path.splitext(filename)[0].upper()
    
    # create the header 
    code = f"#ifndef {level_name}_H\n"
    code += f"#define {level_name}_H\n\n"
    code += "namespace MapData {\n"
    code += f"    constexpr int {level_name}_WIDTH = {MAP_W};\n"
    code += f"    constexpr int {level_name}_HEIGHT = {MAP_H};\n"
    
    # add the spawn point
    if player_pos:
        code += f"    constexpr int {level_name}_INIT_X = {player_pos[0]};\n"
        code += f"    constexpr int {level_name}_INIT_Y = {player_pos[1]};\n\n"
    else:
        # pick an arbitrary blank point as spawn point
        found = False
        for y in range(1, MAP_H-1):
            for x in range(1, MAP_W-1):
                if mapdata[y][x] == 0:
                    code += f"    constexpr int {level_name}_INIT_X = {x};\n"
                    code += f"    constexpr int {level_name}_INIT_Y = {y};\n\n"
                    found = True
                    break
            if found:
                break
        if not found:
            code += f"    constexpr int {level_name}_INIT_X = 2;\n"
            code += f"    constexpr int {level_name}_INIT_Y = 2;\n\n"
    
    # add the map data
    code += f"    static int {level_name}[{level_name}_HEIGHT][{level_name}_WIDTH] = {{\n"
    for y in range(MAP_H):
        code += "        {" + ",".join(str(mapdata[y][x]) for x in range(MAP_W)) + "},\n"
    code += "    };\n"
    code += "}\n\n"
    code += "#endif\n"
    
    try:
        with open(current_map_file, "w", encoding="utf-8") as f:
            f.write(code)
        print(f"Exported to {current_map_file}")
    except Exception as e:
        print(f"Error exporting map: {e}")

def quit_editor():
    pygame.quit()
    sys.exit()

mode_buttons = [
    Button("Mode(Draw)",   (10, 32, TOOLBAR_W-20, BUTTON_H), 'mode', DRAW_MODE)
]
tool_buttons = [
    Button("Point", (10, 32+1*(BUTTON_H+6), TOOLBAR_W-20, BUTTON_H), 'tool', POINT_TOOL),
    Button("Line",  (10, 32+2*(BUTTON_H+6), TOOLBAR_W-20, BUTTON_H), 'tool', LINE_TOOL),
    Button("Rect(Border)", (10, 32+3*(BUTTON_H+6), TOOLBAR_W-20, BUTTON_H), 'tool', RECT_TOOL),
    Button("Ellipse(Border)", (10, 32+4*(BUTTON_H+6), TOOLBAR_W-20, BUTTON_H), 'tool', ELLIPSE_TOOL),
]
extra_buttons = [
    Button("Load Map",     (10, 32+5*(BUTTON_H+6), TOOLBAR_W-20, BUTTON_H), 'extra', -6),
    Button("Export",       (10, 32+6*(BUTTON_H+6), TOOLBAR_W-20, BUTTON_H), 'extra', -1),
    Button("Undo",         (10, 32+7*(BUTTON_H+6), TOOLBAR_W-20, BUTTON_H), 'extra', -3),
    Button("Redo",         (10, 32+8*(BUTTON_H+6), TOOLBAR_W-20, BUTTON_H), 'extra', -4),
    Button("Quit",         (10, 32+9*(BUTTON_H+6), TOOLBAR_W-20, BUTTON_H), 'extra', -2),
    Button("Help",         (10, 32+10*(BUTTON_H+6), TOOLBAR_W-20, BUTTON_H), 'extra', -5),
]

tool_buttons[1].active = True
tool_buttons[2].name = "Rect(Border)"
tool_buttons[3].name = "Ellipse(Border)"
mode_buttons[0].active = True

def draw_toolbar():
    pygame.draw.rect(screen, (230,230,240), (0,0,TOOLBAR_W,HEIGHT))
    font = pygame.font.SysFont(None, 18)
    title = font.render("Mode & Tools", True, (55,55,80))
    screen.blit(title, (17,7))
    for b in mode_buttons: b.draw(screen)
    for b in tool_buttons: b.draw(screen)
    for b in extra_buttons: b.draw(screen)

def draw_new_map_input():
    """draw new map input window"""
    input_area = pygame.Rect(TOOLBAR_W, 0, WIDTH - TOOLBAR_W, HEIGHT)
    pygame.draw.rect(screen, (245, 245, 250), input_area)
    
    font_title = pygame.font.SysFont(None, 28, bold=True)
    font_text = pygame.font.SysFont(None, 20)
    font_small = pygame.font.SysFont(None, 16)
    
    y_offset = HEIGHT // 2 - 100
    x_margin = TOOLBAR_W + 50
    
    # head
    title = font_title.render("CREATE NEW MAP", True, (40, 40, 80))
    title_rect = title.get_rect(center=(TOOLBAR_W + (WIDTH - TOOLBAR_W) // 2, y_offset))
    screen.blit(title, title_rect)
    y_offset += 60
    
    # hint information
    prompt = font_text.render("Enter map name (e.g., level2, boss_room):", True, (60, 60, 60))
    screen.blit(prompt, (x_margin, y_offset))
    y_offset += 40
    
    # input box
    input_box = pygame.Rect(x_margin, y_offset, 400, 40)
    pygame.draw.rect(screen, (255, 255, 255), input_box)
    pygame.draw.rect(screen, (100, 100, 150), input_box, 2)
    
    # show the input text
    input_text = font_text.render(new_map_name + "_", True, (40, 40, 40))
    screen.blit(input_text, (x_margin + 10, y_offset + 10))
    y_offset += 60
    
    # hint information
    hint1 = font_small.render("Press ENTER to create", True, (100, 100, 100))
    hint2 = font_small.render("Press ESC to cancel", True, (100, 100, 100))
    hint3 = font_small.render("Use only letters, numbers, and underscores", True, (120, 120, 120))
    screen.blit(hint1, (x_margin, y_offset))
    screen.blit(hint2, (x_margin, y_offset + 20))
    screen.blit(hint3, (x_margin, y_offset + 50))

def draw_map_selector():
    """draw the map selector window"""
    selector_area = pygame.Rect(TOOLBAR_W, 0, WIDTH - TOOLBAR_W, HEIGHT)
    pygame.draw.rect(screen, (245, 245, 250), selector_area)
    
    font_title = pygame.font.SysFont(None, 28, bold=True)
    font_text = pygame.font.SysFont(None, 18)
    font_small = pygame.font.SysFont(None, 14)
    
    y_offset = 20
    x_margin = TOOLBAR_W + 20
    
    # head
    title = font_title.render("SELECT MAP TO EDIT", True, (40, 40, 80))
    screen.blit(title, (x_margin, y_offset))
    y_offset += 50
    
    # show current map file
    if current_map_file:
        current_text = font_small.render(f"Current: {os.path.basename(current_map_file)}", True, (100, 100, 100))
        screen.blit(current_text, (x_margin, y_offset))
        y_offset += 30
    else:
        current_text = font_small.render("No map loaded", True, (150, 50, 50))
        screen.blit(current_text, (x_margin, y_offset))
        y_offset += 30
    
    # "create new map" button
    new_map_button_rect = pygame.Rect(x_margin, y_offset, 400, 35)
    mouse_pos = pygame.mouse.get_pos()
    is_hover_new = new_map_button_rect.collidepoint(mouse_pos)
    
    button_color = (150, 220, 150) if is_hover_new else (180, 240, 180)
    pygame.draw.rect(screen, button_color, new_map_button_rect)
    pygame.draw.rect(screen, (80, 150, 80), new_map_button_rect, 3)
    
    new_map_text = font_text.render("+ CREATE NEW MAP", True, (40, 100, 40))
    new_map_text_rect = new_map_text.get_rect(center=new_map_button_rect.center)
    screen.blit(new_map_text, new_map_text_rect)
    
    # store the map for detection
    if not hasattr(draw_map_selector, 'new_map_button'):
        draw_map_selector.new_map_button = new_map_button_rect
    else:
        draw_map_selector.new_map_button = new_map_button_rect
    
    y_offset += 50
    
    # split line
    pygame.draw.line(screen, (150, 150, 150), (x_margin, y_offset), (x_margin + 400, y_offset), 2)
    y_offset += 20
    
    # display the list of available maps
    if available_maps:
        existing_maps_title = font_text.render("Existing Maps:", True, (80, 80, 80))
        screen.blit(existing_maps_title, (x_margin, y_offset))
        y_offset += 30
        
        for i, map_file in enumerate(available_maps):
            map_name = os.path.basename(map_file)
            is_current = (map_file == current_map_file)
            
            button_rect = pygame.Rect(x_margin, y_offset, 400, 30)
            is_hover = button_rect.collidepoint(mouse_pos)
            
            # button's background
            if is_current:
                color = (180, 220, 180)
            elif is_hover:
                color = (220, 220, 230)
            else:
                color = (240, 240, 240)
            
            pygame.draw.rect(screen, color, button_rect)
            pygame.draw.rect(screen, (115, 115, 130), button_rect, 2)
            
            # draw text
            text_color = (40, 100, 40) if is_current else (60, 60, 60)
            text = font_text.render(f"{i+1}. {map_name}", True, text_color)
            screen.blit(text, (x_margin + 10, y_offset + 7))
            
            y_offset += 35
            
            # store the button's information for detection
            if not hasattr(draw_map_selector, 'map_buttons'):
                draw_map_selector.map_buttons = []
            if i >= len(draw_map_selector.map_buttons):
                draw_map_selector.map_buttons.append((button_rect, map_file))
            else:
                draw_map_selector.map_buttons[i] = (button_rect, map_file)
    else:
        no_maps_text = font_text.render("No existing maps found", True, (150, 50, 50))
        screen.blit(no_maps_text, (x_margin, y_offset))
        y_offset += 30
        hint_text = font_small.render("Click 'CREATE NEW MAP' to start", True, (100, 100, 100))
        screen.blit(hint_text, (x_margin, y_offset))
    
    # hint at button
    y_offset = HEIGHT - 60
    hint1 = font_small.render("Click on a map file to load it", True, (100, 100, 100))
    hint2 = font_small.render("Press Ctrl+L or click 'Load Map' to return", True, (100, 100, 100))
    screen.blit(hint1, (x_margin, y_offset))
    screen.blit(hint2, (x_margin, y_offset + 20))

def draw_help():
    """draw help document"""
    help_area = pygame.Rect(TOOLBAR_W, 0, WIDTH - TOOLBAR_W, HEIGHT)
    pygame.draw.rect(screen, (245, 245, 250), help_area)
    
    font_title = pygame.font.SysFont(None, 28, bold=True)
    font_section = pygame.font.SysFont(None, 20, bold=True)
    font_text = pygame.font.SysFont(None, 16)
    
    y_offset = 15
    x_margin = TOOLBAR_W + 15
    line_spacing = 20
    
    # title
    title = font_title.render("MAP EDITOR - HELP", True, (40, 40, 80))
    screen.blit(title, (x_margin, y_offset))
    y_offset += 35
    
    # help content
    help_content = [
        ("BASIC CONTROLS", [
            "Left Click: Draw/Erase based on current mode",
            "Right Click: Set player spawn point (green)",
            "Shift + Drag: Constrain to straight lines/squares",
        ]),
        ("DRAWING MODES", [
            "Draw Mode: Add walls (black cells)",
            "Erase Mode: Remove walls (white cells)",
            "Toggle: Click Mode button or press 'M'",
        ]),
        ("DRAWING TOOLS", [
            "Point (P): Draw/erase single cells",
            "Line (L): Draw/erase straight lines",
            "Rect (R): Draw/erase rectangles",
            "  - Click again to toggle Fill/Border",
            "Ellipse (E): Draw/erase ellipses",
            "  - Click again to toggle Fill/Border",
        ]),
        ("KEYBOARD SHORTCUTS", [
            "M: Toggle Draw/Erase mode",
            "P: Point tool",
            "L: Line tool",
            "R: Rectangle tool",
            "E: Ellipse tool",
            "U or Ctrl+Z: Undo",
            "Ctrl+R or Ctrl+Y: Redo",
            "Ctrl+E: Export to current map file",
            "Ctrl+L: Toggle Load Map screen",
            "Ctrl+N: Create new map",
            "Q: Quit editor",
            "H: Toggle this help screen",
        ]),
        ("FILE OPERATIONS", [
            "- Create: Use 'CREATE NEW MAP' in Load Map screen",
            "- Load: Select from include/data/maps/ directory",
            "- Export: Saves map to current file (levelX.h)",
            "- Format: C++ header with namespace MapData",
        ]),
        ("NOTES", [
            "- Border cells (edges) cannot be edited",
            "- Green cell indicates player spawn position",
            "- Undo/Redo supports up to 100 operations",
            "- Map size: 64x64 cells (configurable)",
        ]),
    ]
    
    for section_title, items in help_content:
        # section title
        section_surf = font_section.render(section_title, True, (60, 60, 120))
        screen.blit(section_surf, (x_margin, y_offset))
        y_offset += line_spacing + 3
        
        # section content
        for item in items:
            text_surf = font_text.render(item, True, (50, 50, 50))
            screen.blit(text_surf, (x_margin + 10, y_offset))
            y_offset += line_spacing - 2
        
        y_offset += 8
    
    # bottom hint
    y_offset = HEIGHT - 25
    footer = font_text.render("Press 'H' or click Help button to return to editor", True, (100, 100, 100))
    footer_rect = footer.get_rect(center=(TOOLBAR_W + (WIDTH - TOOLBAR_W) // 2, y_offset))
    screen.blit(footer, footer_rect)

def draw_map():
    for y in range(MAP_H):
        for x in range(MAP_W):
            px = TOOLBAR_W + x*CELL_SIZE
            py = y*CELL_SIZE
            color = (40, 40, 40) if mapdata[y][x] else (220, 220, 220)
            if player_pos == (x, y):
                color = (0, 255, 0)
            pygame.draw.rect(screen, color, (px, py, CELL_SIZE-1, CELL_SIZE-1))
            pygame.draw.rect(screen, (200,200,200), (px, py, CELL_SIZE-1, CELL_SIZE-1), 1)
    if tool == LINE_TOOL and line_start and line_end:
        x0, y0 = line_start
        x1, y1 = line_end
        sx0 = TOOLBAR_W + x0*CELL_SIZE + CELL_SIZE//2
        sy0 = y0*CELL_SIZE + CELL_SIZE//2
        sx1 = TOOLBAR_W + x1*CELL_SIZE + CELL_SIZE//2
        sy1 = y1*CELL_SIZE + CELL_SIZE//2
        pygame.draw.line(screen, (255,0,0), (sx0, sy0), (sx1, sy1), 2)
    if tool == RECT_TOOL and rect_start and rect_end:
        rx0, ry0 = rect_start
        rx1, ry1 = rect_end
        if rx0 > rx1: rx0, rx1 = rx1, rx0
        if ry0 > ry1: ry0, ry1 = ry1, ry0
        sx0 = TOOLBAR_W + rx0*CELL_SIZE
        sy0 = ry0*CELL_SIZE
        sx1 = TOOLBAR_W + (rx1+1)*CELL_SIZE
        sy1 = (ry1+1)*CELL_SIZE
        if rect_mode == FILL_RECT:
            pygame.draw.rect(screen, (255, 180, 0, 60), (sx0, sy0, sx1-sx0, sy1-sy0), 0)
        else:
            pygame.draw.rect(screen, (255, 50, 0), (sx0, sy0, sx1-sx0, sy1-sy0), 2)

def preview_ellipse(ellipse_start, ellipse_end, rect_mode):
    x0, y0 = ellipse_start
    x1, y1 = ellipse_end
    left = min(x0, x1)
    right = max(x0, x1)
    top = min(y0, y1)
    bottom = max(y0, y1)

    sx0 = TOOLBAR_W + left * CELL_SIZE
    sy0 = top * CELL_SIZE
    sx1 = TOOLBAR_W + (right + 1) * CELL_SIZE
    sy1 = (bottom + 1) * CELL_SIZE
    rect = pygame.Rect(sx0, sy0, sx1-sx0, sy1-sy0)
    color_fill = (0,180,255,60)
    color_border = (0,50,255)

    if rect_mode == FILL_RECT:
        pygame.draw.ellipse(screen, color_fill, rect, 0)
    else:
        pygame.draw.ellipse(screen, color_border, rect, 2)

def bresenham_line(x0, y0, x1, y1):
    cells = []
    dx = abs(x1 - x0)
    dy = abs(y1 - y0)
    x, y = x0, y0
    sx = 1 if x0 < x1 else -1
    sy = 1 if y0 < y1 else -1
    if dx > dy:
        err = dx / 2.0
        while x != x1:
            cells.append((x, y))
            err -= dy
            if err < 0:
                y += sy
                err += dx
            x += sx
    else:
        err = dy / 2.0
        while y != y1:
            cells.append((x, y))
            err -= dx
            if err < 0:
                x += sx
                err += dy
            y += sy
    cells.append((x1, y1))
    return cells

def snap_to_eight_dir(x0, y0, x1, y1):
    dx = x1 - x0
    dy = y1 - y0
    if dx == 0 and dy == 0:
        return (x1, y1)
    directions = [
        (1, 0), (1, 1), (0, 1), (-1, 1),
        (-1, 0), (-1, -1), (0, -1), (1, -1)
    ]
    length = max(abs(dx), abs(dy))
    best_dir = directions[0]
    best_dot = -float("inf")
    for dirx, diry in directions:
        dot = dirx * dx + diry * dy
        mag = math.hypot(dirx, diry) * math.hypot(dx, dy)
        dot_ratio = dot / (mag or 1)
        if dot_ratio > best_dot:
            best_dot = dot_ratio
            best_dir = (dirx, diry)
    steps = max(abs(dx), abs(dy))
    snap_x = x0 + best_dir[0]*steps
    snap_y = y0 + best_dir[1]*steps
    return (snap_x, snap_y)

def handle_button_click(pos):
    global show_help, map_select_mode
    for b in mode_buttons + tool_buttons + extra_buttons:
        if b.rect.collidepoint(pos):
            if b.group in ['mode', 'tool']:
                b.handle_click()
            elif b.name == "Export":
                export_map()
                b.active = True
            elif b.name == "Undo":
                undo()
                b.active = True
            elif b.name == "Redo":
                redo()
            elif b.name == "Help":
                show_help = not show_help
                b.active = show_help
            elif b.name == "Load Map":
                map_select_mode = not map_select_mode
                b.active = map_select_mode
            elif b.name == "Quit":
                quit_editor()
                b.active = True

def handle_button_by_name(name):
    global show_help, map_select_mode
    found = False
    for b in mode_buttons + tool_buttons + extra_buttons:
        if b.name.startswith(name):
            b.handle_click()
            if b.name != "Redo" and not b.name.startswith("Rect") and not b.name.startswith("Ellipse"):
                b.active = True
            found = True
            if b.name == "Export":
                export_map()
            elif b.name == "Undo":
                undo()
            elif b.name == "Redo":
                redo()
            elif b.name == "Help":
                show_help = not show_help
                b.active = show_help
            elif b.name == "Load Map":
                map_select_mode = not map_select_mode
                b.active = map_select_mode
            elif b.name == "Quit":
                quit_editor()
    return found

def handle_draw_erase(x, y, value):
    if x==0 or x==MAP_W-1 or y==0 or y==MAP_H-1:
        return
    if 0 <= x < MAP_W and 0 <= y < MAP_H:
        save_state()
        mapdata[y][x] = value

def set_player_pos(x, y):
    if not (0 <= x < MAP_W and 0 <= y < MAP_H):
        return
    if mapdata[y][x] == 1:
        return
    global player_pos
    save_state()
    player_pos = (x, y)

def handle_line_draw(line_cells, value):
    save_state()
    for px, py in line_cells:
        if px==0 or px==MAP_W-1 or py==0 or py==MAP_H-1:
            continue
        if 0 <= px < MAP_W and 0 <= py < MAP_H:
            mapdata[py][px] = value

def clamp(val, lo, hi):
    return max(lo, min(val, hi))

def handle_rect_draw(x0, y0, x1, y1, fillmode, value):
    save_state()
    x0 = clamp(x0, 0, MAP_W-1)
    x1 = clamp(x1, 0, MAP_W-1)
    y0 = clamp(y0, 0, MAP_H-1)
    y1 = clamp(y1, 0, MAP_H-1)
    if x0 > x1: x0, x1 = x1, x0
    if y0 > y1: y0, y1 = y1, y0
    for yy in range(y0, y1 + 1):
        for xx in range(x0, x1 + 1):
            if xx == 0 or xx == MAP_W-1 or yy == 0 or yy == MAP_H-1:
                continue
            if fillmode == FILL_RECT:
                mapdata[yy][xx] = value
            else:
                if xx == x0 or xx == x1 or yy == y0 or yy == y1:
                    mapdata[yy][xx] = value

def ellipse_points(x0, y0, x1, y1, fillmode):
    pts = []
    left = min(x0, x1)
    right = max(x0, x1)
    top = min(y0, y1)
    bottom = max(y0, y1)
    cx = (left + right) / 2
    cy = (top + bottom) / 2
    rx = max(abs(right - left) / 2, 1)
    ry = max(abs(bottom - top) / 2, 1)
    for y in range(top, bottom+1):
        for x in range(left, right+1):
            dx = (x-cx)/(rx)
            dy = (y-cy)/(ry)
            dist = dx*dx + dy*dy
            if fillmode == FILL_RECT:
                if dist <= 1.0:
                    pts.append((int(x), int(y)))
            else:
                if 0.88 <= dist <= 1.12:
                    pts.append((int(x), int(y)))
    return pts

def handle_ellipse_draw(x0, y0, x1, y1, fillmode, value):
    save_state()
    pts = ellipse_points(x0, y0, x1, y1, fillmode)
    for x, y in pts:
        if x < 0 or x >= MAP_W or y < 0 or y >= MAP_H:
            continue
        if x==0 or x==MAP_W-1 or y==0 or y==MAP_H-1:
            continue
        mapdata[y][x] = value

def main():
    global player_pos, line_start, line_end, mode, tool, rect_start, rect_end, ellipse_start, ellipse_end, rect_mode, show_help, map_select_mode, available_maps, new_map_input_mode, new_map_name
    running = True
    dragging_line = False
    dragging_rect = False
    dragging_ellipse = False
    undo_flash_time = 0
    redo_flash_time = 0
    export_flash_time = 0
    load_flash_time = 0

    # scan available maps
    available_maps = scan_map_files()
    
    # try to load the first available maps or create a new map
    if available_maps:
        read_map_from_file(available_maps[0])
    else:
        print("No map files found. Starting with empty map.")
        print("Hint: Use Ctrl+N or 'CREATE NEW MAP' to create your first map")

    while running:
        screen.fill((0,0,0))
        
        if new_map_input_mode:
            draw_new_map_input()
        elif map_select_mode:
            draw_map_selector()
            draw_map_selector.map_buttons = []  # reset the button list
        elif show_help:
            draw_help()
        else:
            draw_map()
            if tool == ELLIPSE_TOOL and ellipse_start and ellipse_end:
                preview_ellipse(ellipse_start, ellipse_end, rect_mode)

        # show the tool bar
        pygame.draw.rect(screen, (230,230,240), (0, 0, TOOLBAR_W, HEIGHT))
        draw_toolbar()

        pygame.display.flip()
        now = pygame.time.get_ticks()
        
        # deal with the highlight of the buttons
        if undo_flash_time and now > undo_flash_time:
            extra_buttons[2].flash = False
            undo_flash_time = 0
        if redo_flash_time and now > redo_flash_time:
            extra_buttons[3].flash = False
            redo_flash_time = 0
        if export_flash_time and now > export_flash_time:
            extra_buttons[1].flash = False
            export_flash_time = 0
        if load_flash_time and now > load_flash_time:
            extra_buttons[0].flash = False
            load_flash_time = 0

        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                quit_editor()
            elif event.type == pygame.MOUSEBUTTONDOWN:
                if event.pos[0] < TOOLBAR_W:
                    # click on tool bar
                    for idx, b in enumerate(extra_buttons):
                        if b.rect.collidepoint(event.pos):
                            if b.name == "Undo":
                                undo()
                                b.flash = True
                                undo_flash_time = now + 120
                            elif b.name == "Redo":
                                redo()
                                b.flash = True
                                redo_flash_time = now + 120
                            elif b.name == "Export":
                                export_map()
                                b.flash = True
                                export_flash_time = now + 120
                            elif b.name == "Help":
                                show_help = not show_help
                                b.active = show_help
                            elif b.name == "Load Map":
                                map_select_mode = not map_select_mode
                                available_maps = scan_map_files()  # update the list
                                b.active = map_select_mode
                                b.flash = True
                                load_flash_time = now + 120
                            elif b.name == "Quit":
                                quit_editor()
                                b.active = True
                            break
                    else:
                        handle_button_click(event.pos)
                else:
                    # click on the map area
                    if new_map_input_mode:
                        pass  
                    elif map_select_mode:
                        # check whether hit the new map button
                        if hasattr(draw_map_selector, 'new_map_button') and draw_map_selector.new_map_button.collidepoint(event.pos):
                            new_map_input_mode = True
                            new_map_name = ""
                        # check whether click on the available maps
                        elif hasattr(draw_map_selector, 'map_buttons'):
                            for button_rect, map_file in draw_map_selector.map_buttons:
                                if button_rect.collidepoint(event.pos):
                                    if read_map_from_file(map_file):
                                        map_select_mode = False
                                        extra_buttons[0].active = False
                                    break
                    elif not show_help:
                        # edit mode
                        x = (event.pos[0] - TOOLBAR_W) // CELL_SIZE
                        y = event.pos[1] // CELL_SIZE
                        x = clamp(x, 0, MAP_W-1)
                        y = clamp(y, 0, MAP_H-1)
                        if event.button == 1:
                            if tool == POINT_TOOL:
                                value = 1 if mode == DRAW_MODE else 0
                                handle_draw_erase(x, y, value)
                            elif tool == LINE_TOOL:
                                line_start = (x, y)
                                line_end = (x, y)
                                dragging_line = True
                            elif tool == RECT_TOOL:
                                rect_start = (x, y)
                                rect_end = (x, y)
                                dragging_rect = True
                            elif tool == ELLIPSE_TOOL:
                                ellipse_start = (x, y)
                                ellipse_end = (x, y)
                                dragging_ellipse = True
                        elif event.button == 3:
                            set_player_pos(x, y)
            elif event.type == pygame.MOUSEMOTION:
                if not show_help and not map_select_mode and not new_map_input_mode:
                    if tool == LINE_TOOL and dragging_line and line_start:
                        mx = (event.pos[0] - TOOLBAR_W) // CELL_SIZE
                        my = event.pos[1] // CELL_SIZE
                        mx = clamp(mx, 0, MAP_W-1)
                        my = clamp(my, 0, MAP_H-1)
                        mods = pygame.key.get_mods()
                        if mods & pygame.KMOD_SHIFT:
                            mx, my = snap_to_eight_dir(line_start[0], line_start[1], mx, my)
                            mx = clamp(mx, 0, MAP_W-1)
                            my = clamp(my, 0, MAP_H-1)
                        line_end = (mx, my)
                    elif tool == RECT_TOOL and dragging_rect and rect_start:
                        mx = (event.pos[0] - TOOLBAR_W) // CELL_SIZE
                        my = event.pos[1] // CELL_SIZE
                        mx = clamp(mx, 0, MAP_W-1)
                        my = clamp(my, 0, MAP_H-1)
                        mods = pygame.key.get_mods()
                        if mods & pygame.KMOD_SHIFT:
                            dx = mx - rect_start[0]
                            dy = my - rect_start[1]
                            length = max(abs(dx), abs(dy))
                            if dx >= 0:
                                mx = rect_start[0] + length
                            else:
                                mx = rect_start[0] - length
                            if dy >= 0:
                                my = rect_start[1] + length
                            else:
                                my = rect_start[1] - length
                            mx = clamp(mx, 0, MAP_W-1)
                            my = clamp(my, 0, MAP_H-1)
                        rect_end = (mx, my)
                    elif tool == ELLIPSE_TOOL and dragging_ellipse and ellipse_start:
                        mx = (event.pos[0] - TOOLBAR_W) // CELL_SIZE
                        my = event.pos[1] // CELL_SIZE
                        mods = pygame.key.get_mods()
                        if mods & pygame.KMOD_SHIFT:
                            dx = mx - ellipse_start[0]
                            dy = my - ellipse_start[1]
                            length = max(abs(dx), abs(dy))
                            if dx >= 0:
                                mx = ellipse_start[0] + length
                            else:
                                mx = ellipse_start[0] - length
                            if dy >= 0:
                                my = ellipse_start[1] + length
                            else:
                                my = ellipse_start[1] - length
                        ellipse_end = (mx, my)
            elif event.type == pygame.MOUSEBUTTONUP:
                if not show_help and not map_select_mode and not new_map_input_mode:
                    if event.button == 1 and dragging_line and line_start and line_end:
                        value = 1 if mode == DRAW_MODE else 0
                        line_cells = bresenham_line(line_start[0], line_start[1], line_end[0], line_end[1])
                        handle_line_draw(line_cells, value)
                        line_start, line_end = None, None
                        dragging_line = False
                    elif event.button == 1 and dragging_rect and rect_start and rect_end:
                        value = 1 if mode == DRAW_MODE else 0
                        rx0, ry0 = rect_start
                        rx1, ry1 = rect_end
                        rx1 = clamp(rx1, 0, MAP_W-1)
                        ry1 = clamp(ry1, 0, MAP_H-1)
                        handle_rect_draw(rx0, ry0, rx1, ry1, rect_mode, value)
                        rect_start, rect_end = None, None
                        dragging_rect = False
                    elif event.button == 1 and dragging_ellipse and ellipse_start and ellipse_end:
                        value = 1 if mode == DRAW_MODE else 0
                        ex0, ey0 = ellipse_start
                        ex1, ey1 = ellipse_end
                        handle_ellipse_draw(ex0, ey0, ex1, ey1, rect_mode, value)
                        ellipse_start, ellipse_end = None, None
                        dragging_ellipse = False
            elif event.type == pygame.KEYDOWN:
                if new_map_input_mode:
                    # keyboard input for new map mode
                    if event.key == pygame.K_RETURN:
                        if new_map_name:
                            if create_new_map(new_map_name):
                                new_map_input_mode = False
                                map_select_mode = False
                                extra_buttons[0].active = False
                                available_maps = scan_map_files()  # update the map list
                            new_map_name = ""
                    elif event.key == pygame.K_ESCAPE:
                        new_map_input_mode = False
                        new_map_name = ""
                    elif event.key == pygame.K_BACKSPACE:
                        new_map_name = new_map_name[:-1]
                    elif event.unicode.isalnum() or event.unicode == '_':
                        if len(new_map_name) < 30:  # limit the map name
                            new_map_name += event.unicode
                else:
                    # keyboard for normal mode
                    if event.key == pygame.K_n and (pygame.key.get_mods() & pygame.KMOD_CTRL):
                        # Ctrl+N to create new map
                        new_map_input_mode = True
                        new_map_name = ""
                    elif event.key == pygame.K_h:
                        show_help = not show_help
                        extra_buttons[5].active = show_help
                    elif event.key == pygame.K_l and (pygame.key.get_mods() & pygame.KMOD_CTRL):
                        map_select_mode = not map_select_mode
                        available_maps = scan_map_files()  
                        extra_buttons[0].active = map_select_mode
                        extra_buttons[0].flash = True
                        load_flash_time = now + 120
                    elif event.key == pygame.K_l and not (pygame.key.get_mods() & pygame.KMOD_CTRL):
                        if not map_select_mode:
                            handle_button_by_name("Line")
                    elif event.key == pygame.K_u and not (pygame.key.get_mods() & pygame.KMOD_CTRL):
                        undo()
                        extra_buttons[2].flash = True
                        undo_flash_time = now + 120
                    elif event.key == pygame.K_e and (pygame.key.get_mods() & pygame.KMOD_CTRL):
                        export_map()
                        extra_buttons[1].flash = True
                        export_flash_time = now + 120
                    elif event.key == pygame.K_e and not (pygame.key.get_mods() & pygame.KMOD_CTRL):
                        handle_button_by_name("Ellipse")
                    elif event.key == pygame.K_m:
                        handle_button_by_name("Mode")
                    elif event.key == pygame.K_p:
                        handle_button_by_name("Point")
                    elif event.key == pygame.K_r and not (pygame.key.get_mods() & pygame.KMOD_CTRL):
                        handle_button_by_name("Rect")
                    elif event.key == pygame.K_r and (pygame.key.get_mods() & pygame.KMOD_CTRL):
                        redo()
                        extra_buttons[3].flash = True
                        redo_flash_time = now + 120
                    elif event.key == pygame.K_q:
                        handle_button_by_name("Quit")
                    elif event.key == pygame.K_z and (pygame.key.get_mods() & pygame.KMOD_CTRL):
                        undo()
                        extra_buttons[2].flash = True
                        undo_flash_time = now + 120
                    elif event.key == pygame.K_y and (pygame.key.get_mods() & pygame.KMOD_CTRL):
                        redo()
                        extra_buttons[3].flash = True
                        redo_flash_time = now + 120

if __name__ == "__main__":
    main()
