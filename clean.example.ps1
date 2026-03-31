param(
    [switch]$All
)

$ErrorActionPreference = "Stop"

$repoRoot = $PSScriptRoot

$paths = @(
    (Join-Path $repoRoot "x64"),
    (Join-Path $repoRoot "MDI_to_IQ\x64"),
    (Join-Path $repoRoot "sample.mdi"),
    (Join-Path $repoRoot "sample.iq")
)

if ($All) {
    $paths += @(
        (Join-Path $repoRoot "mf_test_output.iq")
    )
}

foreach ($path in $paths) {
    if (Test-Path -LiteralPath $path) {
        Remove-Item -LiteralPath $path -Recurse -Force
        Write-Host "Removed:" $path
    }
}

Write-Host "Clean completed."

