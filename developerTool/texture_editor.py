"""
Texture Editor - RGB Texture Editor for CSC3002 Raycaster Game

Features:
- 64x64 RGB texture editor (non-grayscale)
- Drawing tools: Point, Line, Rectangle, Ellipse (filled/border)
- Advanced color selection: RGB sliders, color palette, eyedropper
- Import PNG and convert to pixel art with auto-scaling
- Export to .h file and PNG
- Similar UI style to map_editor.py
"""

import pygame
import sys
import math
import copy
import os
import re
from pathlib import Path

try:
    from PIL import Image
    import numpy as np
except ImportError:
    print("Error: Need Pillow and numpy")
    print("Run: pip install Pillow numpy")
    sys.exit(1)

# Try to import tkinter for file dialogs (cross-platform)
try:
    import tkinter as tk
    from tkinter import filedialog, messagebox
    HAS_TKINTER = True
except ImportError:
    HAS_TKINTER = False
    print("Warning: tkinter not available. Import PNG will use console input.")
    print("To enable file dialog: sudo apt-get install python3-tk (Linux)")

# Fixed texture size to 64x64
TEXTURE_SIZE = 64
PIXEL_SIZE = 14  # Larger pixels for better visibility
TOOLBAR_W = 220  # Wider toolbar for controls
BUTTON_H = 26
COLOR_PANEL_W = 250  # Right panel for color controls
WIDTH = TEXTURE_SIZE * PIXEL_SIZE + TOOLBAR_W + COLOR_PANEL_W  # Add space for color panel
HEIGHT = TEXTURE_SIZE * PIXEL_SIZE

# State
texture_data = np.full((TEXTURE_SIZE, TEXTURE_SIZE, 3), [255, 255, 255], dtype=np.uint8)
texture_name = ""  # Empty means unnamed
current_texture_file = None
available_textures = []

undo_stack = []
redo_stack = []
show_help = False
load_screen = False
create_new_mode = False
input_text = ""
eyedropper_mode = False  # Eyedropper tool active
color_picker_mode = False  # Old color picker mode (can be removed but keeping for compatibility)

# Color state
draw_color = [255, 0, 0]  # RGB - Red by default
rgb_input_mode = False  # RGB input mode
rgb_input_active_field = None  # Which RGB field is active (0=R, 1=G, 2=B, None=inactive)
rgb_input_texts = ["255", "0", "0"]  # Text for each RGB input

# Color sliders state
dragging_slider = None  # Which slider is being dragged (0=R, 1=G, 2=B)

