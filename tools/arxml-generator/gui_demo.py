#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
yuleASR ARXML Generator - Tkinter Demo GUI

使用Python内置tkinter的演示版本
"""

import sys
import tkinter as tk
from tkinter import ttk, messagebox, scrolledtext, filedialog
from pathlib import Path

# 添加src到路径
sys.path.insert(0, str(Path(__file__).parent / "src"))

from mcal_config_generator import create_mcu_config, create_port_config, create_can_config
from bsw_config_generator import create_com_config, create_nvm_config


class ArxmlDemoGUI:
    """ARXML生成器演示GUI"""
    
    def __init__(self, root):
        self.root = root
        self.root.title("yuleASR ARXML Generator - 可视化配置工具 (Demo)")
        self.root.geometry("1200x800")
        self.root.configure(bg='#f0f0f0')
        
        # 当前状态
        self.current_module = None
        self.generated_arxml = ""
        self.config_widgets = {}
        
        self.init_ui()
        
    def init_ui(self):
        """初始化界面"""
        # 创建主框架
        main_frame = ttk.Frame(self.root, padding="10")
        main_frame.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        
        # 配置网格权重
        self.root.columnconfigure(0, weight=1)
        self.root.rowconfigure(0, weight=1)
        main_frame.columnconfigure(1, weight=2)
        main_frame.columnconfigure(2, weight=2)
        main_frame.rowconfigure(0, weight=1)
        
        # ========== 左侧面板：模块选择 ==========
        left_frame = ttk.LabelFrame(main_frame, text="📦 模块选择", padding="10")
        left_frame.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S), padx=5, pady=5)
        
        # MCAL模块
        ttk.Label(left_frame, text="MCAL 微控制器驱动", font=('Microsoft YaHei', 10, 'bold')).pack(anchor=tk.W, pady=(0, 5))
        
        mcal_modules = [
            ("Mcu", "🔫 微控制器驱动", '#dbeafe'),
            ("Port", "🔗 GPIO配置", '#dbeafe'),
            ("Can", "📡 CAN通信", '#dbeafe'),
            ("Spi", "🔄 SPI通信", '#dbeafe'),
        ]
        
        for mod_id, mod_name, color in mcal_modules:
            btn = tk.Button(left_frame, text=mod_name, bg=color, fg='#1e40af',
                          font=('Microsoft YaHei', 10), relief=tk.FLAT,
                          command=lambda m=mod_id: self.select_module(m))
            btn.pack(fill=tk.X, pady=2)
            
        # BSW模块
        ttk.Label(left_frame, text="BSW 基础软件", font=('Microsoft YaHei', 10, 'bold')).pack(anchor=tk.W, pady=(15, 5))
        
        bsw_modules = [
            ("Com", "💬 通信服务", '#d1fae5'),
            ("PduR", "📋 PDU路由", '#d1fae5'),
            ("NvM", "💾 NVRAM管理", '#d1fae5'),
        ]
        
        for mod_id, mod_name, color in bsw_modules:
            btn = tk.Button(left_frame, text=mod_name, bg=color, fg='#065f46',
                          font=('Microsoft YaHei', 10), relief=tk.FLAT,
                          command=lambda m=mod_id: self.select_module(m))
            btn.pack(fill=tk.X, pady=2)
            
        # ========== 中间面板：配置编辑 ==========
        center_frame = ttk.LabelFrame(main_frame, text="⚙️ 配置编辑", padding="10")
        center_frame.grid(row=0, column=1, sticky=(tk.W, tk.E, tk.N, tk.S), padx=5, pady=5)
        
        # ECU名称
        ecu_frame = ttk.Frame(center_frame)
        ecu_frame.pack(fill=tk.X, pady=5)
        ttk.Label(ecu_frame, text="ECU名称:").pack(side=tk.LEFT)
        self.ecu_entry = ttk.Entry(ecu_frame, width=20)
        self.ecu_entry.insert(0, "ECU0")
        self.ecu_entry.pack(side=tk.LEFT, padx=5)
        
        # 配置区域（带滚动条）
        self.config_canvas = tk.Canvas(center_frame, bg='#fafafa', highlightthickness=1, highlightbackground='#ddd')
        scrollbar = ttk.Scrollbar(center_frame, orient=tk.VERTICAL, command=self.config_canvas.yview)
        self.config_frame = ttk.Frame(self.config_canvas)
        
        self.config_frame.bind("<Configure>", lambda e: self.config_canvas.configure(scrollregion=self.config_canvas.bbox("all")))
        
        self.config_canvas.create_window((0, 0), window=self.config_frame, anchor=tk.NW, width=380)
        self.config_canvas.configure(yscrollcommand=scrollbar.set)
        
        self.config_canvas.pack(side=tk.LEFT, fill=tk.BOTH, expand=True, pady=10)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
        
        # 默认提示
        self.default_label = ttk.Label(self.config_frame, text="👆 请从左侧选择一个模块\n进行配置",
                                      font=('Microsoft YaHei', 12), foreground='#999')
        self.default_label.pack(pady=50)
        
        # 生成按钮
        self.generate_btn = tk.Button(center_frame, text="🚀 生成ARXML", bg='#3b82f6', fg='white',
                                     font=('Microsoft YaHei', 12, 'bold'), relief=tk.FLAT,
                                     command=self.generate_arxml, state=tk.DISABLED)
        self.generate_btn.pack(fill=tk.X, pady=(10, 0), ipady=5)
        
        # ========== 右侧面板：ARXML预览 ==========
        right_frame = ttk.LabelFrame(main_frame, text="📄 ARXML 预览", padding="10")
        right_frame.grid(row=0, column=2, sticky=(tk.W, tk.E, tk.N, tk.S), padx=5, pady=5)
        
        # 工具按钮
        btn_frame = ttk.Frame(right_frame)
        btn_frame.pack(fill=tk.X, pady=(0, 5))
        
        ttk.Button(btn_frame, text="📋 复制", command=self.copy_to_clipboard).pack(side=tk.LEFT, padx=2)
        ttk.Button(btn_frame, text="💾 下载", command=self.download_arxml).pack(side=tk.LEFT, padx=2)
        
        # 预览文本框
        self.preview_text = scrolledtext.ScrolledText(right_frame, wrap=tk.NONE, font=('Consolas', 10),
                                                     bg='#1f2937', fg='#4ade80', insertbackground='white')
        self.preview_text.pack(fill=tk.BOTH, expand=True)
        self.preview_text.insert(tk.END, "<!-- 选择模块并配置后，点击'生成ARXML'查看结果 -->")
        
        # 状态栏
        self.status_var = tk.StringVar(value="就绪 - 请选择模块")
        status_bar = ttk.Label(main_frame, textvariable=self.status_var, relief=tk.SUNKEN)
        status_bar.grid(row=1, column=0, columnspan=3, sticky=(tk.W, tk.E), pady=(5, 0))
        
    def select_module(self, module_id):
        """选择模块"""
        self.current_module = module_id
        self.status_var.set(f"当前模块: {module_id}")
        self.generate_btn.config(state=tk.NORMAL)
        
        # 清空配置区域
        for widget in self.config_frame.winfo_children():
            widget.destroy()
            
        self.config_widgets = {}
        
        # 根据模块创建配置表单
        if module_id == "Mcu":
            self.create_mcu_form()
        elif module_id == "Port":
            self.create_port_form()
        elif module_id == "Can":
            self.create_can_form()
        elif module_id == "Com":
            self.create_com_form()
        elif module_id == "NvM":
            self.create_nvm_form()
        else:
            ttk.Label(self.config_frame, text=f"{module_id} 配置表单", font=('Microsoft YaHei', 12)).pack(pady=20)
            
    def create_mcu_form(self):
        """创建MCU配置表单"""
        # 通用配置
        general_frame = ttk.LabelFrame(self.config_frame, text="通用配置", padding="10")
        general_frame.pack(fill=tk.X, pady=5, padx=5)
        
        self.config_widgets['dev_error'] = tk.BooleanVar(value=True)
        ttk.Checkbutton(general_frame, text="启用开发错误检测", variable=self.config_widgets['dev_error']).pack(anchor=tk.W)
        
        self.config_widgets['init_clock'] = tk.BooleanVar(value=True)
        ttk.Checkbutton(general_frame, text="初始化时钟", variable=self.config_widgets['init_clock']).pack(anchor=tk.W)
        
        self.config_widgets['version_api'] = tk.BooleanVar(value=False)
        ttk.Checkbutton(general_frame, text="版本信息API", variable=self.config_widgets['version_api']).pack(anchor=tk.W)
        
        # 时钟配置
        clock_frame = ttk.LabelFrame(self.config_frame, text="时钟配置", padding="10")
        clock_frame.pack(fill=tk.X, pady=5, padx=5)
        
        ttk.Label(clock_frame, text="CPU时钟频率:").pack(anchor=tk.W)
        self.config_widgets['cpu_clock'] = ttk.Spinbox(clock_frame, from_=1000000, to=200000000, increment=1000000)
        self.config_widgets['cpu_clock'].set(80000000)
        self.config_widgets['cpu_clock'].pack(fill=tk.X, pady=2)
        
        ttk.Label(clock_frame, text="外设时钟频率:").pack(anchor=tk.W, pady=(10, 0))
        self.config_widgets['periph_clock'] = ttk.Spinbox(clock_frame, from_=1000000, to=100000000, increment=1000000)
        self.config_widgets['periph_clock'].set(40000000)
        self.config_widgets['periph_clock'].pack(fill=tk.X, pady=2)
        
    def create_port_form(self):
        """创建Port配置表单"""
        general_frame = ttk.LabelFrame(self.config_frame, text="通用配置", padding="10")
        general_frame.pack(fill=tk.X, pady=5, padx=5)
        
        self.config_widgets['dev_error'] = tk.BooleanVar(value=True)
        ttk.Checkbutton(general_frame, text="启用开发错误检测", variable=self.config_widgets['dev_error']).pack(anchor=tk.W)
        
        # 引脚配置
        pin_frame = ttk.LabelFrame(self.config_frame, text="引脚配置 (前4个)", padding="10")
        pin_frame.pack(fill=tk.X, pady=5, padx=5)
        
        self.config_widgets['pins'] = []
        for i in range(4):
            pin_subframe = ttk.Frame(pin_frame)
            pin_subframe.pack(fill=tk.X, pady=3)
            
            ttk.Label(pin_subframe, text=f"引脚{i}:", width=6).pack(side=tk.LEFT)
            
            name_entry = ttk.Entry(pin_subframe, width=12)
            name_entry.insert(0, f"PortPin_{i}")
            name_entry.pack(side=tk.LEFT, padx=2)
            
            dir_combo = ttk.Combobox(pin_subframe, values=["PORT_PIN_OUT", "PORT_PIN_IN"], width=14, state='readonly')
            dir_combo.set("PORT_PIN_OUT")
            dir_combo.pack(side=tk.LEFT, padx=2)
            
            mode_combo = ttk.Combobox(pin_subframe, values=["PORT_PIN_MODE_GPIO", "PORT_PIN_MODE_CAN", "PORT_PIN_MODE_SPI"], width=18, state='readonly')
            mode_combo.set("PORT_PIN_MODE_GPIO")
            mode_combo.pack(side=tk.LEFT, padx=2)
            
            self.config_widgets['pins'].append((name_entry, dir_combo, mode_combo))
            
    def create_can_form(self):
        """创建CAN配置表单"""
        general_frame = ttk.LabelFrame(self.config_frame, text="通用配置", padding="10")
        general_frame.pack(fill=tk.X, pady=5, padx=5)
        
        self.config_widgets['dev_error'] = tk.BooleanVar(value=True)
        ttk.Checkbutton(general_frame, text="启用开发错误检测", variable=self.config_widgets['dev_error']).pack(anchor=tk.W)
        
        ttk.Label(general_frame, text="控制器索引:").pack(anchor=tk.W, pady=(10, 0))
        self.config_widgets['index'] = ttk.Spinbox(general_frame, from_=0, to=255)
        self.config_widgets['index'].set(0)
        self.config_widgets['index'].pack(fill=tk.X)
        
        # 控制器配置
        ctrl_frame = ttk.LabelFrame(self.config_frame, text="控制器配置", padding="10")
        ctrl_frame.pack(fill=tk.X, pady=5, padx=5)
        
        ttk.Label(ctrl_frame, text="波特率 (bps):").pack(anchor=tk.W)
        self.config_widgets['baudrate'] = ttk.Combobox(ctrl_frame, values=["125000", "250000", "500000", "1000000"], state='readonly')
        self.config_widgets['baudrate'].set("500000")
        self.config_widgets['baudrate'].pack(fill=tk.X, pady=2)
        
        ttk.Label(ctrl_frame, text="Tx对象数量:").pack(anchor=tk.W, pady=(10, 0))
        self.config_widgets['tx_objects'] = ttk.Spinbox(ctrl_frame, from_=1, to=64)
        self.config_widgets['tx_objects'].set(4)
        self.config_widgets['tx_objects'].pack(fill=tk.X)
        
        ttk.Label(ctrl_frame, text="Rx对象数量:").pack(anchor=tk.W, pady=(10, 0))
        self.config_widgets['rx_objects'] = ttk.Spinbox(ctrl_frame, from_=1, to=64)
        self.config_widgets['rx_objects'].set(2)
        self.config_widgets['rx_objects'].pack(fill=tk.X)
        
    def create_com_form(self):
        """创建COM配置表单"""
        general_frame = ttk.LabelFrame(self.config_frame, text="通用配置", padding="10")
        general_frame.pack(fill=tk.X, pady=5, padx=5)
        
        self.config_widgets['dev_error'] = tk.BooleanVar(value=True)
        ttk.Checkbutton(general_frame, text="启用开发错误检测", variable=self.config_widgets['dev_error']).pack(anchor=tk.W)
        
        self.config_widgets['update_check'] = tk.BooleanVar(value=True)
        ttk.Checkbutton(general_frame, text="启用Update位检查", variable=self.config_widgets['update_check']).pack(anchor=tk.W)
        
        self.config_widgets['signal_check'] = tk.BooleanVar(value=True)
        ttk.Checkbutton(general_frame, text="启用信号检查", variable=self.config_widgets['signal_check']).pack(anchor=tk.W)
        
        ttk.Label(self.config_frame, text="📋 COM详细配置\n(信号/IPDU配置请使用完整版)", 
                 font=('Microsoft YaHei', 10), foreground='#666').pack(pady=20)
        
    def create_nvm_form(self):
        """创建NvM配置表单"""
        common_frame = ttk.LabelFrame(self.config_frame, text="通用配置", padding="10")
        common_frame.pack(fill=tk.X, pady=5, padx=5)
        
        ttk.Label(common_frame, text="NVRAM块数量:").pack(anchor=tk.W)
        self.config_widgets['blocks'] = ttk.Spinbox(common_frame, from_=1, to=256)
        self.config_widgets['blocks'].set(8)
        self.config_widgets['blocks'].pack(fill=tk.X, pady=2)
        
        ttk.Label(common_frame, text="CRC类型:").pack(anchor=tk.W, pady=(10, 0))
        self.config_widgets['crc'] = ttk.Combobox(common_frame, values=["NVM_CRC32", "NVM_CRC16", "NVM_CRC8", "NVM_CRC_NONE"], state='readonly')
        self.config_widgets['crc'].set("NVM_CRC32")
        self.config_widgets['crc'].pack(fill=tk.X)
        
        self.config_widgets['dev_error'] = tk.BooleanVar(value=True)
        ttk.Checkbutton(common_frame, text="启用开发错误检测", variable=self.config_widgets['dev_error']).pack(anchor=tk.W, pady=(10, 0))
        
    def generate_arxml(self):
        """生成ARXML"""
        if not self.current_module:
            messagebox.showwarning("警告", "请先选择一个模块")
            return
            
        ecu_name = self.ecu_entry.get() or "ECU0"
        
        try:
            if self.current_module == "Mcu":
                gen = create_mcu_config(ecu_name)
                gen.add_general_config(
                    dev_error_detect=self.config_widgets['dev_error'].get(),
                    init_clock=self.config_widgets['init_clock'].get(),
                    version_info_api=self.config_widgets['version_api'].get()
                )
                gen.add_clock_config(
                    cpu_clock=int(self.config_widgets['cpu_clock'].get()),
                    peripheral_clock=int(self.config_widgets['periph_clock'].get())
                )
                
            elif self.current_module == "Port":
                gen = create_port_config(ecu_name)
                gen.add_general_config(dev_error_detect=self.config_widgets['dev_error'].get())
                for name_entry, dir_combo, mode_combo in self.config_widgets['pins']:
                    gen.add_pin_config(
                        pin_name=name_entry.get(),
                        pin_id=0,
                        direction=dir_combo.get(),
                        mode=mode_combo.get()
                    )
                    
            elif self.current_module == "Can":
                gen = create_can_config(ecu_name)
                gen.add_general_config(
                    dev_error_detect=self.config_widgets['dev_error'].get(),
                    index=int(self.config_widgets['index'].get()),
                    main_function_period=10.0
                )
                gen.add_controller_config(
                    controller_id=0,
                    baudrate=int(self.config_widgets['baudrate'].get()),
                    tx_objects=int(self.config_widgets['tx_objects'].get()),
                    rx_objects=int(self.config_widgets['rx_objects'].get())
                )
                
            elif self.current_module == "Com":
                gen = create_com_config(ecu_name)
                gen.add_general_config(
                    dev_error_detect=self.config_widgets['dev_error'].get(),
                    enable_update_bit_check=self.config_widgets['update_check'].get(),
                    signal_change_check=self.config_widgets['signal_check'].get()
                )
                
            elif self.current_module == "NvM":
                gen = create_nvm_config(ecu_name)
                crc_map = {"NVM_CRC8": 1, "NVM_CRC16": 2, "NVM_CRC32": 4, "NVM_CRC_NONE": 0}
                crc_bytes = crc_map.get(self.config_widgets['crc'].get(), 4)
                gen.add_common_config(crc_num_bytes=crc_bytes)
                
                num_blocks = int(self.config_widgets['blocks'].get())
                for i in range(min(num_blocks, 8)):  # 限制演示版本最多8个块
                    gen.add_block_descriptor(
                        block_name=f"NvMBlockDescriptor_{i}",
                        block_id=i,
                        block_size=32 + i * 16,
                        crc_type=self.config_widgets['crc'].get()
                    )
            else:
                messagebox.showinfo("提示", f"{self.current_module} 演示版暂未实现")
                return
                
            self.generated_arxml = gen.to_arxml()
            
            # 显示在预览区
            self.preview_text.delete(1.0, tk.END)
            # 简单的高亮效果
            lines = self.generated_arxml.split('\n')
            for line in lines[:100]:  # 限制显示行数
                self.preview_text.insert(tk.END, line + '\n')
            if len(lines) > 100:
                self.preview_text.insert(tk.END, f"\n... ({len(lines) - 100} 行已省略) ...")
                
            self.status_var.set(f"✓ {self.current_module} 配置生成成功")
            
        except Exception as e:
            messagebox.showerror("错误", f"生成ARXML失败:\n{str(e)}")
            
    def copy_to_clipboard(self):
        """复制到剪贴板"""
        if not self.generated_arxml:
            messagebox.showwarning("警告", "请先生成ARXML")
            return
        self.root.clipboard_clear()
        self.root.clipboard_append(self.generated_arxml)
        self.status_var.set("✓ 已复制到剪贴板")
        
    def download_arxml(self):
        """下载ARXML"""
        if not self.generated_arxml:
            messagebox.showwarning("警告", "请先生成ARXML")
            return
            
        filename = filedialog.asksaveasfilename(
            defaultextension=".arxml",
            initialfile=f"{self.current_module}.arxml",
            filetypes=[("ARXML files", "*.arxml"), ("All files", "*.*")]
        )
        
        if filename:
            try:
                with open(filename, 'w', encoding='utf-8') as f:
                    f.write(self.generated_arxml)
                self.status_var.set(f"✓ 已保存到 {filename}")
            except Exception as e:
                messagebox.showerror("错误", f"保存失败:\n{str(e)}")


def main():
    """主函数"""
    print("=" * 60)
    print("🚀 yuleASR ARXML Generator - Demo GUI")
    print("=" * 60)
    print("使用Python内置tkinter库，无需额外安装")
    print("支持的模块: Mcu, Port, Can, Com, NvM")
    print("=" * 60)
    
    root = tk.Tk()
    app = ArxmlDemoGUI(root)
    
    print("\n✅ GUI已启动")
    print("💡 提示: 从左侧选择模块，配置参数，点击生成")
    print()
    
    root.mainloop()


if __name__ == "__main__":
    main()
