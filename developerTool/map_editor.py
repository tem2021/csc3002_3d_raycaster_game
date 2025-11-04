import pygame
import sys
import math
import re  
import copy

MAP_W, MAP_H = 64, 64
CELL_SIZE = 12
TOOLBAR_W = 100
BUTTON_H = 26
WIDTH, HEIGHT = MAP_W * CELL_SIZE + TOOLBAR_W, MAP_H * CELL_SIZE

mapdata = [[1 if x == 0 or x == MAP_W-1 or y == 0 or y == MAP_H-1 else 0 for x in range(MAP_W)] for y in range(MAP_H)]
player_pos = None

undo_stack = []
redo_stack = []
show_help = False  # 新增：控制Help文档显示

def read_map_from_file(filename="data.h"):
    global mapdata, player_pos
    try:
        with open(filename, "r", encoding="utf-8") as f:
            content = f.read()
        # 解析地图尺寸
        mx = int(re.search(r'#define\s+mapX\s+(\d+)', content).group(1))
        my = int(re.search(r'#define\s+mapY\s+(\d+)', content).group(1))
        # 解析地图数据
        map_array_match = re.search(r'static int map\[mapX\]\[mapY\] = \{(.+?)\};', content, re.DOTALL)
        map_content = map_array_match.group(1)
        rows = re.findall(r'\{([^\}]+)\}', map_content)
        mapdata = []
        for row in rows:
            mapdata.append([int(v) for v in row.split(',') if v.strip() != ''])
        # 解析出生点
        match_x = re.search(r'static int initX = (\d+);', content)
        match_y = re.search(r'static int initY = (\d+);', content)
        if match_x and match_y:
            player_pos = (int(match_x.group(1)), int(match_y.group(1)))
        else:
            player_pos = None
        print("parse map data from data.h")
        return True
    except Exception as e:
        print("failed to find the data.h or parse, initialize with the empty map")
        return False

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
        global mode, tool, rect_mode, show_help
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
            elif self.name != "Redo":
                for b in extra_buttons: 
                    if b.name != "Help":
                        b.active = False
                self.active = True

def export_map():
    code = "#define mapX {}\n#define mapY {}\n\nstatic int map[mapX][mapY] = {{\n".format(MAP_W, MAP_H)
    for y in range(MAP_H):
        code += "    {" + ",".join(str(mapdata[y][x]) for x in range(MAP_W)) + "},\n"
    code += "};\n"
    if player_pos:
        code += "static int initX = {};\nstatic int initY = {};\n".format(player_pos[0], player_pos[1])
    else:
        # 自动选择一个空白点作为出生点
        found = False
        for y in range(1, MAP_H-1):
            for x in range(1, MAP_W-1):
                if mapdata[y][x] == 0:
                    code += "static int initX = {};\nstatic int initY = {};\n".format(x, y)
                    found = True
                    break
            if found:
                break
        if not found:
            code += "// No available spawn position\n"
    with open("data.h", "w", encoding="utf-8") as f:
        f.write(code)
    print("Exported to data.h")


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
    Button("Export",       (10, 32+5*(BUTTON_H+6), TOOLBAR_W-20, BUTTON_H), 'extra', -1),
    Button("Undo",         (10, 32+6*(BUTTON_H+6), TOOLBAR_W-20, BUTTON_H), 'extra', -3),
    Button("Redo",         (10, 32+7*(BUTTON_H+6), TOOLBAR_W-20, BUTTON_H), 'extra', -4),
    Button("Quit",         (10, 32+8*(BUTTON_H+6), TOOLBAR_W-20, BUTTON_H), 'extra', -2),
    Button("Help",         (10, 32+9*(BUTTON_H+6), TOOLBAR_W-20, BUTTON_H), 'extra', -5),  # 新增Help按钮
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

def draw_help():
    """绘制帮助文档"""
    help_area = pygame.Rect(TOOLBAR_W, 0, WIDTH - TOOLBAR_W, HEIGHT)
    pygame.draw.rect(screen, (245, 245, 250), help_area)
    
    font_title = pygame.font.SysFont(None, 28, bold=True)
    font_section = pygame.font.SysFont(None, 20, bold=True)
    font_text = pygame.font.SysFont(None, 16)
    
    y_offset = 15
    x_margin = TOOLBAR_W + 15
    line_spacing = 20
    
    # 标题
    title = font_title.render("MAP EDITOR - HELP", True, (40, 40, 80))
    screen.blit(title, (x_margin, y_offset))
    y_offset += 35
    
    # 帮助内容
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
            "Ctrl+E: Export to data.h",
            "Q: Quit editor",
            "H: Toggle this help screen",
        ]),
        ("FILE OPERATIONS", [
            "- On startup: Loads map from 'data.h' if exists",
            "- Export: Saves map to 'data.h' file",
            "- Format: C header with mapX, mapY, and initX/initY",
        ]),
        ("NOTES", [
            "- Border cells (edges) cannot be edited",
            "- Green cell indicates player spawn position",
            "- Undo/Redo supports up to 100 operations",
            "- Map size: 64x64 cells",
        ]),
    ]
    
    for section_title, items in help_content:
        # 章节标题
        section_surf = font_section.render(section_title, True, (60, 60, 120))
        screen.blit(section_surf, (x_margin, y_offset))
        y_offset += line_spacing + 3
        
        # 章节内容
        for item in items:
            text_surf = font_text.render(item, True, (50, 50, 50))
            screen.blit(text_surf, (x_margin + 10, y_offset))
            y_offset += line_spacing - 2
        
        y_offset += 8
    
    # 底部提示
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

    # 计算实际像素坐标（椭圆允许溢出地图）
    sx0 = TOOLBAR_W + left * CELL_SIZE
    sy0 = top * CELL_SIZE
    sx1 = TOOLBAR_W + (right + 1) * CELL_SIZE
    sy1 = (bottom + 1) * CELL_SIZE
    rect = pygame.Rect(sx0, sy0, sx1-sx0, sy1-sy0)
    color_fill = (0,180,255,60)
    color_border = (0,50,255)

    # 椭圆渲染（允许跨越地图范围和下方，超出工具栏部分不显示）
    if rect_mode == FILL_RECT:
        pygame.draw.ellipse(screen, color_fill, rect, 0)
    else:
        pygame.draw.ellipse(screen, color_border, rect, 2)
    # 工具栏区域遮罩，不画按钮（draw_toolbar负责按钮绘制）

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
    global show_help
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
            elif b.name == "Quit":
                quit_editor()
                b.active = True

