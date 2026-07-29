#==================================================================================================
# Project              : YuleTech AutoSAR BSW
# Script               : Build and Run Unit Tests (PowerShell)
# Date                 : 2026-04-27
#
# (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
# All Rights Reserved.
#
# Description: 编译并运行所有单元测试 (Windows)
#==================================================================================================

$ErrorActionPreference = "Stop"

Write-Host "==============================================" -ForegroundColor Cyan
Write-Host "  YuleTech BSW 单元测试构建与执行" -ForegroundColor Cyan
Write-Host "==============================================" -ForegroundColor Cyan
Write-Host ""

# 获取脚本所在目录
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$TestDir = Join-Path $ScriptDir "unit"
$BuildDir = Join-Path $ScriptDir "build"

# 创建构建目录
if (-not (Test-Path $BuildDir)) {
    New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null
}

# GCC 编译器路径 (假设已安装并添加到 PATH)
$GCC = "gcc"

# 包含路径
$Includes = @(
    "-I$(Join-Path $ScriptDir "..\..\src\bsw\common")",
    "-I$(Join-Path $ScriptDir "..\..\src\bsw\mcal\mcu\include")",
    "-I$(Join-Path $ScriptDir "..\..\src\bsw\mcal\port\include")",
    "-I$(Join-Path $ScriptDir "..\..\src\bsw\mcal\can\include")",
    "-I$(Join-Path $ScriptDir "..\..\src\bsw\services\pdur\include")",
    "-I$(Join-Path $ScriptDir "..\..\src\bsw\services\com\include")",
    "-I$(Join-Path $ScriptDir "..\..\src\bsw\services\nvm\include")"
)

# 查找所有测试文件
Set-Location $TestDir
$TestFiles = Get-ChildItem -Recurse -Filter "test_*.c" -File

$FailedTests = @()
$PassedTests = @()

foreach ($testFile in $TestFiles) {
    $testName = $testFile.BaseName
    Write-Host "`n编译: $testName" -ForegroundColor Yellow
    
    $outputExe = Join-Path $BuildDir "$testName.exe"
    
    # 编译测试
    $compileArgs = @("-o", $outputExe, $testFile.FullName) + $Includes + @("-lm")
    
    try {
        & $GCC @compileArgs 2>&1 | Out-Null
        Write-Host "✓ 编译成功" -ForegroundColor Green
        
        # 运行测试
        Write-Host "执行: $testName" -ForegroundColor Yellow
        $process = Start-Process -FilePath $outputExe -NoNewWindow -Wait -PassThru
        
        if ($process.ExitCode -eq 0) {
            Write-Host "✓ 测试通过" -ForegroundColor Green
            $PassedTests += $testName
        } else {
            Write-Host "✗ 测试失败 (退出码: $($process.ExitCode))" -ForegroundColor Red
            $FailedTests += $testName
        }
    } catch {
        Write-Host "✗ 编译失败: $_" -ForegroundColor Red
        $FailedTests += "$testName (编译失败)"
    }
}

# 打印汇总
Write-Host ""
Write-Host "==============================================" -ForegroundColor Cyan
Write-Host "  测试汇总" -ForegroundColor Cyan
Write-Host "==============================================" -ForegroundColor Cyan

Write-Host "`n通过的测试 ($($PassedTests.Count)):" -ForegroundColor Green
foreach ($test in $PassedTests) {
    Write-Host "  ✓ $test" -ForegroundColor Green
}

if ($FailedTests.Count -gt 0) {
    Write-Host "`n失败的测试 ($($FailedTests.Count)):" -ForegroundColor Red
    foreach ($test in $FailedTests) {
        Write-Host "  ✗ $test" -ForegroundColor Red
    }
    
    Write-Host "`n==============================================" -ForegroundColor Red
    Write-Host "  测试结果: 失败" -ForegroundColor Red
    Write-Host "==============================================" -ForegroundColor Red
    exit 1
} else {
    Write-Host "`n==============================================" -ForegroundColor Green
    Write-Host "  测试结果: 全部通过" -ForegroundColor Green
    Write-Host "==============================================" -ForegroundColor Green
    exit 0
}
