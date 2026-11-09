param(
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Release'
)

$ErrorActionPreference = 'Stop'

$repoRoot = Split-Path -Parent $PSScriptRoot
$sourceDir = Join-Path $repoRoot 'src\TrafficMonitorJr.IpPlugin'
$sourceFile = Join-Path $sourceDir 'TrafficMonitorJrIp.cpp'
$outputDir = Join-Path $repoRoot 'payload\runtime\plugins'
$outputDll = Join-Path $outputDir 'TrafficMonitorJrIp.dll'

if (-not (Test-Path -LiteralPath $sourceFile)) {
    throw "No se encontró el código del plugin: $sourceFile"
}

$vswhereCandidates = @(
    (Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'),
    (Join-Path $env:ProgramFiles 'Microsoft Visual Studio\Installer\vswhere.exe')
)
$vswhere = $vswhereCandidates | Where-Object { $_ -and (Test-Path -LiteralPath $_) } | Select-Object -First 1
if (-not $vswhere) {
    throw 'No se encontró vswhere.exe. Instala Visual Studio con la carga "Desktop development with C++" (MSVC).'
}

$vsPath = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $vsPath) {
    throw 'No se encontró Visual Studio con las herramientas de C++ (MSVC).'
}

$vcvars = Join-Path $vsPath 'VC\Auxiliary\Build\vcvars64.bat'
if (-not (Test-Path -LiteralPath $vcvars)) {
    throw "No se encontró vcvars64.bat en: $vcvars"
}

$buildDir = Join-Path $env:TEMP ('TMJrIp-' + [Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Force -Path $buildDir | Out-Null
New-Item -ItemType Directory -Force -Path $outputDir | Out-Null

$optimization = if ($Configuration -eq 'Debug') { '/Od /Zi' } else { '/O2' }
$builtDll = Join-Path $buildDir 'TrafficMonitorJrIp.dll'

$batchFile = Join-Path $env:TEMP ('build-plugin-' + [Guid]::NewGuid().ToString('N') + '.bat')
$batchLines = @(
    '@echo off',
    "call `"$vcvars`" >nul",
    "cd /d `"$buildDir`"",
    ("cl /nologo /LD /EHsc /std:c++17 /W3 /DUNICODE /D_UNICODE $optimization " +
        "/I`"$sourceDir`" `"$sourceFile`" " +
        "/Fe:`"$builtDll`" " +
        "/link /IMPLIB:`"$buildDir\TrafficMonitorJrIp.lib`"")
)
Set-Content -LiteralPath $batchFile -Value $batchLines -Encoding ASCII

try {
    & cmd.exe /c $batchFile
    if ($LASTEXITCODE -ne 0) {
        throw "cl.exe falló con código de salida $LASTEXITCODE."
    }
}
finally {
    Remove-Item -LiteralPath $batchFile -Force -ErrorAction SilentlyContinue
}

if (-not (Test-Path -LiteralPath $builtDll)) {
    throw "No se generó el plugin $builtDll"
}

Copy-Item -LiteralPath $builtDll -Destination $outputDll -Force
Remove-Item -LiteralPath $buildDir -Recurse -Force -ErrorAction SilentlyContinue

Write-Host ''
Write-Host 'Plugin compilado:' -ForegroundColor Green
Write-Host $outputDll
Get-FileHash $outputDll -Algorithm SHA256 | Format-List
