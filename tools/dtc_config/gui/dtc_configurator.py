#!/usr/bin/env python3
"""
DTC Configurator Tool - Graphical Configuration Editor
AUTOSAR Diagnostic Trouble Code Management Tool
"""

import tkinter as tk
from tkinter import ttk, messagebox, filedialog, scrolledtext
import json
import os
import sys
from datetime import datetime
from typing import Dict, List, Any, Optional

# Add parent directory to path for imports
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

try:
    from config_parser import DTCConfigParser, validate_config
except ImportError:
    messagebox.showerror("Error", "config_parser.py not found in the same directory")
    sys.exit(1)

class DTCConfiguratorApp:
    """Main DTC Configurator Application"""
    
    def __init__(self, root: tk.Tk):
        self.root = root
        self.root.title("DTC Configurator Tool - AUTOSAR Diagnostic Configuration")
        self.root.geometry("1400x900")
        self.root.minsize(1200, 700)
        
        # Data storage
        self.config = {
            'project_name': 'New_AUTOSAR_Project',
            'version': {'major': 1, 'minor': 0, 'patch': 0},
            'generation_date': datetime.now().strftime('%Y-%m-%d'),
            'dtcs': [],
            'events': [],
            'indicators': [],
            'operation_cycles': []
        }
        
        self.current_file = None
        self.modified = False
        
        self._create_menu()
        self._create_main_layout()
        self._create_status_bar()
        
        # Bind keyboard shortcuts
        self.root.bind('<Control-s>', lambda e: self.save_config())
        self.root.bind('<Control-o>', lambda e: self.load_config())
        self.root.bind('<Control-n>', lambda e: self.new_config())
        
    def _create_menu(self):
        """Create application menu"""
        menubar = tk.Menu(self.root)
        self.root.config(menu=menubar)
        
        # File menu
        file_menu = tk.Menu(menubar, tearoff=0)
        menubar.add_cascade(label="File", menu=file_menu)
        file_menu.add_command(label="New", command=self.new_config, accelerator="Ctrl+N")
        file_menu.add_command(label="Open...", command=self.load_config, accelerator="Ctrl+O")
        file_menu.add_separator()
        file_menu.add_command(label="Save", command=self.save_config, accelerator="Ctrl+S")
        file_menu.add_command(label="Save As...", command=self.save_as_config)
        file_menu.add_separator()
        file_menu.add_command(label="Import CSV...", command=self.import_csv)
        file_menu.add_command(label="Export JSON...", command=self.export_json)
        file_menu.add_separator()
        file_menu.add_command(label="Exit", command=self.on_closing)
        
        # Tools menu
        tools_menu = tk.Menu(menubar, tearoff=0)
        menubar.add_cascade(label="Tools", menu=tools_menu)
        tools_menu.add_command(label="Validate Configuration", command=self.validate_config)
        tools_menu.add_command(label="Generate C Code...", command=self.generate_code)
        tools_menu.add_separator()
        tools_menu.add_command(label="Add Sample Data", command=self.add_sample_data)
        tools_menu.add_command(label="Clear All Data", command=self.clear_all_data)
        
        # Help menu
        help_menu = tk.Menu(menubar, tearoff=0)
        menubar.add_cascade(label="Help", menu=help_menu)
        help_menu.add_command(label="About", command=self.show_about)
        
    def _create_main_layout(self):
        """Create main application layout"""
        # Main container
        main_container = ttk.Frame(self.root)
        main_container.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        # Project info frame (top)
        self._create_project_frame(main_container)
        
        # Notebook for tabs
        self.notebook = ttk.Notebook(main_container)
        self.notebook.pack(fill=tk.BOTH, expand=True, pady=5)
        
        # Create tabs
        self.dtc_tab = ttk.Frame(self.notebook)
        self.event_tab = ttk.Frame(self.notebook)
        self.indicator_tab = ttk.Frame(self.notebook)
        self.cycle_tab = ttk.Frame(self.notebook)
        
        self.notebook.add(self.dtc_tab, text="DTC Definitions")
        self.notebook.add(self.event_tab, text="Event Configuration")
        self.notebook.add(self.indicator_tab, text="Indicators")
        self.notebook.add(self.cycle_tab, text="Operation Cycles")
        
        # Setup each tab
        self._setup_dtc_tab()
        self._setup_event_tab()
        self._setup_indicator_tab()
        self._setup_cycle_tab()
        
    def _create_project_frame(self, parent):
        """Create project information frame"""
        frame = ttk.LabelFrame(parent, text="Project Information", padding=5)
        frame.pack(fill=tk.X, pady=5)
        
        # Row 1
        row1 = ttk.Frame(frame)
        row1.pack(fill=tk.X)
        
        ttk.Label(row1, text="Project Name:").pack(side=tk.LEFT)
        self.project_name_var = tk.StringVar(value=self.config['project_name'])
        ttk.Entry(row1, textvariable=self.project_name_var, width=40).pack(side=tk.LEFT, padx=5)
        
        ttk.Label(row1, text="Version:").pack(side=tk.LEFT, padx=(20, 0))
        self.version_major_var = tk.StringVar(value="1")
        self.version_minor_var = tk.StringVar(value="0")
        self.version_patch_var = tk.StringVar(value="0")
        ttk.Entry(row1, textvariable=self.version_major_var, width=5).pack(side=tk.LEFT, padx=2)
        ttk.Label(row1, text=".").pack(side=tk.LEFT)
        ttk.Entry(row1, textvariable=self.version_minor_var, width=5).pack(side=tk.LEFT, padx=2)
        ttk.Label(row1, text=".").pack(side=tk.LEFT)
        ttk.Entry(row1, textvariable=self.version_patch_var, width=5).pack(side=tk.LEFT, padx=2)
        
        # Statistics
        self.stats_label = ttk.Label(row1, text="DTCs: 0 | Events: 0 | Indicators: 0 | Cycles: 0")
        self.stats_label.pack(side=tk.RIGHT, padx=10)
        
    def _setup_dtc_tab(self):
        """Setup DTC definitions tab"""
        # Toolbar
        toolbar = ttk.Frame(self.dtc_tab)
        toolbar.pack(fill=tk.X, pady=5)
        
        ttk.Button(toolbar, text="Add DTC", command=self.add_dtc).pack(side=tk.LEFT, padx=2)
        ttk.Button(toolbar, text="Edit DTC", command=self.edit_dtc).pack(side=tk.LEFT, padx=2)
        ttk.Button(toolbar, text="Delete DTC", command=self.delete_dtc).pack(side=tk.LEFT, padx=2)
        ttk.Button(toolbar, text="Duplicate DTC", command=self.duplicate_dtc).pack(side=tk.LEFT, padx=2)
        
        # Treeview
        columns = ('code', 'name', 'description', 'severity', 'group', 'priority', 'immediate', 'aging')
        self.dtc_tree = ttk.Treeview(self.dtc_tab, columns=columns, show='headings', height=20)
        
        # Column headings
        self.dtc_tree.heading('code', text='DTC Code')
        self.dtc_tree.heading('name', text='Name')
        self.dtc_tree.heading('description', text='Description')
        self.dtc_tree.heading('severity', text='Severity')
        self.dtc_tree.heading('group', text='Group')
        self.dtc_tree.heading('priority', text='Priority')
        self.dtc_tree.heading('immediate', text='Immediate')
        self.dtc_tree.heading('aging', text='Aging')
        
        # Column widths
        self.dtc_tree.column('code', width=100)
        self.dtc_tree.column('name', width=150)
        self.dtc_tree.column('description', width=300)
        self.dtc_tree.column('severity', width=80)
        self.dtc_tree.column('group', width=120)
        self.dtc_tree.column('priority', width=60)
        self.dtc_tree.column('immediate', width=70)
        self.dtc_tree.column('aging', width=70)
        
        # Scrollbar
        scrollbar = ttk.Scrollbar(self.dtc_tab, orient=tk.VERTICAL, command=self.dtc_tree.yview)
        self.dtc_tree.configure(yscrollcommand=scrollbar.set)
        
        self.dtc_tree.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
        
        # Double click to edit
        self.dtc_tree.bind('<Double-1>', lambda e: self.edit_dtc())
        
    def _setup_event_tab(self):
        """Setup event configuration tab"""
        toolbar = ttk.Frame(self.event_tab)
        toolbar.pack(fill=tk.X, pady=5)
        
        ttk.Button(toolbar, text="Add Event", command=self.add_event).pack(side=tk.LEFT, padx=2)
        ttk.Button(toolbar, text="Edit Event", command=self.edit_event).pack(side=tk.LEFT, padx=2)
        ttk.Button(toolbar, text="Delete Event", command=self.delete_event).pack(side=tk.LEFT, padx=2)
        
        columns = ('id', 'name', 'dtc', 'debounce', 'ff_dids', 'ext_size')
        self.event_tree = ttk.Treeview(self.event_tab, columns=columns, show='headings', height=20)
        
        self.event_tree.heading('id', text='Event ID')
        self.event_tree.heading('name', text='Name')
        self.event_tree.heading('dtc', text='DTC Code')
        self.event_tree.heading('debounce', text='Debounce')
        self.event_tree.heading('ff_dids', text='Freeze Frame DIDs')
        self.event_tree.heading('ext_size', text='Ext Data Size')
        
        self.event_tree.column('id', width=70)
        self.event_tree.column('name', width=150)
        self.event_tree.column('dtc', width=100)
        self.event_tree.column('debounce', width=150)
        self.event_tree.column('ff_dids', width=200)
        self.event_tree.column('ext_size', width=100)
        
        scrollbar = ttk.Scrollbar(self.event_tab, orient=tk.VERTICAL, command=self.event_tree.yview)
        self.event_tree.configure(yscrollcommand=scrollbar.set)
        
        self.event_tree.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
        
        self.event_tree.bind('<Double-1>', lambda e: self.edit_event())
        
    def _setup_indicator_tab(self):
        """Setup indicator configuration tab"""
        toolbar = ttk.Frame(self.indicator_tab)
        toolbar.pack(fill=tk.X, pady=5)
        
        ttk.Button(toolbar, text="Add Indicator", command=self.add_indicator).pack(side=tk.LEFT, padx=2)
        ttk.Button(toolbar, text="Edit Indicator", command=self.edit_indicator).pack(side=tk.LEFT, padx=2)
        ttk.Button(toolbar, text="Delete Indicator", command=self.delete_indicator).pack(side=tk.LEFT, padx=2)
        
        columns = ('event', 'indicator', 'behavior', 'fail_cycles', 'heal_cycles')
        self.indicator_tree = ttk.Treeview(self.indicator_tab, columns=columns, show='headings', height=20)
        
        self.indicator_tree.heading('event', text='Event ID')
        self.indicator_tree.heading('indicator', text='Indicator ID')
        self.indicator_tree.heading('behavior', text='Behavior')
        self.indicator_tree.heading('fail_cycles', text='Failure Cycles')
        self.indicator_tree.heading('heal_cycles', text='Healing Cycles')
        
        scrollbar = ttk.Scrollbar(self.indicator_tab, orient=tk.VERTICAL, command=self.indicator_tree.yview)
        self.indicator_tree.configure(yscrollcommand=scrollbar.set)
        
        self.indicator_tree.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
        
    def _setup_cycle_tab(self):
        """Setup operation cycle tab"""
        toolbar = ttk.Frame(self.cycle_tab)
        toolbar.pack(fill=tk.X, pady=5)
        
        ttk.Button(toolbar, text="Add Cycle", command=self.add_cycle).pack(side=tk.LEFT, padx=2)
        ttk.Button(toolbar, text="Edit Cycle", command=self.edit_cycle).pack(side=tk.LEFT, padx=2)
        ttk.Button(toolbar, text="Delete Cycle", command=self.delete_cycle).pack(side=tk.LEFT, padx=2)
        
        columns = ('id', 'type', 'auto_start')
        self.cycle_tree = ttk.Treeview(self.cycle_tab, columns=columns, show='headings', height=20)
        
        self.cycle_tree.heading('id', text='Cycle ID')
        self.cycle_tree.heading('type', text='Type')
        self.cycle_tree.heading('auto_start', text='Auto Start')
        
        scrollbar = ttk.Scrollbar(self.cycle_tab, orient=tk.VERTICAL, command=self.cycle_tree.yview)
        self.cycle_tree.configure(yscrollcommand=scrollbar.set)
        
        self.cycle_tree.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
        
    def _create_status_bar(self):
        """Create status bar"""
        self.status_var = tk.StringVar(value="Ready")
        status_bar = ttk.Label(self.root, textvariable=self.status_var, relief=tk.SUNKEN, anchor=tk.W)
        status_bar.pack(side=tk.BOTTOM, fill=tk.X)
        
    # ==================== DTC Operations ====================
    
    def add_dtc(self):
        """Add new DTC"""
        dialog = DTCDialog(self.root, "Add DTC", None)
        if dialog.result:
            self.config['dtcs'].append(dialog.result)
            self.refresh_dtc_tree()
            self.mark_modified()
            
    def edit_dtc(self):
        """Edit selected DTC"""
        selection = self.dtc_tree.selection()
        if not selection:
            messagebox.showwarning("Warning", "Please select a DTC to edit")
            return
            
        item = self.dtc_tree.item(selection[0])
        code = int(item['values'][0], 16)
        
        # Find DTC in config
        for i, dtc in enumerate(self.config['dtcs']):
            if dtc['code'] == code:
                dialog = DTCDialog(self.root, "Edit DTC", dtc)
                if dialog.result:
                    self.config['dtcs'][i] = dialog.result
                    self.refresh_dtc_tree()
                    self.mark_modified()
                break
                
    def delete_dtc(self):
        """Delete selected DTC"""
        selection = self.dtc_tree.selection()
        if not selection:
            messagebox.showwarning("Warning", "Please select a DTC to delete")
            return
            
        if messagebox.askyesno("Confirm", "Delete selected DTC?"):
            item = self.dtc_tree.item(selection[0])
            code = int(item['values'][0], 16)
            
            self.config['dtcs'] = [d for d in self.config['dtcs'] if d['code'] != code]
            self.refresh_dtc_tree()
            self.mark_modified()
            
    def duplicate_dtc(self):
        """Duplicate selected DTC"""
        selection = self.dtc_tree.selection()
        if not selection:
            messagebox.showwarning("Warning", "Please select a DTC to duplicate")
            return
            
        item = self.dtc_tree.item(selection[0])
        code = int(item['values'][0], 16)
        
        for dtc in self.config['dtcs']:
            if dtc['code'] == code:
                new_dtc = dtc.copy()
                new_dtc['code'] = code + 1  # Increment
                new_dtc['name'] = dtc['name'] + '_COPY'
                self.config['dtcs'].append(new_dtc)
                self.refresh_dtc_tree()
                self.mark_modified()
                break
                
    def refresh_dtc_tree(self):
        """Refresh DTC treeview"""
        # Clear tree
        for item in self.dtc_tree.get_children():
            self.dtc_tree.delete(item)
            
        # Add items
        for dtc in sorted(self.config['dtcs'], key=lambda x: x['code']):
            self.dtc_tree.insert('', tk.END, values=(
                f"0x{dtc['code']:06X}",
                dtc['name'],
                dtc['description'],
                dtc['severity'],
                dtc['group'],
                dtc['priority'],
                'Yes' if dtc['immediate_storage'] else 'No',
                'Yes' if dtc['aging_allowed'] else 'No'
            ))
            
        self.update_stats()
        
    # ==================== Event Operations ====================
    
    def add_event(self):
        """Add new event"""
        dialog = EventDialog(self.root, "Add Event", None, self.config['dtcs'])
        if dialog.result:
            self.config['events'].append(dialog.result)
            self.refresh_event_tree()
            self.mark_modified()
            
    def edit_event(self):
        """Edit selected event"""
        selection = self.event_tree.selection()
        if not selection:
            messagebox.showwarning("Warning", "Please select an event to edit")
            return
            
        item = self.event_tree.item(selection[0])
        event_id = int(item['values'][0])
        
        for i, event in enumerate(self.config['events']):
            if event['id'] == event_id:
                dialog = EventDialog(self.root, "Edit Event", event, self.config['dtcs'])
                if dialog.result:
                    self.config['events'][i] = dialog.result
                    self.refresh_event_tree()
                    self.mark_modified()
                break
                
    def delete_event(self):
        """Delete selected event"""
        selection = self.event_tree.selection()
        if not selection:
            messagebox.showwarning("Warning", "Please select an event to delete")
            return
            
        if messagebox.askyesno("Confirm", "Delete selected event?"):
            item = self.event_tree.item(selection[0])
            event_id = int(item['values'][0])
            
            self.config['events'] = [e for e in self.config['events'] if e['id'] != event_id]
            self.refresh_event_tree()
            self.mark_modified()
            
    def refresh_event_tree(self):
        """Refresh event treeview"""
        for item in self.event_tree.get_children():
            self.event_tree.delete(item)
            
        for event in sorted(self.config['events'], key=lambda x: x['id']):
            dids_str = ', '.join([f"0x{d:04X}" for d in event.get('freeze_frame_dids', [])])
            self.event_tree.insert('', tk.END, values=(
                event['id'],
                event['name'],
                f"0x{event['dtc_code']:06X}",
                f"{event['debounce_type']} ({event['debounce_failed_thr']}/{event['debounce_passed_thr']})",
                dids_str,
                event.get('extended_data_size', 0)
            ))
            
        self.update_stats()
        
    # ==================== Indicator Operations ====================
    
    def add_indicator(self):
        """Add new indicator"""
        dialog = IndicatorDialog(self.root, "Add Indicator", None, self.config['events'])
        if dialog.result:
            self.config['indicators'].append(dialog.result)
            self.refresh_indicator_tree()
            self.mark_modified()
            
    def edit_indicator(self):
        selection = self.indicator_tree.selection()
        if selection:
            item = self.indicator_tree.item(selection[0])
            event_id = int(item['values'][0])
            indicator_id = int(item['values'][1])
            
            for i, ind in enumerate(self.config['indicators']):
                if ind['event_id'] == event_id and ind['indicator_id'] == indicator_id:
                    dialog = IndicatorDialog(self.root, "Edit Indicator", ind, self.config['events'])
                    if dialog.result:
                        self.config['indicators'][i] = dialog.result
                        self.refresh_indicator_tree()
                        self.mark_modified()
                    break
                    
    def delete_indicator(self):
        selection = self.indicator_tree.selection()
        if selection and messagebox.askyesno("Confirm", "Delete selected indicator?"):
            item = self.indicator_tree.item(selection[0])
            event_id = int(item['values'][0])
            
            self.config['indicators'] = [i for i in self.config['indicators'] if i['event_id'] != event_id]
            self.refresh_indicator_tree()
            self.mark_modified()
            
    def refresh_indicator_tree(self):
        for item in self.indicator_tree.get_children():
            self.indicator_tree.delete(item)
            
        for ind in self.config['indicators']:
            self.indicator_tree.insert('', tk.END, values=(
                ind['event_id'],
                ind['indicator_id'],
                ind['behavior'],
                ind['failure_cycles'],
                ind['healing_cycles']
            ))
            
        self.update_stats()
        
    # ==================== Cycle Operations ====================
    
    def add_cycle(self):
        dialog = CycleDialog(self.root, "Add Operation Cycle", None)
        if dialog.result:
            self.config['operation_cycles'].append(dialog.result)
            self.refresh_cycle_tree()
            self.mark_modified()
            
    def edit_cycle(self):
        selection = self.cycle_tree.selection()
        if selection:
            item = self.cycle_tree.item(selection[0])
            cycle_id = int(item['values'][0])
            
            for i, cycle in enumerate(self.config['operation_cycles']):
                if cycle['id'] == cycle_id:
                    dialog = CycleDialog(self.root, "Edit Operation Cycle", cycle)
                    if dialog.result:
                        self.config['operation_cycles'][i] = dialog.result
                        self.refresh_cycle_tree()
                        self.mark_modified()
                    break
                    
    def delete_cycle(self):
        selection = self.cycle_tree.selection()
        if selection and messagebox.askyesno("Confirm", "Delete selected cycle?"):
            item = self.cycle_tree.item(selection[0])
            cycle_id = int(item['values'][0])
            
            self.config['operation_cycles'] = [c for c in self.config['operation_cycles'] if c['id'] != cycle_id]
            self.refresh_cycle_tree()
            self.mark_modified()
            
    def refresh_cycle_tree(self):
        for item in self.cycle_tree.get_children():
            self.cycle_tree.delete(item)
            
        for cycle in self.config['operation_cycles']:
            self.cycle_tree.insert('', tk.END, values=(
                cycle['id'],
                cycle['type'],
                'Yes' if cycle['auto_start'] else 'No'
            ))
            
        self.update_stats()
        
    # ==================== File Operations ====================
    
    def new_config(self):
        """Create new configuration"""
        if self.modified:
            if not messagebox.askyesno("Confirm", "Discard unsaved changes?"):
                return
                
        self.config = {
            'project_name': 'New_AUTOSAR_Project',
            'version': {'major': 1, 'minor': 0, 'patch': 0},
            'generation_date': datetime.now().strftime('%Y-%m-%d'),
            'dtcs': [],
            'events': [],
            'indicators': [],
            'operation_cycles': []
        }
        self.current_file = None
        self.refresh_all()
        self.mark_modified(False)
        
    def load_config(self):
        """Load configuration from file"""
        filepath = filedialog.askopenfilename(
            title="Load Configuration",
            filetypes=[("JSON files", "*.json"), ("All files", "*.*")]
        )
        if filepath:
            try:
                with open(filepath, 'r') as f:
                    self.config = json.load(f)
                self.current_file = filepath
                self.refresh_all()
                self.mark_modified(False)
                self.status_var.set(f"Loaded: {filepath}")
            except Exception as e:
                messagebox.showerror("Error", f"Failed to load file: {str(e)}")
                
    def save_config(self):
        """Save configuration"""
        if self.current_file:
            self._save_to_file(self.current_file)
        else:
            self.save_as_config()
            
    def save_as_config(self):
        """Save configuration as new file"""
        filepath = filedialog.asksaveasfilename(
            title="Save Configuration",
            defaultextension=".json",
            filetypes=[("JSON files", "*.json"), ("All files", "*.*")]
        )
        if filepath:
            self._save_to_file(filepath)
            
    def _save_to_file(self, filepath: str):
        """Save configuration to file"""
        try:
            # Update config with current values
            self.config['project_name'] = self.project_name_var.get()
            self.config['version'] = {
                'major': int(self.version_major_var.get() or 1),
                'minor': int(self.version_minor_var.get() or 0),
                'patch': int(self.version_patch_var.get() or 0)
            }
            self.config['generation_date'] = datetime.now().strftime('%Y-%m-%d')
            
            with open(filepath, 'w') as f:
                json.dump(self.config, f, indent=2)
                
            self.current_file = filepath
            self.mark_modified(False)
            self.status_var.set(f"Saved: {filepath}")
            messagebox.showinfo("Success", "Configuration saved successfully")
        except Exception as e:
            messagebox.showerror("Error", f"Failed to save file: {str(e)}")
            
    def import_csv(self):
        """Import from CSV file"""
        filepath = filedialog.askopenfilename(
            title="Import CSV",
            filetypes=[("CSV files", "*.csv"), ("All files", "*.*")]
        )
        if filepath:
            try:
                parser = DTCConfigParser()
                config = parser.parse_csv(filepath)
                self.config = config
                self.refresh_all()
                self.mark_modified(True)
                self.status_var.set(f"Imported: {filepath}")
            except Exception as e:
                messagebox.showerror("Error", f"Failed to import CSV: {str(e)}")
                
    def export_json(self):
        """Export to JSON file"""
        filepath = filedialog.asksaveasfilename(
            title="Export JSON",
            defaultextension=".json",
            filetypes=[("JSON files", "*.json"), ("All files", "*.*")]
        )
        if filepath:
            self._save_to_file(filepath)
            
    # ==================== Tools Operations ====================
    
    def validate_config(self):
        """Validate current configuration"""
        errors = validate_config(self.config)
        if errors:
            msg = "Validation Errors:\n\n" + "\n".join([f"  - {e}" for e in errors])
            messagebox.showerror("Validation Failed", msg)
        else:
            messagebox.showinfo("Validation Passed", "Configuration is valid!")
            
    def generate_code(self):
        """Generate C code from configuration"""
        # Check for jinja2
        try:
            from jinja2 import Environment, FileSystemLoader
        except ImportError:
            messagebox.showerror("Error", "jinja2 is required for code generation\nInstall with: pip install jinja2")
            return
            
        errors = validate_config(self.config)
        if errors:
            messagebox.showerror("Validation Failed", "Please fix validation errors first")
            return
            
        output_dir = filedialog.askdirectory(title="Select Output Directory")
        if not output_dir:
            return
            
        try:
            # Load template
            template_dir = os.path.join(os.path.dirname(__file__), '..', 'templates')
            env = Environment(loader=FileSystemLoader(template_dir))
            template = env.get_template('c_code_template.j2')
            
            # Generate code
            output = template.render(
                project_name=self.config['project_name'],
                version_major=self.config['version']['major'],
                version_minor=self.config['version']['minor'],
                version_patch=self.config['version']['patch'],
                generation_date=datetime.now().strftime('%Y-%m-%d'),
                dtcs=self.config['dtcs'],
                events=self.config['events'],
                indicators=self.config['indicators'],
                operation_cycles=self.config['operation_cycles']
            )
            
            # Save file
            output_file = os.path.join(output_dir, 'Dem_DtcConfig.h')
            with open(output_file, 'w') as f:
                f.write(output)
                
            messagebox.showinfo("Success", f"Generated: {output_file}")
            self.status_var.set(f"Generated: {output_file}")
        except Exception as e:
            messagebox.showerror("Error", f"Code generation failed: {str(e)}")
            
    def add_sample_data(self):
        """Add sample data for testing"""
        if messagebox.askyesno("Confirm", "Add sample DTCs and events? This will append to existing data."):
            # Add sample DTCs
            sample_dtcs = [
                {'code': 0x010101, 'name': 'MISFIRE_CYL_1', 'description': 'Engine Misfire Cylinder 1', 'severity': 'HIGH', 'functional_unit': 1, 'group': 'EMISSION_DTCS', 'priority': 1, 'kind': 'EMISSION_REL_DTCS', 'immediate_storage': True, 'aging_allowed': True, 'aging_threshold': 40, 'aging_cycle': 0},
                {'code': 0x010102, 'name': 'O2_SENSOR_LOW', 'description': 'O2 Sensor Low Voltage', 'severity': 'MEDIUM', 'functional_unit': 1, 'group': 'EMISSION_DTCS', 'priority': 2, 'kind': 'EMISSION_REL_DTCS', 'immediate_storage': False, 'aging_allowed': True, 'aging_threshold': 40, 'aging_cycle': 0},
            ]
            
            sample_events = [
                {'id': 0, 'name': 'Misfire_Cyl_1', 'dtc_code': 0x010101, 'debounce_type': 'COUNTER', 'debounce_failed_thr': 127, 'debounce_passed_thr': -128, 'immediate_storage': True, 'freeze_frame_record': 1, 'freeze_frame_count': 1, 'freeze_frame_dids': [0x0100, 0x0101, 0x0105, 0x015C], 'extended_data_record': 1, 'extended_data_count': 1, 'extended_data_size': 4, 'extended_data_rule': 'UPDATE'},
                {'id': 1, 'name': 'O2_Sensor_Low', 'dtc_code': 0x010102, 'debounce_type': 'COUNTER', 'debounce_failed_thr': 64, 'debounce_passed_thr': -64, 'immediate_storage': False, 'freeze_frame_record': 1, 'freeze_frame_count': 1, 'freeze_frame_dids': [0x0100, 0x0101, 0x0105], 'extended_data_record': 1, 'extended_data_count': 1, 'extended_data_size': 4, 'extended_data_rule': 'UPDATE'},
            ]
            
            self.config['dtcs'].extend(sample_dtcs)
            self.config['events'].extend(sample_events)
            self.refresh_all()
            self.mark_modified()
            
    def clear_all_data(self):
        """Clear all data"""
        if messagebox.askyesno("Confirm", "Clear all data? This cannot be undone."):
            self.config['dtcs'] = []
            self.config['events'] = []
            self.config['indicators'] = []
            self.config['operation_cycles'] = []
            self.refresh_all()
            self.mark_modified()
            
    # ==================== Helper Methods ====================
    
    def refresh_all(self):
        """Refresh all treeviews"""
        self.project_name_var.set(self.config['project_name'])
        self.version_major_var.set(str(self.config['version']['major']))
        self.version_minor_var.set(str(self.config['version']['minor']))
        self.version_patch_var.set(str(self.config['version']['patch']))
        
        self.refresh_dtc_tree()
        self.refresh_event_tree()
        self.refresh_indicator_tree()
        self.refresh_cycle_tree()
        
    def update_stats(self):
        """Update statistics label"""
        stats_text = f"DTCs: {len(self.config['dtcs'])} | Events: {len(self.config['events'])} | Indicators: {len(self.config['indicators'])} | Cycles: {len(self.config['operation_cycles'])}"
        self.stats_label.config(text=stats_text)
        
    def mark_modified(self, modified: bool = True):
        """Mark configuration as modified"""
        self.modified = modified
        title = "DTC Configurator Tool"
        if self.current_file:
            title += f" - {os.path.basename(self.current_file)}"
        if modified:
            title += " *"
        self.root.title(title)
        
    def show_about(self):
        """Show about dialog"""
        messagebox.showinfo("About", """DTC Configurator Tool v1.0
AUTOSAR Diagnostic Trouble Code Management

Features:
- DTC definition management
- Event configuration
- Indicator configuration  
- Operation cycle setup
- C code generation
- CSV import/export

Compatible with AUTOSAR Classic Platform
""")
        
    def on_closing(self):
        """Handle window close"""
        if self.modified:
            if messagebox.askyesno("Confirm", "Save changes before exiting?"):
                self.save_config()
        self.root.destroy()


