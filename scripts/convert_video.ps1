# PowerShell wrapper for convert_video.py
param (
    [string]$InputVideo = "",
    [string]$OutputFile = "bad_apple.bin",
    [int]$Width = 160,
    [int]$Height = 60,
    [double]$Fps = 30.0,
    [int]$Levels = 16,
    [switch]$Invert
)

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$pyScript = Join-Path $scriptDir "convert_video.py"

$pyArgs = @("`"$pyScript`"")
if ($InputVideo) { $pyArgs += "-i `"$InputVideo`"" }
if ($OutputFile) { $pyArgs += "-o `"$OutputFile`"" }
if ($Width) { $pyArgs += "-w $Width" }
if ($Height) { $pyArgs += "-H $Height" }
if ($Fps) { $pyArgs += "--fps $Fps" }
if ($Levels) { $pyArgs += "--levels $Levels" }
if ($Invert) { $pyArgs += "--invert" }

Write-Host "Running video converter with Python..." -ForegroundColor Cyan
$cmd = "python " + ($pyArgs -join " ")
Invoke-Expression $cmd