def handle_button_by_name(name):
    global show_help
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
    global player_pos, line_start, line_end, mode, tool, rect_start, rect_end, ellipse_start, ellipse_end, rect_mode, show_help
    running = True
    dragging_line = False
    dragging_rect = False
    dragging_ellipse = False
    undo_flash_time = 0
    redo_flash_time = 0
    export_flash_time = 0

    if not read_map_from_file("data.h"):
        mapdata = [[1 if x == 0 or x == MAP_W-1 or y == 0 or y == MAP_H-1 else 0 for x in range(MAP_W)] for y in range(MAP_H)]
        player_pos = None

    while running:
        screen.fill((0,0,0))
        
        # 根据show_help状态决定显示地图还是帮助文档
        if show_help:
            draw_help()
        else:
            draw_map()
            if tool == ELLIPSE_TOOL and ellipse_start and ellipse_end:
                preview_ellipse(ellipse_start, ellipse_end, rect_mode)

        # 工具栏和按钮始终最后绘制
        pygame.draw.rect(screen, (230,230,240), (0, 0, TOOLBAR_W, HEIGHT))
        draw_toolbar()

        pygame.display.flip()
        now = pygame.time.get_ticks()
        if undo_flash_time and now > undo_flash_time:
            extra_buttons[1].flash = False
            undo_flash_time = 0
        if redo_flash_time and now > redo_flash_time:
            extra_buttons[2].flash = False
            redo_flash_time = 0
        if export_flash_time and now > export_flash_time:
            extra_buttons[0].flash = False
            export_flash_time = 0

        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                quit_editor()
            elif event.type == pygame.MOUSEBUTTONDOWN:
                if event.pos[0] < TOOLBAR_W:
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
                                b.active = True
                                b.flash = True
                                export_flash_time = now + 120
                            elif b.name == "Help":
                                show_help = not show_help
                                b.active = show_help
                            elif b.name == "Quit":
                                quit_editor()
                                b.active = True
                            break
                    else:
                        handle_button_click(event.pos)
                else:
                    # 只在非Help模式下响应地图编辑
                    if not show_help:
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
                if not show_help:  # 只在非Help模式下响应鼠标移动
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
                if not show_help:  # 只在非Help模式下响应鼠标释放
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
                if event.key == pygame.K_h:  # 新增：H键切换帮助
                    show_help = not show_help
                    extra_buttons[4].active = show_help
                elif event.key == pygame.K_u and not (pygame.key.get_mods() & pygame.KMOD_CTRL):
                    undo()
                    extra_buttons[1].flash = True
                    undo_flash_time = now + 120
                elif event.key == pygame.K_e and (pygame.key.get_mods() & pygame.KMOD_CTRL):
                    export_map()
                    extra_buttons[0].flash = True
                    export_flash_time = now + 120
                elif event.key == pygame.K_e and not (pygame.key.get_mods() & pygame.KMOD_CTRL):
                    handle_button_by_name("Ellipse")
                elif event.key == pygame.K_m:
                    handle_button_by_name("Mode")
                elif event.key == pygame.K_p:
                    handle_button_by_name("Point")
                elif event.key == pygame.K_l:
                    handle_button_by_name("Line")
                elif event.key == pygame.K_r and not (pygame.key.get_mods() & pygame.KMOD_CTRL):
                    handle_button_by_name("Rect")
                elif event.key == pygame.K_r and (pygame.key.get_mods() & pygame.KMOD_CTRL):
                    redo()
                    extra_buttons[2].flash = True
                    redo_flash_time = now + 120
                elif event.key == pygame.K_q:
                    handle_button_by_name("Quit")
                if event.key == pygame.K_z and (pygame.key.get_mods() & pygame.KMOD_CTRL):
                    undo()
                    extra_buttons[1].flash = True
                    undo_flash_time = now + 120
                if event.key == pygame.K_y and (pygame.key.get_mods() & pygame.KMOD_CTRL):
                    redo()
                    extra_buttons[2].flash = True
                    redo_flash_time = now + 120

if __name__ == "__main__":
    main()
