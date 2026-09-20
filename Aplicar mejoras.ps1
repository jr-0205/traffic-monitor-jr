param(
    [ValidateSet('Apply', 'Restore', 'Status')]
    [string]$Mode = 'Apply'
)

$ErrorActionPreference = 'Stop'
$appDir = $PSScriptRoot
$configPath = Join-Path $appDir 'config.ini'
$exePath = Join-Path $appDir 'TrafficMonitor.exe'
$skinDir = Join-Path $appDir 'skins\JR Modern'
$backupDir = Join-Path $appDir 'backups'
$newLine = [Environment]::NewLine

function Get-IniValue {
    param(
        [string]$Text,
        [string]$Section,
        [string]$Key
    )

    $sectionName = [regex]::Escape($Section)
    $sectionPattern = "(?ms)(^\[$sectionName\]\s*\r?\n)(.*?)(?=^\[|\z)"
    $sectionMatch = [regex]::Match($Text, $sectionPattern)
    if (-not $sectionMatch.Success) { return $null }

    $keyName = [regex]::Escape($Key)
    $keyMatch = [regex]::Match($sectionMatch.Groups[2].Value, "(?m)^$keyName\s*=\s*(.*)$")
    if (-not $keyMatch.Success) { return $null }

    return $keyMatch.Groups[1].Value.Trim()
}

function Set-IniValue {
    param(
        [string]$Text,
        [string]$Section,
        [string]$Key,
        [string]$Value
    )

    $sectionName = [regex]::Escape($Section)
    $sectionPattern = "(?ms)(^\[$sectionName\]\s*\r?\n)(.*?)(?=^\[|\z)"
    $sectionMatch = [regex]::Match($Text, $sectionPattern)

    if (-not $sectionMatch.Success) {
        throw "No se encontró la sección [$Section] en config.ini."
    }

    $body = $sectionMatch.Groups[2].Value
    $keyName = [regex]::Escape($Key)
    $keyPattern = "(?m)^$keyName\s*=.*$"
    $replacement = "$Key = $Value"

    if ([regex]::IsMatch($body, $keyPattern)) {
        $newBody = [regex]::Replace($body, $keyPattern, $replacement)
    }
    else {
        if ($body.Length -gt 0 -and -not $body.EndsWith($newLine)) {
            $body += $newLine
        }
        $newBody = $body + $replacement + $newLine
    }

    $start = $sectionMatch.Groups[2].Index
    $length = $sectionMatch.Groups[2].Length
    return $Text.Substring(0, $start) + $newBody + $Text.Substring($start + $length)
}

function Assert-Files {
    $required = @(
        $exePath,
        $configPath,
        (Join-Path $skinDir 'skin.xml'),
        (Join-Path $skinDir 'background.png'),
        (Join-Path $skinDir 'background_l.png')
    )

    foreach ($file in $required) {
        if (-not (Test-Path -LiteralPath $file)) {
            throw "Falta un archivo requerido: $file"
        }
    }

    [void][xml]([IO.File]::ReadAllText((Join-Path $skinDir 'skin.xml')))
}

function Ensure-TrafficMonitorStopped {
    $process = Get-Process TrafficMonitor -ErrorAction SilentlyContinue
    if (-not $process) { return }

    Write-Host ''
    Write-Host 'TrafficMonitor está abierto.' -ForegroundColor Yellow
    Write-Host 'Ciérralo desde el icono de la bandeja: clic derecho > Salir.'
    [void](Read-Host 'Pulsa Enter cuando lo hayas cerrado')

    if (Get-Process TrafficMonitor -ErrorAction SilentlyContinue) {
        throw 'TrafficMonitor sigue abierto. No se modificó la configuración.'
    }
}

function Start-TrafficMonitor {
    if (-not (Get-Process TrafficMonitor -ErrorAction SilentlyContinue)) {
        Start-Process -FilePath $exePath -WorkingDirectory $appDir -WindowStyle Hidden
    }
}

function Backup-Config {
    if (-not (Test-Path -LiteralPath $backupDir)) {
        [void](New-Item -ItemType Directory -Path $backupDir)
    }

    $stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
    $backupPath = Join-Path $backupDir "config-$stamp.ini"
    Copy-Item -LiteralPath $configPath -Destination $backupPath -Force

    Get-ChildItem -LiteralPath $backupDir -Filter 'config-*.ini' -File |
        Sort-Object LastWriteTime -Descending |
        Select-Object -Skip 10 |
        Remove-Item -Force

    return $backupPath
}

