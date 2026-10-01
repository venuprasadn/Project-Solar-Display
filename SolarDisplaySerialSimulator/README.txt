========================================================================
 SOLAR DISPLAY SERIAL SIMULATOR (INDUSTRIAL HMI TEST BENCH)
========================================================================

DESCRIPTION:
  High-precision Windows MFC desktop application designed to test, validate,
  and demonstrate the ESP32 Solar Display LCD system over hardware serial
  ports (COMx) without requiring real solar panels or a physical inverter.

OPERATIONAL MODES:

1. MODE 0: INVERTER TELEMETRY TRANSMITTER (Default @ 9600 Baud)
   - Simulates physical Inverter/PCU sending live status frames to the
     ESP32 LCD hardware (ESP32 UART2 GPIO16 RX, GPIO17 TX).
   - Generates production-grade telemetry frames:
     $PCU,<GridV>,<SolarV>,<BattV>,<AcOutV>,<LoadW>,<ChgA>,<DischA>,<DcBoostV>,<InvTempC>,<MainsInvMode>,<PvState>,<ChgState>,<DcOk>,<ShareMode>,<BatGravity>*<CS>
   - Interactive Live Sliders:
     * Mains / Grid Voltage (0 - 300 V)
     * Solar PV Voltage (0 - 150 V)
     * Battery Voltage (0 - 70 V)
     * Inverter AC Output (0 - 300 V)
     * Load Power (0 - 5000 W)
     * Charging Current (0 - 100 A)
     * Discharging Current (0 - 100 A)
     * DC Boost Voltage (0 - 450 V)
     * Heat Sink Temperature (0 - 120 °C)
   - Status & Mode Selectors:
     * Mains / Inverter Mode (0: Mains Mode, 1: Inverter Mode)
     * Solar PV State (0: Off, 1: Active, 2: Float)
     * Charger State (0: Standby, 1: Bulk, 2: Float)
     * DC Bus Status (DC OK / Low DC)
     * Share Mode / Peak Shaving
     * Battery Gravity Equalization
   - Fast Scenario Presets:
     * [Preset: Sunny Harvest]: 230V Grid, 95V Solar, 54.2V Batt, 1250W Load, 35A Chg.
     * [Preset: Night Grid Chg]: 230V Grid, 0V Solar, 51.5V Batt, 800W Load, 15A Chg.
     * [Preset: Grid Outage]: 0V Grid, 85V Solar, 49.8V Batt, 2200W Load, 45A Disch.
     * [Preset: Overload & Heat]: 210V Grid, 40V Solar, 46.2V Batt, 4800W Load, 88°C Heat.
     * [Dynamic Sweep (Wave)]: Generates realistic sine/cosine load fluctuations.
   - Remote LCD Page Switching:
     * Sends $PAGE,0 through $PAGE,5 commands to navigate LCD screens remotely:
       Page 0: System Hub (Flow diagram)
       Page 1: Solar Charger (PV details)
       Page 2: Battery & Inverter (DC analysis)
       Page 3: Operational Diagnostics
       Page 4: 24-Hour Production Graph
       Page 5: Device Identity & QC Specs

2. MODE 1: VIRTUAL ESP32 TARGET ECHO (@ 115200 Baud)
   - Emulates an ESP32 hardware device on a COM port.
   - Allows testing `SolarDisplayManager.exe` (PC Provisioning App) without
     connecting physical ESP32 hardware.
   - Fully responds to:
     * CMD_PING (0x01) -> RESP_ACK
     * CMD_READ_CONFIG (0x02) -> RESP_CONFIG_DATA (Brand, Model, Theme, Serial)
     * CMD_WRITE_CONFIG (0x03) -> RESP_ACK (Updates internal simulated NVS)
     * CMD_COMMIT_NVS (0x04) -> RESP_ACK
     * CMD_WRITE_LOGO_CHUNK (0x08) -> RESP_ACK
     * CMD_COMMIT_LOGO (0x09) -> RESP_ACK
     * CMD_CLEAR_LOGO (0x0A) -> RESP_ACK
     * CMD_FACTORY_RESET (0x05) -> RESP_ACK
     * CMD_REBOOT (0x07) -> RESP_ACK

HOW TO RUN:
  1. Launch SolarDisplaySerialSimulator.exe.
  2. Select target COM port and baud rate (9600 for Inverter TX, 115200 for Echo).
  3. Click "Connect".
  4. Adjust sliders or click Scenario Presets to inject data in real-time.
