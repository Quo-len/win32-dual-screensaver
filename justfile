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

# Run headless performance benchmark & GDI leak tests (default: 1080p, or specify '4k')
bench res="": build
    @Write-Host "`n==> Running Headless Benchmark Suite..." -ForegroundColor Yellow
    Start-Process -FilePath "{{bin}}" -ArgumentList ("--benchmark " + "{{res}}").Trim() -Wait -NoNewWindow
    @Get-Content benchmark_report.txt

# Run 4K benchmark (3840x2160)
bench-4k: (bench "4k")

# Run on-screen visual benchmark showcase (e.g. 'just bench-visual' or 'just bench-visual 4k')
bench-visual res="": build
    @Write-Host "`n==> Launching Visual Benchmark Showcase..." -ForegroundColor Yellow
    Start-Process -FilePath "{{bin}}" -ArgumentList ("--benchmark visual " + "{{res}}").Trim() -Wait -NoNewWindow
    @Get-Content benchmark_report.txt

# Alias for bench
test res="": (bench res)

# Launch screensaver configuration dialog (/c)
config: build
    & "{{bin}}" /c

# Launch screensaver in fullscreen (/s) or custom resolution/mode (e.g. 'just run', 'just run 4k', 'just run matrix')
run *args: build
    & "{{bin}}" $(if ("{{args}}" -eq "4k") { "/s /4k" } elseif ("{{args}}" -ne "") { "{{args}}" } else { "/s" })

# Launch screensaver in 4K resolution
run-4k: (run "4k")

# List all available animation modes for 'just run <mode>'
modes:
    @Write-Host "`nAvailable screensaver modes for 'just run <mode>':" -ForegroundColor Cyan
    @Write-Host "  Simulations     : " -NoNewline -ForegroundColor Yellow; Write-Host "matrix, gol, ant, brain, perlin, fire"
    @Write-Host "  Geometry & Math : " -NoNewline -ForegroundColor Yellow; Write-Host "donut, julia, mandelbrot, clifford, curl, harmonograph (harmo), grid, pipes"
    @Write-Host "  Retro & Visuals : " -NoNewline -ForegroundColor Yellow; Write-Host "tetris (blocks), pacman (pac), invaders (space/galaga), snake, nyancat (nyan), asciiquarium (aquarium/fish), cbonsai (tree/bonsai), badapple (apple), stars, dvd, pong, maze, sort, memory, clock, earth, blank`n"


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
