import { useSearchParams, Link } from 'react-router-dom';
import { useMemo } from 'react';
import { Search as SearchIcon } from 'lucide-react';
import Fuse from 'fuse.js';
import { searchIndex } from '../data/searchIndex';
import type { SearchIndexEntry } from '../data/searchIndex';

function escapeRegExp(str: string): string {
  return str.replace(/[.*+?^${}()|[\]\\]/g, '\\$&');
}

function highlightText(text: string, query: string) {
  const words = query
    .trim()
    .split(/\s+/)
    .filter((w) => w.length > 0);
  if (words.length === 0) return text;
  const pattern = new RegExp(`(${words.map(escapeRegExp).join('|')})`, 'gi');
  const parts = text.split(pattern);
  return parts.map((part, i) =>
    i % 2 === 1 ? (
      <mark key={i} className="bg-cyan-500/30 text-cyan-200 rounded px-0.5">
        {part}
      </mark>
    ) : (
      <span key={i}>{part}</span>
    )
  );
}

export default function SearchResultsPage() {
  const [searchParams] = useSearchParams();
  const q = searchParams.get('q') || '';

  const fuse = useMemo(
    () =>
      new Fuse(searchIndex, {
        keys: ['title', 'content'],
        threshold: 0.3,
      }),
    []
  );

  const results = useMemo(() => {
    if (!q.trim()) return [];
    return fuse.search(q);
  }, [q, fuse]);

  const categoryColors: Record<SearchIndexEntry['category'], string> = {
    模块: 'bg-cyan-900/50 text-cyan-300',
    API: 'bg-violet-900/50 text-violet-300',
    场景: 'bg-emerald-900/50 text-emerald-300',
    类型: 'bg-amber-900/50 text-amber-300',
    配置: 'bg-slate-700 text-slate-300',
    错误码: 'bg-rose-900/50 text-rose-300',
    文档: 'bg-blue-900/50 text-blue-300',
  };

  return (
    <div className="space-y-8">
      <div className="flex items-center gap-3">
        <SearchIcon className="w-6 h-6 text-cyan-400" />
        <h1 className="text-2xl font-bold text-slate-100">
          搜索结果：{q ? `“${q}”` : '请输入关键词'}
        </h1>
      </div>

      {!q.trim() ? (
        <p className="text-slate-400">请在搜索框中输入关键词以查找文档。</p>
      ) : results.length === 0 ? (
        <p className="text-slate-400">未找到与 “{q}” 相关的文档。</p>
      ) : (
        <div className="space-y-4">
          <p className="text-sm text-slate-500">共找到 {results.length} 条结果</p>
          <div className="space-y-3">
            {results.map((result) => (
              <div
                key={result.item.id}
                className="p-4 rounded-xl bg-slate-900 border border-slate-800 hover:border-slate-700 transition-colors"
              >
                <div className="flex items-center gap-2 mb-2 flex-wrap">
                  <span
                    className={`text-xs px-1.5 py-0.5 rounded font-medium shrink-0 ${
                      categoryColors[result.item.category]
                    }`}
                  >
                    {result.item.category}
                  </span>
                  <Link
                    to={`${result.item.path}${result.item.path.includes('?') ? '&' : '?'}highlight=${encodeURIComponent(q)}`}
                    className="text-lg font-semibold text-cyan-400 hover:text-cyan-300 transition-colors no-underline"
                  >
                    {highlightText(result.item.title, q)}
                  </Link>
                </div>
                <p className="text-sm text-slate-400 mb-2 line-clamp-3">
                  {highlightText(
                    result.item.content.slice(0, 200) +
                      (result.item.content.length > 200 ? '...' : ''),
                    q
                  )}
                </p>
                <Link
                  to={`${result.item.path}${result.item.path.includes('?') ? '&' : '?'}highlight=${encodeURIComponent(q)}`}
                  className="text-xs text-slate-500 hover:text-slate-400 transition-colors no-underline"
                >
                  {result.item.path}
                </Link>
              </div>
            ))}
          </div>
        </div>
      )}
    </div>
  );
}
