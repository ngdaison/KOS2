[CmdletBinding()]
param(
    [ValidateRange(64, 4096)]
    [int]$MemoryMB = 256,
    [switch]$GdbWait,
    [switch]$Display
)

$ErrorActionPreference = 'Stop'

$projectRoot = Split-Path -Parent $PSScriptRoot
$espDirectory = Join-Path $projectRoot 'build\esp'
$buildDirectory = Join-Path $projectRoot 'build'
$qemuCandidates = @(
    'C:\Program Files\qemu\qemu-system-x86_64.exe',
    (Get-Command qemu-system-x86_64 -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Source)
)
$firmwareCandidates = @(
    'C:\Program Files\qemu\share\edk2-x86_64-code.fd'
)
$firmwareVariablesCandidates = @(
    'C:\Program Files\qemu\share\edk2-i386-vars.fd'
)

if (-not (Test-Path -LiteralPath $espDirectory)) {
    throw 'UEFI ESP is missing. Run scripts/build.ps1 first.'
}

$qemu = $qemuCandidates | Where-Object { $_ -and (Test-Path -LiteralPath $_) } | Select-Object -First 1
if ($null -eq $qemu) {
    throw 'qemu-system-x86_64 was not found. Install QEMU and run scripts/check-environment.ps1.'
}

$firmware = $firmwareCandidates | Where-Object { Test-Path -LiteralPath $_ } | Select-Object -First 1
if ($null -eq $firmware) {
    throw 'QEMU UEFI firmware was not found. Install the QEMU edk2/OVMF firmware package.'
}

$firmwareVariablesTemplate = $firmwareVariablesCandidates | Where-Object { Test-Path -LiteralPath $_ } | Select-Object -First 1
if ($null -eq $firmwareVariablesTemplate) {
    throw 'QEMU UEFI variable-store template was not found. Install the QEMU edk2/OVMF firmware package.'
}

$firmwareVariables = Join-Path $buildDirectory 'edk2-vars.fd'
if (-not (Test-Path -LiteralPath $firmwareVariables)) {
    Copy-Item -LiteralPath $firmwareVariablesTemplate -Destination $firmwareVariables
}

Push-Location $projectRoot
try {
    $qemuArguments = @(
        '-machine', 'q35', '-m', "$MemoryMB",
        '-drive', "if=pflash,format=raw,readonly=on,file=$firmware",
        '-drive', "if=pflash,format=raw,file=$firmwareVariables",
        '-drive', 'format=raw,file=fat:rw:build/esp',
        '-serial', 'stdio', '-monitor', 'none', '-no-reboot'
    )
    if ($Display) {
        $qemuArguments += @('-display', 'default')
    }
    else {
        $qemuArguments += @('-display', 'none')
    }
    if ($GdbWait) {
        $qemuArguments += @('-S', '-s')
    }

    $displayStatus = if ($Display) { 'the QEMU display window and this terminal' } else { 'this terminal' }
    Write-Host "Starting QEMU. Output appears in $displayStatus; press Ctrl+C to exit." -ForegroundColor Cyan
    & $qemu @qemuArguments
    if ($LASTEXITCODE -ne 0) {
        throw "QEMU exited with code $LASTEXITCODE"
    }
}
finally {
    Pop-Location
}
