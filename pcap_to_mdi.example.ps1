param(
    [string]$InputPcap = ".\mf.pcap",
    [string]$OutputMdi = ".\mf_from_pcap.mdi",
    [int]$DstPort = 1234,
    [Nullable[int]]$SrcPort = $null,
    [switch]$AllUdp
)

$ErrorActionPreference = "Stop"

$repoRoot = $PSScriptRoot
$pythonScript = Join-Path $repoRoot "pcap_to_mdi.py"
$inputPath = Join-Path $repoRoot $InputPcap
$outputPath = Join-Path $repoRoot $OutputMdi

if (-not (Test-Path -LiteralPath $pythonScript)) {
    throw "Missing helper script: $pythonScript"
}

if (-not (Test-Path -LiteralPath $inputPath)) {
    throw "Input pcap not found: $inputPath"
}

$cmd = @("-u", $pythonScript, $inputPath, $outputPath, "--dst-port", $DstPort)
if ($SrcPort -ne $null) {
    $cmd += @("--src-port", $SrcPort)
}
if ($AllUdp) {
    $cmd += "--all-udp"
}

Write-Host "Running pcap_to_mdi.py..."
Write-Host "Input:  $inputPath"
Write-Host "Output: $outputPath"

python @cmd
if ($LASTEXITCODE -ne 0) {
    throw "pcap_to_mdi.py failed with exit code $LASTEXITCODE"
}

Write-Host "PCAP to MDI conversion completed successfully."
