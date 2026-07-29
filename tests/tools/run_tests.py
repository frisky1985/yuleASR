#!/usr/bin/env python3
#==================================================================================================
# Project              : YuleTech AutoSAR BSW
# Script               : Test Runner
#
# SW Version           : 1.0.0
# Build Date           : 2026-04-23
#
# (c) Copyright 2024-2026 Shanghai Yule Electronics Technology Co., Ltd.
# All Rights Reserved.
#==================================================================================================

"""
YuleTech BSW Test Runner

This script provides comprehensive test execution capabilities for the AutoSAR BSW project:
- Run unit tests for individual modules or all modules
- Generate test reports in various formats
- Perform code coverage analysis
- Integrate with CI/CD pipelines

Usage:
    python run_tests.py [options]
    
Options:
    -h, --help              Show this help message
    -m, --module            Run tests for specific module (e.g., mcu, port, dio)
    -l, --layer             Run tests for specific layer (mcal, ecual, services, all)
    -c, --coverage          Enable code coverage analysis
    -r, --report            Generate test report (text, html, xml)
    -v, --verbose           Verbose output
    -j, --jobs              Number of parallel jobs (default: 1)
    --build                 Build tests before running
    --clean                 Clean build directory before building
"""

import argparse
import os
import subprocess
import sys
import json
import time
from datetime import datetime
from pathlib import Path
from typing import List, Dict, Tuple, Optional

#==================================================================================================
# CONFIGURATION
#==================================================================================================

PROJECT_ROOT = Path(__file__).parent.parent.parent.absolute()
TESTS_DIR = PROJECT_ROOT / "tests"
BUILD_DIR = TESTS_DIR / "build"

# Test module mapping
MCAL_MODULES = ["mcu", "port", "dio", "can", "spi", "gpt", "pwm", "adc", "wdg"]
ECUAL_MODULES = ["canif", "cantp", "iohwab", "ethif", "memif", "fee", "ea", "frif", "linif"]
SERVICE_MODULES = ["com", "pdur", "nvm", "dcm", "dem"]
OS_MODULES = ["os"]

ALL_MODULES = {
    "mcal": MCAL_MODULES,
    "ecual": ECUAL_MODULES,
    "services": SERVICE_MODULES,
    "os": OS_MODULES
}

#==================================================================================================
# COLORS
#==================================================================================================

class Colors:
    RESET = "\033[0m"
    RED = "\033[31m"
    GREEN = "\033[32m"
    YELLOW = "\033[33m"
    BLUE = "\033[34m"
    MAGENTA = "\033[35m"
    CYAN = "\033[36m"
    WHITE = "\033[37m"
    BOLD = "\033[1m"

    @classmethod
    def disable(cls):
        cls.RESET = ""
        cls.RED = ""
        cls.GREEN = ""
        cls.YELLOW = ""
        cls.BLUE = ""
        cls.MAGENTA = ""
        cls.CYAN = ""
        cls.WHITE = ""
        cls.BOLD = ""

#==================================================================================================
# UTILITY FUNCTIONS
#==================================================================================================

def print_header(text: str):
    """Print a formatted header"""
    print(f"\n{Colors.CYAN}{'='*70}{Colors.RESET}")
    print(f"{Colors.CYAN}{Colors.BOLD}{text.center(70)}{Colors.RESET}")
    print(f"{Colors.CYAN}{'='*70}{Colors.RESET}\n")

def print_subheader(text: str):
    """Print a formatted subheader"""
    print(f"\n{Colors.BLUE}{text}{Colors.RESET}")
    print(f"{Colors.BLUE}{'-'*len(text)}{Colors.RESET}")

def print_success(text: str):
    """Print success message"""
    print(f"{Colors.GREEN}✓ {text}{Colors.RESET}")

def print_error(text: str):
    """Print error message"""
    print(f"{Colors.RED}✗ {text}{Colors.RESET}", file=sys.stderr)

