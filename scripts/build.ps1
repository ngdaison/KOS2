[CmdletBinding()]
param(
    [switch]$KernelOnly,
    [switch]$PanicTest
)

$ErrorActionPreference = 'Stop'

$projectRoot = Split-Path -Parent $PSScriptRoot
$buildDirectory = Join-Path $projectRoot 'build'
$objectDirectory = Join-Path $buildDirectory 'obj'
$protocolInclude = Join-Path $projectRoot 'third_party\limine-protocol\include'
$fontInclude = Join-Path $projectRoot 'third_party\font8x8'
$limineBinary = Join-Path $projectRoot 'third_party\limine-binary\BOOTX64.EFI'
$initramfsScript = Join-Path $projectRoot 'scripts\build-initramfs.ps1'

function Find-Tool([string[]]$commands, [string[]]$paths, [string]$description) {
    foreach ($commandName in $commands) {
        $command = Get-Command $commandName -ErrorAction SilentlyContinue
        if ($null -ne $command) {
            return $command.Source
        }
    }
    foreach ($candidatePath in $paths) {
        if (Test-Path -LiteralPath $candidatePath) {
            return $candidatePath
        }
    }
    throw "Required tool not found: $description"
}

function Invoke-Tool([string]$tool, [string[]]$arguments) {
    & $tool @arguments
    if ($LASTEXITCODE -ne 0) {
        throw "Command failed: $tool"
    }
}

if (-not (Test-Path -LiteralPath $protocolInclude)) {
    throw 'Limine protocol headers are missing. Run scripts/bootstrap-limine.ps1 first.'
}
if (-not (Test-Path -LiteralPath (Join-Path $fontInclude 'font8x8_basic.h'))) {
    throw 'The public-domain font8x8 asset is missing. Run scripts/bootstrap-assets.ps1 first.'
}
if (-not (Test-Path -LiteralPath $initramfsScript)) {
    throw 'Initramfs builder is missing.'
}

$clang = Find-Tool @('clang') @('C:\Program Files\LLVM\bin\clang.exe') 'Clang'
$lld = Find-Tool @('ld.lld') @('C:\Program Files\LLVM\bin\ld.lld.exe') 'LLD'
$nasm = Find-Tool @('nasm') @('C:\Program Files\NASM\nasm.exe', (Join-Path $projectRoot 'third_party\tools\nasm-3.02\nasm.exe')) 'NASM'
$readobj = Find-Tool @('llvm-readobj') @('C:\Program Files\LLVM\bin\llvm-readobj.exe') 'llvm-readobj'
$nm = Find-Tool @('llvm-nm') @('C:\Program Files\LLVM\bin\llvm-nm.exe') 'llvm-nm'

New-Item -ItemType Directory -Path $objectDirectory -Force | Out-Null

$commonFlags = @(
    '--target=x86_64-unknown-none-elf', '-std=c17', '-ffreestanding', '-fno-stack-protector',
    '-fno-pic', '-fno-pie', '-mno-red-zone', '-mcmodel=kernel', '-mno-mmx', '-mno-sse',
    '-mno-80387', '-ffunction-sections', '-fdata-sections', '-fno-common',
    '-fno-asynchronous-unwind-tables', '-fno-unwind-tables', '-Wall', '-Wextra', '-Werror', '-O2', '-g', '-I',
    (Join-Path $projectRoot 'include'), '-I', $protocolInclude, '-I', $fontInclude
)
if ($PanicTest) {
    $commonFlags += '-DKOS_PANIC_TEST'
}

$cSources = @('boot\limine_requests.c', 'arch\x86_64\cpu_control.c', 'arch\x86_64\descriptors.c', 'arch\x86_64\interrupts.c', 'arch\x86_64\pci.c', 'arch\x86_64\pic.c', 'drivers\driver.c', 'drivers\framebuffer.c', 'drivers\keyboard.c', 'drivers\serial.c', 'drivers\timer.c', 'fs\elf.c', 'fs\initramfs.c', 'fs\vfs.c', 'kernel\command.c', 'kernel\console.c', 'kernel\log.c', 'kernel\main.c', 'kernel\task.c', 'kernel\terminal.c', 'mm\heap.c', 'mm\pmm.c', 'mm\vmm.c')
$objects = @()
foreach ($sourceRelative in $cSources) {
    $source = Join-Path $projectRoot $sourceRelative
    if (-not (Test-Path -LiteralPath $source)) {
        throw "Kernel C source is missing: $sourceRelative"
    }
    $objectName = (($sourceRelative -replace '[\\/]', '__') -replace '\.c$', '.o')
    $object = Join-Path $objectDirectory $objectName
    Invoke-Tool $clang ($commonFlags + @('-c', $source, '-o', $object))
    $objects += $object
}

