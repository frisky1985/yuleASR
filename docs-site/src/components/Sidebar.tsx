import { useState } from 'react';
import { Link, useParams } from 'react-router-dom';
import { ChevronDown, ChevronRight } from 'lucide-react';
import { layers } from '../data/modules';

export default function Sidebar() {
  const { layer: currentLayer, moduleId: currentModuleId } = useParams<{ layer?: string; moduleId?: string }>();
  const [expanded, setExpanded] = useState<Record<string, boolean>>(() => {
    const init: Record<string, boolean> = {};
    if (currentLayer) {
      init[currentLayer] = true;
    }
    return init;
  });

  return (
    <aside className="w-64 shrink-0 hidden lg:block">
      <div className="sticky top-20 max-h-[calc(100vh-6rem)] overflow-y-auto pr-2">
        <h3 className="text-xs font-semibold text-slate-500 uppercase tracking-wider mb-3 px-2">
          模块导航
        </h3>
        <div className="space-y-1">
          {layers.map((layer) => {
            const isExpanded = expanded[layer.id] ?? false;
            const isActiveLayer = currentLayer === layer.id;
            return (
              <div key={layer.id}>
                <button
                  onClick={() =>
                    setExpanded((prev) => ({ ...prev, [layer.id]: !isExpanded }))
                  }
                  className={`w-full flex items-center gap-2 px-2 py-1.5 rounded-md text-sm font-medium transition-colors text-left ${
                    isActiveLayer
                      ? 'text-cyan-400 bg-slate-800/60'
                      : 'text-slate-300 hover:text-slate-100 hover:bg-slate-800/40'
                  }`}
                >
                  {isExpanded ? (
                    <ChevronDown className="w-3.5 h-3.5 shrink-0" />
                  ) : (
                    <ChevronRight className="w-3.5 h-3.5 shrink-0" />
                  )}
                  {layer.name}
                </button>
                {isExpanded && (
                  <div className="ml-5 mt-1 space-y-0.5 border-l border-slate-800 pl-2">
                    {layer.modules.map((mod) => {
                      const isActive = currentLayer === layer.id && currentModuleId === mod.id;
                      return (
                        <Link
                          key={mod.id}
                          to={`/module/${layer.id}/${mod.id}`}
                          className={`block px-2 py-1 rounded-md text-sm transition-colors no-underline ${
                            isActive
                              ? 'text-cyan-400 bg-slate-800/80'
                              : 'text-slate-400 hover:text-slate-200 hover:bg-slate-800/40'
                          }`}
                        >
                          {mod.name}
                        </Link>
                      );
                    })}
                  </div>
                )}
              </div>
            );
          })}
        </div>
      </div>
    </aside>
  );
}
