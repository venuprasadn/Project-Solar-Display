import os
from PIL import Image

im = Image.open(r'D:\Project_Solar_Display\logo.png').convert('RGBA')
bbox = im.getbbox()
cropped = im.crop(bbox)
targetW, targetH = 52, 52
scaled = cropped.resize((targetW, targetH), Image.Resampling.LANCZOS)

pixel_bytes = []
for y in range(targetH):
    for x in range(targetW):
        r, g, b, a = scaled.getpixel((x, y))
        pixel_bytes.extend([b, g, r, a])

lines = []
lines.append('/* Generated Master OEM Logo (52x52 TrueColor ARGB8888) */')
lines.append('#include "lvgl.h"')
lines.append('')
lines.append('static const uint8_t master_oem_logo_map[52 * 52 * 4] = {')

for i in range(0, len(pixel_bytes), 16):
    chunk = pixel_bytes[i:i+16]
    hex_str = ', '.join([f'0x{b:02X}' for b in chunk])
    comma = ',' if i + 16 < len(pixel_bytes) else ''
    lines.append(f'    {hex_str}{comma}')

lines.append('};')
lines.append('')
lines.append('const lv_image_dsc_t master_oem_logo_dsc = {')
lines.append('    .header = {')
lines.append('        .magic = LV_IMAGE_HEADER_MAGIC,')
lines.append('        .cf = LV_COLOR_FORMAT_ARGB8888,')
lines.append('        .flags = 0,')
lines.append('        .w = 52,')
lines.append('        .h = 52,')
lines.append('        .stride = 52 * 4,')
lines.append('        .reserved_2 = 0,')
lines.append('    },')
lines.append('    .data_size = sizeof(master_oem_logo_map),')
lines.append('    .data = master_oem_logo_map,')
lines.append('    .reserved = NULL,')
lines.append('    .reserved_2 = NULL,')
lines.append('};')
lines.append('')

with open(r'D:\Project_2026\Project-ESP32-LCD\main\assets\master_oem_logo.c', 'w', encoding='utf-8') as f:
    f.write('\n'.join(lines))

with open(r'D:\Project_2026\Project-ESP32-LCD\main\assets\master_oem_logo.h', 'w', encoding='utf-8') as f:
    f.write('#pragma once\n#include "lvgl.h"\nextern const lv_image_dsc_t master_oem_logo_dsc;\n')

print('Generated master_oem_logo successfully!')
