import { GitBranch } from 'lucide-react';

export default function Footer() {
  return (
    <footer className="border-t border-slate-800 bg-slate-900 mt-auto">
      <div className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8 py-8">
        <div className="flex flex-col sm:flex-row items-center justify-between gap-4">
          <div className="text-sm text-slate-400">
            &copy; {new Date().getFullYear()} 上海予乐电子科技有限公司. 保留所有权利.
          </div>
          <div className="flex items-center gap-4">
            <a
              href="https://github.com/yuletech-openspec"
              target="_blank"
              rel="noopener noreferrer"
              className="flex items-center gap-1.5 text-sm text-slate-400 hover:text-cyan-400 transition-colors no-underline"
            >
              <GitBranch className="w-4 h-4" />
              GitHub
            </a>
          </div>
        </div>
        <div className="mt-4 text-xs text-slate-600 text-center">
          基于 AutoSAR Classic Platform 4.x 标准 | 硬件平台: NXP i.MX8M Mini
        </div>
      </div>
    </footer>
  );
}
