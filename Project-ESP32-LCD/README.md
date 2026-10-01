# DONPOWER Solar Hybrid PCU — Front-Panel Color HMI

Production-grade front-panel HMI display firmware running on **ESP32-DevKit v1** paired with a **2.8" ILI9341 SPI TFT LCD (240xRGBx320 V1.1)** in **Landscape mode (320x240)**.

Designed for commercial Solar Hybrid PCU inverters to render telemetry, energy routing flows, and battery/inverter health in real time.

---

## 📌 Hardware Architecture & Pinout

### 1. ILI9341 Display Connection (SPI2 / HSPI)
| ILI9341 Pin | ESP32 GPIO | Mode / Function | Hardware Notes |
| :--- | :--- | :--- | :--- |
| **MOSI / SDI** | **GPIO 23** | SPI Master Out | Hardware SPI Data Line |
| **CLK / SCK** | **GPIO 22** | SPI Clock | 40 MHz SPI Clock |
| **CS** | **GPIO 19** | SPI Chip Select | Active LOW |
| **DC / RS** | **GPIO 3** | Data / Command | Reset from UART0 RX via `gpio_reset_pin()` |
| **RESET / RST** | **GPIO 21** | Hardware Reset | Active LOW |
| **LED / BLK** | **GPIO 1** | Backlight Control | Active HIGH driven, reset from UART0 TX |
| **GND** | **GND** | Ground | Common Ground |
| **VCC** | **3.3V / 5V** | Power Supply | 3.3V logic compatible |
| **MISO / SDO** | *NC* | Not Connected | Display is write-only |

> ⚠️ **GPIO 1 & GPIO 3 Conflict Resolution**:
> GPIO 1 (TX0) and GPIO 3 (RX0) are default ESP32 boot/console pins. To prevent console UART signals from corrupting LCD commands or flickering the backlight:
> 1. `CONFIG_ESP_CONSOLE_NONE=y` is set in `sdkconfig.defaults`.
> 2. `gpio_reset_pin(PIN_NUM_BK_LIGHT)` and `gpio_reset_pin(PIN_NUM_DC)` decouple them from IOMUX upon boot.

### 2. Inverter Telemetry Connection (UART2)
| Function | ESP32 GPIO | Inverter MCU Connection | Baud Rate |
| :--- | :--- | :--- | :--- |
| **Telemetry RX** | **GPIO 16** | TX of Inverter Controller MCU | **9600 baud, 8-N-1** |
| **Telemetry TX** | **GPIO 17** | RX of Inverter MCU (optional commands) | **9600 baud, 8-N-1** |

---

## 🛰️ Inverter Telemetry Serial Protocol

The ESP32 runs a dedicated background task on **Core 0** (`inverter_uart_task`) listening to UART2 on **GPIO 16**.

### Packet Structure (ASCII CSV)
Each line ends with `\n` or `\r\n`:
```text
$PCU,mains,solar,batt,acout,load,chg,disch,dc,heat,onflag,solarstate,chargerstate,dcok,sharemode,batgravity\n
```

### Parameter Map (15 Fields)
| Index | Field | Data Type | Units / Range | Meaning |
| :---: | :--- | :---: | :--- | :--- |
| 1 | `mains` | `float` | `0.0` – `300.0` V | AC Mains Grid Voltage |
| 2 | `solar` | `float` | `0.0` – `150.0` V | Solar PV Array Voltage |
| 3 | `batt` | `float` | `20.0` – `32.0` V | Battery Bank Voltage |
| 4 | `acout` | `float` | `0.0` – `250.0` V | Inverter Output Voltage |
| 5 | `load` | `float` | `0.0` – `120.0` % | Inverter Load Percentage |
| 6 | `chg` | `float` | `0.0` – `60.0` A | Charging Current |
| 7 | `disch` | `float` | `0.0` – `60.0` A | Discharging Current |
| 8 | `dc` | `float` | `0.0` – `450.0` V | Raw DC Boost Bus Voltage |
| 9 | `heat` | `float` | `0.0` – `100.0` °C | Inverter Heatsink Temperature |
| 10 | `onflag` | `int` | `0` = Mains, `1` = Inverter | Power Routing Mode |
| 11 | `solarstate` | `int` | `0` = Off, `1` = On | Solar Generation Active |
| 12 | `chargerstate`| `int` | `0`=Off, `1`=AC, `2`=Solar, `3`=Share | Active Charger State |
| 13 | `dcok` | `int` | `1` = Boost OK, `0` = Show Raw V | DC Boost Voltage Status |
| 14 | `sharemode` | `int` | `0` = Solar Priority, `1` = Share | Solar Sharing Preference |
| 15 | `batgravity` | `int` | `0` = Healthy, `1` = Gravity Full | Battery Specific Gravity / State |

### Example Telemetry Packet
```text
$PCU,228.0,76.5,26.8,230.0,42.0,16.4,0.0,385.0,38.5,1,1,2,1,0,0
```

### Manual Page Jump Command
The inverter MCU can jump directly to any page via serial:
```text
$PAGE,0   # Energy Routing Hub
$PAGE,1   # Solar PV & Charger Deep Dive
$PAGE,2   # Battery Health & Inverter Load
$PAGE,3   # Grid, Thermal & Diagnostics
```

