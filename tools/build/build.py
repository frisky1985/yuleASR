#!/usr/bin/env python3
"""
YuleTech AutoSAR BSW - 构建脚本
Usage: python3 tools/build/build.py [options]
"""

import argparse
import os
import subprocess
import sys

PROJECT_ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

def run_command(cmd, cwd=None):
    """Run shell command"""
    print(f">>> {' '.join(cmd)}")
    result = subprocess.run(cmd, cwd=cwd or PROJECT_ROOT)
    return result.returncode

def configure(args):
    """Configure build"""
    build_dir = os.path.join(PROJECT_ROOT, 'build')
    os.makedirs(build_dir, exist_ok=True)
    
    cmd = ['cmake', '..']
    
    if args.tests:
        cmd.append('-DBUILD_TESTS=ON')
    if args.coverage:
        cmd.append('-DENABLE_COVERAGE=ON')
    if args.sanitizer:
        cmd.append('-DENABLE_SANITIZER=ON')
    
    return run_command(cmd, cwd=build_dir)

def build(args):
    """Build project"""
    build_dir = os.path.join(PROJECT_ROOT, 'build')
    
    if not os.path.exists(os.path.join(build_dir, 'CMakeCache.txt')):
        if configure(args) != 0:
            return 1
    
    cmd = ['cmake', '--build', '.', '-j']
    return run_command(cmd, cwd=build_dir)

def test(args):
    """Run tests"""
    build_dir = os.path.join(PROJECT_ROOT, 'build')
    
    cmd = ['ctest', '--output-on-failure']
    if args.verbose:
        cmd.append('--verbose')
    
    return run_command(cmd, cwd=build_dir)

def clean(args):
    """Clean build"""
    build_dir = os.path.join(PROJECT_ROOT, 'build')
    if os.path.exists(build_dir):
        import shutil
        shutil.rmtree(build_dir)
        print(f"Removed {build_dir}")
    return 0

def lint(args):
    """Run static analysis"""
    cmd = [
        'cppcheck',
        '--enable=all',
        '--suppress=missingIncludeSystem',
        '-I', 'src/bsw/general/inc',
        'src/bsw'
    ]
    return run_command(cmd)

def format_code(args):
    """Format code"""
    import glob
    
    files = []
    for pattern in ['src/**/*.c', 'src/**/*.h', 'tests/**/*.c', 'tests/**/*.h']:
        files.extend(glob.glob(os.path.join(PROJECT_ROOT, pattern), recursive=True))
    
    if not files:
        print("No source files found")
        return 0
    
    cmd = ['clang-format', '-i'] + files
    return run_command(cmd)

def main():
    parser = argparse.ArgumentParser(description='YuleTech BSW Build Script')
    subparsers = parser.add_subparsers(dest='command')
    
    # configure
    configure_parser = subparsers.add_parser('configure', help='Configure build')
    configure_parser.add_argument('--tests', action='store_true', help='Enable tests')
    configure_parser.add_argument('--coverage', action='store_true', help='Enable coverage')
    configure_parser.add_argument('--sanitizer', action='store_true', help='Enable sanitizer')
    
    # build
    build_parser = subparsers.add_parser('build', help='Build project')
    build_parser.add_argument('--tests', action='store_true', help='Enable tests')
    
    # test
    test_parser = subparsers.add_parser('test', help='Run tests')
    test_parser.add_argument('-v', '--verbose', action='store_true', help='Verbose output')
    
    # clean
    subparsers.add_parser('clean', help='Clean build')
    
    # lint
    subparsers.add_parser('lint', help='Run static analysis')
    
    # format
    subparsers.add_parser('format', help='Format code')
    
    args = parser.parse_args()
    
    if args.command == 'configure':
        return configure(args)
    elif args.command == 'build':
        return build(args)
    elif args.command == 'test':
        return test(args)
    elif args.command == 'clean':
        return clean(args)
    elif args.command == 'lint':
        return lint(args)
    elif args.command == 'format':
        return format_code(args)
    else:
        parser.print_help()
        return 1

if __name__ == '__main__':
    sys.exit(main())
