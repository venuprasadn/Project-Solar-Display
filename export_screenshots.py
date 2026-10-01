import os
import time
from selenium import webdriver
from selenium.webdriver.common.by import By
from selenium.webdriver.chrome.options import Options
from PIL import Image

def main():
    print("Starting precision screenshot generator...")
    base_dir = r"D:\Project_2026\Project-ESP32-LCD"
    output_dir = os.path.join(base_dir, "screenshots")
    os.makedirs(output_dir, exist_ok=True)

    html_file = os.path.join(base_dir, "demo_dashboard.html")
    file_url = "file:///" + html_file.replace("\\", "/")

    opts = Options()
    opts.add_argument("--headless=new")
    opts.add_argument("--disable-gpu")
    opts.add_argument("--no-sandbox")
    opts.add_argument("--window-size=1920,1200")
    opts.add_argument("--force-device-scale-factor=1")

    driver = webdriver.Chrome(options=opts)
    try:
        driver.get(file_url)
        time.sleep(2) # wait for fonts & tailwind

        # Pause autoPlay and clear live clock timer
        driver.execute_script("""
            clearInterval(timer);
            if (window.clockTimer) clearInterval(window.clockTimer);
            autoPlay = false;
        """)

        pages = [
            ("page_1_energy_flow_hub", "Page 1 - Energy Flow Hub", 0, "00:48:00"),
            ("page_2_solar_pv_charger", "Page 2 - Solar PV & Charger", 1, "00:48:05"),
            ("page_3_battery_inverter", "Page 3 - Battery & Inverter Load", 2, "00:47:50"),
            ("page_4_operational_status", "Page 4 - Operational Status Board", 3, "00:47:55"),
        ]

        screen_elem = driver.find_element(By.ID, "screen")

        generated_hd = []
        generated_lcd = []

        for name, title, idx, clock_time in pages:
            driver.execute_script(f"""
                setPage({idx});
                document.getElementById('rtc-clock').textContent = '{clock_time}';
            """)
            time.sleep(0.5)

            # Capture High-Resolution PNG
            hd_filename = f"{name}_hd.png"
            hd_path = os.path.join(output_dir, hd_filename)
            screen_elem.screenshot(hd_path)
            generated_hd.append(hd_path)
            print(f"Captured: {hd_filename}")

            # Resize to physical LCD 320x240 resolution
            img = Image.open(hd_path)
            lcd_img = img.resize((320, 240), Image.Resampling.LANCZOS)
            lcd_filename = f"{name}_320x240.png"
            lcd_path = os.path.join(output_dir, lcd_filename)
            lcd_img.save(lcd_path, format="PNG")
            generated_lcd.append(lcd_path)
            print(f"Captured: {lcd_filename}")

        # Create 2x2 Collage of High-Res images
        img1 = Image.open(generated_hd[0])
        w, h = img1.size
        gap = 20
        collage_w = (w * 2) + (gap * 3)
        collage_h = (h * 2) + (gap * 3)
        collage = Image.new("RGBA", (collage_w, collage_h), (11, 15, 25, 255)) # Dark slate matte border

        coords = [
            (gap, gap),
            (w + (gap * 2), gap),
            (gap, h + (gap * 2)),
            (w + (gap * 2), h + (gap * 2))
        ]

        for i in range(4):
            im = Image.open(generated_hd[i])
            collage.paste(im, coords[i])

        collage_path = os.path.join(output_dir, "all_4_screens_collage.png")
        collage.save(collage_path, format="PNG")
        print(f"Generated 2x2 Collage: {collage_path}")

        # Mirror files to root directory
        for f in os.listdir(output_dir):
            src = os.path.join(output_dir, f)
            dst = os.path.join(base_dir, f)
            if os.path.isfile(src):
                img_copy = Image.open(src)
                img_copy.save(dst)
        print("All PNG files mirrored to root workspace directory.")

    finally:
        driver.quit()

if __name__ == "__main__":
    main()
