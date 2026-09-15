[CmdletBinding()]
param(
    [string]$LimineRepository = 'https://github.com/limine-bootloader/limine.git',
    [string]$ProtocolRepository = 'https://github.com/limine-bootloader/limine-protocol.git',
    [string]$LimineVersion = 'v12.9.0',
    [switch]$SkipBinary
)

$ErrorActionPreference = 'Stop'

$git = Get-Command git -ErrorAction SilentlyContinue
if ($null -eq $git) {
    throw 'Git is required. Install Git, then run this script again.'
}

$projectRoot = Split-Path -Parent $PSScriptRoot
$thirdParty = Join-Path $projectRoot 'third_party'
$limineSource = Join-Path $thirdParty 'limine'
$protocolSource = Join-Path $thirdParty 'limine-protocol'
$binaryDestination = Join-Path $thirdParty 'limine-binary'

function Get-DirectoryHasContent([string]$path) {
    if (-not (Test-Path -LiteralPath $path)) {
        return $false
    }
    return $null -ne (Get-ChildItem -LiteralPath $path -Force | Select-Object -First 1)
}

New-Item -ItemType Directory -Path $thirdParty -Force | Out-Null

if (-not (Get-DirectoryHasContent $limineSource)) {
    Write-Host "Cloning Limine $LimineVersion into $limineSource" -ForegroundColor Cyan
    & $git.Source clone --depth 1 --branch $LimineVersion $LimineRepository $limineSource
    if ($LASTEXITCODE -ne 0) {
        throw 'Limine source clone failed.'
    }
}
else {
    Write-Host "Limine source already exists: $limineSource" -ForegroundColor Yellow
}

if (-not (Get-DirectoryHasContent $protocolSource)) {
    Write-Host "Cloning Limine protocol headers into $protocolSource" -ForegroundColor Cyan
    & $git.Source clone --depth 1 $ProtocolRepository $protocolSource
    if ($LASTEXITCODE -ne 0) {
        throw 'Limine protocol clone failed.'
    }
}
else {
    Write-Host "Limine protocol headers already exist: $protocolSource" -ForegroundColor Yellow
}

if ($SkipBinary) {
    Write-Host 'Skipped the Limine UEFI binary download by request.' -ForegroundColor Yellow
    exit 0
}

if (Get-DirectoryHasContent $binaryDestination) {
    Write-Host "Limine UEFI binary already exists: $binaryDestination" -ForegroundColor Yellow
    exit 0
}

$temporaryDirectory = Join-Path $env:TEMP ('kos-limine-' + [guid]::NewGuid().ToString('N'))
$archivePath = Join-Path $temporaryDirectory 'limine-binary.zip'
$expandedDirectory = Join-Path $temporaryDirectory 'expanded'
$releaseUrl = "https://github.com/limine-bootloader/limine/releases/download/$LimineVersion/limine-binary.zip"

New-Item -ItemType Directory -Path $temporaryDirectory | Out-Null
try {
    Write-Host "Downloading Limine $LimineVersion UEFI binary" -ForegroundColor Cyan
    Invoke-WebRequest -Uri $releaseUrl -OutFile $archivePath
    Expand-Archive -LiteralPath $archivePath -DestinationPath $expandedDirectory

    $binaryRoot = Join-Path $expandedDirectory 'limine-binary'
    $bootExecutable = Join-Path $binaryRoot 'BOOTX64.EFI'
    if (-not (Test-Path -LiteralPath $bootExecutable)) {
        throw 'The Limine release does not contain BOOTX64.EFI.'
    }

    New-Item -ItemType Directory -Path $binaryDestination -Force | Out-Null
    Copy-Item -Path (Join-Path $binaryRoot '*') -Destination $binaryDestination -Recurse
}
finally {
    if (Test-Path -LiteralPath $temporaryDirectory) {
        Remove-Item -LiteralPath $temporaryDirectory -Recurse -Force
    }
}

Write-Host 'Limine source, protocol headers, and UEFI boot binary are available.' -ForegroundColor Green
