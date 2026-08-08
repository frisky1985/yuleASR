#!/usr/bin/env python3
"""
Quick test script for DDS Web GUI Server
"""

import sys
import os

# Add server path
sys.path.insert(0, os.path.join(os.path.dirname(__file__), 'server'))

def test_imports():
    """Test that all required modules can be imported"""
    print("Testing imports...")
    try:
        from flask import Flask
        print("  Flask: OK")
    except ImportError as e:
        print(f"  Flask: FAILED - {e}")
        return False
    
    try:
        from flask_cors import CORS
        print("  Flask-CORS: OK")
    except ImportError as e:
        print(f"  Flask-CORS: FAILED - {e}")
        return False
    
    try:
        from flask_socketio import SocketIO
        print("  Flask-SocketIO: OK")
    except ImportError as e:
        print(f"  Flask-SocketIO: FAILED - {e}")
        return False
    
    try:
        import jwt
        print("  PyJWT: OK")
    except ImportError as e:
        print(f"  PyJWT: FAILED - {e}")
        return False
    
    return True

def test_dds_client():
    """Test DDS client"""
    print("\nTesting DDS client...")
    try:
        from dds_client import MockDDSRuntime, DDSRuntimeClient
        
        # Test mock client
        mock = MockDDSRuntime()
        participants = mock.get_participants()
        print(f"  Mock participants: {len(participants)} found")
        
        topics = mock.get_topics()
        print(f"  Mock topics: {len(topics)} found")
        
        topology = mock.get_topology()
        print(f"  Mock topology: {len(topology['nodes'])} nodes, {len(topology['edges'])} edges")
        
        print("  DDS Client: OK")
        return True
    except Exception as e:
        print(f"  DDS Client: FAILED - {e}")
        return False

def test_flask_app():
    """Test Flask app initialization"""
    print("\nTesting Flask app...")
    try:
        from app import app, dds_state
        
        # Test app context
        with app.app_context():
            print("  App context: OK")
        
        # Test DDS state
        participants = dds_state.get_participants()
        print(f"  DDS state participants: {len(participants)} found")
        
        print("  Flask App: OK")
        return True
    except Exception as e:
        print(f"  Flask App: FAILED - {e}")
        import traceback
        traceback.print_exc()
        return False

def test_file_structure():
    """Test that all required files exist"""
    print("\nTesting file structure...")
    base_dir = os.path.dirname(__file__)
    
    required_files = [
        'server/app.py',
        'server/config.py',
        'server/dds_client.py',
        'server/requirements.txt',
        'web/templates/index.html',
        'web/js/app.js',
        'web/css/custom.css',
        'start_server.sh',
        'install.sh',
        'Makefile',
        'README.md'
    ]
    
    all_exist = True
    for file in required_files:
        path = os.path.join(base_dir, file)
        exists = os.path.exists(path)
        status = "OK" if exists else "MISSING"
        print(f"  {file}: {status}")
        if not exists:
            all_exist = False
    
    return all_exist

def main():
    print("=" * 50)
    print("DDS Web GUI Server - Test Suite")
    print("=" * 50)
    
    results = []
    
    # Run tests
    results.append(("File Structure", test_file_structure()))
    results.append(("Imports", test_imports()))
    results.append(("DDS Client", test_dds_client()))
    results.append(("Flask App", test_flask_app()))
    
    # Summary
    print("\n" + "=" * 50)
    print("Test Summary")
    print("=" * 50)
    
    for name, passed in results:
        status = "PASS" if passed else "FAIL"
        symbol = "✓" if passed else "✗"
        print(f"  {symbol} {name}: {status}")
    
    all_passed = all(r[1] for r in results)
    
    if all_passed:
        print("\n✓ All tests passed! Server is ready to run.")
        print("\nStart the server with:")
        print("  ./start_server.sh")
        return 0
    else:
        print("\n✗ Some tests failed. Please check the errors above.")
        print("\nInstall dependencies with:")
        print("  ./install.sh")
        return 1

if __name__ == '__main__':
    sys.exit(main())
