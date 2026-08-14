import { Link } from 'react-router-dom';
import { Layers, Cpu, Radio, HardDrive, Terminal, TestTube, ArrowRight, CheckCircle2 } from 'lucide-react';

const stats = [
  { label: 'MCAL 驱动', count: 9, icon: Cpu, color: 'text-emerald-400', bg: 'bg-emerald-400/10' },
  { label: 'ECUAL 模块', count: 9, icon: HardDrive, color: 'text-blue-400', bg: 'bg-blue-400/10' },
  { label: 'Service 模块', count: 8, icon: Radio, color: 'text-violet-400', bg: 'bg-violet-400/10' },
  { label: 'RTE / OS / 集成', count: 4, icon: Terminal, color: 'text-amber-400', bg: 'bg-amber-400/10' },
];

const features = [
  {
    icon: Layers,
    title: '分层架构',
    description: '严格遵循 AutoSAR 分层约束：MCAL → ECUAL → Service → RTE，上层调用下层，同层不直接调用。',
  },
  {
    icon: CheckCircle2,
    title: '规范驱动开发',
    description: '基于 OpenSpec 规范进行 TDD 开发，每个模块都有详细的规格说明、API 定义和测试场景。',
  },
  {
    icon: TestTube,
    title: '跨层集成测试',
    description: '完整的集成测试框架，使用 Mock 替代硬件依赖，验证端到端数据流和初始化序列。',
  },
];

const architectureLayers = [
  { name: 'RTE', desc: 'Run Time Environment - 运行时环境', modules: 'RTE Generator, SWC 接口', color: 'bg-amber-500/20 border-amber-500/30 text-amber-300' },
  { name: 'Service', desc: '服务层 - 通信、存储、诊断', modules: 'Com, PduR, NvM, Dcm, Dem, DoIp, DoCan, BswM', color: 'bg-violet-500/20 border-violet-500/30 text-violet-300' },
  { name: 'ECUAL', desc: 'ECU 抽象层 - 硬件抽象', modules: 'CanIf, CanTp, IoHwAb, EthIf, MemIf, Fee, Ea, FrIf, LinIf', color: 'bg-blue-500/20 border-blue-500/30 text-blue-300' },
  { name: 'MCAL', desc: '微控制器驱动层', modules: 'Mcu, Port, Dio, Can, Spi, Gpt, Pwm, Adc, Wdg', color: 'bg-emerald-500/20 border-emerald-500/30 text-emerald-300' },
  { name: 'Hardware', desc: '硬件层', modules: 'NXP i.MX8M Mini', color: 'bg-slate-700/50 border-slate-600 text-slate-300' },
];

export default function HomePage() {
  return (
    <div className="space-y-16">
      {/* Hero */}
      <section className="text-center py-12 sm:py-20">
        <div className="inline-flex items-center gap-2 px-3 py-1 rounded-full bg-cyan-500/10 text-cyan-400 text-sm font-medium mb-6 border border-cyan-500/20">
          <CheckCircle2 className="w-4 h-4" />
          AutoSAR Classic Platform 4.x
        </div>
        <h1 className="text-4xl sm:text-5xl lg:text-6xl font-bold text-slate-100 mb-6 tracking-tight">
          YuleTech AutoSAR BSW
        </h1>
        <p className="text-lg sm:text-xl text-slate-400 max-w-2xl mx-auto mb-8">
          基于 AutoSAR Classic Platform 4.x 的完整基础软件栈，<br className="hidden sm:block" />
          为 NXP i.MX8M Mini 处理器构建的企业级嵌入式解决方案
        </p>
        <div className="flex flex-wrap items-center justify-center gap-4">
          <Link
            to="/guide"
            className="inline-flex items-center gap-2 px-6 py-3 rounded-lg bg-cyan-600 text-white font-medium hover:bg-cyan-500 transition-colors no-underline"
          >
            快速开始
            <ArrowRight className="w-4 h-4" />
          </Link>
          <Link
            to="/architecture"
            className="inline-flex items-center gap-2 px-6 py-3 rounded-lg bg-slate-800 text-slate-200 font-medium hover:bg-slate-700 transition-colors border border-slate-700 no-underline"
          >
            浏览架构
          </Link>
        </div>
      </section>

      {/* Stats */}
      <section className="grid grid-cols-2 lg:grid-cols-4 gap-4">
        {stats.map((s) => (
          <div
            key={s.label}
            className="p-5 rounded-xl bg-slate-900 border border-slate-800 hover:border-slate-700 transition-colors"
          >
            <div className={`w-10 h-10 rounded-lg ${s.bg} flex items-center justify-center mb-3`}>
              <s.icon className={`w-5 h-5 ${s.color}`} />
            </div>
            <div className="text-2xl font-bold text-slate-100">{s.count}</div>
            <div className="text-sm text-slate-400">{s.label}</div>
          </div>
        ))}
      </section>

      {/* Features */}
      <section>
        <h2 className="text-2xl font-bold text-slate-100 mb-6">核心特性</h2>
        <div className="grid sm:grid-cols-2 lg:grid-cols-3 gap-6">
          {features.map((f) => (
            <div
              key={f.title}
              className="p-6 rounded-xl bg-slate-900 border border-slate-800 hover:border-slate-700 transition-colors"
            >
              <div className="w-10 h-10 rounded-lg bg-cyan-500/10 flex items-center justify-center mb-4">
                <f.icon className="w-5 h-5 text-cyan-400" />
              </div>
              <h3 className="text-lg font-semibold text-slate-100 mb-2">{f.title}</h3>
              <p className="text-slate-400 text-sm leading-relaxed">{f.description}</p>
            </div>
          ))}
        </div>
      </section>

      {/* Architecture Overview */}
      <section>
        <h2 className="text-2xl font-bold text-slate-100 mb-6">架构概览</h2>
        <div className="p-6 rounded-xl bg-slate-900 border border-slate-800">
          <div className="space-y-3">
            {architectureLayers.map((layer, idx) => (
              <div key={layer.name}>
                <div
                  className={`flex flex-col sm:flex-row sm:items-center gap-2 sm:gap-4 p-4 rounded-lg border ${layer.color} ${idx === 0 ? 'rounded-t-lg' : ''} ${idx === architectureLayers.length - 1 ? 'rounded-b-lg' : ''}`}
                >
                  <div className="font-mono font-bold text-lg min-w-[80px]">{layer.name}</div>
                  <div className="flex-1 text-sm opacity-90">{layer.desc}</div>
                  <div className="text-xs opacity-70 font-mono">{layer.modules}</div>
                </div>
                {idx < architectureLayers.length - 1 && (
                  <div className="flex justify-center py-1">
                    <ArrowRight className="w-4 h-4 text-slate-600 rotate-90" />
                  </div>
                )}
              </div>
            ))}
          </div>
          <div className="mt-6 text-sm text-slate-400">
            <p>依赖方向：上层可以调用下层，下层不能调用上层，同层之间不能直接调用（需通过 Service 层路由）。</p>
          </div>
        </div>
      </section>
    </div>
  );
}
