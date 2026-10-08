$env:IDF_PATH = 'C:/esp/v6.1/esp-idf'
$tools = 'C:\Espressif\tools\xtensa-esp-elf\esp-15.2.0_20251204\xtensa-esp-elf\bin;C:\Espressif\tools\ninja\1.12.1;C:\Espressif\tools\cmake\4.0.3\bin;C:\Espressif\python_env\idf6.1_py3.10_env\Scripts;C:\Espressif\tools\ccache\4.12.1\ccache-4.12.1-windows-x86_64;'
$env:PATH = $tools + $env:PATH
& 'C:\Espressif\tools\ninja\1.12.1\ninja.exe' -C 'D:\Project_2026\Project-ESP32-LCD\build'
