#!/usr/bin/env python3
"""
HIL Test Report Generator
测试报告生成器

支持输出格式:
- HTML报告 (with CSS styling)
- PDF报告 (via weasyprint or similar)
- JSON数据导出
- JUnit XML (for CI integration)
- Markdown报告

功能:
- 测试结果统计
- 覆盖率分析
- 性能指标
- 趋势分析
"""

import os
import sys
import json
import time
import hashlib
from datetime import datetime
from typing import List, Dict, Any, Optional
from dataclasses import dataclass, asdict
from pathlib import Path

# Report template uses Jinja2
try:
    from jinja2 import Template
    JINJA2_AVAILABLE = True
except ImportError:
    JINJA2_AVAILABLE = False
    print("Warning: jinja2 not available, using basic templates")

# PDF generation support
try:
    from weasyprint import HTML
    WEASYPRINT_AVAILABLE = True
except ImportError:
    WEASYPRINT_AVAILABLE = False

sys.path.insert(0, '/home/admin/eth-dds-integration/tests/hil')
from hil_host import TestResult


###############################################################################
# Data Classes
###############################################################################

@dataclass
class TestSuiteMetrics:
    """Test suite metrics"""
    total_tests: int = 0
    passed: int = 0
    failed: int = 0
    skipped: int = 0
    pass_rate: float = 0.0
    total_duration_ms: float = 0.0
    avg_duration_ms: float = 0.0
    min_duration_ms: float = 0.0
    max_duration_ms: float = 0.0


@dataclass
class CoverageMetrics:
    """Code coverage metrics"""
    line_coverage: float = 0.0
    branch_coverage: float = 0.0
    function_coverage: float = 0.0
    statement_coverage: float = 0.0


@dataclass
class PerformanceMetrics:
    """Performance metrics"""
    avg_latency_ms: float = 0.0
    max_latency_ms: float = 0.0
    throughput_mbps: float = 0.0
    cpu_usage_percent: float = 0.0
    memory_usage_mb: float = 0.0


@dataclass
class TestReport:
    """Complete test report"""
    report_id: str
    project_name: str
    timestamp: str
    test_environment: str
    simulation_mode: bool
    test_suites: Dict[str, List[TestResult]]
    metrics: TestSuiteMetrics
    coverage: CoverageMetrics
    performance: PerformanceMetrics
    metadata: Dict[str, Any]


###############################################################################
# HTML Templates
###############################################################################

