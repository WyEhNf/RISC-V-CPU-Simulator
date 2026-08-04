param(
    [string]$Executable = "",
    [string]$DataDirectory = ""
)

$ErrorActionPreference = "Continue"
$projectRoot = $PSScriptRoot

if ([string]::IsNullOrWhiteSpace($DataDirectory)) {
    $DataDirectory = Join-Path $projectRoot "data\testcases"
}

if ([string]::IsNullOrWhiteSpace($Executable)) {
    $candidates = @(
        (Join-Path $projectRoot "code.exe"),
        (Join-Path $projectRoot "build\code.exe"),
        (Join-Path $projectRoot "build\Release\code.exe"),
        (Join-Path $projectRoot "build\Debug\code.exe")
    )
    foreach ($candidate in $candidates) {
        if (Test-Path -LiteralPath $candidate -PathType Leaf) {
            $Executable = $candidate
            break
        }
    }
}

if ([string]::IsNullOrWhiteSpace($Executable) -or
    -not (Test-Path -LiteralPath $Executable -PathType Leaf)) {
    Write-Error "code.exe was not found. Build it first or pass -Executable."
    exit 2
}

if (-not (Test-Path -LiteralPath $DataDirectory -PathType Container)) {
    Write-Error "Data directory was not found: $DataDirectory"
    exit 2
}

$cases = Get-ChildItem -LiteralPath $DataDirectory -Filter "*.data" -File |
    Sort-Object Name

if ($cases.Count -eq 0) {
    Write-Error "No .data files were found in: $DataDirectory"
    exit 2
}

$failed = 0
foreach ($case in $cases) {
    # --input consumes the textual .data memory image described by README.md.
    # This runs only the CPU: no reference engine or expected-value comparison.
    $cpuOutput = & $Executable --input $case.FullName 2>$null
    $exitCode = $LASTEXITCODE

    if ($exitCode -eq 0) {
        $value = ($cpuOutput -join "`n").Trim()
        Write-Output ("{0}: {1}" -f $case.BaseName, $value)
    }
    else {
        ++$failed
        Write-Output ("{0}: ERROR (exit code {1})" -f $case.BaseName, $exitCode)
    }
}

if ($failed -ne 0) {
    exit 1
}
