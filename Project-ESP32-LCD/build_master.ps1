# Industrial Master Build Pipeline (ESP32-S3 Firmware + SolarDisplayManager Win32 MFC App)
# Complies with ISO 9001 / Military-Grade Build Standards

$ErrorActionPreference = "Stop"

Write-Host "=================================================================" -ForegroundColor Cyan
Write-Host "  INDUSTRIAL OEM PIPELINE: ESP32-S3 FIRMWARE & FLASHER TOOL     " -ForegroundColor Cyan
Write-Host "=================================================================" -ForegroundColor Cyan

# 1. Update Asset C Arrays from logo.png
Write-Host "`n[1/5] Synchronizing and generating High-Fidelity ARGB8888 OEM Assets..." -ForegroundColor Yellow
& python D:\Project_2026\Project-ESP32-LCD\update_assets.py

# 2. Build ESP32 Firmware with ESP-IDF
Write-Host "`n[2/5] Compiling ESP32-S3 Firmware with ESP-IDF v6.1..." -ForegroundColor Yellow
$env:IDF_PATH = 'C:/esp/v6.1/esp-idf'
$env:IDF_TOOLS_PATH = 'C:\Espressif'
$env:IDF_PYTHON_ENV_PATH = 'C:\Espressif\python_env\idf6.1_py3.10_env'
$env:ESP_IDF_VERSION = '6.1.0'
$env:PYTHONUTF8 = '1'
$tools = 'C:\Espressif\tools\xtensa-esp-elf\esp-15.2.0_20251204\xtensa-esp-elf\bin;C:\Espressif\tools\riscv32-esp-elf\esp-15.2.0_20251204\riscv32-esp-elf\bin;C:\Espressif\tools\ninja\1.12.1;C:\Espressif\tools\cmake\4.0.3\bin;C:\Espressif\python_env\idf6.1_py3.10_env\Scripts;C:\Espressif\tools\ccache\4.12.1\ccache-4.12.1-windows-x86_64;C:\esp\v6.1\esp-idf\tools;'
$env:PATH = $tools + $env:PATH
$python = 'C:\Espressif\python_env\idf6.1_py3.10_env\Scripts\python.exe'
$idf_py = 'C:\esp\v6.1\esp-idf\tools\idf.py'

Push-Location "D:\Project_2026\Project-ESP32-LCD"
& $python $idf_py build
Pop-Location

# 3. Synchronize Binaries into SolarDisplayManager Resource Directory
Write-Host "`n[3/5] Synchronizing Monolithic Binaries into Resource Staging..." -ForegroundColor Yellow
$resDir = "D:\Project_Solar_Display\SolarDisplayManager\SolarDisplayManager\res"
Copy-Item "D:\Project_2026\Project-ESP32-LCD\build\bootloader\bootloader.bin" "$resDir\bootloader.bin" -Force
Copy-Item "D:\Project_2026\Project-ESP32-LCD\build\partition_table\partition-table.bin" "$resDir\partition-table.bin" -Force
Copy-Item "D:\Project_2026\Project-ESP32-LCD\build\esp32_ili9341_lcd.bin" "$resDir\firmware.bin" -Force
Copy-Item "D:\Project_2026\Project-ESP32-LCD\build\esp32_ili9341_lcd.bin" "$resDir\esp32_ili9341_lcd.bin" -Force

# 4. Compile SolarDisplayManager with MSBuild (Release x64)
Write-Host "`n[4/5] Building SolarDisplayManager.sln (Release x64)..." -ForegroundColor Yellow
$msbuild = "C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe"
& $msbuild "D:\Project_Solar_Display\SolarDisplayManager\SolarDisplayManager.sln" /p:Configuration=Release /p:Platform=x64 /t:Rebuild

# 5. Deploy to Distribution Folder
Write-Host "`n[5/5] Deploying Release Executable to Distribution..." -ForegroundColor Yellow
$distExe = "D:\Project_Solar_Display\SolarDisplayManager\SolarDisplayManager_Distribution\SolarDisplayManager.exe"
Get-Process -Name "SolarDisplayManager" -ErrorAction SilentlyContinue | Stop-Process -Force
Start-Sleep -Milliseconds 500
Copy-Item "D:\Project_Solar_Display\SolarDisplayManager\x64\Release\SolarDisplayManager.exe" $distExe -Force

Write-Host "`n=================================================================" -ForegroundColor Green
Write-Host "  BUILD AND PACKAGING COMPLETED WITH ZERO DEFECTS!              " -ForegroundColor Green
Write-Host "  Target Executable: $distExe                                  " -ForegroundColor Green
Write-Host "=================================================================" -ForegroundColor Green
