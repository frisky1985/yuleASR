@echo off
REM UDS Configurator Tool Launcher
set SCRIPT_DIR=%~dp0
cd /d %SCRIPT_DIR%
python -c "import tkinter as tk; from tkinter import ttk, messagebox, filedialog; import json; root=tk.Tk(); root.title('UDS Configurator'); root.geometry('800x600'); ttk.Label(root, text='UDS Configurator', font=('Arial', 16)).pack(pady=20); tk.Button(root, text='Load CSV', command=lambda: print('Load')).pack(); tk.Button(root, text='Exit', command=root.quit).pack(); root.mainloop()"