HTML_TEMPLATE = """
<!DOCTYPE html>
<html lang="zh-CN">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>{{ report.project_name }} - HIL Test Report</title>
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }
        
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Oxygen, Ubuntu, sans-serif;
            line-height: 1.6;
            color: #333;
            background-color: #f5f5f5;
        }
        
        .container {
            max-width: 1400px;
            margin: 0 auto;
            padding: 20px;
        }
        
        header {
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            padding: 40px;
            border-radius: 10px;
            margin-bottom: 30px;
            box-shadow: 0 4px 6px rgba(0, 0, 0, 0.1);
        }
        
        header h1 {
            font-size: 2.5em;
            margin-bottom: 10px;
        }
        
        header .subtitle {
            opacity: 0.9;
            font-size: 1.1em;
        }
        
        .summary-cards {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(250px, 1fr));
            gap: 20px;
            margin-bottom: 30px;
        }
        
        .card {
            background: white;
            padding: 25px;
            border-radius: 10px;
            box-shadow: 0 2px 4px rgba(0, 0, 0, 0.1);
            transition: transform 0.2s;
        }
        
        .card:hover {
            transform: translateY(-2px);
            box-shadow: 0 4px 8px rgba(0, 0, 0, 0.15);
        }
        
        .card h3 {
            color: #667eea;
            margin-bottom: 15px;
            font-size: 0.9em;
            text-transform: uppercase;
            letter-spacing: 1px;
        }
        
        .card .value {
            font-size: 2.5em;
            font-weight: bold;
            margin-bottom: 5px;
        }
        
        .card .label {
            color: #666;
            font-size: 0.9em;
        }
        
        .pass-rate {
            color: #28a745;
        }
        
        .fail-rate {
            color: #dc3545;
        }
        
        .progress-bar {
            width: 100%;
            height: 20px;
            background-color: #e9ecef;
            border-radius: 10px;
            overflow: hidden;
            margin-top: 10px;
        }
        
        .progress-fill {
            height: 100%;
            background: linear-gradient(90deg, #28a745 0%, #20c997 100%);
            transition: width 0.5s ease;
        }
        
        .progress-fill.warning {
            background: linear-gradient(90deg, #ffc107 0%, #ff9800 100%);
        }
        
        .progress-fill.danger {
            background: linear-gradient(90deg, #dc3545 0%, #c82333 100%);
        }
        
        .section {
            background: white;
            padding: 30px;
            border-radius: 10px;
            margin-bottom: 30px;
            box-shadow: 0 2px 4px rgba(0, 0, 0, 0.1);
        }
        
        .section h2 {
            color: #333;
            margin-bottom: 20px;
            padding-bottom: 10px;
            border-bottom: 2px solid #667eea;
        }
        
        table {
            width: 100%;
            border-collapse: collapse;
            margin-top: 20px;
        }
        
        th, td {
            padding: 12px;
            text-align: left;
            border-bottom: 1px solid #dee2e6;
        }
        
        th {
            background-color: #f8f9fa;
            font-weight: 600;
            color: #495057;
        }
        
        tr:hover {
            background-color: #f8f9fa;
        }
        
        .status-badge {
            display: inline-block;
            padding: 4px 12px;
            border-radius: 20px;
            font-size: 0.85em;
            font-weight: 600;
        }
        
        .status-pass {
            background-color: #d4edda;
            color: #155724;
        }
        
        .status-fail {
            background-color: #f8d7da;
            color: #721c24;
        }
        
        .status-skip {
            background-color: #fff3cd;
            color: #856404;
        }
        
        .error-details {
            background-color: #f8d7da;
            padding: 15px;
            border-radius: 5px;
            margin-top: 10px;
            font-family: monospace;
            font-size: 0.9em;
        }
        
        .metrics-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
            gap: 15px;
        }
        
        .metric-item {
            background: #f8f9fa;
            padding: 15px;
            border-radius: 5px;
        }
        
        .metric-item .metric-value {
            font-size: 1.5em;
            font-weight: bold;
            color: #667eea;
        }
        
        .metric-item .metric-label {
            font-size: 0.85em;
            color: #666;
            margin-top: 5px;
        }
        
        .environment-info {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(300px, 1fr));
            gap: 20px;
        }
        
        .info-item {
            display: flex;
            justify-content: space-between;
            padding: 10px 0;
            border-bottom: 1px solid #eee;
        }
        
        .info-item:last-child {
            border-bottom: none;
        }
        
        footer {
            text-align: center;
            padding: 30px;
            color: #666;
            font-size: 0.9em;
        }
        
        .chart-container {
            height: 300px;
            margin-top: 20px;
        }
        
        @media print {
            body {
                background: white;
            }
            .card, .section {
                box-shadow: none;
                border: 1px solid #ddd;
                break-inside: avoid;
            }
        }
    </style>
</head>
<body>
    <div class="container">
        <header>
            <h1>🔧 HIL Test Report</h1>
            <p class="subtitle">{{ report.project_name }} | {{ report.timestamp }}</p>
            <p class="subtitle">Environment: {{ report.test_environment }} | Mode: {{ "Simulation" if report.simulation_mode else "Hardware" }}</p>
        </header>
        
        <!-- Summary Cards -->
        <div class="summary-cards">
            <div class="card">
                <h3>Total Tests</h3>
                <div class="value">{{ metrics.total_tests }}</div>
                <div class="label">Executed test cases</div>
            </div>
            <div class="card">
                <h3>Pass Rate</h3>
                <div class="value {{ 'pass-rate' if metrics.pass_rate >= 80 else 'fail-rate' }}">{{ "%.1f"|format(metrics.pass_rate) }}%</div>
                <div class="label">{{ metrics.passed }} passed / {{ metrics.failed }} failed</div>
                <div class="progress-bar">
                    <div class="progress-fill {{ 'warning' if metrics.pass_rate < 90 else '' }} {{ 'danger' if metrics.pass_rate < 80 else '' }}" style="width: {{ metrics.pass_rate }}%"></div>
                </div>
            </div>
            <div class="card">
                <h3>Duration</h3>
                <div class="value">{{ "%.2f"|format(metrics.total_duration_ms / 1000) }}s</div>
                <div class="label">Total execution time</div>
            </div>
            <div class="card">
                <h3>Average Time</h3>
                <div class="value">{{ "%.1f"|format(metrics.avg_duration_ms) }}ms</div>
                <div class="label">Per test case</div>
            </div>
        </div>
        
        <!-- Coverage Section -->
        <div class="section">
            <h2>📊 Code Coverage</h2>
            <div class="metrics-grid">
                <div class="metric-item">
                    <div class="metric-value">{{ "%.1f"|format(coverage.line_coverage) }}%</div>
                    <div class="metric-label">Line Coverage</div>
                </div>
                <div class="metric-item">
                    <div class="metric-value">{{ "%.1f"|format(coverage.branch_coverage) }}%</div>
                    <div class="metric-label">Branch Coverage</div>
                </div>
                <div class="metric-item">
                    <div class="metric-value">{{ "%.1f"|format(coverage.function_coverage) }}%</div>
                    <div class="metric-label">Function Coverage</div>
                </div>
                <div class="metric-item">
                    <div class="metric-value">{{ "%.1f"|format(coverage.statement_coverage) }}%</div>
                    <div class="metric-label">Statement Coverage</div>
                </div>
            </div>
        </div>
        
        <!-- Performance Section -->
        <div class="section">
            <h2>⚡ Performance Metrics</h2>
            <div class="metrics-grid">
                <div class="metric-item">
                    <div class="metric-value">{{ "%.2f"|format(performance.avg_latency_ms) }}ms</div>
                    <div class="metric-label">Average Latency</div>
                </div>
                <div class="metric-item">
                    <div class="metric-value">{{ "%.2f"|format(performance.max_latency_ms) }}ms</div>
                    <div class="metric-label">Max Latency</div>
                </div>
                <div class="metric-item">
                    <div class="metric-value">{{ "%.2f"|format(performance.throughput_mbps) }} Mbps</div>
                    <div class="metric-label">Throughput</div>
                </div>
                <div class="metric-item">
                    <div class="metric-value">{{ "%.1f"|format(performance.cpu_usage_percent) }}%</div>
                    <div class="metric-label">CPU Usage</div>
                </div>
            </div>
        </div>
        
        <!-- Test Results Section -->
        <div class="section">
            <h2>📋 Test Results by Suite</h2>
            {% for suite_name, results in test_suites.items() %}
            <h3 style="margin-top: 30px; color: #667eea;">{{ suite_name }}</h3>
            <table>
                <thead>
                    <tr>
                        <th>Test Name</th>
                        <th>Status</th>
                        <th>Duration</th>
                        <th>Details</th>
                    </tr>
                </thead>
                <tbody>
                    {% for result in results %}
                    <tr>
                        <td>{{ result.test_name }}</td>
                        <td>
                            <span class="status-badge status-{{ 'pass' if result.passed else 'fail' }}">
                                {{ "PASS" if result.passed else "FAIL" }}
                            </span>
                        </td>
                        <td>{{ "%.1f"|format(result.duration_ms) }}ms</td>
                        <td>
                            {% if result.error_message %}
                            <div class="error-details">{{ result.error_message }}</div>
                            {% else %}
                            <span style="color: #666;">-</span>
                            {% endif %}
                        </td>
                    </tr>
                    {% endfor %}
                </tbody>
            </table>
            {% endfor %}
        </div>
        
        <!-- Environment Section -->
        <div class="section">
            <h2>🔧 Environment Information</h2>
            <div class="environment-info">
                {% for key, value in metadata.items() %}
                <div class="info-item">
                    <span>{{ key }}</span>
                    <strong>{{ value }}</strong>
                </div>
                {% endfor %}
            </div>
        </div>
        
        <footer>
            <p>Generated by HIL Test Framework v1.0 | Report ID: {{ report.report_id }}</p>
            <p>{{ report.timestamp }}</p>
        </footer>
    </div>
</body>
</html>
"""


