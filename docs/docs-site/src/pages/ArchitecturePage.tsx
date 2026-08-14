import { ArrowDown, Layers, Cpu, Radio, HardDrive, Terminal, Globe } from 'lucide-react';

const archLayers = [
  {
    id: 'rte',
    name: 'RTE',
    fullName: 'Run Time Environment',
    icon: Terminal,
    color: 'bg-amber-500/15 border-amber-500/30 text-amber-300',
    modules: ['RTE', 'RTE Generator'],
    description: '运行时环境，提供软件组件间的标准接口。RTE Generator 根据 JSON 配置自动生成 SWC 接口代码。',
  },
  {
    id: 'services',
    name: 'Service Layer',
    fullName: '服务层',
    icon: Radio,
    color: 'bg-violet-500/15 border-violet-500/30 text-violet-300',
    modules: ['Com', 'PduR', 'NvM', 'Dcm', 'Dem', 'DoIp', 'DoCan', 'BswM'],
    description: '提供通信、存储、诊断、模式管理等核心服务。同层模块间不直接调用，通过标准化接口交互。',
  },
  {
    id: 'ecual',
    name: 'ECUAL',
    fullName: 'ECU Abstraction Layer',
    icon: HardDrive,
    color: 'bg-blue-500/15 border-blue-500/30 text-blue-300',
    modules: ['CanIf', 'CanTp', 'IoHwAb', 'EthIf', 'MemIf', 'Fee', 'Ea', 'FrIf', 'LinIf'],
    description: 'ECU 抽象层，将硬件外设抽象为标准化接口，使上层模块与具体硬件解耦。',
  },
  {
    id: 'mcal',
    name: 'MCAL',
    fullName: 'Microcontroller Driver Layer',
    icon: Cpu,
    color: 'bg-emerald-500/15 border-emerald-500/30 text-emerald-300',
    modules: ['Mcu', 'Port', 'Dio', 'Can', 'Spi', 'Gpt', 'Pwm', 'Adc', 'Wdg'],
    description: '微控制器驱动层，直接操作 i.MX8M Mini 的片上外设（CAN、SPI、ADC、PWM 等）。',
  },
  {
    id: 'os',
    name: 'OS / Integration',
    fullName: '操作系统与集成层',
    icon: Layers,
    color: 'bg-cyan-500/15 border-cyan-500/30 text-cyan-300',
    modules: ['OS', 'EcuM', 'BswM', 'Integration Tests'],
    description: '操作系统层和集成层，负责任务调度、ECU 状态管理和跨层集成测试。',
  },
  {
    id: 'hw',
    name: 'Hardware',
    fullName: '硬件层',
    icon: Globe,
    color: 'bg-slate-700/40 border-slate-600 text-slate-300',
    modules: ['NXP i.MX8M Mini'],
    description: '目标硬件平台：NXP i.MX8M Mini 应用处理器，ARM Cortex-A53 内核。',
  },
];

const hwMapping = [
  { mcal: 'Mcu', hw: 'Clock, Reset, Power', desc: '时钟树配置、复位控制、电源管理' },
  { mcal: 'Port / Dio', hw: 'GPIO', desc: '引脚复用配置和数字 IO' },
  { mcal: 'Can', hw: 'FlexCAN', desc: 'CAN 2.0B / CAN FD 控制器' },
  { mcal: 'Spi', hw: 'ECSPI / LPSPI', desc: 'SPI 主/从通信' },
  { mcal: 'Gpt', hw: 'GPT / TTMR', desc: '通用定时器和周期中断' },
  { mcal: 'Pwm', hw: 'PWM', desc: '脉宽调制输出' },
  { mcal: 'Adc', hw: 'ADC1 / ADC2', desc: '12 位模数转换器' },
  { mcal: 'Wdg', hw: 'WDOG / RWDOG', desc: '看门狗定时器' },
];

export default function ArchitecturePage() {
  return (
    <div className="space-y-12">
      <div>
        <h1>系统架构</h1>
        <p className="text-slate-400">
          YuleTech AutoSAR BSW 栈采用经典的分层架构，严格遵循 AutoSAR Classic Platform 4.x 标准。
          每一层都有明确的职责边界和调用约束。
        </p>
      </div>

      {/* Layer Diagram */}
      <section>
        <h2>六层架构图</h2>
        <div className="space-y-2">
          {archLayers.map((layer, idx) => (
            <div key={layer.id}>
              <div className={`p-5 rounded-xl border ${layer.color} transition-all hover:brightness-110`}>
                <div className="flex items-start gap-4">
                  <div className="w-10 h-10 rounded-lg bg-white/5 flex items-center justify-center shrink-0">
                    <layer.icon className="w-5 h-5" />
                  </div>
                  <div className="flex-1 min-w-0">
                    <div className="flex flex-col sm:flex-row sm:items-center gap-1 sm:gap-3 mb-2">
                      <h3 className="text-lg font-bold">{layer.name}</h3>
                      <span className="text-xs opacity-70 font-mono">{layer.fullName}</span>
                    </div>
                    <p className="text-sm opacity-90 mb-3">{layer.description}</p>
                    <div className="flex flex-wrap gap-2">
                      {layer.modules.map((m) => (
                        <span
                          key={m}
                          className="px-2.5 py-1 rounded-md bg-black/20 text-xs font-mono font-medium"
                        >
                          {m}
                        </span>
                      ))}
                    </div>
                  </div>
                </div>
              </div>
              {idx < archLayers.length - 1 && (
                <div className="flex justify-center py-1">
                  <ArrowDown className="w-4 h-4 text-slate-600" />
                </div>
              )}
            </div>
          ))}
        </div>
      </section>

      {/* Dependency Rules */}
      <section>
        <h2>依赖方向约束</h2>
        <div className="grid sm:grid-cols-3 gap-4">
          <div className="p-5 rounded-xl bg-slate-900 border border-slate-800">
            <div className="text-emerald-400 font-semibold mb-2">上层调用下层</div>
            <p className="text-sm text-slate-400">
              Service 层可以调用 ECUAL 层和 MCAL 层的 API。ECUAL 层可以调用 MCAL 层。
            </p>
          </div>
          <div className="p-5 rounded-xl bg-slate-900 border border-slate-800">
            <div className="text-rose-400 font-semibold mb-2">下层不能调用上层</div>
            <p className="text-sm text-slate-400">
              MCAL 驱动不能直接调用 ECUAL 或 Service 层。回调通过配置的函数指针实现。
            </p>
          </div>
          <div className="p-5 rounded-xl bg-slate-900 border border-slate-800">
            <div className="text-amber-400 font-semibold mb-2">同层不直接调用</div>
            <p className="text-sm text-slate-400">
              同层模块之间不直接调用，通信需通过 Service 层路由（如 PduR）或 OS 事件机制。
            </p>
          </div>
        </div>
      </section>

      {/* Hardware Mapping */}
      <section>
        <h2>i.MX8M Mini 硬件映射</h2>
        <div className="overflow-x-auto">
          <table>
            <thead>
              <tr>
                <th>MCAL 驱动</th>
                <th>硬件外设</th>
                <th>功能描述</th>
              </tr>
            </thead>
            <tbody>
              {hwMapping.map((row) => (
                <tr key={row.mcal}>
                  <td className="font-mono text-cyan-400">{row.mcal}</td>
                  <td>{row.hw}</td>
                  <td>{row.desc}</td>
                </tr>
              ))}
            </tbody>
          </table>
        </div>
      </section>
    </div>
  );
}