$assemblySources = @('boot\entry.asm', 'arch\x86_64\cpu.asm', 'arch\x86_64\interrupt_stubs.asm', 'arch\x86_64\task_switch.asm')
foreach ($sourceRelative in $assemblySources) {
    $source = Join-Path $projectRoot $sourceRelative
    if (-not (Test-Path -LiteralPath $source)) {
        throw "Kernel Assembly source is missing: $sourceRelative"
    }
    $objectName = (($sourceRelative -replace '[\\/]', '__') -replace '\.asm$', '.o')
    $object = Join-Path $objectDirectory $objectName
    Invoke-Tool $nasm @('-f', 'elf64', $source, '-o', $object)
    $objects += $object
}

$kernelElf = Join-Path $buildDirectory 'kos.elf'
Invoke-Tool $lld (@('-m', 'elf_x86_64', '-nostdlib', '-static', '-z', 'max-page-size=0x1000',
    '--gc-sections', '--build-id=none', '--fatal-warnings', '-T', (Join-Path $projectRoot 'boot\linker.ld'), '-o', $kernelElf) + $objects)

Write-Host 'Verifying kernel ELF header' -ForegroundColor Cyan
Invoke-Tool $readobj @('--file-headers', $kernelElf)
Write-Host 'Verifying kernel load-segment layout' -ForegroundColor Cyan
$programHeaderOutput = & $readobj '--program-headers' $kernelElf
if ($LASTEXITCODE -ne 0) {
    throw 'llvm-readobj failed while verifying kernel program headers.'
}
$loadSegments = @($programHeaderOutput | Where-Object { $_ -match 'Type: PT_LOAD' })
$executableSegments = @($programHeaderOutput | Where-Object { $_ -match 'PF_X' })
$writableSegments = @($programHeaderOutput | Where-Object { $_ -match 'PF_W' })
if ($loadSegments.Count -ne 4 -or $executableSegments.Count -ne 1 -or $writableSegments.Count -ne 1) {
    throw 'Kernel ELF must contain four load segments with exactly one executable and one writable segment.'
}
Write-Host 'Verifying required entry symbols' -ForegroundColor Cyan
$symbolOutput = & $nm $kernelElf
if ($LASTEXITCODE -ne 0) {
    throw 'llvm-nm failed while verifying kernel symbols.'
}
foreach ($requiredSymbol in @('_start', 'kernel_main', 'kos_boot_stack_top')) {
    $symbolMatches = @($symbolOutput | Where-Object { $_ -match ('\s' + [regex]::Escape($requiredSymbol) + '$') })
    if ($symbolMatches.Count -eq 0) {
        throw "Required symbol is absent from the kernel ELF: $requiredSymbol"
    }
}
Write-Host 'Required boot symbols are present.' -ForegroundColor Green

if ($KernelOnly) {
    Write-Host "Kernel ELF built: $kernelElf" -ForegroundColor Green
    exit 0
}

if (-not (Test-Path -LiteralPath $limineBinary)) {
    throw 'Limine BOOTX64.EFI is missing. Run scripts/bootstrap-limine.ps1 first.'
}

$espDirectory = Join-Path $buildDirectory 'esp'
$efiBootDirectory = Join-Path $espDirectory 'EFI\BOOT'
$kernelDirectory = Join-Path $espDirectory 'boot'
New-Item -ItemType Directory -Path $efiBootDirectory -Force | Out-Null
New-Item -ItemType Directory -Path $kernelDirectory -Force | Out-Null
& $initramfsScript
if ($LASTEXITCODE -ne 0) {
    throw 'Initramfs build failed.'
}
Copy-Item -LiteralPath $limineBinary -Destination (Join-Path $efiBootDirectory 'BOOTX64.EFI') -Force
Copy-Item -LiteralPath (Join-Path $projectRoot 'boot\limine.conf') -Destination (Join-Path $efiBootDirectory 'limine.conf') -Force
Copy-Item -LiteralPath $kernelElf -Destination (Join-Path $kernelDirectory 'kos.elf') -Force
Copy-Item -LiteralPath (Join-Path $buildDirectory 'initramfs.cpio') -Destination (Join-Path $kernelDirectory 'initramfs.cpio') -Force

Write-Host "Kernel ELF built and UEFI ESP staged: $espDirectory" -ForegroundColor Green