###############################################################################
# Report Generator Class
###############################################################################

class ReportGenerator:
    """Generate test reports in various formats"""
    
    def __init__(self, output_dir: str = "reports"):
        self.output_dir = Path(output_dir)
        self.output_dir.mkdir(parents=True, exist_ok=True)
    
    def generate_report(self, test_suites: Dict[str, List[TestResult]],
                       project_name: str = "ETH-DDS-Integration",
                       simulation_mode: bool = True,
                       environment: str = "HIL Test Environment",
                       metadata: Optional[Dict[str, Any]] = None) -> TestReport:
        """Generate complete test report"""
        
        # Calculate metrics
        all_results = []
        for suite_results in test_suites.values():
            all_results.extend(suite_results)
        
        metrics = self._calculate_metrics(all_results)
        
        # Create report
        report = TestReport(
            report_id=self._generate_report_id(),
            project_name=project_name,
            timestamp=datetime.now().strftime("%Y-%m-%d %H:%M:%S"),
            test_environment=environment,
            simulation_mode=simulation_mode,
            test_suites=test_suites,
            metrics=metrics,
            coverage=CoverageMetrics(),  # Would integrate with coverage tool
            performance=PerformanceMetrics(),  # Would integrate with performance metrics
            metadata=metadata or {}
        )
        
        return report
    
    def _generate_report_id(self) -> str:
        """Generate unique report ID"""
        timestamp = str(time.time())
        return hashlib.md5(timestamp.encode()).hexdigest()[:12]
    
    def _calculate_metrics(self, results: List[TestResult]) -> TestSuiteMetrics:
        """Calculate test suite metrics"""
        total = len(results)
        passed = sum(1 for r in results if r.passed)
        failed = total - passed
        
        durations = [r.duration_ms for r in results]
        total_duration = sum(durations) if durations else 0
        avg_duration = total_duration / total if total > 0 else 0
        min_duration = min(durations) if durations else 0
        max_duration = max(durations) if durations else 0
        
        return TestSuiteMetrics(
            total_tests=total,
            passed=passed,
            failed=failed,
            skipped=0,
            pass_rate=(passed / total * 100) if total > 0 else 0,
            total_duration_ms=total_duration,
            avg_duration_ms=avg_duration,
            min_duration_ms=min_duration,
            max_duration_ms=max_duration
        )
    
    def generate_html(self, report: TestReport, filename: str = "hil_test_report.html") -> str:
        """Generate HTML report"""
        if JINJA2_AVAILABLE:
            template = Template(HTML_TEMPLATE)
            html_content = template.render(
                report=report,
                metrics=report.metrics,
                coverage=report.coverage,
                performance=report.performance,
                test_suites=report.test_suites,
                metadata=report.metadata
            )
        else:
            # Basic HTML without Jinja2
            html_content = self._generate_basic_html(report)
        
        output_path = self.output_dir / filename
        with open(output_path, 'w', encoding='utf-8') as f:
            f.write(html_content)
        
        print(f"HTML report generated: {output_path}")
        return str(output_path)
    
    def _generate_basic_html(self, report: TestReport) -> str:
        """Generate basic HTML without Jinja2"""
        html = f"""<!DOCTYPE html>
<html>
<head>
    <title>{report.project_name} - HIL Test Report</title>
    <style>
        body {{ font-family: Arial, sans-serif; margin: 40px; }}
        h1 {{ color: #333; }}
        table {{ border-collapse: collapse; width: 100%; margin-top: 20px; }}
        th, td {{ border: 1px solid #ddd; padding: 12px; text-align: left; }}
        th {{ background-color: #f2f2f2; }}
        .pass {{ color: green; }}
        .fail {{ color: red; }}
    </style>
</head>
<body>
    <h1>HIL Test Report - {report.project_name}</h1>
    <p>Generated: {report.timestamp}</p>
    <p>Environment: {report.test_environment}</p>
    <p>Mode: {"Simulation" if report.simulation_mode else "Hardware"}</p>
    
    <h2>Summary</h2>
    <p>Total Tests: {report.metrics.total_tests}</p>
    <p>Passed: {report.metrics.passed}</p>
    <p>Failed: {report.metrics.failed}</p>
    <p>Pass Rate: {report.metrics.pass_rate:.1f}%</p>
    <p>Total Duration: {report.metrics.total_duration_ms/1000:.2f}s</p>
"""
        
        for suite_name, results in report.test_suites.items():
            html += f"<h2>{suite_name}</h2><table>"
            html += "<tr><th>Test Name</th><th>Status</th><th>Duration</th></tr>"
            
            for result in results:
                status_class = "pass" if result.passed else "fail"
                status_text = "PASS" if result.passed else "FAIL"
                html += f"<tr><td>{result.test_name}</td><td class='{status_class}'>{status_text}</td><td>{result.duration_ms:.1f}ms</td></tr>"
            
            html += "</table>"
        
        html += "</body></html>"
        return html
    
    def generate_pdf(self, report: TestReport, filename: str = "hil_test_report.pdf") -> Optional[str]:
        """Generate PDF report"""
        if not WEASYPRINT_AVAILABLE:
            print("Warning: weasyprint not available, skipping PDF generation")
            return None
        
        # Generate HTML first
        html_content = self.generate_html(report, "temp_report.html")
        
        # Convert to PDF
        output_path = self.output_dir / filename
        HTML(html_content).write_pdf(str(output_path))
        
        # Clean up temp file
        (self.output_dir / "temp_report.html").unlink(missing_ok=True)
        
        print(f"PDF report generated: {output_path}")
        return str(output_path)
    
    def generate_json(self, report: TestReport, filename: str = "hil_test_report.json") -> str:
        """Generate JSON report"""
        output_path = self.output_dir / filename
        
        report_dict = {
            'report_id': report.report_id,
            'project_name': report.project_name,
            'timestamp': report.timestamp,
            'test_environment': report.test_environment,
            'simulation_mode': report.simulation_mode,
            'metrics': asdict(report.metrics),
            'coverage': asdict(report.coverage),
            'performance': asdict(report.performance),
            'test_suites': {
                name: [asdict(r) for r in results]
                for name, results in report.test_suites.items()
            },
            'metadata': report.metadata
        }
        
        with open(output_path, 'w', encoding='utf-8') as f:
            json.dump(report_dict, f, indent=2, default=str)
        
        print(f"JSON report generated: {output_path}")
        return str(output_path)
    
    def generate_junit_xml(self, report: TestReport, filename: str = "hil_test_report.xml") -> str:
        """Generate JUnit XML report for CI integration"""
        output_path = self.output_dir / filename
        
        total_tests = report.metrics.total_tests
        failures = report.metrics.failed
        time_seconds = report.metrics.total_duration_ms / 1000
        
        xml_lines = [
            '<?xml version="1.0" encoding="UTF-8"?>',
            f'<testsuites name="HIL Tests" tests="{total_tests}" failures="{failures}" time="{time_seconds:.3f}">'
        ]
        
        for suite_name, results in report.test_suites.items():
            suite_tests = len(results)
            suite_failures = sum(1 for r in results if not r.passed)
            suite_time = sum(r.duration_ms for r in results) / 1000
            
            xml_lines.append(f'  <testsuite name="{suite_name}" tests="{suite_tests}" failures="{suite_failures}" time="{suite_time:.3f}">')
            
            for result in results:
                xml_lines.append(f'    <testcase name="{result.test_name}" time="{result.duration_ms/1000:.3f}">')
                
                if not result.passed and result.error_message:
                    xml_lines.append(f'      <failure message="{self._escape_xml(result.error_message)}"></failure>')
                
                xml_lines.append('    </testcase>')
            
            xml_lines.append('  </testsuite>')
        
        xml_lines.append('</testsuites>')
        
        with open(output_path, 'w', encoding='utf-8') as f:
            f.write('\n'.join(xml_lines))
        
        print(f"JUnit XML report generated: {output_path}")
        return str(output_path)
    
    def generate_markdown(self, report: TestReport, filename: str = "hil_test_report.md") -> str:
        """Generate Markdown report"""
        output_path = self.output_dir / filename
        
        md_lines = [
            f"# HIL Test Report - {report.project_name}",
            "",
            f"**Generated:** {report.timestamp}",
            f"**Environment:** {report.test_environment}",
            f"**Mode:** {'Simulation' if report.simulation_mode else 'Hardware'}",
            "",
            "## Summary",
            "",
            f"- **Total Tests:** {report.metrics.total_tests}",
            f"- **Passed:** {report.metrics.passed} ✅",
            f"- **Failed:** {report.metrics.failed} ❌",
            f"- **Pass Rate:** {report.metrics.pass_rate:.1f}%",
            f"- **Total Duration:** {report.metrics.total_duration_ms/1000:.2f}s",
            "",
            "## Coverage",
            "",
            f"- **Line Coverage:** {report.coverage.line_coverage:.1f}%",
            f"- **Branch Coverage:** {report.coverage.branch_coverage:.1f}%",
            f"- **Function Coverage:** {report.coverage.function_coverage:.1f}%",
            "",
            "## Test Results",
            ""
        ]
        
        for suite_name, results in report.test_suites.items():
            md_lines.append(f"### {suite_name}")
            md_lines.append("")
            md_lines.append("| Test Name | Status | Duration |")
            md_lines.append("|-----------|--------|----------|")
            
            for result in results:
                status = "✅ PASS" if result.passed else "❌ FAIL"
                md_lines.append(f"| {result.test_name} | {status} | {result.duration_ms:.1f}ms |")
            
            md_lines.append("")
        
        md_lines.append("---")
        md_lines.append(f"Report ID: {report.report_id}")
        
        with open(output_path, 'w', encoding='utf-8') as f:
            f.write('\n'.join(md_lines))
        
        print(f"Markdown report generated: {output_path}")
        return str(output_path)
    
    def _escape_xml(self, text: str) -> str:
        """Escape XML special characters"""
        return (text
                .replace("&", "&amp;")
                .replace("<", "&lt;")
                .replace(">", "&gt;")
                .replace('"', "&quot;")
                .replace("'", "&apos;"))
    
    def generate_all(self, report: TestReport) -> Dict[str, str]:
        """Generate all report formats"""
        outputs = {}
        
        outputs['html'] = self.generate_html(report)
        outputs['json'] = self.generate_json(report)
        outputs['xml'] = self.generate_junit_xml(report)
        outputs['markdown'] = self.generate_markdown(report)
        
        pdf_path = self.generate_pdf(report)
        if pdf_path:
            outputs['pdf'] = pdf_path
        
        return outputs


