import { Link } from 'react-router-dom';
import { CheckCircle2, Circle, Clock, Layers } from 'lucide-react';
import { layers } from '../data/modules';
import type { ModuleStatus } from '../data/modules';

const statusConfig: Record<ModuleStatus, { label: string; icon: typeof CheckCircle2; color: string; bg: string }> = {
  completed: { label: '已完成', icon: CheckCircle2, color: 'text-emerald-400', bg: 'bg-emerald-400/10 border-emerald-400/20' },
  in_progress: { label: '进行中', icon: Clock, color: 'text-amber-400', bg: 'bg-amber-400/10 border-amber-400/20' },
  planned: { label: '规划中', icon: Circle, color: 'text-slate-400', bg: 'bg-slate-700/30 border-slate-600/30' },
};

export default function ModulesPage() {
  return (
    <div className="space-y-12">
      <div>
        <h1>模块索引</h1>
        <p className="text-slate-400">
          按层分类展示 YuleTech AutoSAR BSW 栈中的所有模块。点击模块卡片查看详细文档。
        </p>
      </div>

      {layers.map((layer) => (
        <section key={layer.id}>
          <div className="flex items-center gap-3 mb-4">
            <Layers className="w-5 h-5 text-cyan-400" />
            <h2 className="text-xl font-bold text-slate-100">{layer.name}</h2>
            <span className="text-sm text-slate-500">{layer.modules.length} 个模块</span>
          </div>
          <p className="text-sm text-slate-400 mb-4">{layer.description}</p>
          <div className="grid sm:grid-cols-2 lg:grid-cols-3 gap-4">
            {layer.modules.map((mod) => {
              const status = statusConfig[mod.status];
              const StatusIcon = status.icon;
              return (
                <Link
                  key={mod.id}
                  to={`/module/${mod.layer}/${mod.id}`}
                  className="group p-5 rounded-xl bg-slate-900 border border-slate-800 hover:border-cyan-500/40 transition-all no-underline"
                >
                  <div className="flex items-center justify-between mb-3">
                    <h3 className="text-lg font-semibold text-slate-100 group-hover:text-cyan-400 transition-colors">
                      {mod.name}
                    </h3>
                    <span
                      className={`inline-flex items-center gap-1 px-2 py-0.5 rounded-full text-xs font-medium border ${status.bg} ${status.color}`}
                    >
                      <StatusIcon className="w-3 h-3" />
                      {status.label}
                    </span>
                  </div>
                  <p className="text-sm text-slate-400 leading-relaxed">{mod.description}</p>
                </Link>
              );
            })}
          </div>
        </section>
      ))}
    </div>
  );
}
