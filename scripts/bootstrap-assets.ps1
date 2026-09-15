[CmdletBinding()]
param(
    [string]$FontRepository = 'https://github.com/dhepper/font8x8.git'
)

$ErrorActionPreference = 'Stop'

$git = Get-Command git -ErrorAction SilentlyContinue
if ($null -eq $git) {
    throw 'Git is required. Install Git, then run this script again.'
}

$projectRoot = Split-Path -Parent $PSScriptRoot
$destination = Join-Path $projectRoot 'third_party\font8x8'
$fontHeader = Join-Path $destination 'font8x8_basic.h'
if (Test-Path -LiteralPath $fontHeader) {
    Write-Host "font8x8 is already available: $destination" -ForegroundColor Yellow
    exit 0
}
if (Test-Path -LiteralPath $destination) {
    $existingItem = Get-ChildItem -LiteralPath $destination -Force | Select-Object -First 1
    if ($null -ne $existingItem) {
        throw "Refusing to overwrite a non-empty asset directory: $destination"
    }
}

Write-Host "Cloning the public-domain font8x8 asset into $destination" -ForegroundColor Cyan
& $git.Source clone --depth 1 $FontRepository $destination
if ($LASTEXITCODE -ne 0) {
    throw 'font8x8 clone failed.'
}
if (-not (Test-Path -LiteralPath $fontHeader)) {
    throw 'font8x8 clone completed but the required basic font header is absent.'
}

Write-Host 'font8x8 basic bitmap font is available.' -ForegroundColor Green
