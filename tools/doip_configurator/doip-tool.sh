#!/bin/bash
# DOIP Configurator Tool Launcher

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$SCRIPT_DIR"

if ! command -v python3 &> /dev/null; then
    echo "Error: Python 3 is not installed"
    exit 1
fi

python3 -c "
import sys
import os
sys.path.insert(0, os.path.join('$SCRIPT_DIR', 'gui'))

# 简易配置编辑器
import tkinter as tk
from tkinter import ttk, messagebox, filedialog
import json

class SimpleConfigurator:
    def __init__(self, root):
        self.root = root
        self.root.title('DOIP Configurator')
        self.root.geometry('800x600')
        
        # Menu
        menubar = tk.Menu(root)
        file_menu = tk.Menu(menubar, tearoff=0)
        file_menu.add_command(label='Load CSV', command=self.load_csv)
        file_menu.add_command(label='Save JSON', command=self.save_json)
        file_menu.add_separator()
        file_menu.add_command(label='Exit', command=root.quit)
        menubar.add_cascade(label='File', menu=file_menu)
        root.config(menu=menubar)
        
        # Main frame
        frame = ttk.Frame(root, padding=10)
        frame.pack(fill=tk.BOTH, expand=True)
        
        ttk.Label(frame, text='DOIP Configuration', font=('Arial', 16, 'bold')).pack(pady=10)
        
        # Info text
        self.info_text = tk.Text(frame, wrap=tk.WORD, height=20)
        self.info_text.pack(fill=tk.BOTH, expand=True)
        self.info_text.insert('1.0', 'Use File menu to load CSV configuration\n')
        
        self.config = None
        
    def load_csv(self):
        filepath = filedialog.askopenfilename(filetypes=[('CSV files', '*.csv')])
        if filepath:
            try:
                from parser import AutosarConfigParser
                parser = AutosarConfigParser('DOIP')
                self.config = parser.parse_csv(filepath)
                self.info_text.delete('1.0', tk.END)
                self.info_text.insert('1.0', json.dumps(self.config, indent=2))
            except Exception as e:
                messagebox.showerror('Error', str(e))
    
    def save_json(self):
        if self.config:
            filepath = filedialog.asksaveasfilename(defaultextension='.json')
            if filepath:
                with open(filepath, 'w') as f:
                    json.dump(self.config, f, indent=2)
                messagebox.showinfo('Success', 'Saved!')

root = tk.Tk()
app = SimpleConfigurator(root)
root.mainloop()
"