# ==================== Dialog Classes ====================

class DTCDialog:
    """Dialog for adding/editing DTC"""
    
    def __init__(self, parent, title: str, dtc: Optional[Dict]):
        self.result = None
        
        dialog = tk.Toplevel(parent)
        dialog.title(title)
        dialog.geometry("500x500")
        dialog.transient(parent)
        dialog.grab_set()
        
        # Form frame
        form = ttk.Frame(dialog, padding=10)
        form.pack(fill=tk.BOTH, expand=True)
        
        row = 0
        
        # DTC Code
        ttk.Label(form, text="DTC Code (hex):").grid(row=row, column=0, sticky=tk.W, pady=5)
        self.code_var = tk.StringVar(value=f"0x{dtc['code']:06X}" if dtc else "0x000000")
        ttk.Entry(form, textvariable=self.code_var, width=15).grid(row=row, column=1, sticky=tk.W, pady=5)
        row += 1
        
        # Name
        ttk.Label(form, text="Name:").grid(row=row, column=0, sticky=tk.W, pady=5)
        self.name_var = tk.StringVar(value=dtc['name'] if dtc else "")
        ttk.Entry(form, textvariable=self.name_var, width=40).grid(row=row, column=1, sticky=tk.W, pady=5)
        row += 1
        
        # Description
        ttk.Label(form, text="Description:").grid(row=row, column=0, sticky=tk.W, pady=5)
        self.desc_var = tk.StringVar(value=dtc['description'] if dtc else "")
        ttk.Entry(form, textvariable=self.desc_var, width=40).grid(row=row, column=1, sticky=tk.W, pady=5)
        row += 1
        
        # Severity
        ttk.Label(form, text="Severity:").grid(row=row, column=0, sticky=tk.W, pady=5)
        self.severity_var = tk.StringVar(value=dtc['severity'] if dtc else 'MEDIUM')
        severity_combo = ttk.Combobox(form, textvariable=self.severity_var, values=['LOW', 'MEDIUM', 'HIGH', 'CRITICAL'], state='readonly', width=15)
        severity_combo.grid(row=row, column=1, sticky=tk.W, pady=5)
        row += 1
        
        # Functional Unit
        ttk.Label(form, text="Functional Unit (hex):").grid(row=row, column=0, sticky=tk.W, pady=5)
        self.unit_var = tk.StringVar(value=f"0x{dtc['functional_unit']:02X}" if dtc else "0x01")
        ttk.Entry(form, textvariable=self.unit_var, width=10).grid(row=row, column=1, sticky=tk.W, pady=5)
        row += 1
        
        # Group
        ttk.Label(form, text="Group:").grid(row=row, column=0, sticky=tk.W, pady=5)
        self.group_var = tk.StringVar(value=dtc['group'] if dtc else 'ALL_DTCS')
        group_combo = ttk.Combobox(form, textvariable=self.group_var, values=['ALL_DTCS', 'EMISSION_DTCS', 'POWERTRAIN_DTCS', 'CHASSIS_DTCS', 'BODY_DTCS', 'NETWORK_DTCS'], state='readonly', width=20)
        group_combo.grid(row=row, column=1, sticky=tk.W, pady=5)
        row += 1
        
        # Priority
        ttk.Label(form, text="Priority:").grid(row=row, column=0, sticky=tk.W, pady=5)
        self.priority_var = tk.StringVar(value=str(dtc['priority']) if dtc else '1')
        ttk.Spinbox(form, from_=1, to=255, textvariable=self.priority_var, width=10).grid(row=row, column=1, sticky=tk.W, pady=5)
        row += 1
        
        # Kind
        ttk.Label(form, text="Kind:").grid(row=row, column=0, sticky=tk.W, pady=5)
        self.kind_var = tk.StringVar(value=dtc['kind'] if dtc else 'ALL_DTCS')
        kind_combo = ttk.Combobox(form, textvariable=self.kind_var, values=['ALL_DTCS', 'EMISSION_REL_DTCS'], state='readonly', width=20)
        kind_combo.grid(row=row, column=1, sticky=tk.W, pady=5)
        row += 1
        
        # Immediate Storage
        self.immediate_var = tk.BooleanVar(value=dtc['immediate_storage'] if dtc else False)
        ttk.Checkbutton(form, text="Immediate Storage", variable=self.immediate_var).grid(row=row, column=1, sticky=tk.W, pady=5)
        row += 1
        
        # Aging
        aging_frame = ttk.LabelFrame(form, text="Aging Configuration", padding=5)
        aging_frame.grid(row=row, column=0, columnspan=2, sticky=tk.EW, pady=10)
        
        self.aging_var = tk.BooleanVar(value=dtc['aging_allowed'] if dtc else False)
        ttk.Checkbutton(aging_frame, text="Aging Allowed", variable=self.aging_var).grid(row=0, column=0, sticky=tk.W)
        
        ttk.Label(aging_frame, text="Threshold:").grid(row=0, column=1, sticky=tk.W, padx=(20, 0))
        self.aging_threshold_var = tk.StringVar(value=str(dtc['aging_threshold']) if dtc else '40')
        ttk.Entry(aging_frame, textvariable=self.aging_threshold_var, width=10).grid(row=0, column=2, sticky=tk.W, padx=5)
        
        ttk.Label(aging_frame, text="Cycle ID:").grid(row=0, column=3, sticky=tk.W, padx=(10, 0))
        self.aging_cycle_var = tk.StringVar(value=str(dtc['aging_cycle']) if dtc else '0')
        ttk.Entry(aging_frame, textvariable=self.aging_cycle_var, width=10).grid(row=0, column=4, sticky=tk.W, padx=5)
        
        # Buttons
        btn_frame = ttk.Frame(dialog)
        btn_frame.pack(fill=tk.X, padx=10, pady=10)
        
        ttk.Button(btn_frame, text="OK", command=lambda: self.on_ok(dialog)).pack(side=tk.RIGHT, padx=5)
        ttk.Button(btn_frame, text="Cancel", command=dialog.destroy).pack(side=tk.RIGHT, padx=5)
        
        # Wait for dialog
        dialog.wait_window()
        
    def on_ok(self, dialog):
        try:
            code_str = self.code_var.get()
            code = int(code_str, 16) if code_str.startswith('0x') else int(code_str)
            
            unit_str = self.unit_var.get()
            unit = int(unit_str, 16) if unit_str.startswith('0x') else int(unit_str)
            
            self.result = {
                'code': code,
                'name': self.name_var.get() or f'DTC_{code:06X}',
                'description': self.desc_var.get() or 'No description',
                'severity': self.severity_var.get(),
                'functional_unit': unit,
                'group': self.group_var.get(),
                'priority': int(self.priority_var.get()),
                'kind': self.kind_var.get(),
                'immediate_storage': self.immediate_var.get(),
                'aging_allowed': self.aging_var.get(),
                'aging_threshold': int(self.aging_threshold_var.get() or 0),
                'aging_cycle': int(self.aging_cycle_var.get() or 0)
            }
            dialog.destroy()
        except ValueError as e:
            messagebox.showerror("Error", f"Invalid input: {str(e)}")


