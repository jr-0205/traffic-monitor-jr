param(
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Release'
)

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
$project = Join-Path $repoRoot 'src\TrafficMonitorJr.App\TrafficMonitorJr.App.csproj'
$output = Join-Path $repoRoot 'artifacts\win-x64'

if (Test-Path $output) {
    Remove-Item $output -Recurse -Force
}

dotnet publish $project `
    -c $Configuration `
    -r win-x64 `
    --self-contained true `
    -p:PublishSingleFile=true `
    -p:IncludeNativeLibrariesForSelfExtract=true `
    -o $output

$exe = Join-Path $output 'TrafficMonitorJr.exe'
if (-not (Test-Path $exe)) {
    throw 'No se generó TrafficMonitorJr.exe.'
}

Write-Host ''
Write-Host 'Build correcto:' -ForegroundColor Green
Write-Host $exe
Get-FileHash $exe -Algorithm SHA256