# Enhanced color palette with gradients (light to dark, left to right different hues)
def generate_color_palette():
    """Generate a professional color palette with gradients - 15 rows with light-to-dark per row"""
    palette = []
    
    # 15 base colors (one per row)
    base_colors = [
        (255, 0, 0),      # Red
        (255, 128, 0),    # Orange
        (255, 200, 0),    # Yellow-Orange
        (255, 255, 0),    # Yellow
        (128, 255, 0),    # Yellow-Green
        (0, 255, 0),      # Green
        (0, 255, 128),    # Green-Cyan
        (0, 255, 255),    # Cyan
        (0, 128, 255),    # Sky Blue
        (0, 64, 255),     # Blue-Purple
        (0, 0, 255),      # Blue
        (128, 0, 255),    # Purple
        (192, 0, 255),    # Purple-Magenta
        (255, 0, 255),    # Magenta
        (255, 0, 128),    # Pink
    ]
    
    # For each base color (row), create gradient from light to dark (columns)
    cols_per_row = 12
    
    for base_r, base_g, base_b in base_colors:
        for col in range(cols_per_row):
            # Gradient from white (left) -> pure color (middle) -> black (right)
            # col 0: very light (almost white)
            # col 5-6: pure color
            # col 11: very dark (almost black)
            
            if col < cols_per_row // 2:
                # Left half: mix with white
                mix = col / (cols_per_row // 2)  # 0 to 1
                r = int(base_r * mix + 255 * (1 - mix))
                g = int(base_g * mix + 255 * (1 - mix))
                b = int(base_b * mix + 255 * (1 - mix))
            else:
                # Right half: mix with black
                mix = (cols_per_row - 1 - col) / (cols_per_row // 2)  # 1 to 0
                r = int(base_r * mix)
                g = int(base_g * mix)
                b = int(base_b * mix)
            
            palette.append((r, g, b))
    
    # Add final row: grayscale gradient from white to black
    for col in range(cols_per_row):
        gray_val = int(255 - (col / (cols_per_row - 1)) * 255)
        palette.append((gray_val, gray_val, gray_val))
    
    # Add one more grayscale row for better coverage
    for col in range(cols_per_row):
        gray_val = int(255 - (col / (cols_per_row - 1)) * 255)
        palette.append((gray_val, gray_val, gray_val))
    
    return palette

COLOR_PALETTE = generate_color_palette()


def save_state():
    """Save state for undo"""
    undo_stack.append(copy.deepcopy(texture_data))
    if len(undo_stack) > 100:
        undo_stack.pop(0)
    redo_stack.clear()

def undo():
    """Undo"""
    global texture_data
    if undo_stack:
        redo_stack.append(copy.deepcopy(texture_data))
        texture_data = copy.deepcopy(undo_stack.pop())

def redo():
    """Redo"""
    global texture_data
    if redo_stack:
        undo_stack.append(copy.deepcopy(texture_data))
        texture_data = copy.deepcopy(redo_stack.pop())


def scan_texture_files(base_path="../include/data/textures"):
    """Scan texture files"""
    textures = []
    try:
        for path_str in [base_path, "include/data/textures"]:
            path = Path(path_str)
            if path.exists():
                for file in path.glob("*.h"):
                    textures.append(str(file))
                if textures:
                    break
    except Exception as e:
        print(f"Error scanning texture files: {e}")
    return sorted(textures) if textures else []

def create_new_texture(tex_name):
    """Create new texture"""
    global texture_data, texture_name, current_texture_file
    
    if not tex_name or not tex_name.replace('_', '').isalnum():
        print("Invalid texture name. Use only letters, numbers, and underscores.")
        return False
    
    textures_dir = Path("../include/data/textures")
    if not textures_dir.exists():
        textures_dir = Path("include/data/textures")
    
    try:
        textures_dir.mkdir(parents=True, exist_ok=True)
    except Exception as e:
        print(f"Error creating directory: {e}")
        return False
    
    filename = textures_dir / f"{tex_name}.h"
    
    if filename.exists():
        print(f"Texture file {filename} already exists!")
        return False
    
    # Create new texture (white background)
    texture_data = np.full((TEXTURE_SIZE, TEXTURE_SIZE, 3), [255, 255, 255], dtype=np.uint8)
    
    texture_name = tex_name
    current_texture_file = str(filename)
    
    undo_stack.clear()
    redo_stack.clear()
    
    # Save immediately
    export_texture()
    
    print(f"Created new texture: {filename}")
    return True


def load_rgb_texture(filepath):
    """Load RGB texture from .h file"""
    global texture_data, texture_name, current_texture_file
    
    try:
        with open(filepath, 'r') as f:
            content = f.read()
        
        texture_name = Path(filepath).stem
        current_texture_file = filepath
        
        # Find data array
        pattern = r'DATA\[\d+\]\[\d+\]\[3\]\s*=\s*\{(.*?)\};'
        match = re.search(pattern, content, re.DOTALL)
        
        if not match:
            print(f"Error: Could not find RGB texture data in {filepath}")
            return False
        
        data_str = match.group(1)
        
        # Parse RGB data
        rows = []
        for line in data_str.split('\n'):
            line = line.strip()
            if not line or line.startswith('//'):
                continue
            
            pixel_pattern = r'\{(\d+),\s*(\d+),\s*(\d+)\}'
            pixels = []
            for pixel_match in re.finditer(pixel_pattern, line):
                r = int(pixel_match.group(1))
                g = int(pixel_match.group(2))
                b = int(pixel_match.group(3))
                pixels.append([r, g, b])
            
            if pixels:
                if len(pixels) == TEXTURE_SIZE:
                    rows.append(pixels)
                elif len(pixels) < TEXTURE_SIZE and rows and len(rows[-1]) < TEXTURE_SIZE:
                    rows[-1].extend(pixels)
                else:
                    rows.append(pixels)
        
        valid_rows = [row for row in rows if len(row) == TEXTURE_SIZE]
        
        if len(valid_rows) != TEXTURE_SIZE:
            print(f"Error: Expected {TEXTURE_SIZE} rows, got {len(valid_rows)}")
            return False
        
        for y in range(TEXTURE_SIZE):
            for x in range(TEXTURE_SIZE):
                texture_data[y, x] = valid_rows[y][x]
        
        print(f"✓ Loaded RGB texture: {texture_name} from {filepath}")
        undo_stack.clear()
        redo_stack.clear()
        return True
        
    except Exception as e:
        print(f"Error loading texture: {e}")
        import traceback
        traceback.print_exc()
        return False


def export_texture():
    """Export texture as C++ header file (RGB format)"""
    global current_texture_file, texture_name
    
    if not texture_name or not current_texture_file:
        print("Error: No texture name set. Please create a new texture first (Ctrl+N)")
        return False
    
    filename = os.path.basename(current_texture_file)
    tex_name = os.path.splitext(filename)[0].upper()
    
    code = f"#ifndef {tex_name}_TEXTURE_H\n"
    code += f"#define {tex_name}_TEXTURE_H\n\n"
    code += f"// Texture name: {texture_name}\n"
    code += f"// Format: RGB (Red, Green, Blue)\n"
    code += f"// Size: {TEXTURE_SIZE}x{TEXTURE_SIZE}\n\n"
    code += f"constexpr int {tex_name}_SIZE = {TEXTURE_SIZE};\n\n"
    
    code += f"constexpr unsigned char {tex_name}_DATA[{TEXTURE_SIZE}][{TEXTURE_SIZE}][3] = {{\n"
    for y in range(TEXTURE_SIZE):
        code += "    {"
        for x in range(TEXTURE_SIZE):
            r, g, b = texture_data[y, x]
            code += f"{{{r:3d},{g:3d},{b:3d}}}"
            if x < TEXTURE_SIZE - 1:
                code += ","
        code += "},\n"
    code += "};\n\n"
    code += f"#endif // {tex_name}_TEXTURE_H\n"
    
    try:
        with open(current_texture_file, 'w', encoding='utf-8') as f:
            f.write(code)
        print(f"✓ Exported to {current_texture_file}")
        return True
    except Exception as e:
        print(f"Error exporting texture: {e}")
        return False

def export_png():
    """Export texture as PNG image"""
    global texture_data, texture_name
    
    if not texture_name:
        print("Error: No texture name set. Please create a new texture first (Ctrl+N)")
        return False
    
    textures_dir = Path("../include/data/textures")
    if not textures_dir.exists():
        textures_dir = Path("include/data/textures")
    
    try:
        textures_dir.mkdir(parents=True, exist_ok=True)
    except Exception as e:
        print(f"Error creating directory: {e}")
        return False
    
    png_filename = textures_dir / f"{texture_name}.png"
    
    try:
        img = Image.fromarray(texture_data, mode='RGB')
        img.save(png_filename, 'PNG')
        print(f"✓ Exported PNG to {png_filename}")
        return True
    except Exception as e:
        print(f"Error exporting PNG: {e}")
        return False

def confirm_save_dialog():
    """Ask user to save current work before importing - using pygame dialog"""
    if not HAS_TKINTER:
        print("\nSave current texture before importing? (y/n): ", end="")
        response = input().strip().lower()
        return response == 'y'
    
    # Use pygame to draw the dialog
    dialog_active = True
    result = None
    
    # Dialog dimensions
    dialog_w = 500
    dialog_h = 200
    dialog_x = (WIDTH - dialog_w) // 2
    dialog_y = (HEIGHT - dialog_h) // 2
    
    # Button dimensions
    button_w = 120
    button_h = 40
    button_y = dialog_y + dialog_h - 60
    button_spacing = 20
    
    # Calculate button positions
    total_button_width = 3 * button_w + 2 * button_spacing
    start_x = dialog_x + (dialog_w - total_button_width) // 2
    
    yes_button = pygame.Rect(start_x, button_y, button_w, button_h)
    no_button = pygame.Rect(start_x + button_w + button_spacing, button_y, button_w, button_h)
    cancel_button = pygame.Rect(start_x + 2 * (button_w + button_spacing), button_y, button_w, button_h)
    
    hover_button = None
    
    while dialog_active:
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                return None
            elif event.type == pygame.MOUSEMOTION:
                mx, my = event.pos
                hover_button = None
                if yes_button.collidepoint(mx, my):
                    hover_button = 'yes'
                elif no_button.collidepoint(mx, my):
                    hover_button = 'no'
                elif cancel_button.collidepoint(mx, my):
                    hover_button = 'cancel'
            elif event.type == pygame.MOUSEBUTTONDOWN:
                if event.button == 1:
                    mx, my = event.pos
                    if yes_button.collidepoint(mx, my):
                        result = True
                        dialog_active = False
                    elif no_button.collidepoint(mx, my):
                        result = False
                        dialog_active = False
                    elif cancel_button.collidepoint(mx, my):
                        result = None
                        dialog_active = False
            elif event.type == pygame.KEYDOWN:
                if event.key == pygame.K_y:
                    result = True
                    dialog_active = False
                elif event.key == pygame.K_n:
                    result = False
                    dialog_active = False
                elif event.key == pygame.K_ESCAPE:
                    result = None
                    dialog_active = False
        
        # Draw
        screen.fill((0, 0, 0))
        
        # Draw semi-transparent overlay
        overlay = pygame.Surface((WIDTH, HEIGHT))
        overlay.set_alpha(180)
        overlay.fill((0, 0, 0))
        screen.blit(overlay, (0, 0))
        
        # Draw dialog box
        pygame.draw.rect(screen, (240, 240, 245), (dialog_x, dialog_y, dialog_w, dialog_h))
        pygame.draw.rect(screen, (100, 100, 120), (dialog_x, dialog_y, dialog_w, dialog_h), 3)
        
        # Draw title
        font_title = pygame.font.SysFont(None, 28, bold=True)
        title_text = font_title.render("Save Current Work?", True, (40, 40, 80))
        title_rect = title_text.get_rect(center=(dialog_x + dialog_w // 2, dialog_y + 35))
        screen.blit(title_text, title_rect)
        
        # Draw message
        font_msg = pygame.font.SysFont(None, 20)
        msg_lines = [
            "Do you want to save the current texture",
            "before importing a new image?"
        ]
        y_offset = dialog_y + 75
        for line in msg_lines:
            msg_text = font_msg.render(line, True, (60, 60, 60))
            msg_rect = msg_text.get_rect(center=(dialog_x + dialog_w // 2, y_offset))
            screen.blit(msg_text, msg_rect)
            y_offset += 25
        
        # Draw buttons
        font_button = pygame.font.SysFont(None, 22, bold=True)
        
        # Yes button
        yes_color = (100, 180, 100) if hover_button == 'yes' else (120, 200, 120)
        pygame.draw.rect(screen, yes_color, yes_button)
        pygame.draw.rect(screen, (80, 160, 80), yes_button, 2)
        yes_text = font_button.render("Yes", True, (255, 255, 255))
        yes_text_rect = yes_text.get_rect(center=yes_button.center)
        screen.blit(yes_text, yes_text_rect)
        
        # No button
        no_color = (180, 100, 100) if hover_button == 'no' else (200, 120, 120)
        pygame.draw.rect(screen, no_color, no_button)
        pygame.draw.rect(screen, (160, 80, 80), no_button, 2)
        no_text = font_button.render("No", True, (255, 255, 255))
        no_text_rect = no_text.get_rect(center=no_button.center)
        screen.blit(no_text, no_text_rect)
        
        # Cancel button
        cancel_color = (140, 140, 150) if hover_button == 'cancel' else (160, 160, 170)
        pygame.draw.rect(screen, cancel_color, cancel_button)
        pygame.draw.rect(screen, (120, 120, 130), cancel_button, 2)
        cancel_text = font_button.render("Cancel", True, (255, 255, 255))
        cancel_text_rect = cancel_text.get_rect(center=cancel_button.center)
        screen.blit(cancel_text, cancel_text_rect)
        
        # Draw keyboard hints
        font_hint = pygame.font.SysFont(None, 16)
        hint_text = font_hint.render("Keyboard: Y = Yes, N = No, ESC = Cancel", True, (100, 100, 100))
        hint_rect = hint_text.get_rect(center=(dialog_x + dialog_w // 2, dialog_y + dialog_h - 15))
        screen.blit(hint_text, hint_rect)
        
        pygame.display.flip()
    
    return result

def pygame_file_dialog():
    """Pygame-based file browser to select PNG files"""
    import os
    from pathlib import Path
    
    # Start from home directory
    current_dir = Path.home()
    selected_file = None
    
    # Dialog state
    dialog_active = True
    scroll_offset = 0
    hover_index = -1
    
    # Get list of items in directory
    def get_directory_items(path):
        try:
            items = []
            # Add parent directory option
            if path != path.parent:
                items.append(("..", path.parent, True))
            
            # Get all directories
            dirs = sorted([d for d in path.iterdir() if d.is_dir() and not d.name.startswith('.')], 
                         key=lambda x: x.name.lower())
            for d in dirs:
                items.append((d.name, d, True))
            
            # Get PNG files
            pngs = sorted([f for f in path.iterdir() if f.suffix.lower() == '.png'],
                         key=lambda x: x.name.lower())
            for f in pngs:
                items.append((f.name, f, False))
            
            return items
        except PermissionError:
            return []
    
    items = get_directory_items(current_dir)
    
    # Dialog dimensions
    dialog_w = 700
    dialog_h = 600
    dialog_x = (WIDTH - dialog_w) // 2
    dialog_y = (HEIGHT - dialog_h) // 2
    
    # List dimensions
    list_y = dialog_y + 80
    list_h = dialog_h - 160
    item_h = 30
    visible_items = list_h // item_h
    
    # Button dimensions
    button_w = 120
    button_h = 40
    cancel_button = pygame.Rect(dialog_x + dialog_w - button_w - 20, 
                                 dialog_y + dialog_h - 60, button_w, button_h)
    
    while dialog_active:
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                return None
            elif event.type == pygame.MOUSEMOTION:
                mx, my = event.pos
                # Check which item is hovered
                if dialog_x + 20 <= mx <= dialog_x + dialog_w - 20:
                    if list_y <= my <= list_y + list_h:
                        hover_index = (my - list_y) // item_h + scroll_offset
                        if hover_index >= len(items):
                            hover_index = -1
                    else:
                        hover_index = -1
                else:
                    hover_index = -1
                    
            elif event.type == pygame.MOUSEBUTTONDOWN:
                if event.button == 1:  # Left click
                    mx, my = event.pos
                    
                    # Check cancel button
                    if cancel_button.collidepoint(mx, my):
                        return None
                    
                    # Check item click
                    if dialog_x + 20 <= mx <= dialog_x + dialog_w - 20:
                        if list_y <= my <= list_y + list_h:
                            click_index = (my - list_y) // item_h + scroll_offset
                            if 0 <= click_index < len(items):
                                name, path, is_dir = items[click_index]
                                if is_dir:
                                    # Navigate to directory
                                    current_dir = path
                                    items = get_directory_items(current_dir)
                                    scroll_offset = 0
                                    hover_index = -1
                                else:
                                    # Select file
                                    return str(path)
                
                elif event.button == 4:  # Scroll up
                    scroll_offset = max(0, scroll_offset - 1)
                elif event.button == 5:  # Scroll down
                    max_scroll = max(0, len(items) - visible_items)
                    scroll_offset = min(max_scroll, scroll_offset + 1)
            
            elif event.type == pygame.KEYDOWN:
                if event.key == pygame.K_ESCAPE:
                    return None
                elif event.key == pygame.K_UP:
                    scroll_offset = max(0, scroll_offset - 1)
                elif event.key == pygame.K_DOWN:
                    max_scroll = max(0, len(items) - visible_items)
                    scroll_offset = min(max_scroll, scroll_offset + 1)
        
        # Draw
        screen.fill((0, 0, 0))
        
        # Draw semi-transparent overlay
        overlay = pygame.Surface((WIDTH, HEIGHT))
        overlay.set_alpha(180)
        overlay.fill((0, 0, 0))
        screen.blit(overlay, (0, 0))
        
        # Draw dialog box
        pygame.draw.rect(screen, (240, 240, 245), (dialog_x, dialog_y, dialog_w, dialog_h))
        pygame.draw.rect(screen, (100, 100, 120), (dialog_x, dialog_y, dialog_w, dialog_h), 3)
        
        # Draw title
        font_title = pygame.font.SysFont(None, 28, bold=True)
        title_text = font_title.render("Select PNG Image to Import", True, (40, 40, 80))
        screen.blit(title_text, (dialog_x + 20, dialog_y + 20))
        
        # Draw current path
        font_path = pygame.font.SysFont(None, 18)
        path_str = str(current_dir)
        if len(path_str) > 80:
            path_str = "..." + path_str[-77:]
        path_text = font_path.render(path_str, True, (80, 80, 80))
        screen.blit(path_text, (dialog_x + 20, dialog_y + 55))
        
        # Draw list background
        pygame.draw.rect(screen, (255, 255, 255), (dialog_x + 20, list_y, dialog_w - 40, list_h))
        pygame.draw.rect(screen, (180, 180, 180), (dialog_x + 20, list_y, dialog_w - 40, list_h), 2)
        
        # Draw items
        font_item = pygame.font.SysFont(None, 20)
        for i in range(visible_items):
            item_index = i + scroll_offset
            if item_index >= len(items):
                break
            
            name, path, is_dir = items[item_index]
            item_y = list_y + i * item_h
            
            # Draw hover background
            if item_index == hover_index:
                pygame.draw.rect(screen, (200, 220, 240), 
                               (dialog_x + 20, item_y, dialog_w - 40, item_h))
            
            # Draw icon and text
            icon = "📁" if is_dir else "🖼️"
            color = (60, 60, 180) if is_dir else (60, 60, 60)
            
            icon_text = font_item.render(icon, True, color)
            screen.blit(icon_text, (dialog_x + 30, item_y + 5))
            
            # Truncate long names
            display_name = name
            if len(name) > 60:
                display_name = name[:57] + "..."
            
            name_text = font_item.render(display_name, True, color)
            screen.blit(name_text, (dialog_x + 60, item_y + 5))
        
        # Draw scrollbar if needed
        if len(items) > visible_items:
            scrollbar_h = list_h
            scrollbar_x = dialog_x + dialog_w - 35
            scrollbar_y = list_y
            
            # Scrollbar track
            pygame.draw.rect(screen, (200, 200, 200), 
                           (scrollbar_x, scrollbar_y, 10, scrollbar_h))
            
            # Scrollbar thumb
            thumb_h = max(30, scrollbar_h * visible_items // len(items))
            thumb_y = scrollbar_y + (scrollbar_h - thumb_h) * scroll_offset // max(1, len(items) - visible_items)
            pygame.draw.rect(screen, (120, 120, 120), 
                           (scrollbar_x, thumb_y, 10, thumb_h))
        
        # Draw cancel button
        hover_cancel = cancel_button.collidepoint(pygame.mouse.get_pos())
        cancel_color = (180, 100, 100) if hover_cancel else (200, 120, 120)
        pygame.draw.rect(screen, cancel_color, cancel_button)
        pygame.draw.rect(screen, (160, 80, 80), cancel_button, 2)
        
        font_button = pygame.font.SysFont(None, 22, bold=True)
        cancel_text = font_button.render("Cancel", True, (255, 255, 255))
        cancel_text_rect = cancel_text.get_rect(center=cancel_button.center)
        screen.blit(cancel_text, cancel_text_rect)
        
        # Draw instructions
        font_hint = pygame.font.SysFont(None, 16)
        hint_text = font_hint.render("Double-click folder to open, click PNG file to select. ESC to cancel. Scroll with mouse wheel.", 
                                     True, (100, 100, 100))
        hint_rect = hint_text.get_rect(center=(dialog_x + dialog_w // 2, dialog_y + dialog_h - 20))
        screen.blit(hint_text, hint_rect)
        
        pygame.display.flip()
    
    return None

def import_png():
    """Import PNG image and convert to 64x64 pixel art with auto-naming"""
    global texture_data, texture_name, current_texture_file
    
    # Ask to save current work first
    if texture_name:  # Only if there's a current texture
        save_choice = confirm_save_dialog()
        
        if save_choice is None:  # Cancel
            print("Import cancelled")
            return False
        elif save_choice:  # Yes - save
            export_texture()
            export_png()
            print("✓ Saved current texture")
    
    try:
        # Use pygame file dialog
        filepath = pygame_file_dialog()
        
        if not filepath:
            print("Import cancelled")
            return False
        
        if not os.path.exists(filepath):
            print(f"Error: File not found: {filepath}")
            return False
        
        # Load image
        img = Image.open(filepath)
        img = img.convert('RGB')
        
        # Calculate proportional scaling
        orig_w, orig_h = img.size
        
        if orig_w >= orig_h:
            new_w = TEXTURE_SIZE
            new_h = int((orig_h / orig_w) * TEXTURE_SIZE)
        else:
            new_h = TEXTURE_SIZE
            new_w = int((orig_w / orig_h) * TEXTURE_SIZE)
        
        new_w = max(1, new_w)
        new_h = max(1, new_h)
        
        # Resize image
        img_resized = img.resize((new_w, new_h), Image.Resampling.LANCZOS)
        
        # Create 64x64 white canvas and convert to numpy array filled with white
        texture_data = np.full((TEXTURE_SIZE, TEXTURE_SIZE, 3), [255, 255, 255], dtype=np.uint8)
        
        # Calculate center position
        paste_x = (TEXTURE_SIZE - new_w) // 2
        paste_y = (TEXTURE_SIZE - new_h) // 2
        
        # Convert resized image to numpy array and paste into texture_data
        img_array = np.array(img_resized, dtype=np.uint8)
        texture_data[paste_y:paste_y+new_h, paste_x:paste_x+new_w] = img_array
        
        # Auto-name from PNG filename
        png_basename = Path(filepath).stem
        # Sanitize name
        new_name = re.sub(r'[^a-zA-Z0-9_]', '_', png_basename)
        
        # Create new texture with this name
        texture_name = new_name
        
        # Set file path
        textures_dir = Path("../include/data/textures")
        if not textures_dir.exists():
            textures_dir = Path("include/data/textures")
        
        textures_dir.mkdir(parents=True, exist_ok=True)
        current_texture_file = str(textures_dir / f"{texture_name}.h")
        
        # Save state for undo
        save_state()
        
        # Auto-save
        export_texture()
        
        print(f"✓ Imported: {os.path.basename(filepath)}")
        print(f"  Original size: {orig_w}x{orig_h}")
        print(f"  Resized to: {new_w}x{new_h} (centered in 64x64)")
        print(f"  Texture named: {texture_name}")
        return True
        
    except Exception as e:
        print(f"Error importing image: {e}")
        import traceback
        traceback.print_exc()
        return False

pygame.init()
screen = pygame.display.set_mode((WIDTH, HEIGHT))
pygame.display.set_caption("Texture Editor - 64x64 RGB")
clock = pygame.time.Clock()

# Drawing modes and tools
DRAW_MODE, ERASE_MODE, PICK_MODE = 0, 1, 2
POINT_TOOL, LINE_TOOL, RECT_TOOL, ELLIPSE_TOOL = 0, 1, 2, 3
FILL_RECT, BORDER_RECT = 0, 1

mode = DRAW_MODE
tool = POINT_TOOL
rect_mode = BORDER_RECT
draw_color = [255, 0, 0]  # RGB

# Tool state
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
        color = (180, 220, 180) if self.active else (200, 200, 210)
        if self.flash:
            color = (180, 220, 180)
        if not self.enabled:
            color = (200, 200, 200)
        pygame.draw.rect(surface, color, self.rect)
        pygame.draw.rect(surface, (115, 115, 130), self.rect, 2)
        font = pygame.font.SysFont(None, 16)
        text = font.render(self.name, True, (80, 80, 80) if self.enabled else (175, 175, 175))
        text_rect = text.get_rect(center=self.rect.center)
        surface.blit(text, text_rect)

    def handle_click(self):
        global mode, tool, rect_mode, show_help, color_picker_mode
        if self.group == 'mode':
            if self.name == "Pick Color":
                mode = PICK_MODE
            elif mode == DRAW_MODE:
                mode = ERASE_MODE
                self.name = "Mode(Erase)"
            else:
                mode = DRAW_MODE
                self.name = "Mode(Draw)"
            for b in mode_buttons:
                b.active = (b == self)
        elif self.group == 'tool':
            if self.value == RECT_TOOL:
                if tool != RECT_TOOL:
                    tool = RECT_TOOL
                    for b in tool_buttons:
                        b.active = (b.value == tool)
                    tool_buttons[2].name = "Rect(Border)" if rect_mode == BORDER_RECT else "Rect(Fill)"
                else:
                    rect_mode = FILL_RECT if rect_mode == BORDER_RECT else BORDER_RECT
                    tool_buttons[2].name = "Rect(Border)" if rect_mode == BORDER_RECT else "Rect(Fill)"
                for b in tool_buttons:
                    b.active = (b.value == tool)
            elif self.value == ELLIPSE_TOOL:
                if tool != ELLIPSE_TOOL:
                    tool = ELLIPSE_TOOL
                    for b in tool_buttons:
                        b.active = (b.value == tool)
                    tool_buttons[3].name = "Ellipse(Border)" if rect_mode == BORDER_RECT else "Ellipse(Fill)"
                else:
                    rect_mode = FILL_RECT if rect_mode == BORDER_RECT else BORDER_RECT
                    tool_buttons[3].name = "Ellipse(Border)" if rect_mode == BORDER_RECT else "Ellipse(Fill)"
                for b in tool_buttons:
                    b.active = (b.value == tool)
            else:
                tool = self.value
                for b in tool_buttons:
                    b.active = (b.value == tool)
                tool_buttons[2].name = "Rect(Border)" if rect_mode == BORDER_RECT else "Rect(Fill)"
                tool_buttons[3].name = "Ellipse(Border)" if rect_mode == BORDER_RECT else "Ellipse(Fill)"
        elif self.group == 'extra':
            if self.name == "Help":
                show_help = not show_help
                self.active = show_help
            elif self.name == "Color Picker":
                color_picker_mode = not color_picker_mode
                self.active = color_picker_mode

def quit_editor():
    pygame.quit()
    sys.exit()

# UI Buttons
mode_buttons = [
    Button("Mode(Draw)", (10, 32, TOOLBAR_W-20, BUTTON_H), 'mode', DRAW_MODE),
]

tool_buttons = [
    Button("Point", (10, 32+1*(BUTTON_H+6), TOOLBAR_W-20, BUTTON_H), 'tool', POINT_TOOL),
    Button("Line", (10, 32+2*(BUTTON_H+6), TOOLBAR_W-20, BUTTON_H), 'tool', LINE_TOOL),
    Button("Rect(Border)", (10, 32+3*(BUTTON_H+6), TOOLBAR_W-20, BUTTON_H), 'tool', RECT_TOOL),
    Button("Ellipse(Border)", (10, 32+4*(BUTTON_H+6), TOOLBAR_W-20, BUTTON_H), 'tool', ELLIPSE_TOOL),
]

# Extra buttons - removed Color Picker and Pick Color (now in color panel)
extra_buttons = [
    Button("Create New", (10, 32+5*(BUTTON_H+6), TOOLBAR_W-20, BUTTON_H), 'extra', None),
    Button("Load .h", (10, 32+6*(BUTTON_H+6), TOOLBAR_W-20, BUTTON_H), 'extra', None),
    Button("Export .h", (10, 32+7*(BUTTON_H+6), TOOLBAR_W-20, BUTTON_H), 'extra', None),
    Button("Import PNG", (10, 32+8*(BUTTON_H+6), TOOLBAR_W-20, BUTTON_H), 'extra', None),
    Button("Export PNG", (10, 32+9*(BUTTON_H+6), TOOLBAR_W-20, BUTTON_H), 'extra', None),
    Button("Undo", (10, 32+10*(BUTTON_H+6), TOOLBAR_W-20, BUTTON_H), 'extra', None),
    Button("Redo", (10, 32+11*(BUTTON_H+6), TOOLBAR_W-20, BUTTON_H), 'extra', None),
    Button("Clear", (10, 32+12*(BUTTON_H+6), TOOLBAR_W-20, BUTTON_H), 'extra', None),
    Button("Help", (10, 32+13*(BUTTON_H+6), TOOLBAR_W-20, BUTTON_H), 'extra', None),
    Button("Quit", (10, 32+14*(BUTTON_H+6), TOOLBAR_W-20, BUTTON_H), 'extra', None),
]

mode_buttons[0].active = True
tool_buttons[0].active = True

def draw_color_panel():
    """Draw color panel on the right side (Option B)"""
    global rgb_input_texts
    
    panel_x = TOOLBAR_W + TEXTURE_SIZE * PIXEL_SIZE
    panel_rect = pygame.Rect(panel_x, 0, COLOR_PANEL_W, HEIGHT)
    pygame.draw.rect(screen, (240, 240, 245), panel_rect)
    
    font_title = pygame.font.SysFont(None, 20, bold=True)
    font_text = pygame.font.SysFont(None, 16)
    font_small = pygame.font.SysFont(None, 14)
    
    y = 15
    x_margin = panel_x + 10
    
    # Title
    title = font_title.render("COLOR PANEL", True, (50, 50, 80))
    screen.blit(title, (x_margin, y))
    y += 30
    
    # 1. EYEDROPPER TOOL
    eyedropper_button = pygame.Rect(x_margin, y, COLOR_PANEL_W - 20, 30)
    is_eyedropper_active = (mode == PICK_MODE)
    button_color = (150, 200, 150) if is_eyedropper_active else (200, 200, 210)
    pygame.draw.rect(screen, button_color, eyedropper_button)
    pygame.draw.rect(screen, (100, 100, 120), eyedropper_button, 2)
    
    eyedropper_text = font_text.render("🎨 Eyedropper (Pick Color)", True, (60, 60, 60))
    eyedropper_rect = eyedropper_text.get_rect(center=eyedropper_button.center)
    screen.blit(eyedropper_text, eyedropper_rect)
    
    # Store button for interaction
    if not hasattr(draw_color_panel, 'eyedropper_btn'):
        draw_color_panel.eyedropper_btn = eyedropper_button
    else:
        draw_color_panel.eyedropper_btn = eyedropper_button
    
    y += 40
    
    # 2. CURRENT COLOR PREVIEW
    preview_label = font_text.render("Current Color:", True, (60, 60, 60))
    screen.blit(preview_label, (x_margin, y))
    y += 20
    
    preview_rect = pygame.Rect(x_margin, y, COLOR_PANEL_W - 20, 50)
    pygame.draw.rect(screen, tuple(draw_color), preview_rect)
    pygame.draw.rect(screen, (80, 80, 80), preview_rect, 3)
    y += 60
    
    # 3. RGB NUMERIC INPUTS
    rgb_label = font_text.render("RGB Values (0-255):", True, (60, 60, 60))
    screen.blit(rgb_label, (x_margin, y))
    y += 22
    
    rgb_names = ["R:", "G:", "B:"]
    rgb_colors = [(255, 100, 100), (100, 255, 100), (100, 100, 255)]
    
    if not hasattr(draw_color_panel, 'rgb_input_boxes'):
        draw_color_panel.rgb_input_boxes = []
    else:
        draw_color_panel.rgb_input_boxes.clear()
    
    for i in range(3):
        label = font_small.render(rgb_names[i], True, rgb_colors[i])
        screen.blit(label, (x_margin, y + 7))
        
        input_box = pygame.Rect(x_margin + 25, y, 60, 24)
        box_color = (255, 255, 200) if rgb_input_active_field == i else (255, 255, 255)
        pygame.draw.rect(screen, box_color, input_box)
        pygame.draw.rect(screen, (100, 100, 120), input_box, 2)
        
        # Display text
        display_text = rgb_input_texts[i]
        if rgb_input_active_field == i:
            display_text += "_"
        
        text_surf = font_small.render(display_text, True, (40, 40, 40))
        screen.blit(text_surf, (input_box.x + 5, input_box.y + 5))
        
        draw_color_panel.rgb_input_boxes.append(input_box)
        
        y += 28
    
    y += 10
    
    # 4. RGB SLIDERS
    slider_label = font_text.render("RGB Sliders:", True, (60, 60, 60))
    screen.blit(slider_label, (x_margin, y))
    y += 22
    
    slider_width = COLOR_PANEL_W - 20
    slider_height = 18
    
    if not hasattr(draw_color_panel, 'sliders'):
        draw_color_panel.sliders = []
    else:
        draw_color_panel.sliders.clear()
    
    for i, (name, base_color) in enumerate([("R", (255, 0, 0)), ("G", (0, 255, 0)), ("B", (0, 0, 255))]):
        # Label
        label_text = font_small.render(f"{name}: {draw_color[i]}", True, (50, 50, 50))
        screen.blit(label_text, (x_margin, y))
        y += 18
        
        # Slider background with gradient
        slider_rect = pygame.Rect(x_margin, y, slider_width, slider_height)
        
        # Draw gradient
        for x_offset in range(slider_width):
            val = int((x_offset / slider_width) * 255)
            temp_color = [0, 0, 0]
            temp_color[i] = val
            pygame.draw.line(screen, tuple(temp_color),
                           (slider_rect.x + x_offset, slider_rect.y),
                           (slider_rect.x + x_offset, slider_rect.y + slider_height))
        
        pygame.draw.rect(screen, (80, 80, 80), slider_rect, 2)
        
        # Slider handle
        handle_x = slider_rect.x + int((draw_color[i] / 255) * slider_width)
        handle_x = max(slider_rect.x, min(handle_x, slider_rect.x + slider_width))
        pygame.draw.circle(screen, (60, 60, 60), (handle_x, slider_rect.centery), 8)
        pygame.draw.circle(screen, (240, 240, 240), (handle_x, slider_rect.centery), 6)
        
        draw_color_panel.sliders.append((slider_rect, i))
        
        y += 26
    
    y += 10
    
    # 5. COLOR PALETTE - Professional gradient palette
    palette_label = font_text.render("Color Palette:", True, (60, 60, 60))
    screen.blit(palette_label, (x_margin, y))
    y += 22
    
    # Draw palette grid (12 columns for hues, rows for brightness)
    cols = 12
    swatch_size = 18  # Smaller swatches to fit more colors
    spacing = 2
    
    if not hasattr(draw_color_panel, 'palette_swatches'):
        draw_color_panel.palette_swatches = []
    else:
        draw_color_panel.palette_swatches.clear()
    
    for idx, color in enumerate(COLOR_PALETTE):
        row = idx // cols
        col = idx % cols
        
        swatch_x = x_margin + col * (swatch_size + spacing)
        swatch_y = y + row * (swatch_size + spacing)
        
        swatch_rect = pygame.Rect(swatch_x, swatch_y, swatch_size, swatch_size)
        pygame.draw.rect(screen, color, swatch_rect)
        
        # Highlight if selected
        if list(color) == draw_color:
            pygame.draw.rect(screen, (255, 255, 0), swatch_rect, 2)
        else:
            pygame.draw.rect(screen, (80, 80, 80), swatch_rect, 1)
        
        draw_color_panel.palette_swatches.append((swatch_rect, color))


def draw_toolbar():
    """绘制工具栏"""
    pygame.draw.rect(screen, (230, 230, 240), (0, 0, TOOLBAR_W, HEIGHT))
    
    # Display currently editing file - larger font at top
    font_filename = pygame.font.SysFont(None, 22, bold=True)
    if texture_name:
        file_text = font_filename.render(f"{texture_name}.h", True, (60, 80, 140))
        screen.blit(file_text, (10, 10))
    else:
        file_text = font_filename.render("No file loaded", True, (180, 60, 60))
        screen.blit(file_text, (10, 10))
    
    for b in mode_buttons:
        b.draw(screen)
    for b in tool_buttons:
        b.draw(screen)
    for b in extra_buttons:
        b.draw(screen)

def draw_create_new():
    """绘制创建新纹理界面 - 参考map_editor"""
    global create_new_mode, input_text
    
    create_area = pygame.Rect(TOOLBAR_W, 0, WIDTH - TOOLBAR_W, HEIGHT)
    pygame.draw.rect(screen, (245, 245, 250), create_area)
    
    font_title = pygame.font.SysFont(None, 28, bold=True)
    font_text = pygame.font.SysFont(None, 20)
    font_small = pygame.font.SysFont(None, 16)
    
    y_offset = HEIGHT // 2 - 100
    x_margin = TOOLBAR_W + 50
    
    # 标题
    title = font_title.render("CREATE NEW TEXTURE", True, (40, 40, 80))
    title_rect = title.get_rect(center=(TOOLBAR_W + (WIDTH - TOOLBAR_W) // 2, y_offset))
    screen.blit(title, title_rect)
    y_offset += 60
    
    # 提示信息
    prompt = font_text.render("Enter texture name (e.g., brick2, wood_dark):", True, (60, 60, 60))
    screen.blit(prompt, (x_margin, y_offset))
    y_offset += 40
    
    # 输入框
    input_box = pygame.Rect(x_margin, y_offset, 300, 40)
    pygame.draw.rect(screen, (255, 255, 255), input_box)
    pygame.draw.rect(screen, (100, 100, 150), input_box, 2)
    
    # 显示输入文本
    input_surface = font_text.render(input_text + "_", True, (40, 40, 40))
    screen.blit(input_surface, (x_margin + 10, y_offset + 10))
    y_offset += 60
    
    # 提示信息
    hint1 = font_small.render("Press ENTER to create", True, (100, 100, 100))
    hint2 = font_small.render("Press ESC to cancel", True, (100, 100, 100))
    hint3 = font_small.render("Use only letters, numbers, and underscores", True, (120, 120, 120))
    screen.blit(hint1, (x_margin, y_offset))
    screen.blit(hint2, (x_margin, y_offset + 20))
    screen.blit(hint3, (x_margin, y_offset + 50))

def draw_load_screen():
    """绘制加载纹理界面 - 参考map_editor"""
    global available_textures
    
    selector_area = pygame.Rect(TOOLBAR_W, 0, WIDTH - TOOLBAR_W, HEIGHT)
    pygame.draw.rect(screen, (245, 245, 250), selector_area)
    
    font_title = pygame.font.SysFont(None, 28, bold=True)
    font_text = pygame.font.SysFont(None, 18)
    font_small = pygame.font.SysFont(None, 14)
    
    y_offset = 20
    x_margin = TOOLBAR_W + 20
    
    # 标题
    title = font_title.render("SELECT TEXTURE TO EDIT", True, (40, 40, 80))
    screen.blit(title, (x_margin, y_offset))
    y_offset += 50
    
    # 显示当前纹理
    if current_texture_file:
        current_text = font_small.render(f"Current: {os.path.basename(current_texture_file)}", True, (100, 100, 100))
        screen.blit(current_text, (x_margin, y_offset))
        y_offset += 30
    else:
        current_text = font_small.render("No texture loaded", True, (150, 50, 50))
        screen.blit(current_text, (x_margin, y_offset))
        y_offset += 30
    
    # "CREATE NEW TEXTURE" 按钮
    new_button_rect = pygame.Rect(x_margin, y_offset, 400, 35)
    mouse_pos = pygame.mouse.get_pos()
    is_hover_new = new_button_rect.collidepoint(mouse_pos)
    
    button_color = (150, 220, 150) if is_hover_new else (180, 240, 180)
    pygame.draw.rect(screen, button_color, new_button_rect)
    pygame.draw.rect(screen, (80, 150, 80), new_button_rect, 3)
    
    new_text = font_text.render("+ CREATE NEW TEXTURE", True, (40, 100, 40))
    new_text_rect = new_text.get_rect(center=new_button_rect.center)
    screen.blit(new_text, new_text_rect)
    
    # 存储按钮用于检测
    if not hasattr(draw_load_screen, 'new_button'):
        draw_load_screen.new_button = new_button_rect
    else:
        draw_load_screen.new_button = new_button_rect
    
    y_offset += 50
    
    # 分隔线
    pygame.draw.line(screen, (150, 150, 150), (x_margin, y_offset), (x_margin + 400, y_offset), 2)
    y_offset += 20
    
    # 显示可用纹理列表
    available_textures = scan_texture_files()
    
    if available_textures:
        existing_title = font_text.render("Existing Textures:", True, (80, 80, 80))
        screen.blit(existing_title, (x_margin, y_offset))
        y_offset += 30
        
        for i, tex_file in enumerate(available_textures):
            tex_name = os.path.basename(tex_file)
            is_current = (tex_file == current_texture_file)
            
            button_rect = pygame.Rect(x_margin, y_offset, 400, 30)
            is_hover = button_rect.collidepoint(mouse_pos)
            
            # 按钮背景
            if is_current:
                color = (180, 220, 180)
            elif is_hover:
                color = (220, 220, 230)
            else:
                color = (240, 240, 240)
            
            pygame.draw.rect(screen, color, button_rect)
            pygame.draw.rect(screen, (115, 115, 130), button_rect, 2)
            
            # 文本
            text_color = (40, 100, 40) if is_current else (60, 60, 60)
            text = font_text.render(f"{i+1}. {tex_name}", True, text_color)
            screen.blit(text, (x_margin + 10, y_offset + 7))
            
            y_offset += 35
            
            # 存储按钮信息
            if not hasattr(draw_load_screen, 'texture_buttons'):
                draw_load_screen.texture_buttons = []
            if i >= len(draw_load_screen.texture_buttons):
                draw_load_screen.texture_buttons.append((button_rect, tex_file))
            else:
                draw_load_screen.texture_buttons[i] = (button_rect, tex_file)
    else:
        no_text = font_text.render("No existing textures found", True, (150, 50, 50))
        screen.blit(no_text, (x_margin, y_offset))
        y_offset += 30
        hint_text = font_small.render("Click 'CREATE NEW TEXTURE' to start", True, (100, 100, 100))
        screen.blit(hint_text, (x_margin, y_offset))
    
    # 底部提示
    y_offset = HEIGHT - 40
    hint1 = font_small.render("Click on a texture file to load it", True, (100, 100, 100))
    screen.blit(hint1, (x_margin, y_offset))

def draw_help():
    """绘制帮助界面"""
    help_area = pygame.Rect(TOOLBAR_W, 0, WIDTH - TOOLBAR_W, HEIGHT)
    pygame.draw.rect(screen, (245, 245, 250), help_area)
    
    font_title = pygame.font.SysFont(None, 24, bold=True)
    font_section = pygame.font.SysFont(None, 18, bold=True)
    font_text = pygame.font.SysFont(None, 14)
    
    y_offset = 15
    x_margin = TOOLBAR_W + 15
    line_spacing = 18
    
    title = font_title.render("TEXTURE EDITOR - HELP", True, (40, 40, 80))
    screen.blit(title, (x_margin, y_offset))
    y_offset += 30
    
    help_content = [
        ("BASIC CONTROLS", [
            "Left Click: Draw/Erase/Pick color",
            "Shift + Drag: Straight lines/squares",
        ]),
        ("DRAWING MODES", [
            "Draw Mode: Paint with selected color",
            "Pick Color: Click to pick color from texture",
            "Toggle: Click Mode button or press 'M'",
        ]),
        ("DRAWING TOOLS", [
            "Point (P): Draw single pixels",
            "Line (L): Draw straight lines",
            "Rect (R): Draw rectangles (Fill/Border)",
            "Ellipse (E): Draw ellipses (Fill/Border)",
        ]),
        ("COLOR PICKER", [
            "Click 'Color Picker' to open RGB sliders",
            "Adjust R, G, B values (0-255)",
            "Preview shows current color",
            "Pick Color mode: Click texture to sample",
        ]),
        ("IMPORT PNG", [
            "Click 'Import PNG' or press Ctrl+I",
            "Select PNG file from dialog",
            "Image auto-resized with aspect ratio",
            "Centered in 64x64 canvas",
            "Converts to RGB pixel art",
        ]),
        ("KEYBOARD SHORTCUTS", [
            "M: Toggle Draw/Pick mode",
            "P/L/R/E: Select tool",
            "Ctrl+N: Create new texture",
            "Ctrl+L: Load texture",
            "Ctrl+E: Export texture (.h)",
            "Ctrl+P: Export as PNG",
            "Ctrl+I: Import PNG image",
            "U or Ctrl+Z: Undo",
            "Ctrl+Y: Redo",
            "Ctrl+K: Toggle color picker",
            "H: Toggle help (press again to close)",
            "Q: Quit",
        ]),
        ("FILE MANAGEMENT", [
            "- Create: Use Ctrl+N or Load screen",
            "- Load: Click texture in Load screen",
            "- Export: Saves to current file",
            "- Cannot save without a name",
        ]),
        ("FILE FORMAT", [
            "Format: 64x64 RGB texture",
            "Output: C++ header file (.h)",
            "Data: [64][64][3] array (R,G,B)",
            "Location: include/data/textures/",
        ]),
    ]
    
    for section_title, items in help_content:
        section_surf = font_section.render(section_title, True, (60, 60, 120))
        screen.blit(section_surf, (x_margin, y_offset))
        y_offset += line_spacing + 2
        
        for item in items:
            text_surf = font_text.render(item, True, (50, 50, 50))
            screen.blit(text_surf, (x_margin + 8, y_offset))
            y_offset += line_spacing - 2
        
        y_offset += 6
    
    footer = font_text.render("Press 'H' again to return to editor", True, (100, 100, 100))
    footer_rect = footer.get_rect(center=(TOOLBAR_W + (WIDTH - TOOLBAR_W) // 2, HEIGHT - 20))
    screen.blit(footer, footer_rect)

def draw_color_picker():
    """绘制颜色选择器"""
    picker_area = pygame.Rect(TOOLBAR_W, 0, WIDTH - TOOLBAR_W, HEIGHT)
    pygame.draw.rect(screen, (245, 245, 250), picker_area)
    
    font_title = pygame.font.SysFont(None, 24, bold=True)
    font_text = pygame.font.SysFont(None, 16)
    
    y_offset = 40
    x_margin = TOOLBAR_W + 40
    
    title = font_title.render("COLOR PICKER", True, (40, 40, 80))
    screen.blit(title, (x_margin, y_offset))
    y_offset += 50
    
    # RGB滑块
    slider_width = 300
    slider_height = 20
    
    for i, (name, color_name) in enumerate([("Red", (255, 0, 0)), ("Green", (0, 255, 0)), ("Blue", (0, 0, 255))]):
        label = font_text.render(f"{name}: {draw_color[i]}", True, (40, 40, 40))
        screen.blit(label, (x_margin, y_offset))
        y_offset += 25
        
        slider_rect = pygame.Rect(x_margin, y_offset, slider_width, slider_height)
        pygame.draw.rect(screen, (200, 200, 200), slider_rect)
        
        # 渐变背景
        for x in range(slider_width):
            val = int((x / slider_width) * 255)
            temp_color = [0, 0, 0]
            temp_color[i] = val
            pygame.draw.line(screen, tuple(temp_color), 
                           (slider_rect.x + x, slider_rect.y), 
                           (slider_rect.x + x, slider_rect.y + slider_height))
        
        pygame.draw.rect(screen, (100, 100, 100), slider_rect, 2)
        
        # 滑块位置
        slider_x = slider_rect.x + int((draw_color[i] / 255) * slider_width)
        pygame.draw.circle(screen, (80, 80, 80), (slider_x, slider_rect.centery), 10)
        pygame.draw.circle(screen, (255, 255, 255), (slider_x, slider_rect.centery), 8)
        
        # 保存滑块矩形以便交互
        if not hasattr(draw_color_picker, 'sliders'):
            draw_color_picker.sliders = []
        if i >= len(draw_color_picker.sliders):
            draw_color_picker.sliders.append((slider_rect, i))
        else:
            draw_color_picker.sliders[i] = (slider_rect, i)
        
        y_offset += 40
    
    # 颜色预览
    y_offset += 20
    preview_label = font_text.render("Preview:", True, (40, 40, 40))
    screen.blit(preview_label, (x_margin, y_offset))
    y_offset += 25
    
    preview_rect = pygame.Rect(x_margin, y_offset, 300, 80)
    pygame.draw.rect(screen, tuple(draw_color), preview_rect)
    pygame.draw.rect(screen, (100, 100, 100), preview_rect, 3)
    
    # 提示
    hint = font_text.render("Click and drag sliders to adjust color", True, (100, 100, 100))
    screen.blit(hint, (x_margin, HEIGHT - 40))

def draw_texture():
    """绘制纹理画布"""
    for y in range(TEXTURE_SIZE):
        for x in range(TEXTURE_SIZE):
            px = TOOLBAR_W + x * PIXEL_SIZE
            py = y * PIXEL_SIZE
            color = tuple(texture_data[y, x])
            pygame.draw.rect(screen, color, (px, py, PIXEL_SIZE-1, PIXEL_SIZE-1))
            pygame.draw.rect(screen, (180, 180, 180), (px, py, PIXEL_SIZE-1, PIXEL_SIZE-1), 1)
    
    # 绘制预览线条
    if tool == LINE_TOOL and line_start and line_end:
        x0, y0 = line_start
        x1, y1 = line_end
        sx0 = TOOLBAR_W + x0 * PIXEL_SIZE + PIXEL_SIZE // 2
        sy0 = y0 * PIXEL_SIZE + PIXEL_SIZE // 2
        sx1 = TOOLBAR_W + x1 * PIXEL_SIZE + PIXEL_SIZE // 2
        sy1 = y1 * PIXEL_SIZE + PIXEL_SIZE // 2
        pygame.draw.line(screen, (255, 0, 0), (sx0, sy0), (sx1, sy1), 2)
    
    if tool == RECT_TOOL and rect_start and rect_end:
        rx0, ry0 = rect_start
        rx1, ry1 = rect_end
        if rx0 > rx1:
            rx0, rx1 = rx1, rx0
        if ry0 > ry1:
            ry0, ry1 = ry1, ry0
        sx0 = TOOLBAR_W + rx0 * PIXEL_SIZE
        sy0 = ry0 * PIXEL_SIZE
        sx1 = TOOLBAR_W + (rx1 + 1) * PIXEL_SIZE
        sy1 = (ry1 + 1) * PIXEL_SIZE
        if rect_mode == FILL_RECT:
            s = pygame.Surface((sx1 - sx0, sy1 - sy0))
            s.set_alpha(128)
            s.fill((255, 180, 0))
            screen.blit(s, (sx0, sy0))
        else:
            pygame.draw.rect(screen, (255, 50, 0), (sx0, sy0, sx1 - sx0, sy1 - sy0), 2)
    
    if tool == ELLIPSE_TOOL and ellipse_start and ellipse_end:
        preview_ellipse(ellipse_start, ellipse_end, rect_mode)

def preview_ellipse(ellipse_start, ellipse_end, rect_mode):
    """预览椭圆 - 参考map_editor，绘制实际椭圆而非矩形"""
    x0, y0 = ellipse_start
    x1, y1 = ellipse_end
    left = min(x0, x1)
    right = max(x0, x1)
    top = min(y0, y1)
    bottom = max(y0, y1)
    
    sx0 = TOOLBAR_W + left * PIXEL_SIZE
    sy0 = top * PIXEL_SIZE
    sx1 = TOOLBAR_W + (right + 1) * PIXEL_SIZE
    sy1 = (bottom + 1) * PIXEL_SIZE
    rect = pygame.Rect(sx0, sy0, sx1 - sx0, sy1 - sy0)
    
    if rect_mode == FILL_RECT:
        # 填充模式：绘制半透明填充椭圆
        s = pygame.Surface((rect.width, rect.height), pygame.SRCALPHA)
        pygame.draw.ellipse(s, (0, 180, 255, 100), s.get_rect())
        screen.blit(s, (rect.x, rect.y))
    else:
        # 边框模式：绘制椭圆边框
        pygame.draw.ellipse(screen, (0, 50, 255), rect, 2)

def bresenham_line(x0, y0, x1, y1):
    """Bresenham直线算法"""
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
    """对齐到8个方向"""
    dx = x1 - x0
    dy = y1 - y0
    if dx == 0 and dy == 0:
        return (x1, y1)
    directions = [
        (1, 0), (1, 1), (0, 1), (-1, 1),
        (-1, 0), (-1, -1), (0, -1), (1, -1)
    ]
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
    snap_x = x0 + best_dir[0] * steps
    snap_y = y0 + best_dir[1] * steps
    return (snap_x, snap_y)

def clamp(val, lo, hi):
    return max(lo, min(val, hi))

def handle_draw(x, y, color):
    """处理绘制"""
    if 0 <= x < TEXTURE_SIZE and 0 <= y < TEXTURE_SIZE:
        save_state()
        texture_data[y, x] = color

def handle_line_draw(line_cells, color):
    """处理直线绘制"""
    save_state()
    for px, py in line_cells:
        if 0 <= px < TEXTURE_SIZE and 0 <= py < TEXTURE_SIZE:
            texture_data[py, px] = color

def handle_rect_draw(x0, y0, x1, y1, fillmode, color):
    """处理矩形绘制"""
    save_state()
    x0 = clamp(x0, 0, TEXTURE_SIZE - 1)
    x1 = clamp(x1, 0, TEXTURE_SIZE - 1)
    y0 = clamp(y0, 0, TEXTURE_SIZE - 1)
    y1 = clamp(y1, 0, TEXTURE_SIZE - 1)
    if x0 > x1:
        x0, x1 = x1, x0
    if y0 > y1:
        y0, y1 = y1, y0
    
    for yy in range(y0, y1 + 1):
        for xx in range(x0, x1 + 1):
            if fillmode == FILL_RECT:
                texture_data[yy, xx] = color
            else:
                if xx == x0 or xx == x1 or yy == y0 or yy == y1:
                    texture_data[yy, xx] = color

def ellipse_points(x0, y0, x1, y1, fillmode):
    """计算椭圆点"""
    pts = []
    left = min(x0, x1)
    right = max(x0, x1)
    top = min(y0, y1)
    bottom = max(y0, y1)
    cx = (left + right) / 2
    cy = (top + bottom) / 2
    rx = max(abs(right - left) / 2, 1)
    ry = max(abs(bottom - top) / 2, 1)
    
    for y in range(top, bottom + 1):
        for x in range(left, right + 1):
            dx = (x - cx) / rx
            dy = (y - cy) / ry
            dist = dx * dx + dy * dy
            if fillmode == FILL_RECT:
                if dist <= 1.0:
                    pts.append((int(x), int(y)))
            else:
                if 0.88 <= dist <= 1.12:
                    pts.append((int(x), int(y)))
    return pts

def handle_ellipse_draw(x0, y0, x1, y1, fillmode, color):
    """处理椭圆绘制"""
    save_state()
    pts = ellipse_points(x0, y0, x1, y1, fillmode)
    for x, y in pts:
        if 0 <= x < TEXTURE_SIZE and 0 <= y < TEXTURE_SIZE:
            texture_data[y, x] = color

def pick_color_from_texture(x, y):
    """从纹理取色"""
    global draw_color, rgb_input_texts
    if 0 <= x < TEXTURE_SIZE and 0 <= y < TEXTURE_SIZE:
        draw_color = texture_data[y, x].tolist()
        rgb_input_texts = [str(c) for c in draw_color]

def handle_button_click(pos, now, undo_time, redo_time, export_time, load_time, clear_time, create_time):
    """处理按钮点击 - 参考map_editor添加闪烁计时器"""
    global show_help, color_picker_mode, load_screen, create_new_mode
    
    # 返回更新后的计时器值
    timers = {
        'undo': undo_time,
        'redo': redo_time,
        'export': export_time,
        'load': load_time,
        'clear': clear_time,
        'create': create_time
    }
    
    for b in mode_buttons + tool_buttons + extra_buttons:
        if b.rect.collidepoint(pos):
            if b.group in ['mode', 'tool']:
                b.handle_click()
            elif b.name == "Create New":
                create_new_mode = not create_new_mode
                load_screen = False
                b.flash = True
                timers['create'] = now + 120
            elif b.name == "Export .h":
                if export_texture():
                    b.flash = True
                    timers['export'] = now + 120
            elif b.name == "Export PNG":
                if export_png():
                    b.flash = True
                    timers['export'] = now + 120
            elif b.name == "Import PNG":
                # 使用文件对话框导入PNG
                if import_png():
                    print("✓ PNG imported successfully")
            elif b.name == "Undo":
                undo()
                b.flash = True
                timers['undo'] = now + 120
            elif b.name == "Redo":
                redo()
                b.flash = True
                timers['redo'] = now + 120
            elif b.name == "Clear":
                save_state()
                texture_data.fill(255)  # 清除为白色
                b.flash = True
                timers['clear'] = now + 120
            elif b.name == "Load .h":
                load_screen = not load_screen
                create_new_mode = False
                b.active = load_screen  # Highlight when active
                b.flash = True
                timers['load'] = now + 120
            elif b.name == "Help":
                show_help = not show_help
                b.active = show_help
            elif b.name == "Quit":
                quit_editor()
            break
    
    return timers['undo'], timers['redo'], timers['export'], timers['load'], timers['clear'], timers['create']

def main():
    """主循环"""
    global line_start, line_end, rect_start, rect_end, ellipse_start, ellipse_end
    global show_help, color_picker_mode, load_screen, create_new_mode, draw_color, texture_name
    global mode, tool, rect_mode, input_text, rgb_input_active_field, rgb_input_texts
    
    # 尝试加载第一个可用的纹理
    available_textures = scan_texture_files()
    if available_textures:
        first_texture = available_textures[0]
        print(f"Loading default texture: {os.path.basename(first_texture)}")
        if load_rgb_texture(first_texture):
            print("✓ Default texture loaded successfully")
        else:
            print("⚠ Failed to load default texture")
    else:
        print("No existing textures found. Use Ctrl+N to create a new texture.")
    
    running = True
    dragging_line = False
    dragging_rect = False
    dragging_ellipse = False
    dragging_slider = None
    
    # 按钮闪烁计时器 - 参考map_editor
    undo_flash_time = 0
    redo_flash_time = 0
    export_flash_time = 0
    load_flash_time = 0
    clear_flash_time = 0
    create_flash_time = 0
    
    print("\n" + "="*50)
    print("TEXTURE EDITOR - 64x64 RGB")
    print("="*50)
    print("Controls:")
    print("  - Draw with mouse")
    print("  - Press 'H' for help")
    print("  - Press 'Ctrl+N' to create new texture")
    print("  - Press 'Ctrl+L' to load texture")
    print("  - Press 'Ctrl+E' to export texture (.h)")
    print("  - Press 'Ctrl+P' to export as PNG")
    print("  - Press 'Ctrl+I' to import PNG")
    print("="*50 + "\n")
    
    # 如果没有纹理,提示创建
    if not texture_name:
        print("Hint: Use Ctrl+N or 'Load -> CREATE NEW TEXTURE' to create your first texture")
    
    while running:
        now = pygame.time.get_ticks()
        
        # 管理按钮闪烁计时器 - 参考map_editor
        # 按钮索引: Create New=0, Load .h=1, Export .h=2, Import PNG=3, Export PNG=4, Undo=5, Redo=6, Clear=7
        if undo_flash_time and now > undo_flash_time:
            extra_buttons[5].flash = False
            undo_flash_time = 0
        if redo_flash_time and now > redo_flash_time:
            extra_buttons[6].flash = False
            redo_flash_time = 0
        if export_flash_time and now > export_flash_time:
            extra_buttons[2].flash = False  # Export .h
            extra_buttons[4].flash = False  # Export PNG
            export_flash_time = 0
        if load_flash_time and now > load_flash_time:
            extra_buttons[1].flash = False
            load_flash_time = 0
        if clear_flash_time and now > clear_flash_time:
            extra_buttons[7].flash = False
            clear_flash_time = 0
        if create_flash_time and now > create_flash_time:
            extra_buttons[0].flash = False
            create_flash_time = 0
        
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                quit_editor()
            
            elif event.type == pygame.KEYDOWN:
                if create_new_mode:
                    # 创建新纹理输入模式
                    if event.key == pygame.K_RETURN:
                        if input_text:
                            if create_new_texture(input_text):
                                create_new_mode = False
                                load_screen = False
                            input_text = ""
                    elif event.key == pygame.K_ESCAPE:
                        create_new_mode = False
                        load_screen = False
                        input_text = ""
                    elif event.key == pygame.K_BACKSPACE:
                        input_text = input_text[:-1]
                    else:
                        if event.unicode and event.unicode.isprintable() and len(input_text) < 50:
                            input_text += event.unicode
                elif rgb_input_active_field is not None:
                    # RGB input mode - typing in a field
                    if event.key == pygame.K_RETURN or event.key == pygame.K_TAB:
                        # Submit value
                        try:
                            val = int(rgb_input_texts[rgb_input_active_field])
                            val = clamp(val, 0, 255)
                            draw_color[rgb_input_active_field] = val
                            rgb_input_texts[rgb_input_active_field] = str(val)
                        except ValueError:
                            rgb_input_texts[rgb_input_active_field] = str(draw_color[rgb_input_active_field])
                        
                        # Move to next field or deactivate
                        if event.key == pygame.K_TAB:
                            rgb_input_active_field = (rgb_input_active_field + 1) % 3
                        else:
                            rgb_input_active_field = None
                    elif event.key == pygame.K_ESCAPE:
                        # Cancel input
                        rgb_input_texts[rgb_input_active_field] = str(draw_color[rgb_input_active_field])
                        rgb_input_active_field = None
                    elif event.key == pygame.K_BACKSPACE:
                        rgb_input_texts[rgb_input_active_field] = rgb_input_texts[rgb_input_active_field][:-1]
                    else:
                        # Type digit
                        if event.unicode.isdigit() and len(rgb_input_texts[rgb_input_active_field]) < 3:
                            rgb_input_texts[rgb_input_active_field] += event.unicode
                elif show_help:
                    # 帮助模式 - 按 'h' 返回
                    if event.key == pygame.K_h:
                        show_help = False
                        # Update Help button state
                        for b in extra_buttons:
                            if b.name == "Help":
                                b.active = False
                                break
                elif not color_picker_mode and not load_screen:
                    # 正常模式快捷键
                    if event.key == pygame.K_h:
                        show_help = True
                        # Update Help button state
                        for b in extra_buttons:
                            if b.name == "Help":
                                b.active = True
                                break
                    elif event.key == pygame.K_n and (pygame.key.get_mods() & pygame.KMOD_CTRL):
                        # Ctrl+N: 创建新纹理
                        create_new_mode = True
                        load_screen = False
                        input_text = ""
                    elif event.key == pygame.K_l and (pygame.key.get_mods() & pygame.KMOD_CTRL):
                        # Ctrl+L: 加载纹理
                        load_screen = not load_screen
                        create_new_mode = False
                        # Update Load .h button highlighting
                        for b in extra_buttons:
                            if b.name == "Load .h":
                                b.active = load_screen
                                break
                    elif event.key == pygame.K_e and (pygame.key.get_mods() & pygame.KMOD_CTRL):
                        # Ctrl+E: 导出
                        if export_texture():
                            # Flash Export .h button (index 2 in extra_buttons)
                            extra_buttons[2].flash = True
                            export_flash_time = now + 120
                    elif event.key == pygame.K_p and (pygame.key.get_mods() & pygame.KMOD_CTRL):
                        # Ctrl+P: Export as PNG
                        if export_png():
                            print("✓ Exported as PNG")
                    elif event.key == pygame.K_i and (pygame.key.get_mods() & pygame.KMOD_CTRL):
                        # Ctrl+I: Import PNG
                        if import_png():
                            print("✓ PNG imported successfully")
                    elif event.key == pygame.K_k and (pygame.key.get_mods() & pygame.KMOD_CTRL):
                        # Ctrl+K: Color picker
                        color_picker_mode = not color_picker_mode
                    elif event.key == pygame.K_u or (event.key == pygame.K_z and (pygame.key.get_mods() & pygame.KMOD_CTRL)):
                        undo()
                    elif event.key == pygame.K_y and (pygame.key.get_mods() & pygame.KMOD_CTRL):
                        redo()
                    elif event.key == pygame.K_m:
                        mode_buttons[0].handle_click()
                    elif event.key == pygame.K_p and not (pygame.key.get_mods() & pygame.KMOD_CTRL):
                        tool = POINT_TOOL
                        for b in tool_buttons:
                            b.active = (b.value == tool)
                    elif event.key == pygame.K_l and not (pygame.key.get_mods() & pygame.KMOD_CTRL):
                        tool = LINE_TOOL
                        for b in tool_buttons:
                            b.active = (b.value == tool)
                    elif event.key == pygame.K_r:
                        tool_buttons[2].handle_click()
                    elif event.key == pygame.K_e and not (pygame.key.get_mods() & pygame.KMOD_CTRL):
                        tool_buttons[3].handle_click()
                    elif event.key == pygame.K_q:
                        quit_editor()
            
            elif event.type == pygame.MOUSEBUTTONDOWN:
                mx, my = event.pos
                
                if create_new_mode:
                    # 创建新纹理模式 - 忽略鼠标点击
                    pass
                elif load_screen:
                    # 加载屏幕交互
                    if hasattr(draw_load_screen, 'new_button'):
                        if draw_load_screen.new_button.collidepoint(mx, my):
                            # 点击CREATE NEW按钮
                            create_new_mode = True
                            input_text = ""
                    if hasattr(draw_load_screen, 'texture_buttons'):
                        for button_rect, tex_file in draw_load_screen.texture_buttons:
                            if button_rect.collidepoint(mx, my):
                                # 加载选中的纹理
                                if load_rgb_texture(tex_file):
                                    load_screen = False
                                    # Update Load .h button to not be active
                                    for b in extra_buttons:
                                        if b.name == "Load .h":
                                            b.active = False
                                            b.flash = True
                                            load_flash_time = now + 120
                                            break
                                break
                elif mx < TOOLBAR_W:
                    # 工具栏点击
                    undo_flash_time, redo_flash_time, export_flash_time, load_flash_time, clear_flash_time, create_flash_time = handle_button_click(
                        (mx, my), now, undo_flash_time, redo_flash_time, export_flash_time, load_flash_time, clear_flash_time, create_flash_time
                    )
                elif mx >= TOOLBAR_W + TEXTURE_SIZE * PIXEL_SIZE and not show_help and not color_picker_mode:
                    # Color panel interactions
                    # Eyedropper button
                    if hasattr(draw_color_panel, 'eyedropper_btn'):
                        if draw_color_panel.eyedropper_btn.collidepoint(mx, my):
                            mode = PICK_MODE if mode != PICK_MODE else DRAW_MODE
                            for b in mode_buttons:
                                b.active = (b.value == mode) if hasattr(b, 'value') else False
                    
                    # RGB input boxes
                    if hasattr(draw_color_panel, 'rgb_input_boxes'):
                        for i, input_box in enumerate(draw_color_panel.rgb_input_boxes):
                            if input_box.collidepoint(mx, my):
                                rgb_input_active_field = i
                                rgb_input_texts[i] = ""
                                break
                    
                    # RGB sliders
                    if hasattr(draw_color_panel, 'sliders'):
                        for slider_rect, idx in draw_color_panel.sliders:
                            if slider_rect.collidepoint(mx, my):
                                dragging_slider = idx
                                new_val = int(((mx - slider_rect.x) / slider_rect.width) * 255)
                                draw_color[idx] = clamp(new_val, 0, 255)
                                rgb_input_texts[idx] = str(draw_color[idx])
                                break
                    
                    # Color palette
                    if hasattr(draw_color_panel, 'palette_swatches'):
                        for swatch_rect, color in draw_color_panel.palette_swatches:
                            if swatch_rect.collidepoint(mx, my):
                                draw_color = list(color)
                                rgb_input_texts = [str(c) for c in draw_color]
                                break
                elif color_picker_mode:
                    # 颜色选择器交互
                    if hasattr(draw_color_picker, 'sliders'):
                        for slider_rect, idx in draw_color_picker.sliders:
                            if slider_rect.collidepoint(mx, my):
                                dragging_slider = idx
                                new_val = int(((mx - slider_rect.x) / slider_rect.width) * 255)
                                draw_color[idx] = clamp(new_val, 0, 255)
                elif not show_help:
                    # 画布交互
                    x = (mx - TOOLBAR_W) // PIXEL_SIZE
                    y = my // PIXEL_SIZE
                    x = clamp(x, 0, TEXTURE_SIZE - 1)
                    y = clamp(y, 0, TEXTURE_SIZE - 1)
                    
                    if event.button == 1:
                        if mode == PICK_MODE:
                            pick_color_from_texture(x, y)
                            rgb_input_texts = [str(c) for c in draw_color]
                            # Auto-deactivate eyedropper after picking
                            mode = DRAW_MODE
                        elif tool == POINT_TOOL:
                            color = [255, 255, 255] if mode == ERASE_MODE else draw_color
                            handle_draw(x, y, color)
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
            
            elif event.type == pygame.MOUSEMOTION:
                if dragging_slider is not None:
                    # Dragging color slider in color panel
                    if hasattr(draw_color_panel, 'sliders'):
                        slider_rect, idx = draw_color_panel.sliders[dragging_slider]
                        mx, my = event.pos
                        new_val = int(((mx - slider_rect.x) / slider_rect.width) * 255)
                        draw_color[idx] = clamp(new_val, 0, 255)
                        rgb_input_texts[idx] = str(draw_color[idx])
                    elif color_picker_mode and hasattr(draw_color_picker, 'sliders'):
                        # Old color picker mode
                        slider_rect, idx = draw_color_picker.sliders[dragging_slider]
                        mx, my = event.pos
                        new_val = int(((mx - slider_rect.x) / slider_rect.width) * 255)
                        draw_color[idx] = clamp(new_val, 0, 255)
                elif not show_help and not color_picker_mode and not create_new_mode and not load_screen:
                    if tool == LINE_TOOL and dragging_line and line_start:
                        mx = (event.pos[0] - TOOLBAR_W) // PIXEL_SIZE
                        my = event.pos[1] // PIXEL_SIZE
                        mx = clamp(mx, 0, TEXTURE_SIZE - 1)
                        my = clamp(my, 0, TEXTURE_SIZE - 1)
                        mods = pygame.key.get_mods()
                        if mods & pygame.KMOD_SHIFT:
                            mx, my = snap_to_eight_dir(line_start[0], line_start[1], mx, my)
                            mx = clamp(mx, 0, TEXTURE_SIZE - 1)
                            my = clamp(my, 0, TEXTURE_SIZE - 1)
                        line_end = (mx, my)
                    elif tool == RECT_TOOL and dragging_rect and rect_start:
                        mx = (event.pos[0] - TOOLBAR_W) // PIXEL_SIZE
                        my = event.pos[1] // PIXEL_SIZE
                        mx = clamp(mx, 0, TEXTURE_SIZE - 1)
                        my = clamp(my, 0, TEXTURE_SIZE - 1)
                        mods = pygame.key.get_mods()
                        if mods & pygame.KMOD_SHIFT:
                            dx = mx - rect_start[0]
                            dy = my - rect_start[1]
                            length = max(abs(dx), abs(dy))
                            mx = rect_start[0] + (length if dx >= 0 else -length)
                            my = rect_start[1] + (length if dy >= 0 else -length)
                            mx = clamp(mx, 0, TEXTURE_SIZE - 1)
                            my = clamp(my, 0, TEXTURE_SIZE - 1)
                        rect_end = (mx, my)
                    elif tool == ELLIPSE_TOOL and dragging_ellipse and ellipse_start:
                        mx = (event.pos[0] - TOOLBAR_W) // PIXEL_SIZE
                        my = event.pos[1] // PIXEL_SIZE
                        mods = pygame.key.get_mods()
                        if mods & pygame.KMOD_SHIFT:
                            dx = mx - ellipse_start[0]
                            dy = my - ellipse_start[1]
                            length = max(abs(dx), abs(dy))
                            mx = ellipse_start[0] + (length if dx >= 0 else -length)
                            my = ellipse_start[1] + (length if dy >= 0 else -length)
                        ellipse_end = (mx, my)
            
            elif event.type == pygame.MOUSEBUTTONUP:
                if dragging_slider is not None:
                    dragging_slider = None
                elif not show_help and not color_picker_mode and not create_new_mode and not load_screen:
                    if event.button == 1 and dragging_line and line_start and line_end:
                        color = [255, 255, 255] if mode == ERASE_MODE else draw_color
                        line_cells = bresenham_line(line_start[0], line_start[1], line_end[0], line_end[1])
                        handle_line_draw(line_cells, color)
                        line_start, line_end = None, None
                        dragging_line = False
                    elif event.button == 1 and dragging_rect and rect_start and rect_end:
                        color = [255, 255, 255] if mode == ERASE_MODE else draw_color
                        rx0, ry0 = rect_start
                        rx1, ry1 = rect_end
                        handle_rect_draw(rx0, ry0, rx1, ry1, rect_mode, color)
                        rect_start, rect_end = None, None
                        dragging_rect = False
                    elif event.button == 1 and dragging_ellipse and ellipse_start and ellipse_end:
                        color = [255, 255, 255] if mode == ERASE_MODE else draw_color
                        ex0, ey0 = ellipse_start
                        ex1, ey1 = ellipse_end
                        handle_ellipse_draw(ex0, ey0, ex1, ey1, rect_mode, color)
                        ellipse_start, ellipse_end = None, None
                        dragging_ellipse = False
        
        # 渲染
        screen.fill((0, 0, 0))
        
        if show_help:
            draw_help()
        elif create_new_mode:
            draw_create_new()
        elif load_screen:
            draw_load_screen()
        elif color_picker_mode:
            draw_color_picker()
        else:
            draw_texture()
            # Always show color panel when editing
            draw_color_panel()
        
        # 工具栏始终显示
        draw_toolbar()
        
        pygame.display.flip()
        clock.tick(60)
    
    pygame.quit()

if __name__ == "__main__":
    main()
