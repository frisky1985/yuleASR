import { useState, useRef, useEffect, useMemo } from 'react';
import { useNavigate } from 'react-router-dom';
import { Search as SearchIcon, X } from 'lucide-react';
import Fuse from 'fuse.js';
import { searchIndex } from '../data/searchIndex';
import type { SearchIndexEntry } from '../data/searchIndex';

const MAX_DROPDOWN_RESULTS = 10;

interface SearchProps {
  mobile?: boolean;
}

export default function Search({ mobile }: SearchProps) {
  const [expanded, setExpanded] = useState(mobile);
  const [query, setQuery] = useState('');
  const [isOpen, setIsOpen] = useState(false);
  const [selectedIndex, setSelectedIndex] = useState(-1);
  const inputRef = useRef<HTMLInputElement>(null);
  const containerRef = useRef<HTMLDivElement>(null);
  const navigate = useNavigate();

  const fuse = useMemo(
    () =>
      new Fuse(searchIndex, {
        keys: ['title', 'content'],
        threshold: 0.3,
      }),
    []
  );

  const results = useMemo(() => {
    if (!query.trim()) return [];
    return fuse.search(query).slice(0, MAX_DROPDOWN_RESULTS);
  }, [query, fuse]);

  useEffect(() => {
    if (expanded && inputRef.current) {
      inputRef.current.focus();
    }
  }, [expanded]);

  useEffect(() => {
    function handleClickOutside(event: MouseEvent) {
      if (containerRef.current && !containerRef.current.contains(event.target as Node)) {
        setIsOpen(false);
        if (!mobile) {
          setExpanded(false);
        }
        setQuery('');
      }
    }
    document.addEventListener('mousedown', handleClickOutside);
    return () => document.removeEventListener('mousedown', handleClickOutside);
  }, [mobile]);

  function handleNavigate(path: string) {
    const highlight = query.trim();
    const sep = path.includes('?') ? '&' : '?';
    navigate(highlight ? `${path}${sep}highlight=${encodeURIComponent(highlight)}` : path);
    setQuery('');
    setIsOpen(false);
    if (!mobile) {
      setExpanded(false);
    }
    inputRef.current?.blur();
  }

  function handleKeyDown(e: React.KeyboardEvent<HTMLInputElement>) {
    if (!isOpen) return;

    if (e.key === 'Escape') {
      e.preventDefault();
      setIsOpen(false);
      if (!mobile) {
        setExpanded(false);
      }
      inputRef.current?.blur();
    } else if (e.key === 'ArrowDown') {
      e.preventDefault();
      setSelectedIndex((prev) => (prev + 1) % results.length);
    } else if (e.key === 'ArrowUp') {
      e.preventDefault();
      setSelectedIndex((prev) => (prev - 1 + results.length) % results.length);
    } else if (e.key === 'Enter') {
      e.preventDefault();
      if (selectedIndex >= 0 && results[selectedIndex]) {
        handleNavigate(results[selectedIndex].item.path);
      } else if (query.trim()) {
        navigate(`/search?q=${encodeURIComponent(query.trim())}`);
        setQuery('');
        setIsOpen(false);
        if (!mobile) {
          setExpanded(false);
        }
        inputRef.current?.blur();
      }
    }
  }

  const categoryColors: Record<SearchIndexEntry['category'], string> = {
    模块: 'bg-cyan-900/50 text-cyan-300',
    API: 'bg-violet-900/50 text-violet-300',
    场景: 'bg-emerald-900/50 text-emerald-300',
    类型: 'bg-amber-900/50 text-amber-300',
    配置: 'bg-slate-700 text-slate-300',
    错误码: 'bg-rose-900/50 text-rose-300',
    文档: 'bg-blue-900/50 text-blue-300',
  };

  if (!expanded) {
    return (
      <button
        onClick={() => setExpanded(true)}
        className="p-2 rounded-md text-slate-300 hover:text-slate-100 hover:bg-slate-800 transition-colors"
        aria-label="搜索"
      >
        <SearchIcon className="w-5 h-5" />
      </button>
    );
  }

  return (
    <div ref={containerRef} className={`relative ${mobile ? 'w-full' : ''}`}>
      <div className="flex items-center">
        <div className={`relative flex items-center ${mobile ? 'w-full' : ''}`}>
          <SearchIcon className="absolute left-2.5 w-4 h-4 text-slate-400 pointer-events-none" />
          <input
            ref={inputRef}
            type="text"
            value={query}
            onChange={(e) => {
              setQuery(e.target.value);
              setIsOpen(true);
              setSelectedIndex(-1);
            }}
            onFocus={() => setIsOpen(true)}
            onKeyDown={handleKeyDown}
            placeholder="搜索文档..."
            className={`${mobile ? 'w-full' : 'w-48 md:w-64'} pl-9 pr-8 py-1.5 rounded-md bg-slate-800 border border-slate-700 text-sm text-slate-200 placeholder-slate-500 focus:outline-none focus:ring-1 focus:ring-cyan-500 focus:border-cyan-500`}
          />
          {query && (
            <button
              onClick={() => {
                setQuery('');
                setSelectedIndex(-1);
                inputRef.current?.focus();
              }}
              className="absolute right-2 text-slate-500 hover:text-slate-300"
              aria-label="清除搜索"
            >
              <X className="w-3.5 h-3.5" />
            </button>
          )}
        </div>
      </div>

      {isOpen && query.trim() && (
        <div
          className={`absolute mt-2 bg-slate-900 border border-slate-700 rounded-lg shadow-xl z-50 overflow-hidden ${
            mobile ? 'left-0 right-0' : 'right-0 w-80 md:w-96'
          }`}
        >
          {results.length === 0 ? (
            <div className="px-4 py-6 text-center text-slate-500 text-sm">未找到相关文档</div>
          ) : (
            <>
              <ul className="max-h-80 overflow-y-auto py-1">
                {results.map((result, index) => (
                  <li
                    key={result.item.id}
                    className={`px-4 py-2.5 cursor-pointer transition-colors ${
                      index === selectedIndex ? 'bg-slate-800' : 'hover:bg-slate-800/50'
                    }`}
                    onClick={() => handleNavigate(result.item.path)}
                    onMouseEnter={() => setSelectedIndex(index)}
                  >
                    <div className="flex items-center gap-2 mb-1">
                      <span
                        className={`text-xs px-1.5 py-0.5 rounded font-medium shrink-0 ${
                          categoryColors[result.item.category]
                        }`}
                      >
                        {result.item.category}
                      </span>
                      <span className="text-sm font-medium text-slate-200 truncate">
                        {result.item.title}
                      </span>
                    </div>
                    <p className="text-xs text-slate-400 line-clamp-2">
                      {result.item.content.slice(0, 80)}
                      {result.item.content.length > 80 ? '...' : ''}
                    </p>
                  </li>
                ))}
              </ul>
              <div className="border-t border-slate-800 px-4 py-2 bg-slate-900/50">
                <button
                  onClick={() => {
                    navigate(`/search?q=${encodeURIComponent(query.trim())}`);
                    setQuery('');
                    setIsOpen(false);
                    if (!mobile) {
                      setExpanded(false);
                    }
                    inputRef.current?.blur();
                  }}
                  className="text-xs text-cyan-400 hover:text-cyan-300 transition-colors"
                >
                  查看全部结果
                </button>
              </div>
            </>
          )}
        </div>
      )}
    </div>
  );
}