class EventDialog:
    """Dialog for adding/editing Event"""
    
    def __init__(self, parent, title: str, event: Optional[Dict], dtcs: List[Dict]):
        self.result = None
        
        dialog = tk.Toplevel(parent)
        dialog.title(title)
        dialog.geometry("500x600")
        dialog.transient(parent)
        dialog.grab_set()
        
        form = ttk.Frame(dialog, padding=10)
        form.pack(fill=tk.BOTH, expand=True)
        
        row = 0
        
        # Event ID
        ttk.Label(form, text="Event ID:").grid(row=row, column=0, sticky=tk.W, pady=5)
        self.id_var = tk.StringVar(value=str(event['id']) if event else "0")
        ttk.Entry(form, textvariable=self.id_var, width=15).grid(row=row, column=1, sticky=tk.W, pady=5)
        row += 1
        
        # Name
        ttk.Label(form, text="Name:").grid(row=row, column=0, sticky=tk.W, pady=5)
        self.name_var = tk.StringVar(value=event['name'] if event else "")
        ttk.Entry(form, textvariable=self.name_var, width=40).grid(row=row, column=1, sticky=tk.W, pady=5)
        row += 1
        
        # DTC Selection
        ttk.Label(form, text="DTC:").grid(row=row, column=0, sticky=tk.W, pady=5)
        dtc_codes = [f"0x{d['code']:06X} - {d['name']}" for d in dtcs]
        self.dtc_var = tk.StringVar()
        if event:
            for d in dtcs:
                if d['code'] == event['dtc_code']:
                    self.dtc_var.set(f"0x{d['code']:06X} - {d['name']}")
                    break
        elif dtcs:
            self.dtc_var.set(dtc_codes[0])
        dtc_combo = ttk.Combobox(form, textvariable=self.dtc_var, values=dtc_codes, state='readonly', width=45)
        dtc_combo.grid(row=row, column=1, sticky=tk.W, pady=5)
        row += 1
        
        # Debounce Type
        ttk.Label(form, text="Debounce Type:").grid(row=row, column=0, sticky=tk.W, pady=5)
        self.debounce_var = tk.StringVar(value=event['debounce_type'] if event else 'COUNTER')
        debounce_combo = ttk.Combobox(form, textvariable=self.debounce_var, values=['NO_DEBOUNCE', 'COUNTER', 'TIME', 'MONITOR'], state='readonly', width=20)
        debounce_combo.grid(row=row, column=1, sticky=tk.W, pady=5)
        row += 1
        
        # Debounce Thresholds
        threshold_frame = ttk.LabelFrame(form, text="Debounce Thresholds", padding=5)
        threshold_frame.grid(row=row, column=0, columnspan=2, sticky=tk.EW, pady=5)
        
        ttk.Label(threshold_frame, text="Failed:").grid(row=0, column=0, sticky=tk.W)
        self.fail_var = tk.StringVar(value=str(event['debounce_failed_thr']) if event else '127')
        ttk.Entry(threshold_frame, textvariable=self.fail_var, width=10).grid(row=0, column=1, sticky=tk.W, padx=5)
        
        ttk.Label(threshold_frame, text="Passed:").grid(row=0, column=2, sticky=tk.W, padx=(20, 0))
        self.pass_var = tk.StringVar(value=str(event['debounce_passed_thr']) if event else '-128')
        ttk.Entry(threshold_frame, textvariable=self.pass_var, width=10).grid(row=0, column=3, sticky=tk.W, padx=5)
        row += 1
        
        # Immediate Storage
        self.immediate_var = tk.BooleanVar(value=event['immediate_storage'] if event else False)
        ttk.Checkbutton(form, text="Immediate Storage", variable=self.immediate_var).grid(row=row, column=1, sticky=tk.W, pady=5)
        row += 1
        
        # Freeze Frame
        ff_frame = ttk.LabelFrame(form, text="Freeze Frame Configuration", padding=5)
        ff_frame.grid(row=row, column=0, columnspan=2, sticky=tk.EW, pady=5)
        
        ttk.Label(ff_frame, text="Record Number:").grid(row=0, column=0, sticky=tk.W)
        self.ff_record_var = tk.StringVar(value=str(event['freeze_frame_record']) if event else '1')
        ttk.Entry(ff_frame, textvariable=self.ff_record_var, width=10).grid(row=0, column=1, sticky=tk.W, padx=5)
        
        ttk.Label(ff_frame, text="DID List (hex, comma-separated):").grid(row=1, column=0, columnspan=2, sticky=tk.W, pady=(10, 0))
        dids_str = ', '.join([f"0x{d:04X}" for d in event.get('freeze_frame_dids', [])]) if event else "0x0100, 0x0101, 0x0105"
        self.ff_dids_var = tk.StringVar(value=dids_str)
        ttk.Entry(ff_frame, textvariable=self.ff_dids_var, width=50).grid(row=2, column=0, columnspan=2, sticky=tk.W, pady=5)
        row += 1
        
        # Extended Data
        ext_frame = ttk.LabelFrame(form, text="Extended Data Configuration", padding=5)
        ext_frame.grid(row=row, column=0, columnspan=2, sticky=tk.EW, pady=5)
        
        ttk.Label(ext_frame, text="Record Number:").grid(row=0, column=0, sticky=tk.W)
        self.ext_record_var = tk.StringVar(value=str(event['extended_data_record']) if event else '1')
        ttk.Entry(ext_frame, textvariable=self.ext_record_var, width=10).grid(row=0, column=1, sticky=tk.W, padx=5)
        
        ttk.Label(ext_frame, text="Data Size:").grid(row=0, column=2, sticky=tk.W, padx=(20, 0))
        self.ext_size_var = tk.StringVar(value=str(event['extended_data_size']) if event else '4')
        ttk.Entry(ext_frame, textvariable=self.ext_size_var, width=10).grid(row=0, column=3, sticky=tk.W, padx=5)
        
        ttk.Label(ext_frame, text="Update Rule:").grid(row=1, column=0, sticky=tk.W, pady=(10, 0))
        self.ext_rule_var = tk.StringVar(value=event['extended_data_rule'] if event else 'UPDATE')
        ext_combo = ttk.Combobox(ext_frame, textvariable=self.ext_rule_var, values=['UPDATE', 'STATIC'], state='readonly', width=15)
        ext_combo.grid(row=1, column=1, sticky=tk.W, padx=5, pady=(10, 0))
        row += 1
        
        # Buttons
        btn_frame = ttk.Frame(dialog)
        btn_frame.pack(fill=tk.X, padx=10, pady=10)
        
        ttk.Button(btn_frame, text="OK", command=lambda: self.on_ok(dialog, dtcs)).pack(side=tk.RIGHT, padx=5)
        ttk.Button(btn_frame, text="Cancel", command=dialog.destroy).pack(side=tk.RIGHT, padx=5)
        
        dialog.wait_window()
        
    def on_ok(self, dialog, dtcs):
        try:
            # Parse DTC selection
            dtc_code_str = self.dtc_var.get().split(' - ')[0]
            dtc_code = int(dtc_code_str, 16)
            
            # Parse DID list
            dids_str = self.ff_dids_var.get()
            dids = []
            if dids_str.strip():
                for did in dids_str.split(','):
                    did = did.strip()
                    if did.startswith('0x'):
                        dids.append(int(did, 16))
            
            self.result = {
                'id': int(self.id_var.get()),
                'name': self.name_var.get() or f'Event_{self.id_var.get()}',
                'dtc_code': dtc_code,
                'debounce_type': self.debounce_var.get(),
                'debounce_failed_thr': int(self.fail_var.get()),
                'debounce_passed_thr': int(self.pass_var.get()),
                'immediate_storage': self.immediate_var.get(),
                'freeze_frame_record': int(self.ff_record_var.get()),
                'freeze_frame_count': 1,
                'freeze_frame_dids': dids,
                'extended_data_record': int(self.ext_record_var.get()),
                'extended_data_count': 1,
                'extended_data_size': int(self.ext_size_var.get()),
                'extended_data_rule': self.ext_rule_var.get()
            }
            dialog.destroy()
        except ValueError as e:
            messagebox.showerror("Error", f"Invalid input: {str(e)}")


