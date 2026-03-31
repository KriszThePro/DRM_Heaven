param(
    [string]$DreamExe = "",
    [string]$InputMdi = ".\mf_from_pcap.mdi",
    [switch]$Permissive = $true,
    [switch]$DryRun
)

$ErrorActionPreference = "Stop"

$repoRoot = $PSScriptRoot
$mdiPath = Join-Path $repoRoot $InputMdi

if (-not (Test-Path -LiteralPath $mdiPath)) {
    throw "Input MDI file not found: $mdiPath"
}

if ([string]::IsNullOrWhiteSpace($DreamExe)) {
    throw "Please provide -DreamExe path (Dream executable)"
}

if (-not (Test-Path -LiteralPath $DreamExe)) {
    throw "Dream executable not found: $DreamExe"
}

$argsList = @("-n")
if ($Permissive) {
    # Accept malformed RSCI/MDI frames if capture quality is imperfect.
    $argsList += @("--permissive", "1")
}
$argsList += $mdiPath

Write-Host "Dream executable:" $DreamExe
Write-Host "Input MDI:" $mdiPath
Write-Host "Arguments:" ($argsList -join " ")

if ($DryRun) {
    Write-Host "Dry run enabled; not starting Dream."
    return
}

& $DreamExe @argsList
if ($LASTEXITCODE -ne 0) {
    throw "Dream exited with code $LASTEXITCODE"
}