def print_warning(text: str):
    """Print warning message"""
    print(f"{Colors.YELLOW}⚠ {text}{Colors.RESET}")

def run_command(cmd: List[str], cwd: Path = None, verbose: bool = False) -> Tuple[int, str, str]:
    """Run a command and return exit code, stdout, stderr"""
    if verbose:
        print(f"  Running: {' '.join(cmd)}")
    
    result = subprocess.run(
        cmd,
        cwd=cwd,
        capture_output=True,
        text=True
    )
    
    return result.returncode, result.stdout, result.stderr

#==================================================================================================
# BUILD FUNCTIONS
#==================================================================================================

def clean_build_directory():
    """Clean the build directory"""
    if BUILD_DIR.exists():
        print(f"Cleaning build directory: {BUILD_DIR}")
        import shutil
        shutil.rmtree(BUILD_DIR)
    BUILD_DIR.mkdir(parents=True, exist_ok=True)

def configure_cmake(coverage: bool = False, verbose: bool = False) -> bool:
    """Configure CMake build"""
    print_subheader("Configuring CMake")
    
    cmd = ["cmake", ".."]
    if coverage:
        cmd.append("-DCOVERAGE=ON")
    
    returncode, stdout, stderr = run_command(cmd, cwd=BUILD_DIR, verbose=verbose)
    
    if returncode != 0:
        print_error("CMake configuration failed")
        if verbose:
            print(stderr)
        return False
    
    print_success("CMake configuration successful")
    return True

def build_tests(targets: List[str] = None, verbose: bool = False) -> bool:
    """Build test targets"""
    print_subheader("Building Tests")
    
    cmd = ["cmake", "--build", "."]
    if targets:
        for target in targets:
            cmd.extend(["--target", target])
    
    if verbose:
        cmd.append("-v")
    
    returncode, stdout, stderr = run_command(cmd, cwd=BUILD_DIR, verbose=verbose)
    
    if returncode != 0:
        print_error("Build failed")
        if verbose:
            print(stderr)
        return False
    
    print_success("Build successful")
    return True

#==================================================================================================
# TEST EXECUTION
#==================================================================================================

class TestResult:
    def __init__(self, name: str, passed: bool, output: str, duration: float):
        self.name = name
        self.passed = passed
        self.output = output
        self.duration = duration

class TestRunner:
    def __init__(self, verbose: bool = False, jobs: int = 1):
        self.verbose = verbose
        self.jobs = jobs
        self.results: List[TestResult] = []
        
    def run_test(self, test_name: str) -> TestResult:
        """Run a single test"""
        test_executable = BUILD_DIR / f"test_{test_name}"
        
        if not test_executable.exists():
            return TestResult(test_name, False, f"Test executable not found: {test_executable}", 0)
        
        start_time = time.time()
        returncode, stdout, stderr = run_command([str(test_executable)], verbose=self.verbose)
        duration = time.time() - start_time
        
        passed = (returncode == 0)
        output = stdout + stderr
        
        return TestResult(test_name, passed, output, duration)
    
    def run_tests(self, test_names: List[str]) -> bool:
        """Run multiple tests"""
        print_subheader(f"Running {len(test_names)} test(s)")
        
        all_passed = True
        
        for test_name in test_names:
            if self.verbose:
                print(f"  Running test_{test_name}...")
            
            result = self.run_test(test_name)
            self.results.append(result)
            
            if result.passed:
                print_success(f"test_{test_name} ({result.duration:.3f}s)")
            else:
                print_error(f"test_{test_name} ({result.duration:.3f}s)")
                all_passed = False
                
                if self.verbose:
                    print(result.output)
        
        return all_passed
    
    def print_summary(self):
        """Print test summary"""
        passed = sum(1 for r in self.results if r.passed)
        failed = sum(1 for r in self.results if not r.passed)
        total_duration = sum(r.duration for r in self.results)
        
        print_header("TEST SUMMARY")
        print(f"  Total Tests:  {len(self.results)}")
        print(f"  {Colors.GREEN}Passed:       {passed}{Colors.RESET}")
        print(f"  {Colors.RED}Failed:       {failed}{Colors.RESET}")
        print(f"  Duration:     {total_duration:.3f}s")
        print()
        
        if failed == 0:
            print(f"{Colors.GREEN}{Colors.BOLD}  ALL TESTS PASSED!{Colors.RESET}")
        else:
            print(f"{Colors.RED}{Colors.BOLD}  SOME TESTS FAILED!{Colors.RESET}")
        
        print()

