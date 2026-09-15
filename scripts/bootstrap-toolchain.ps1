[CmdletBinding()]
param(
    [string]$NasmVersion = '3.02'
)

$ErrorActionPreference = 'Stop'

$projectRoot = Split-Path -Parent $PSScriptRoot
$toolsDirectory = Join-Path $projectRoot 'third_party\tools'
$nasmDirectory = Join-Path $toolsDirectory ("nasm-$NasmVersion")
$nasmExecutable = Join-Path $nasmDirectory 'nasm.exe'

if (Test-Path -LiteralPath $nasmExecutable) {
    Write-Host "Portable NASM already exists: $nasmExecutable" -ForegroundColor Yellow
    exit 0
}

$downloadUrl = "https://www.nasm.us/pub/nasm/releasebuilds/$NasmVersion/win64/nasm-$NasmVersion-win64.zip"
$temporaryDirectory = Join-Path $env:TEMP ('kos-nasm-' + [guid]::NewGuid().ToString('N'))
$archivePath = Join-Path $temporaryDirectory 'nasm.zip'
$expandedDirectory = Join-Path $temporaryDirectory 'expanded'

New-Item -ItemType Directory -Path $temporaryDirectory | Out-Null
try {
    Write-Host "Downloading portable NASM $NasmVersion" -ForegroundColor Cyan
    Invoke-WebRequest -Uri $downloadUrl -OutFile $archivePath
    Expand-Archive -LiteralPath $archivePath -DestinationPath $expandedDirectory

    $sourceExecutable = Get-ChildItem -LiteralPath $expandedDirectory -Filter 'nasm.exe' -File -Recurse | Select-Object -First 1
    if ($null -eq $sourceExecutable) {
        throw 'The NASM archive did not contain nasm.exe.'
    }

    New-Item -ItemType Directory -Path $nasmDirectory -Force | Out-Null
    Copy-Item -LiteralPath $sourceExecutable.FullName -Destination $nasmExecutable -Force
}
finally {
    if (Test-Path -LiteralPath $temporaryDirectory) {
        Remove-Item -LiteralPath $temporaryDirectory -Recurse -Force
    }
}

Write-Host "Portable NASM is ready: $nasmExecutable" -ForegroundColor Green
