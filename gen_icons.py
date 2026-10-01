import os
import struct
import math
from PIL import Image, ImageDraw

def create_solar_icon():
    img = Image.new("RGBA", (32, 32), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    # Sun disk (Amber/Gold)
    draw.ellipse([2, 2, 14, 14], fill=(245, 158, 11, 255), outline=(253, 224, 71, 255), width=1)
    # Sun rays
    for angle in [0, 45, 90, 135, 180, 225, 270, 315]:
        rad = math.radians(angle)
        x1 = 8 + 7 * math.cos(rad)
        y1 = 8 + 7 * math.sin(rad)
        x2 = 8 + 10 * math.cos(rad)
        y2 = 8 + 10 * math.sin(rad)
        draw.line([x1, y1, x2, y2], fill=(251, 191, 36, 255), width=1)
    # Solar panel base
    poly = [(12, 16), (29, 16), (31, 30), (10, 30)]
    draw.polygon(poly, fill=(30, 58, 138, 255), outline=(56, 189, 248, 255))
    # Grid lines
    draw.line([(20, 16), (20, 30)], fill=(56, 189, 248, 255), width=1)
    draw.line([(11, 23), (30, 23)], fill=(56, 189, 248, 255), width=1)
    return img

def create_battery_icon():
    img = Image.new("RGBA", (32, 32), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    # Battery terminal
    draw.rectangle([13, 2, 19, 5], fill=(148, 163, 184, 255))
    # Battery body
    draw.rounded_rectangle([6, 5, 26, 29], radius=3, fill=(15, 23, 42, 255), outline=(16, 185, 129, 255), width=1)
    # Charge bars (Green gradient)
    draw.rounded_rectangle([8, 21, 24, 27], radius=1, fill=(16, 185, 129, 255))
    draw.rounded_rectangle([8, 14, 24, 19], radius=1, fill=(52, 211, 153, 255))
    draw.rounded_rectangle([8, 7, 24, 12], radius=1, fill=(110, 231, 183, 255))
    return img

def create_grid_icon():
    img = Image.new("RGBA", (32, 32), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    # Transmission tower legs
    draw.line([(16, 3), (6, 30)], fill=(148, 163, 184, 255), width=2)
    draw.line([(16, 3), (26, 30)], fill=(148, 163, 184, 255), width=2)
    # Cross bars
    draw.line([(4, 10), (28, 10)], fill=(245, 158, 11, 255), width=1)
    draw.line([(7, 18), (25, 18)], fill=(245, 158, 11, 255), width=1)
    draw.line([(8, 25), (24, 25)], fill=(148, 163, 184, 255), width=1)
    # Lattice X
    draw.line([(10, 10), (22, 18)], fill=(100, 116, 139, 255), width=1)
    draw.line([(22, 10), (10, 18)], fill=(100, 116, 139, 255), width=1)
    return img

def create_home_icon():
    img = Image.new("RGBA", (32, 32), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    # Roof
    draw.polygon([(16, 3), (3, 14), (29, 14)], fill=(239, 68, 68, 255), outline=(248, 113, 113, 255))
    # Chimney
    draw.rectangle([22, 5, 25, 11], fill=(185, 28, 28, 255))
    # Walls
    draw.rectangle([7, 14, 25, 29], fill=(30, 41, 59, 255), outline=(148, 163, 184, 255), width=1)
    # Door
    draw.rectangle([13, 20, 19, 29], fill=(56, 189, 248, 255))
    # Glowing Window
    draw.rectangle([8, 16, 12, 20], fill=(253, 224, 71, 255))
    draw.rectangle([20, 16, 24, 20], fill=(253, 224, 71, 255))
    return img

def create_inverter_icon():
    img = Image.new("RGBA", (32, 32), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    # Inverter chassis
    draw.rounded_rectangle([4, 4, 28, 28], radius=4, fill=(15, 23, 42, 255), outline=(56, 189, 248, 255), width=1)
    # Heatsink fins
    for y in [8, 11, 14]:
        draw.line([(7, y), (25, y)], fill=(51, 65, 85, 255), width=1)
    # Waveform / Sine symbol in center
    points = [(7, 22), (10, 18), (13, 22), (16, 26), (19, 22), (22, 18), (25, 22)]
    for i in range(len(points)-1):
        draw.line([points[i], points[i+1]], fill=(16, 185, 129, 255), width=1)
    # Status LED
    draw.ellipse([23, 6, 26, 9], fill=(16, 185, 129, 255))
    return img

def create_temp_icon():
    img = Image.new("RGBA", (32, 32), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    # Thermometer tube
    draw.rounded_rectangle([13, 3, 19, 21], radius=3, fill=(30, 41, 59, 255), outline=(148, 163, 184, 255), width=1)
    # Bulb
    draw.ellipse([10, 19, 22, 30], fill=(239, 68, 68, 255), outline=(148, 163, 184, 255), width=1)
    # Red liquid column
    draw.rectangle([14, 10, 18, 21], fill=(239, 68, 68, 255))
    # Tick marks
    draw.line([(20, 6), (22, 6)], fill=(148, 163, 184, 255), width=1)
    draw.line([(20, 11), (22, 11)], fill=(148, 163, 184, 255), width=1)
    draw.line([(20, 16), (22, 16)], fill=(148, 163, 184, 255), width=1)
    return img

def image_to_lvgl_c(img, name, out_path):
    w, h = img.size
    data = []
    for y in range(h):
        for x in range(w):
            r, g, b, a = img.getpixel((x, y))
            # LVGL ARGB8888 in little-endian order: B, G, R, A
            data.extend([b, g, r, a])
    
    with open(out_path, "w") as f:
        f.write('#include "lvgl.h"\n\n')
        f.write(f'const uint8_t {name}_map[] = {{\n')
        for i in range(0, len(data), 16):
            chunk = data[i:i+16]
            f.write("    " + ", ".join(f"0x{b:02x}" for b in chunk) + ",\n")
        f.write("};\n\n")
        f.write(f'const lv_image_dsc_t {name} = {{\n')
        f.write(f'    .header = {{\n')
        f.write(f'        .magic = LV_IMAGE_HEADER_MAGIC,\n')
        f.write(f'        .cf = LV_COLOR_FORMAT_ARGB8888,\n')
        f.write(f'        .flags = 0,\n')
        f.write(f'        .w = {w},\n')
        f.write(f'        .h = {h},\n')
        f.write(f'        .stride = {w * 4},\n')
        f.write(f'        .reserved_2 = 0,\n')
        f.write(f'    }},\n')
        f.write(f'    .data_size = {len(data)},\n')
        f.write(f'    .data = {name}_map,\n')
        f.write(f'}};\n')

icons = {
    "img_solar": create_solar_icon(),
    "img_battery": create_battery_icon(),
    "img_grid": create_grid_icon(),
    "img_home": create_home_icon(),
    "img_inverter": create_inverter_icon(),
    "img_temp": create_temp_icon(),
}

os.makedirs("main/assets", exist_ok=True)
for name, icon in icons.items():
    image_to_lvgl_c(icon, name, f"main/assets/{name}.c")
    print(f"Generated main/assets/{name}.c (32x32)")
