=============================================================================
 DONPOWER Solar PCU Display Manager - Production Deployment Package v2.0
 High-Speed 2.8" SPI TFT Color HMI & Brand Provisioning Tool
=============================================================================

1. SYSTEM REQUIREMENTS:
   - Operating System: Windows 10 or Windows 11 (64-bit)
   - Zero external dependencies: NO Python, NO virtualenv, NO SDK required!
   - Standalone Espressif Native Flasher: esptools\esptool.exe

2. FOLDER STRUCTURE:
   SolarDisplayManager.exe        -> Main GUI Provisioning & Flashing Utility
   test_logo.png                  -> Sample 48x48 energy logo image
   README.txt                     -> This instruction file
   esptools/
   └── esptool.exe               -> Zero-dependency Native Espressif Flasher (x64)
   firmware/
   ├── esp32_ili9341_lcd.bin     -> 6-Screen Color Dashboard + Boot Animation Firmware
   ├── bootloader.bin            -> ESP32 2nd Stage Bootloader
   ├── partition-table.bin       -> Production Partition Table (NVS, App)
   └── logo_image.bin            -> Converted 48x48 RGB565 custom logo binary

3. CUSTOM LOGO IMAGE (BROWSE & FLASH):
   - You can browse ANY small image format: PNG, JPG, JPEG, BMP, or ICO.
   - Click "Browse..." next to Custom Logo to select your image file.
   - The tool automatically scales the image to a crisp 48x48 icon with
     aspect ratio preservation and dark theme blending (#0c1524).
   - Click "Flash Logo" to immediately transfer and commit your custom logo to
     the ESP32 over USB. The display will instantly reboot and showcase your
     custom logo inside:
       * The Animated Boot Splash Screen (inside the rotating glowing ring)
       * Screen 6: OEM Vendor & Brand Showcase card
   - Click "Clear Logo" at any time to restore the factory default solar emblem.
   - When using "Flash Firmware" with "Auto-Provision", the custom logo is also
     automatically burned into flash at 0x110000.

4. 6 DASHBOARD SCREENS:
   - Screen 1: Energy Flow Hub (5-node real-time power routing diagram)
   - Screen 2: Solar PV Charger & MPPT Boost Telemetry
   - Screen 3: Battery Capacity & Inverter Load Status
   - Screen 4: System Operational Status, Thermals & Safety Grid Diag
   - Screen 5: Solar Harvest & Yield (Today kWh, Peak Watts, Lifetime MWh,
               Hourly Generation Bar Chart for past hours, CO2 Offset)
   - Screen 6: OEM Vendor & Brand Showcase (Large glowing logo badge,
               Brand Title, Model Name, Serial, Support Contact, Website, QC Passed)

5. HARDWARE PINOUT (ESP32 to 2.8" ILI9341 SPI TFT):
   - LCD Pin 1 (VCC)   -> ESP32 VIN (5V power rail)
   - LCD Pin 2 (GND)   -> ESP32 GND
   - LCD Pin 3 (CS)    -> ESP32 GPIO 19
   - LCD Pin 4 (RESET) -> ESP32 GPIO 21
   - LCD Pin 5 (DC)    -> ESP32 GPIO 18
   - LCD Pin 6 (MOSI)  -> ESP32 GPIO 23
   - LCD Pin 7 (SCK)   -> ESP32 GPIO 22
   - LCD Pin 8 (LED)   -> ESP32 GPIO 4 (Backlight control)
   - Inverter Serial   -> ESP32 GPIO 16 (RX2), GPIO 17 (TX2)
   - USB PC Port       -> ESP32 GPIO 1 (TX0), GPIO 3 (RX0) - 24/7 Service Protocol

6. USAGE INSTRUCTIONS:
   - Connect the ESP32 Display board to your PC via USB.
   - Run "SolarDisplayManager.exe".
   - Select the target COM Port (e.g., COM3).
   - Set your Brand Title, Model Name, Helpline, and Website.
   - Click "Browse..." under Custom Logo to pick your company logo (e.g. test_logo.png).
   - Click "Flash Logo" to flash the logo to the display, or click "Flash Firmware"
     to flash the entire firmware and auto-provision branding + logo all at once!

=============================================================================