function Apply-Profile {
    Assert-Files
    Ensure-TrafficMonitorStopped

    $backup = Backup-Config
    $content = [IO.File]::ReadAllText($configPath)

    $changes = @(
        @('general', 'language', '3082'),
        @('general', 'monitor_time_span', '1000'),
        @('general', 'show_all_interface', 'false'),

        @('config', 'transparency', '95'),
        @('config', 'always_on_top', 'true'),
        @('config', 'lock_window_pos', 'true'),
        @('config', 'show_notify_icon', 'true'),
        @('config', 'mouse_penetrate', 'true'),
        @('config', 'hide_main_wnd_when_fullscreen', 'true'),
        @('config', 'skin_selected', 'JR Modern'),
        @('config', 'separate_value_unit_with_space', 'true'),
        @('config', 'show_tool_tip', 'true'),

        @('connection', 'auto_select', 'true'),
        @('connection', 'select_all', 'false'),
        @('connection', 'connection_name', ''),

        @('skins', 'skin_auto_adapt', 'false'),

        @('task_bar', 'font_name', 'Segoe UI'),
        @('task_bar', 'font_size', '10'),
        @('task_bar', 'font_style', '0'),
        @('task_bar', 'up_string', '"↑ "'),
        @('task_bar', 'down_string', '"↓ "'),
        @('task_bar', 'cpu_string', '"CPU "'),
        @('task_bar', 'memory_string', '"RAM "'),
        @('task_bar', 'gpu_string', '"GPU "'),
        @('task_bar', 'hdd_string', '"DISCO "'),
        @('task_bar', 'total_speed_string', '"RED "'),
        @('task_bar', 'today_traffic_string', '"HOY "'),
        @('task_bar', 'value_right_align', 'true'),
        @('task_bar', 'show_status_bar', 'false'),
        @('task_bar', 'item_space', '8'),
        @('task_bar', 'auto_set_background_color', 'true'),
        @('task_bar', 'graph_color_following_system', 'true'),
        @('task_bar', 'enable_colorful_emoji', 'false'),

        @('other', 'debug_log', 'false')
    )

    foreach ($change in $changes) {
        $content = Set-IniValue -Text $content -Section $change[0] -Key $change[1] -Value $change[2]
    }

    [IO.File]::WriteAllText($configPath, $content, [Text.UTF8Encoding]::new($true))

    Write-Host ''
    Write-Host 'Perfil JR Modern aplicado.' -ForegroundColor Green
    Write-Host "Respaldo: $backup"
    Start-TrafficMonitor
}

function Restore-Profile {
    Assert-Files
    Ensure-TrafficMonitorStopped

    if (-not (Test-Path -LiteralPath $backupDir)) {
        throw 'No existe la carpeta de respaldos.'
    }

    $backup = Get-ChildItem -LiteralPath $backupDir -Filter 'config-*.ini' -File |
        Sort-Object LastWriteTime -Descending |
        Select-Object -First 1

    if (-not $backup) {
        throw 'No hay respaldos de config.ini disponibles.'
    }

    Copy-Item -LiteralPath $backup.FullName -Destination $configPath -Force
    Write-Host "Configuración restaurada desde: $($backup.Name)" -ForegroundColor Green
    Start-TrafficMonitor
}

function Show-Status {
    Assert-Files
    $content = [IO.File]::ReadAllText($configPath)

    Write-Host 'TrafficMonitorJr - estado'
    Write-Host '------------------------'
    Write-Host "Versión:        $(Get-IniValue -Text $content -Section 'app' -Key 'version')"
    Write-Host "Skin:           $(Get-IniValue -Text $content -Section 'config' -Key 'skin_selected')"
    Write-Host "Auto red:       $(Get-IniValue -Text $content -Section 'connection' -Key 'auto_select')"
    Write-Host "Interfaz fija:  $(Get-IniValue -Text $content -Section 'connection' -Key 'connection_name')"
    Write-Host "Refresco (ms):  $(Get-IniValue -Text $content -Section 'general' -Key 'monitor_time_span')"
    Write-Host "Transparencia:  $(Get-IniValue -Text $content -Section 'config' -Key 'transparency')"
    Write-Host "Ejecutable:     OK"
    Write-Host "Skin JR Modern: OK"
}

try {
    switch ($Mode) {
        'Apply'   { Apply-Profile }
        'Restore' { Restore-Profile }
        'Status'  { Show-Status }
    }
}
catch {
    Write-Host ''
    Write-Host "ERROR: $($_.Exception.Message)" -ForegroundColor Red
    exit 1
}
