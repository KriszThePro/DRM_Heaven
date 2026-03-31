param(
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Debug",

    [ValidateSet("x64", "Win32")]
    [string]$Platform = "x64",

    [string]$Solution = "DRM_Heaven.sln",

    [string]$VsDevCmd = "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat"
)

$ErrorActionPreference = "Stop"

$repoRoot = $PSScriptRoot
$solutionPath = Join-Path $repoRoot $Solution

if (-not (Test-Path -LiteralPath $solutionPath)) {
    throw "Solution not found: $solutionPath"
}

if (-not (Test-Path -LiteralPath $VsDevCmd)) {
    throw "VsDevCmd.bat not found: $VsDevCmd"
}

$buildCmd = '"{0}" -arch={1} && msbuild "{2}" /t:Build /p:Configuration={3} /p:Platform={4}' -f $VsDevCmd, $Platform, $solutionPath, $Configuration, $Platform

Write-Host "Using VsDevCmd:" $VsDevCmd
Write-Host "Building:" $solutionPath
Write-Host "Configuration/Platform:" "$Configuration|$Platform"

Push-Location -LiteralPath $repoRoot
try {
    cmd.exe /d /c $buildCmd
    if ($LASTEXITCODE -ne 0) {
        throw "MSBuild failed with exit code $LASTEXITCODE"
    }
}
finally {
    Pop-Location
}

Write-Host "Build completed successfully."

