#!/bin/bash
# DTC Configurator Tool Launcher
# Usage: ./dtc-tool.sh [config_file.json]

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$SCRIPT_DIR"

# Check Python
if ! command -v python3 &> /dev/null; then
    echo "Error: Python 3 is not installed"
    exit 1
fi

# Check if config file provided
if [ $# -eq 1 ]; then
    echo "Opening: $1"
    python3 -c "
import sys
sys.path.insert(0, '$SCRIPT_DIR/gui')
from dtc_configurator import main, DTCConfiguratorApp
import tkinter as tk

root = tk.Tk()
app = DTCConfiguratorApp(root)
app.load_config_file('$1')
root.mainloop()
"
else
    python3 -m gui.dtc_configurator
fi