class IndicatorDialog:
    """Dialog for adding/editing Indicator"""
    
    def __init__(self, parent, title: str, indicator: Optional[Dict], events: List[Dict]):
        self.result = None
        
        dialog = tk.Toplevel(parent)
        dialog.title(title)
        dialog.geometry("400x300")
        dialog.transient(parent)
        dialog.grab_set()
        
        form = ttk.Frame(dialog, padding=10)
        form.pack(fill=tk.BOTH, expand=True)
        
        row = 0
        
        # Event Selection
        ttk.Label(form, text="Event:").grid(row=row, column=0, sticky=tk.W, pady=5)
        event_ids = [f"{e['id']} - {e['name']}" for e in events]
        self.event_var = tk.StringVar()
        if indicator:
            for e in events:
                if e['id'] == indicator['event_id']:
                    self.event_var.set(f"{e['id']} - {e['name']}")
                    break
        elif events:
            self.event_var.set(event_ids[0])
        event_combo = ttk.Combobox(form, textvariable=self.event_var, values=event_ids, state='readonly', width=40)
        event_combo.grid(row=row, column=1, sticky=tk.W, pady=5)
        row += 1
        
        # Indicator ID
        ttk.Label(form, text="Indicator ID:").grid(row=row, column=0, sticky=tk.W, pady=5)
        self.id_var = tk.StringVar(value=str(indicator['indicator_id']) if indicator else "0")
        ttk.Entry(form, textvariable=self.id_var, width=15).grid(row=row, column=1, sticky=tk.W, pady=5)
        row += 1
        
        # Behavior
        ttk.Label(form, text="Behavior:").grid(row=row, column=0, sticky=tk.W, pady=5)
        self.behavior_var = tk.StringVar(value=indicator['behavior'] if indicator else 'CONTINUOUS')
        behavior_combo = ttk.Combobox(form, textvariable=self.behavior_var, values=['OFF', 'CONTINUOUS', 'BLINKING', 'SLOW_BLINK', 'FAST_BLINK'], state='readonly', width=20)
        behavior_combo.grid(row=row, column=1, sticky=tk.W, pady=5)
        row += 1
        
        # Cycles
        cycle_frame = ttk.LabelFrame(form, text="Cycle Thresholds", padding=5)
        cycle_frame.grid(row=row, column=0, columnspan=2, sticky=tk.EW, pady=10)
        
        ttk.Label(cycle_frame, text="Failure Cycles:").grid(row=0, column=0, sticky=tk.W)
        self.fail_var = tk.StringVar(value=str(indicator['failure_cycles']) if indicator else '3')
        ttk.Entry(cycle_frame, textvariable=self.fail_var, width=10).grid(row=0, column=1, sticky=tk.W, padx=5)
        
        ttk.Label(cycle_frame, text="Healing Cycles:").grid(row=0, column=2, sticky=tk.W, padx=(20, 0))
        self.heal_var = tk.StringVar(value=str(indicator['healing_cycles']) if indicator else '3')
        ttk.Entry(cycle_frame, textvariable=self.heal_var, width=10).grid(row=0, column=3, sticky=tk.W, padx=5)
        
        # Buttons
        btn_frame = ttk.Frame(dialog)
        btn_frame.pack(fill=tk.X, padx=10, pady=10)
        
        ttk.Button(btn_frame, text="OK", command=lambda: self.on_ok(dialog)).pack(side=tk.RIGHT, padx=5)
        ttk.Button(btn_frame, text="Cancel", command=dialog.destroy).pack(side=tk.RIGHT, padx=5)
        
        dialog.wait_window()
        
    def on_ok(self, dialog):
        try:
            event_id_str = self.event_var.get().split(' - ')[0]
            
            self.result = {
                'event_id': int(event_id_str),
                'indicator_id': int(self.id_var.get()),
                'behavior': self.behavior_var.get(),
                'failure_cycles': int(self.fail_var.get()),
                'healing_cycles': int(self.heal_var.get())
            }
            dialog.destroy()
        except ValueError as e:
            messagebox.showerror("Error", f"Invalid input: {str(e)}")


