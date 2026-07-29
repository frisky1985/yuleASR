# YuleTech AutoSAR Build Script for PowerShell
# Usage: .\build.ps1 [options] [target]

param(
    [switch]$Clean,
    [switch]$Arm,
    [switch]$Native,
    [switch]$Verbose,
    [switch]$Test,
    [switch]$Docs,
    [switch]$Coverage,
    [switch]$Mcal,
    [switch]$NoMcal,
    [string]$Type = "Debug",
    [int]$Jobs = 4,
    [string]$Target = "all"
)

# Default settings
$BUILD_TYPE = $Type
$BUILD_DIR = "build"
$TOOLCHAIN = ""
$CMAKE_ARGS = ""
$BUILD_MCAL = "ON"
$BUILD_TEST = "OFF"
$BUILD_DOCS = "OFF"
$ENABLE_COVERAGE = "OFF"

# Colors
$Red = "Red"
$Green = "Green"
$Yellow = "Yellow"

# Help message
function Show-Help {
    Write-Host "YuleTech AutoSAR Build Script (PowerShell)"
    Write-Host ""
    Write-Host "Usage: .\build.ps1 [options] [target]"
    Write-Host ""
    Write-Host "Options:"
    Write-Host "  -Help           Show this help message"
    Write-Host "  -Clean          Clean build directory before building"
    Write-Host "  -Type TYPE      Build type (Debug, Release, MinSizeRel)"
    Write-Host "  -Arm            Cross-compile for ARM (S32K312)"
    Write-Host "  -Native         Native build (host)"
    Write-Host "  -Verbose        Verbose build output"
    Write-Host "  -Jobs N         Number of parallel jobs (default: 4)"
    Write-Host "  -Test           Enable testing"
    Write-Host "  -Docs           Build documentation"
    Write-Host "  -Coverage       Enable code coverage"
    Write-Host "  -Mcal           Build MCAL drivers"
    Write-Host "  -NoMcal         Skip MCAL drivers"
    Write-Host ""
    Write-Host "Targets: all, clean, test, install, package"
}

if ($Help) {
    Show-Help
    exit 0
}

# Parse arguments
if ($Arm) {
    $TOOLCHAIN = "-DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-arm-none-eabi.cmake"
    $BUILD_DIR = "build-arm"
}

if ($Native) {
    $TOOLCHAIN = ""
    $BUILD_DIR = "build-native"
}

if ($Test) {
    $BUILD_TEST = "ON"
}

if ($Docs) {
    $BUILD_DOCS = "ON"
}

if ($Coverage) {
    $ENABLE_COVERAGE = "ON"
}

if ($Mcal) {
    $BUILD_MCAL = "ON"
}

if ($NoMcal) {
    $BUILD_MCAL = "OFF"
}

# Set cmake arguments
$CMAKE_ARGS = @(
    "-DCMAKE_BUILD_TYPE=$BUILD_TYPE",
    "-DBUILD_TESTING=$BUILD_TEST",
    "-DBUILD_DOCUMENTATION=$BUILD_DOCS",
    "-DENABLE_COVERAGE=$ENABLE_COVERAGE",
    "-DYULE_ENABLE_MCAL=$BUILD_MCAL",
    $TOOLCHAIN
)

# Clean if requested
if ($Clean -or $Target -eq "clean") {
    Write-Host "Cleaning build directory..." -ForegroundColor Yellow
    Remove-Item -Recurse -Force $BUILD_DIR -ErrorAction SilentlyContinue
    if ($Target -eq "clean") {
        exit 0
    }
}

# Create build directory
New-Item -ItemType Directory -Force -Path $BUILD_DIR | Out-Null

Write-Host "========================================" -ForegroundColor Green
Write-Host "YuleTech AutoSAR Build" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Green
Write-Host "Build type:    $BUILD_TYPE"
Write-Host "Build dir:     $BUILD_DIR"
Write-Host "Target:        $Target"
Write-Host "MCAL drivers:  $BUILD_MCAL"
Write-Host "Testing:       $BUILD_TEST"
Write-Host "Documentation: $BUILD_DOCS"
Write-Host "Coverage:      $ENABLE_COVERAGE"
Write-Host "Toolchain:     $(if ($TOOLCHAIN) { 'ARM' } else { 'native' })"
Write-Host "========================================" -ForegroundColor Green

# Configure
Write-Host "Configuring..." -ForegroundColor Yellow
$cmakeCmd = "cmake -B $BUILD_DIR -S . $CMAKE_ARGS"
Invoke-Expression $cmakeCmd

if ($LASTEXITCODE -ne 0) {
    Write-Host "Configuration failed!" -ForegroundColor Red
    exit 1
}

# Build
Write-Host "Building..." -ForegroundColor Yellow
cmake --build $BUILD_DIR --target $Target --parallel $Jobs

if ($LASTEXITCODE -ne 0) {
    Write-Host "Build failed!" -ForegroundColor Red
    exit 1
}

# Run tests if requested
if ($Target -eq "test") {
    Write-Host "Running tests..." -ForegroundColor Yellow
    ctest --test-dir $BUILD_DIR --output-on-failure
}

# Install
if ($Target -eq "install") {
    Write-Host "Installing..." -ForegroundColor Yellow
    cmake --install $BUILD_DIR
}

# Package
if ($Target -eq "package") {
    Write-Host "Creating package..." -ForegroundColor Yellow
    cmake --build $BUILD_DIR --target package
}

Write-Host "========================================" -ForegroundColor Green
Write-Host "Build completed successfully!" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Green
Write-Host "Output directory: $BUILD_DIR/"