#==================================================================================================
# REPORT GENERATION
#==================================================================================================

def generate_text_report(runner: TestRunner, output_file: str = None):
    """Generate text format report"""
    lines = []
    lines.append("="*70)
    lines.append("YuleTech BSW Test Report")
    lines.append("="*70)
    lines.append(f"Generated: {datetime.now().isoformat()}")
    lines.append("")
    
    passed = sum(1 for r in runner.results if r.passed)
    failed = sum(1 for r in runner.results if not r.passed)
    
    lines.append(f"Total Tests: {len(runner.results)}")
    lines.append(f"Passed: {passed}")
    lines.append(f"Failed: {failed}")
    lines.append("")
    
    lines.append("Test Details:")
    lines.append("-"*70)
    
    for result in runner.results:
        status = "PASS" if result.passed else "FAIL"
        lines.append(f"[{status}] {result.name} ({result.duration:.3f}s)")
    
    report = "\n".join(lines)
    
    if output_file:
        with open(output_file, 'w') as f:
            f.write(report)
        print(f"Text report saved to: {output_file}")
    
    return report

def generate_json_report(runner: TestRunner, output_file: str = None) -> str:
    """Generate JSON format report"""
    data = {
        "timestamp": datetime.now().isoformat(),
        "summary": {
            "total": len(runner.results),
            "passed": sum(1 for r in runner.results if r.passed),
            "failed": sum(1 for r in runner.results if not r.passed)
        },
        "tests": [
            {
                "name": r.name,
                "status": "passed" if r.passed else "failed",
                "duration": r.duration
            }
            for r in runner.results
        ]
    }
    
    report = json.dumps(data, indent=2)
    
    if output_file:
        with open(output_file, 'w') as f:
            f.write(report)
        print(f"JSON report saved to: {output_file}")
    
    return report

def generate_html_report(runner: TestRunner, output_file: str = None):
    """Generate HTML format report"""
    passed = sum(1 for r in runner.results if r.passed)
    failed = sum(1 for r in runner.results if not r.passed)
    
    html = f"""<!DOCTYPE html>
<html>
<head>
    <title>YuleTech BSW Test Report</title>
    <style>
        body {{ font-family: Arial, sans-serif; margin: 20px; }}
        h1 {{ color: #333; }}
        .summary {{ background: #f0f0f0; padding: 15px; border-radius: 5px; margin: 20px 0; }}
        .passed {{ color: green; }}
        .failed {{ color: red; }}
        table {{ border-collapse: collapse; width: 100%; margin-top: 20px; }}
        th, td {{ border: 1px solid #ddd; padding: 8px; text-align: left; }}
        th {{ background: #4CAF50; color: white; }}
        tr:nth-child(even) {{ background: #f2f2f2; }}
    </style>
</head>
<body>
    <h1>YuleTech BSW Test Report</h1>
    <p>Generated: {datetime.now().isoformat()}</p>
    
    <div class="summary">
        <h2>Summary</h2>
        <p>Total Tests: {len(runner.results)}</p>
        <p class="passed">Passed: {passed}</p>
        <p class="failed">Failed: {failed}</p>
    </div>
    
    <h2>Test Details</h2>
    <table>
        <tr>
            <th>Test Name</th>
            <th>Status</th>
            <th>Duration</th>
        </tr>
"""
    
    for result in runner.results:
        status_class = "passed" if result.passed else "failed"
        status_text = "PASS" if result.passed else "FAIL"
        html += f"""
        <tr>
            <td>{result.name}</td>
            <td class="{status_class}">{status_text}</td>
            <td>{result.duration:.3f}s</td>
        </tr>
"""
    
    html += """
    </table>
</body>
</html>
"""
    
    if output_file:
        with open(output_file, 'w') as f:
            f.write(html)
        print(f"HTML report saved to: {output_file}")
    
    return html

