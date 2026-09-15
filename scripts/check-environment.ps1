[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'

$requirements = @(
    @{ Name = 'git'; Alternatives = @('git'); Paths = @(); Required = $true },
    @{ Name = 'C compiler'; Alternatives = @('clang'); Paths = @('C:\Program Files\LLVM\bin\clang.exe'); Required = $true },
    @{ Name = 'Linker'; Alternatives = @('ld.lld'); Paths = @('C:\Program Files\LLVM\bin\ld.lld.exe'); Required = $true },
    @{ Name = 'Assembler'; Alternatives = @('nasm'); Paths = @('C:\Program Files\NASM\nasm.exe', (Join-Path $PSScriptRoot '..\third_party\tools\nasm-3.02\nasm.exe')); Required = $true },
    @{ Name = 'QEMU x86_64'; Alternatives = @('qemu-system-x86_64'); Paths = @('C:\Program Files\qemu\qemu-system-x86_64.exe'); Required = $true },
    @{ Name = 'ISO builder'; Alternatives = @('xorriso'); Paths = @(); Required = $false }
)

$missingRequired = @()

Write-Host 'KOS host environment check' -ForegroundColor Cyan
foreach ($requirement in $requirements) {
    $found = $null
    foreach ($candidate in $requirement.Alternatives) {
        $command = Get-Command $candidate -ErrorAction SilentlyContinue
        if ($null -ne $command) {
            $found = $command.Source
            break
        }
    }

    if ($null -eq $found) {
        foreach ($candidatePath in $requirement.Paths) {
            if (Test-Path -LiteralPath $candidatePath) {
                $found = $candidatePath
                break
            }
        }
    }

    if ($null -ne $found) {
        Write-Host ("[OK]      {0}: {1}" -f $requirement.Name, $found) -ForegroundColor Green
    }
    elseif ($requirement.Required) {
        Write-Host ("[MISSING] {0} (tried: {1})" -f $requirement.Name, ($requirement.Alternatives -join ', ')) -ForegroundColor Red
        $missingRequired += $requirement.Name
    }
    else {
        Write-Host ("[OPTIONAL] {0} (tried: {1})" -f $requirement.Name, ($requirement.Alternatives -join ', ')) -ForegroundColor Yellow
    }
}

if ($missingRequired.Count -gt 0) {
    Write-Host ''
    Write-Host 'Install the missing host tools before building KOS.' -ForegroundColor Yellow
    Write-Host 'See docs/environment.md for setup guidance.' -ForegroundColor Yellow
    exit 1
}

Write-Host ''
Write-Host 'Required host tools are available. Run scripts/bootstrap-limine.ps1 if Limine is not already present, then run scripts/build.ps1.' -ForegroundColor Green
