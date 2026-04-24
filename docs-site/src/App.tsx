import { HashRouter, Routes, Route, useLocation } from 'react-router-dom';
import { useEffect } from 'react';
import Navbar from './components/Navbar';
import Sidebar from './components/Sidebar';
import Footer from './components/Footer';
import HomePage from './pages/HomePage';
import ArchitecturePage from './pages/ArchitecturePage';
import ModulesPage from './pages/ModulesPage';
import ModuleDetailPage from './pages/ModuleDetailPage';
import TestsPage from './pages/TestsPage';
import GuidePage from './pages/GuidePage';

function ScrollToTop() {
  const { pathname } = useLocation();
  useEffect(() => {
    window.scrollTo(0, 0);
  }, [pathname]);
  return null;
}

function Layout() {
  const location = useLocation();
  const isModuleDetail = location.pathname.startsWith('/module/');

  return (
    <div className="min-h-screen flex flex-col">
      <Navbar />
      <ScrollToTop />
      <main className="flex-1 max-w-7xl mx-auto w-full px-4 sm:px-6 lg:px-8 py-8">
        <div className="flex gap-8">
          {isModuleDetail && <Sidebar />}
          <div className="flex-1 min-w-0">
            <Routes>
              <Route path="/" element={<HomePage />} />
              <Route path="/architecture" element={<ArchitecturePage />} />
              <Route path="/modules" element={<ModulesPage />} />
              <Route path="/module/:layer/:moduleId" element={<ModuleDetailPage />} />
              <Route path="/tests" element={<TestsPage />} />
              <Route path="/guide" element={<GuidePage />} />
            </Routes>
          </div>
        </div>
      </main>
      <Footer />
    </div>
  );
}

export default function App() {
  return (
    <HashRouter>
      <Layout />
    </HashRouter>
  );
}