#==================================================================================================
# MAIN FUNCTION
#==================================================================================================

def main():
    parser = argparse.ArgumentParser(
        description="YuleTech BSW Test Runner",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  python run_tests.py                    # Run all tests
  python run_tests.py -m mcu             # Run Mcu tests only
  python run_tests.py -l mcal            # Run all MCAL tests
  python run_tests.py -c                 # Run with coverage
  python run_tests.py -r html            # Generate HTML report
        """
    )
    
    parser.add_argument("-m", "--module", 
                       help="Run tests for specific module")
    parser.add_argument("-l", "--layer", 
                       choices=["mcal", "ecual", "services", "os", "all"],
                       default="all",
                       help="Run tests for specific layer")
    parser.add_argument("-c", "--coverage", 
                       action="store_true",
                       help="Enable code coverage")
    parser.add_argument("-r", "--report", 
                       choices=["text", "html", "xml"],
                       help="Generate test report")
    parser.add_argument("-v", "--verbose", 
                       action="store_true",
                       help="Verbose output")
    parser.add_argument("-j", "--jobs", 
                       type=int, default=1,
                       help="Number of parallel jobs")
    parser.add_argument("--build", 
                       action="store_true",
                       help="Build tests before running")
    parser.add_argument("--clean", 
                       action="store_true",
                       help="Clean build directory before building")
    parser.add_argument("--no-color",
                       action="store_true",
                       help="Disable colored output")
    
    args = parser.parse_args()
    
    # Disable colors if requested or not in terminal
    if args.no_color or not sys.stdout.isatty():
        Colors.disable()
    
    print_header("YuleTech BSW Test Runner")
    
    # Clean if requested
    if args.clean:
        clean_build_directory()
    
    # Create build directory
    BUILD_DIR.mkdir(parents=True, exist_ok=True)
    
    # Build if requested
    if args.build or not any(BUILD_DIR.iterdir()):
        if not configure_cmake(args.coverage, args.verbose):
            return 1
        
        # Determine targets
        targets = None
        if args.module:
            targets = [f"test_{args.module}"]
        elif args.layer != "all":
            targets = [f"test_{m}" for m in ALL_MODULES.get(args.layer, [])]
        
        if not build_tests(targets, args.verbose):
            return 1
    
    # Determine which tests to run
    test_names = []
    if args.module:
        test_names = [args.module]
    elif args.layer == "all":
        for layer_modules in ALL_MODULES.values():
            test_names.extend(layer_modules)
    else:
        test_names = ALL_MODULES.get(args.layer, [])
    
    # Run tests
    runner = TestRunner(verbose=args.verbose, jobs=args.jobs)
    all_passed = runner.run_tests(test_names)
    
    # Print summary
    runner.print_summary()
    
    # Generate reports
    if args.report:
        report_dir = TESTS_DIR / "reports"
        report_dir.mkdir(exist_ok=True)
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        
        if args.report == "text":
            output_file = report_dir / f"test_report_{timestamp}.txt"
            generate_text_report(runner, str(output_file))
        elif args.report == "html":
            output_file = report_dir / f"test_report_{timestamp}.html"
            generate_html_report(runner, str(output_file))
        elif args.report == "xml":
            # XML report (JUnit format) could be added here
            print_warning("XML report not yet implemented")
    
    return 0 if all_passed else 1

if __name__ == "__main__":
    sys.exit(main())
