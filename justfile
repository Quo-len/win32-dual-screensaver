# Modern task runner for DualScreenSaver
set windows-shell := ["powershell.exe", "-NoLogo", "-Command"]

# Detect MSBuild automatically using vswhere
msbuild := `powershell.exe -NoLogo -Command "(& '${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe' -prerelease -latest -products * -requires Microsoft.Component.MSBuild -find MSBuild\\**\\Bin\\MSBuild.exe | Select-Object -First 1)"`
solution := "dual-screensaver.slnx"
bin := ".\\dual-screensaver\\x64\\Release\\DualScreenSaver.exe"
scr := ".\\dual-screensaver\\x64\\Release\\DualScreenSaver.scr"

# Show all available commands
default:
    @just --list

# Build Release (x64) and generate DualScreenSaver.scr
build:
    @Write-Host "==> Building Release (x64)..." -ForegroundColor Cyan
    & "{{msbuild}}" {{solution}} /p:Configuration=Release /p:Platform=x64 /v:minimal /nologo

# Alias for build
release: build

# Build Debug (x64)
debug:
    @Write-Host "==> Building Debug (x64)..." -ForegroundColor Cyan
    & "{{msbuild}}" {{solution}} /p:Configuration=Debug /p:Platform=x64 /v:minimal /nologo

# Run headless performance benchmark & GDI leak tests
bench: build
    @Write-Host "`n==> Running Headless Benchmark Suite..." -ForegroundColor Yellow
    Start-Process -FilePath "{{bin}}" -ArgumentList "--benchmark" -Wait -NoNewWindow
    @Get-Content benchmark_report.txt

# Alias for bench
test: bench

# Launch screensaver configuration dialog (/c)
config: build
    & "{{bin}}" /c

# Launch screensaver in fullscreen test mode (/s)
run: build
    & "{{bin}}" /s

# Clean build output and intermediate folders
clean:
    @Write-Host "==> Cleaning build artifacts..." -ForegroundColor Red
    -& "{{msbuild}}" {{solution}} /t:Clean /p:Configuration=Release /p:Platform=x64 /v:minimal /nologo
    -& "{{msbuild}}" {{solution}} /t:Clean /p:Configuration=Debug /p:Platform=x64 /v:minimal /nologo
    Remove-Item -Recurse -Force -ErrorAction SilentlyContinue x64, dual-screensaver\x64, .vs, benchmark_report.txt
    @Write-Host "Clean complete." -ForegroundColor Green

# Print detected toolchain paths
info:
    @Write-Host "MSBuild : {{msbuild}}" -ForegroundColor Cyan
    @Write-Host "Solution: {{solution}}" -ForegroundColor Cyan
    @Write-Host "Binary  : {{bin}}" -ForegroundColor Cyan
    @Write-Host "Scr     : {{scr}}" -ForegroundColor Cyan
