$env:IDF_PATH = 'C:/esp/v6.1/esp-idf'
$env:IDF_TOOLS_PATH = 'C:\Espressif'
$env:IDF_PYTHON_ENV_PATH = 'C:\Espressif\python_env\idf6.1_py3.10_env'
$env:ESP_IDF_VERSION = '6.1.0'
$env:PYTHONUTF8 = '1'

$tools = 'C:\Espressif\tools\xtensa-esp-elf\esp-15.2.0_20251204\xtensa-esp-elf\bin;C:\Espressif\tools\riscv32-esp-elf\esp-15.2.0_20251204\riscv32-esp-elf\bin;C:\Espressif\tools\ninja\1.12.1;C:\Espressif\tools\cmake\4.0.3\bin;C:\Espressif\python_env\idf6.1_py3.10_env\Scripts;C:\Espressif\tools\ccache\4.12.1\ccache-4.12.1-windows-x86_64;C:\esp\v6.1\esp-idf\tools;'
$env:PATH = $tools + $env:PATH

$python = 'C:\Espressif\python_env\idf6.1_py3.10_env\Scripts\python.exe'
$idf_py = 'C:\esp\v6.1\esp-idf\tools\idf.py'

if ($args.Count -eq 0) {
    & $python $idf_py build
} else {
    & $python $idf_py @args
}