###############################################################################
# Main Entry Point
###############################################################################

def main():
    """Example usage"""
    print("HIL Test Report Generator")
    print("="*60)
    
    # Create sample test results
    sample_results = {
        "UDS Tests": [
            TestResult("UDS_Session_Default", True, 15.2),
            TestResult("UDS_Session_Extended", True, 12.5),
            TestResult("UDS_Security_Access", True, 45.8),
            TestResult("UDS_Read_VIN", True, 8.3),
        ],
        "E2E Tests": [
            TestResult("E2E_P01_CRC", True, 5.2),
            TestResult("E2E_P02_Counter", True, 7.1),
            TestResult("E2E_Replay_Detect", True, 18.5),
        ],
        "OTA Tests": [
            TestResult("OTA_Download", True, 125.4),
            TestResult("OTA_Verify", True, 65.2),
            TestResult("OTA_Install", True, 452.1),
        ]
    }
    
    # Generate report
    generator = ReportGenerator(output_dir="/home/admin/eth-dds-integration/tests/hil/reports")
    
    report = generator.generate_report(
        test_suites=sample_results,
        project_name="ETH-DDS-Integration",
        simulation_mode=True,
        environment="HIL Test Environment",
        metadata={
            "Python Version": "3.10",
            "Platform": "Linux",
            "Test Framework": "HIL v1.0"
        }
    )
    
    # Generate all formats
    outputs = generator.generate_all(report)
    
    print("\nGenerated reports:")
    for format_name, path in outputs.items():
        print(f"  {format_name.upper()}: {path}")


if __name__ == "__main__":
    main()
