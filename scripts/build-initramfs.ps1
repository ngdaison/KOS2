[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'

$projectRoot = Split-Path -Parent $PSScriptRoot
$sourceFile = Join-Path $projectRoot 'assets\initramfs\README.TXT'
$buildDirectory = Join-Path $projectRoot 'build'
$kernelElf = Join-Path $buildDirectory 'kos.elf'
$outputFile = Join-Path $buildDirectory 'initramfs.cpio'

if (-not (Test-Path -LiteralPath $sourceFile)) {
    throw "Initramfs source is missing: $sourceFile"
}
if (-not (Test-Path -LiteralPath $kernelElf)) {
    throw "Kernel ELF is missing: $kernelElf"
}

$archiveBytes = [System.Collections.Generic.List[byte]]::new()

function Add-ArchiveBytes([byte[]]$bytes) {
    $archiveBytes.AddRange($bytes)
}

function Align-Archive([int]$alignment) {
    while (($archiveBytes.Count % $alignment) -ne 0) {
        $archiveBytes.Add(0)
    }
}

function Add-NewcEntry([string]$path, [byte[]]$contents, [uint32]$mode) {
    $pathBytes = [System.Text.Encoding]::ASCII.GetBytes($path)
    $fields = @(
        [uint32]1, $mode, [uint32]0, [uint32]0, [uint32]1, [uint32]0,
        [uint32]$contents.Length, [uint32]0, [uint32]0, [uint32]0, [uint32]0,
        [uint32]($pathBytes.Length + 1), [uint32]0
    )
    $header = '070701'
    foreach ($field in $fields) {
        $header += ('{0:X8}' -f $field)
    }
    Add-ArchiveBytes ([System.Text.Encoding]::ASCII.GetBytes($header))
    Add-ArchiveBytes $pathBytes
    $archiveBytes.Add(0)
    Align-Archive 4
    Add-ArchiveBytes $contents
    Align-Archive 4
}

$readmeBytes = [System.IO.File]::ReadAllBytes($sourceFile)
Add-NewcEntry 'README.TXT' $readmeBytes 0x81A4
$kernelElfBytes = [System.IO.File]::ReadAllBytes($kernelElf)
Add-NewcEntry 'KOS.ELF' $kernelElfBytes 0x81A4
Add-NewcEntry 'TRAILER!!!' ([byte[]]@()) 0

New-Item -ItemType Directory -Path $buildDirectory -Force | Out-Null
[System.IO.File]::WriteAllBytes($outputFile, $archiveBytes.ToArray())
Write-Host "Initramfs built: $outputFile" -ForegroundColor Green
