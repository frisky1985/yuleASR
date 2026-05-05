@echo off
REM DTC Configurator Tool Launcher
REM Usage: dtc-tool.bat [config_file.json]

set SCRIPT_DIR=%~dp0
cd /d %SCRIPT_DIR%

REM Check Python
python --version >nul 2>&1
if errorlevel 1 (
    echo Error: Python is not installed or not in PATH
    exit /b 1
)

REM Launch GUI
if "%~1"=="" (
    python -m gui.dtc_configurator
) else (
    python -c "import sys; sys.path.insert(0, '%SCRIPT_DIR%gui'); exec(open('%SCRIPT_DIR%gui\dtc_configurator.py').read())" %1
)
