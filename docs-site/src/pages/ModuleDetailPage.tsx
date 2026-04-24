import { useParams, Link, useSearchParams } from 'react-router-dom';
import { ArrowLeft, AlertTriangle } from 'lucide-react';
import { getModuleById, getModuleDetail, layers } from '../data/modules';

function escapeRegExp(str: string): string {
  return str.replace(/[.*+?^${}()|[\]\\]/g, '\\$&');
}

function useHighlight() {
  const [searchParams] = useSearchParams();
  const highlightQuery = searchParams.get('highlight') || '';

  if (!highlightQuery.trim()) {
    return (text: string) => text;
  }

  const words = highlightQuery
    .trim()
    .split(/\s+/)
    .filter((w) => w.length > 0);
  if (words.length === 0) {
    return (text: string) => text;
  }

  const pattern = new RegExp(`(${words.map(escapeRegExp).join('|')})`, 'gi');

  return (text: string) => {
    const parts = text.split(pattern);
    return parts.map((part, i) =>
      i % 2 === 1 ? (
        <mark
          key={i}
          className="bg-cyan-500/30 text-cyan-200 rounded px-0.5"
        >
          {part}
        </mark>
      ) : (
        <span key={i}>{part}</span>
      )
    );
  };
}

export default function ModuleDetailPage() {
  const { layer, moduleId } = useParams<{ layer: string; moduleId: string }>();
  const moduleInfo = layer && moduleId ? getModuleById(layer, moduleId) : undefined;
  const detail = moduleId ? getModuleDetail(moduleId) : undefined;
  const hl = useHighlight();

  if (!moduleInfo) {
    return (
      <div className="text-center py-20">
        <AlertTriangle className="w-12 h-12 text-amber-400 mx-auto mb-4" />
        <h1 className="text-xl font-bold text-slate-100 mb-2">模块未找到</h1>
        <p className="text-slate-400 mb-6">请求的模块不存在或尚未实现。</p>
        <Link
          to="/modules"
          className="inline-flex items-center gap-2 px-4 py-2 rounded-lg bg-slate-800 text-slate-200 hover:bg-slate-700 transition-colors no-underline"
        >
          <ArrowLeft className="w-4 h-4" />
          返回模块列表
        </Link>
      </div>
    );
  }

  const currentLayer = layers.find((l) => l.id === layer);

  return (
    <div className="space-y-10">
      {/* Breadcrumb */}
      <div className="flex items-center gap-2 text-sm text-slate-500">
        <Link to="/modules" className="hover:text-cyan-400 transition-colors no-underline">
          模块
        </Link>
        <span>/</span>
        <span>{currentLayer?.name}</span>
        <span>/</span>
        <span className="text-slate-300">{moduleInfo.name}</span>
      </div>

      {/* Header */}
      <div>
        <h1>{moduleInfo.name}</h1>
        <p className="text-slate-400">{hl(moduleInfo.description)}</p>
      </div>

      {detail ? (
        <>
          {/* Overview */}
          <section>
            <h2>功能概述</h2>
            <p>{hl(detail.overview)}</p>
          </section>

          {/* APIs */}
          {detail.apis.map((apiGroup) => (
            <section key={apiGroup.category}>
              <h2>{apiGroup.category}</h2>
              <div className="overflow-x-auto">
                <table>
                  <thead>
                    <tr>
                      <th>API</th>
                      <th>描述</th>
                      {apiGroup.items.some((i) => i.calledBy) && <th>调用方</th>}
                    </tr>
                  </thead>
                  <tbody>
                    {apiGroup.items.map((item) => (
                      <tr key={item.name}>
                        <td className="font-mono text-cyan-400 text-sm whitespace-nowrap">
                          {hl(item.name)}
                        </td>
                        <td>{hl(item.description)}</td>
                        {apiGroup.items.some((i) => i.calledBy) && (
                          <td className="text-slate-500 text-sm">
                            {item.calledBy ? hl(item.calledBy) : '-'}
                          </td>
                        )}
                      </tr>
                    ))}
                  </tbody>
                </table>
              </div>
            </section>
          ))}

          {/* Data Types */}
          <section>
            <h2>数据类型</h2>
            <div className="overflow-x-auto">
              <table>
                <thead>
                  <tr>
                    <th>类型名</th>
                    <th>定义</th>
                  </tr>
                </thead>
                <tbody>
                  {detail.dataTypes.map((dt) => (
                    <tr key={dt.name}>
                      <td className="font-mono text-cyan-400 text-sm">{hl(dt.name)}</td>
                      <td className="font-mono text-sm">{hl(dt.definition)}</td>
                    </tr>
                  ))}
                </tbody>
              </table>
            </div>
          </section>

          {/* Scenarios */}
          <section>
            <h2>使用场景</h2>
            <div className="space-y-6">
              {detail.scenarios.map((sc) => (
                <div
                  key={sc.title}
                  className="p-5 rounded-xl bg-slate-900 border border-slate-800"
                >
                  <h3 className="text-lg font-semibold text-slate-100 mb-2">{hl(sc.title)}</h3>
                  <p className="text-slate-400 text-sm mb-4">{hl(sc.description)}</p>
                  <div className="mb-3">
                    <div className="text-xs font-semibold text-slate-500 uppercase tracking-wider mb-2">
                      流程
                    </div>
                    <ol className="space-y-1.5">
                      {sc.flow.map((step, idx) => (
                        <li key={idx} className="text-sm text-slate-300 flex gap-2">
                          <span className="text-cyan-500 font-mono shrink-0">{idx + 1}.</span>
                          <span>{hl(step)}</span>
                        </li>
                      ))}
                    </ol>
                  </div>
                  <div>
                    <div className="text-xs font-semibold text-slate-500 uppercase tracking-wider mb-1">
                      预期结果
                    </div>
                    <p className="text-sm text-emerald-400">{hl(sc.expected)}</p>
                  </div>
                </div>
              ))}
            </div>
          </section>

          {/* Config Params */}
          <section>
            <h2>配置参数</h2>
            <div className="overflow-x-auto">
              <table>
                <thead>
                  <tr>
                    <th>参数名</th>
                    <th>默认值</th>
                    <th>描述</th>
                  </tr>
                </thead>
                <tbody>
                  {detail.configParams.map((cp) => (
                    <tr key={cp.name}>
                      <td className="font-mono text-cyan-400 text-sm">{hl(cp.name)}</td>
                      <td className="font-mono text-sm">{hl(cp.defaultValue)}</td>
                      <td>{hl(cp.description)}</td>
                    </tr>
                  ))}
                </tbody>
              </table>
            </div>
          </section>

          {/* Error Codes */}
          {detail.errorCodes && detail.errorCodes.length > 0 && (
            <section>
              <h2>错误代码 (DET)</h2>
              <div className="overflow-x-auto">
                <table>
                  <thead>
                    <tr>
                      <th>错误码</th>
                      <th>值</th>
                      <th>描述</th>
                    </tr>
                  </thead>
                  <tbody>
                    {detail.errorCodes.map((ec) => (
                      <tr key={ec.name}>
                        <td className="font-mono text-rose-400 text-sm">{hl(ec.name)}</td>
                        <td className="font-mono text-sm">{hl(ec.value)}</td>
                        <td>{hl(ec.description)}</td>
                      </tr>
                    ))}
                  </tbody>
                </table>
              </div>
            </section>
          )}
        </>
      ) : (
        <div className="p-8 rounded-xl bg-slate-900 border border-slate-800 text-center">
          <AlertTriangle className="w-10 h-10 text-amber-400 mx-auto mb-3" />
          <h2 className="text-lg font-semibold text-slate-100 mb-2">文档待补充</h2>
          <p className="text-slate-400 text-sm">
            {moduleInfo.name} 模块的详细文档正在编写中。请稍后查看或参考源代码和 OpenSpec 规范。
          </p>
        </div>
      )}
    </div>
  );
}
