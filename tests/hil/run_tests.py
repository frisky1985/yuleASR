#!/usr/bin/env python3
"""
HIL Test Runner
统一测试运行脚本

使用:
    python run_tests.py                    # 运行所有测试
    python run_tests.py --suite uds        # 运行UDS测试
    python run_tests.py --suite e2e        # 运行E2E测试
    python run_tests.py --suite ota        # 运行OTA测试
    python run_tests.py --suite secoc      # 运行SecOC测试
    python run_tests.py --report html      # 生成HTML报告
    python run_tests.py --hardware         # 使用硬件模式 (默认模拟)
"""

import sys
import argparse
from pathlib import Path

# Add parent directory to path
sys.path.insert(0, str(Path(__file__).parent))

from hil_host import create_hil_manager
from report_generator import ReportGenerator
from test_sequences import (
    get_uds_basic_test_sequence,
    get_uds_full_test_sequence,
    get_e2e_basic_test_sequence,
    get_e2e_full_test_sequence,
    get_ota_basic_test_sequence,
    get_ota_full_test_sequence,
    get_secoc_basic_test_sequence,
    get_secoc_full_test_sequence
)


def parse_args():
    """Parse command line arguments"""
    parser = argparse.ArgumentParser(
        description='HIL Test Framework Runner',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  %(prog)s                          Run all tests in simulation mode
  %(prog)s --suite uds              Run UDS tests only
  %(prog)s --suite e2e --report     Run E2E tests and generate report
  %(prog)s --hardware               Run tests in hardware mode
        """
    )
    
    parser.add_argument(
        '--suite',
        choices=['uds', 'e2e', 'ota', 'secoc', 'all'],
        default='all',
        help='Test suite to run (default: all)'
    )
    
    parser.add_argument(
        '--mode',
        choices=['basic', 'full'],
        default='basic',
        help='Test mode: basic or full (default: basic)'
    )
    
    parser.add_argument(
        '--hardware',
        action='store_true',
        help='Use hardware mode (default: simulation)'
    )
    
    parser.add_argument(
        '--report',
        nargs='?',
        const='all',
        choices=['html', 'json', 'xml', 'md', 'pdf', 'all'],
        help='Generate test report'
    )
    
    parser.add_argument(
        '--output-dir',
        default='reports',
        help='Output directory for reports (default: reports)'
    )
    
    parser.add_argument(
        '-v', '--verbose',
        action='store_true',
        help='Verbose output'
    )
    
    return parser.parse_args()


def get_test_sequence(suite: str, mode: str):
    """Get test sequence based on suite and mode"""
    sequences = {
        'uds': {
            'basic': get_uds_basic_test_sequence(),
            'full': get_uds_full_test_sequence()
        },
        'e2e': {
            'basic': get_e2e_basic_test_sequence(),
            'full': get_e2e_full_test_sequence()
        },
        'ota': {
            'basic': get_ota_basic_test_sequence(),
            'full': get_ota_full_test_sequence()
        },
        'secoc': {
            'basic': get_secoc_basic_test_sequence(),
            'full': get_secoc_full_test_sequence()
        }
    }
    
    if suite == 'all':
        # Combine all test sequences
        combined = []
        for s in ['uds', 'e2e', 'ota', 'secoc']:
            combined.extend(sequences[s][mode])
        return combined
    
    return sequences[suite][mode]


def run_tests(args):
    """Run tests based on arguments"""
    print("="*70)
    print("HIL Test Framework")
    print("="*70)
    print(f"Suite: {args.suite.upper()}")
    print(f"Mode: {args.mode}")
    print(f"Hardware Mode: {'Yes' if args.hardware else 'No (Simulation)'}")
    print("="*70)
    
    # Create HIL manager
    manager = create_hil_manager(simulation_mode=not args.hardware)
    
    if not manager.setup():
        print("\n[ERROR] Failed to setup HIL manager")
        return 1
    
    print("\n[HIL] Setup complete\n")
    
    # Get test sequence
    sequence = get_test_sequence(args.suite, args.mode)
    
    print(f"Running {len(sequence)} tests...\n")
    
    # Run tests
    results = manager.run_test_sequence(sequence)
    
    # Print summary
    print("\n" + "="*70)
    print("Test Summary")
    print("="*70)
    
    summary = manager.get_summary()
    print(f"Total Tests: {summary['total']}")
    print(f"Passed: {summary['passed']} ✅")
    print(f"Failed: {summary['failed']} ❌")
    print(f"Pass Rate: {summary['pass_rate']:.1f}%")
    print(f"Total Duration: {summary['total_duration_ms']:.2f}ms")
    print("="*70)
    
    # Generate report if requested
    if args.report:
        print("\nGenerating reports...")
        
        # Organize results by suite
        test_suites = {}
        for result in results:
            # Extract suite name from test name
            if result.test_name.startswith('UDS'):
                suite_name = 'UDS Tests'
            elif result.test_name.startswith('E2E'):
                suite_name = 'E2E Tests'
            elif result.test_name.startswith('OTA'):
                suite_name = 'OTA Tests'
            elif result.test_name.startswith('SecOC'):
                suite_name = 'SecOC Tests'
            else:
                suite_name = 'Other Tests'
            
            if suite_name not in test_suites:
                test_suites[suite_name] = []
            test_suites[suite_name].append(result)
        
        generator = ReportGenerator(output_dir=args.output_dir)
        report = generator.generate_report(
            test_suites=test_suites,
            project_name="ETH-DDS-Integration",
            simulation_mode=not args.hardware,
            environment="HIL Test Environment"
        )
        
        if args.report == 'all':
            outputs = generator.generate_all(report)
        else:
            outputs = {}
            if args.report == 'html':
                outputs['html'] = generator.generate_html(report)
            elif args.report == 'json':
                outputs['json'] = generator.generate_json(report)
            elif args.report == 'xml':
                outputs['xml'] = generator.generate_junit_xml(report)
            elif args.report == 'md':
                outputs['markdown'] = generator.generate_markdown(report)
            elif args.report == 'pdf':
                outputs['pdf'] = generator.generate_pdf(report)
        
        print("\nGenerated reports:")
        for fmt, path in outputs.items():
            if path:
                print(f"  {fmt.upper()}: {path}")
    
    # Cleanup
    manager.teardown()
    
    # Return exit code
    return 0 if summary['failed'] == 0 else 1


def main():
    """Main entry point"""
    args = parse_args()
    
    try:
        exit_code = run_tests(args)
        sys.exit(exit_code)
    except KeyboardInterrupt:
        print("\n\n[INTERRUPTED] Test run interrupted by user")
        sys.exit(130)
    except Exception as e:
        print(f"\n[ERROR] {e}")
        if args.verbose:
            import traceback
            traceback.print_exc()
        sys.exit(1)


if __name__ == '__main__':
    main()
