$ErrorActionPreference = 'Stop'
$appDir = $PSScriptRoot
if (Get-Process TrafficMonitor -ErrorAction SilentlyContinue) {
    Write-Host 'Primero cierra TrafficMonitor desde su icono: clic derecho > Salir (Exit).'
    Read-Host 'Pulsa Enter cuando lo hayas cerrado'
}
if (Get-Process TrafficMonitor -ErrorAction SilentlyContinue) {
    Write-Host 'TrafficMonitor sigue abierto. No se modificó la configuración.'
    Read-Host 'Pulsa Enter para terminar'
    exit 1
}
$configPath = Join-Path $appDir 'config.ini'
$content = [IO.File]::ReadAllText($configPath)
$changes = @{
    language = '3082'
    font_name = 'Segoe UI'
    font_size = '10'
    today_traffic_string = '"Hoy: "'
    memory_string = '"RAM: "'
    hdd_string = '"Disco: "'
    main_board_temp_string = '"Placa: "'
    auto_select = 'true'
}
foreach ($key in $changes.Keys) {
    $content = [regex]::Replace($content, '(?m)^' + $key + '\s*=.*$', $key + ' = ' + $changes[$key])
}
[IO.File]::WriteAllText($configPath, $content, [Text.UTF8Encoding]::new($true))
Start-Process -FilePath (Join-Path $appDir 'TrafficMonitor.exe') -WorkingDirectory $appDir -WindowStyle Hidden
