import os
from PIL import Image

def generate():
    logo_path = r'D:\Project_Solar_Display\logo.png'
    if not os.path.exists(logo_path):
        print(f"Error: {logo_path} not found")
        return False

    im = Image.open(logo_path).convert('RGBA')
    bbox = im.getbbox()
    if bbox:
        cropped = im.crop(bbox)
    else:
        cropped = im

    # 52x52 Master OEM Logo (for Boot Splash & Vendor Screen)
    w52, h52 = 52, 52
    aspect = cropped.width / cropped.height
    if aspect > 1.0:
        dw = w52
        dh = int(round(w52 / aspect))
    else:
        dh = h52
        dw = int(round(h52 * aspect))
    
    scaled52 = cropped.resize((dw, dh), Image.Resampling.LANCZOS)
    canvas52 = Image.new('RGBA', (w52, h52), (0, 0, 0, 0))
    ox = (w52 - dw) // 2
    oy = (h52 - dh) // 2
    canvas52.paste(scaled52, (ox, oy))

    pixel_bytes52 = []
    for y in range(h52):
        for x in range(w52):
            r, g, b, a = canvas52.getpixel((x, y))
            pixel_bytes52.extend([b, g, r, a])

    lines52 = [
        '/* Generated Master OEM Logo (52x52 TrueColor ARGB8888) */',
        '#include "lvgl.h"',
        '',
        'static const uint8_t master_oem_logo_map[52 * 52 * 4] = {'
    ]
    for i in range(0, len(pixel_bytes52), 16):
        chunk = pixel_bytes52[i:i+16]
        hex_str = ', '.join([f'0x{b:02X}' for b in chunk])
        comma = ',' if i + 16 < len(pixel_bytes52) else ''
        lines52.append(f'    {hex_str}{comma}')
    lines52.extend([
        '};',
        '',
        'const lv_image_dsc_t master_oem_logo_dsc = {',
        '    .header = {',
        '        .magic = LV_IMAGE_HEADER_MAGIC,',
        '        .cf = LV_COLOR_FORMAT_ARGB8888,',
        '        .flags = 0,',
        '        .w = 52,',
        '        .h = 52,',
        '        .stride = 52 * 4,',
        '        .reserved_2 = 0,',
        '    },',
        '    .data_size = sizeof(master_oem_logo_map),',
        '    .data = master_oem_logo_map,',
        '    .reserved = NULL,',
        '    .reserved_2 = NULL,',
        '};',
        ''
    ])

    with open(r'D:\Project_2026\Project-ESP32-LCD\main\assets\master_oem_logo.c', 'w', encoding='utf-8') as f:
        f.write('\n'.join(lines52))

    print("SUCCESS: Master OEM Logo (52x52) generated from logo.png!")
    return True

if __name__ == '__main__':
    generate()
