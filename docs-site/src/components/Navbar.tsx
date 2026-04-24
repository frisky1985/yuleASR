import { Link, useLocation } from 'react-router-dom';
import { GitBranch, Menu, X } from 'lucide-react';
import { useState } from 'react';
import Search from './Search';

const navLinks = [
  { to: '/', label: '首页' },
  { to: '/architecture', label: '架构' },
  { to: '/modules', label: '模块' },
  { to: '/tests', label: '测试' },
  { to: '/guide', label: '指南' },
];

export default function Navbar() {
  const location = useLocation();
  const [mobileOpen, setMobileOpen] = useState(false);

  return (
    <nav className="sticky top-0 z-50 bg-slate-900/95 backdrop-blur border-b border-slate-800">
      <div className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8">
        <div className="flex items-center justify-between h-16">
          <div className="flex items-center gap-3">
            <div className="w-8 h-8 rounded bg-cyan-600 flex items-center justify-center text-white font-bold text-sm">
              YT
            </div>
            <Link to="/" className="text-slate-100 font-semibold text-lg tracking-tight no-underline hover:text-cyan-400 transition-colors">
              YuleTech BSW
            </Link>
          </div>

          <div className="hidden md:flex items-center gap-1">
            {navLinks.map((link) => (
              <Link
                key={link.to}
                to={link.to}
                className={`px-3 py-2 rounded-md text-sm font-medium transition-colors no-underline ${
                  location.pathname === link.to || (link.to !== '/' && location.pathname.startsWith(link.to))
                    ? 'text-cyan-400 bg-slate-800'
                    : 'text-slate-300 hover:text-slate-100 hover:bg-slate-800'
                }`}
              >
                {link.label}
              </Link>
            ))}
            <div className="mx-2">
              <Search />
            </div>
            <a
              href="https://github.com/yuletech-openspec"
              target="_blank"
              rel="noopener noreferrer"
              className="ml-2 flex items-center gap-1.5 px-3 py-2 rounded-md text-sm font-medium text-slate-300 hover:text-slate-100 hover:bg-slate-800 transition-colors no-underline"
            >
              <GitBranch className="w-4 h-4" />
              GitHub
            </a>
          </div>

          <button
            className="md:hidden p-2 rounded-md text-slate-300 hover:text-slate-100 hover:bg-slate-800"
            onClick={() => setMobileOpen(!mobileOpen)}
            aria-label="Toggle menu"
          >
            {mobileOpen ? <X className="w-5 h-5" /> : <Menu className="w-5 h-5" />}
          </button>
        </div>
      </div>

      {mobileOpen && (
        <div className="md:hidden border-t border-slate-800 bg-slate-900">
          <div className="px-2 pt-2 pb-3 space-y-1">
            <div className="px-3 py-2">
              <Search mobile />
            </div>
            {navLinks.map((link) => (
              <Link
                key={link.to}
                to={link.to}
                onClick={() => setMobileOpen(false)}
                className={`block px-3 py-2 rounded-md text-base font-medium no-underline ${
                  location.pathname === link.to || (link.to !== '/' && location.pathname.startsWith(link.to))
                    ? 'text-cyan-400 bg-slate-800'
                    : 'text-slate-300 hover:text-slate-100 hover:bg-slate-800'
                }`}
              >
                {link.label}
              </Link>
            ))}
          </div>
        </div>
      )}
    </nav>
  );
}
