param(
    [string]$Port = ""
)

& 'C:\Espressif\tools\Microsoft.v6.1.PowerShell_profile.ps1'

if ($Port -ne "") {
    idf.py -p $Port flash
} else {
    idf.py flash
}