class CycleDialog:
    """Dialog for adding/editing Operation Cycle"""
    
    def __init__(self, parent, title: str, cycle: Optional[Dict]):
        self.result = None
        
        dialog = tk.Toplevel(parent)
        dialog.title(title)
        dialog.geometry("350x200")
        dialog.transient(parent)
        dialog.grab_set()
        
        form = ttk.Frame(dialog, padding=10)
        form.pack(fill=tk.BOTH, expand=True)
        
        row = 0
        
        # Cycle ID
        ttk.Label(form, text="Cycle ID:").grid(row=row, column=0, sticky=tk.W, pady=5)
        self.id_var = tk.StringVar(value=str(cycle['id']) if cycle else "0")
        ttk.Entry(form, textvariable=self.id_var, width=15).grid(row=row, column=1, sticky=tk.W, pady=5)
        row += 1
        
        # Cycle Type
        ttk.Label(form, text="Cycle Type:").grid(row=row, column=0, sticky=tk.W, pady=5)
        self.type_var = tk.StringVar(value=cycle['type'] if cycle else 'IGNITION')
        type_combo = ttk.Combobox(form, textvariable=self.type_var, 
            values=['IGNITION', 'OBD_DCY', 'WARMUP', 'POWER', 'DRIVING_CYCLE', 'TIME', 'OTHER'],
            state='readonly', width=20)
        type_combo.grid(row=row, column=1, sticky=tk.W, pady=5)
        row += 1
        
        # Auto Start
        self.auto_var = tk.BooleanVar(value=cycle['auto_start'] if cycle else True)
        ttk.Checkbutton(form, text="Auto Start", variable=self.auto_var).grid(row=row, column=1, sticky=tk.W, pady=5)
        row += 1
        
        # Buttons
        btn_frame = ttk.Frame(dialog)
        btn_frame.pack(fill=tk.X, padx=10, pady=10)
        
        ttk.Button(btn_frame, text="OK", command=lambda: self.on_ok(dialog)).pack(side=tk.RIGHT, padx=5)
        ttk.Button(btn_frame, text="Cancel", command=dialog.destroy).pack(side=tk.RIGHT, padx=5)
        
        dialog.wait_window()
        
    def on_ok(self, dialog):
        try:
            self.result = {
                'id': int(self.id_var.get()),
                'type': self.type_var.get(),
                'auto_start': self.auto_var.get()
            }
            dialog.destroy()
        except ValueError as e:
            messagebox.showerror("Error", f"Invalid input: {str(e)}")


def main():
    """Main entry point"""
    root = tk.Tk()
    
    # Set icon (if available)
    try:
        root.iconbitmap('dtc_icon.ico')
    except:
        pass
    
    app = DTCConfiguratorApp(root)
    root.protocol("WM_DELETE_WINDOW", app.on_closing)
    root.mainloop()


if __name__ == '__main__':
    main()