> 💡 **Auto-Simulation Fallback**:
> If no serial packets arrive for over 2 seconds, the dashboard automatically engages smooth sine-wave dynamic simulation so screen gauges, animations, and counters remain lively. As soon as a real packet arrives, it immediately locks to live serial data.

---

## 🖥️ Screen Layout & Page Geometry (320x240 Landscape)

```
+-------------------------------------------------------------+  y = 0
| DONPOWER HYBRID PCU (12pt Gold)         00:00:00 (12pt Cyan)|  h = 26px
+-------------------------------------------------------------+  y = 26
|                                                             |
|                    ACTIVE PAGE CANVAS                       |  h = 190px
|                 (Pages 0, 1, 2, or 3)                       |
|                                                             |
+-------------------------------------------------------------+  y = 216
| ENERGY FLOW HUB (10pt Cyan)            .. - . (4 dots)     |  h = 24px
+-------------------------------------------------------------+  y = 240
```

### Page 0: Energy Routing Hub (5 Power Nodes)
- **Top Row (y: 0, h: 88)**:
  - Solar Card (`100x88`): 32x32 Sun icon, `76.5 V`, `ONLINE` badge.
  - Inverter Card (`106x88`): 32x32 PCU icon, `INV ON` badge, `OUT: 230V`.
  - Mains Grid Card (`100x88`): 32x32 Pylon icon, `228 V`, `GRID OK` badge.
- **Bottom Row (y: 92, h: 88)**:
  - Battery Card (`154x88`): 32x32 Battery icon, `26.8 V`, `CHRG: +16.4A`, mini gauge bar.
  - AC Load Card (`154x88`): 32x32 Home icon, `42% LOAD`, `FEED: SOLAR`, mini gauge bar.

### Page 1: Solar PV & Charger Deep Dive
- **Left Panel (`138x182`)**: 52x52 animated rotating sun arc, 32x32 Sun icon, large `76.5 V` readout, `SOLAR ACTIVE` badge, raw DC readout.
- **Right Panel (`172x182`)**: Structured vertical stack: `CHARGER MODE`, `CHARGE CURRENT` (`16.4 AMPS`), `RAW DC BOOST` (`OK!`), `PRIORITY PREFERENCE` (`PREF: SOLAR`).

### Page 2: Battery Health & Inverter AC Loading
- **Left Panel (`154x182`)**: Battery icon, `26.8 V` (16pt), `CHARGE: +16.4 A`, `GRAVITY: NORMAL`, 8px State-of-Charge gauge bar (`76% FULL`).
- **Right Panel (`154x182`)**: Home icon, `42% LOAD` (16pt), `230 VAC`, `FEED: SOLAR PV`, `INVERTER: ACTIVE`, 8px load consumption gauge bar (`LOAD LEVEL: NORMAL`).

### Page 3: Grid, Thermals & System Operational Status
- **Top-Left (`154x86`)**: Mains Input `228 V`, `50.0 Hz | STABLE`.
- **Top-Right (`154x86`)**: Heatsink Temp `38.5 °C`, thermal bar.
- **Bottom Panel (`314x88`)**: Customer-facing **System Operational Status Board**:
  - `INVERTER: ACTIVE (RUNNING)` | `AC: 50.0 Hz`
  - `PRIORITY: SOLAR FIRST` | `BATTERY: HEALTHY / OK`
  - `DC BOOST: OK (ACTIVE)` | `CHARGER: SOLAR MPPT`
  - `MAINS: 228V STABLE` | `THERMAL: NORMAL COOL`

---

## 🎨 Asset Pipeline (Embedded 32x32 Graphics)

Vector graphic icons are programmatically generated and converted into LVGL v9 native ARGB8888 C structs:
```text
gen_icons.py  -->  main/assets/img_solar.c
                   main/assets/img_battery.c
                   main/assets/img_grid.c
                   main/assets/img_home.c
                   main/assets/img_inverter.c
                   main/assets/img_temp.c
```
To regenerate or modify icons:
```bash
python gen_icons.py
```

---

## ⚙️ LVGL Typography Hierarchy

Enabled in `sdkconfig.defaults`:
- `CONFIG_LV_FONT_MONTSERRAT_10=y` — Badges, units, field headers, footer.
- `CONFIG_LV_FONT_MONTSERRAT_12=y` — Card headers, parameter values, clock.
- `CONFIG_LV_FONT_MONTSERRAT_14=y` — Secondary voltages, load percentages.
- `CONFIG_LV_FONT_MONTSERRAT_16=y` — Primary hero voltages and prominent figures.

---

## 🚀 Build & Flash Commands

### Using ESP-IDF Terminal
```powershell
# Set up ESP-IDF environment:
& 'C:\Espressif\tools\Microsoft.v6.1.PowerShell_profile.ps1'

# Compile project:
idf.py build

# Flash to COM3:
idf.py -p COM3 flash

# Open Serial Monitor:
idf.py -p COM3 monitor
```

### Using Automated PowerShell Script
```powershell
.\flash.ps1 -Port COM3
```
