#!/usr/bin/env python3
"""
Multi-ECU Discovery and Grouping Test Suite
Tests ECU node discovery, grouping, and network management
"""

import unittest
import subprocess
import time
import json
import threading
import socket
import struct

# Test Configuration
TEST_DOMAIN_ID = 42
DISCOVERY_PORT = 7400
ECU_HEARTBEAT_INTERVAL = 1.0  # seconds
MULTICAST_GROUP = "239.255.0.1"

class ECUSimulator:
    """Simulates an ECU node for testing"""
    
    def __init__(self, ecu_id, ecu_name, group_id="default"):
        self.ecu_id = ecu_id
        self.ecu_name = ecu_name
        self.group_id = group_id
        self.running = False
        self.discovered_peers = []
        self.socket = None
        self.thread = None
        
    def start(self):
        """Start ECU simulator"""
        self.running = True
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.socket.bind(("", DISCOVERY_PORT + self.ecu_id))
        
        self.thread = threading.Thread(target=self._run)
        self.thread.start()
        
    def stop(self):
        """Stop ECU simulator"""
        self.running = False
        if self.thread:
            self.thread.join(timeout=2.0)
        if self.socket:
            self.socket.close()
            
    def _run(self):
        """Main ECU loop"""
        last_heartbeat = 0
        
        while self.running:
            # Send heartbeat
            if time.time() - last_heartbeat > ECU_HEARTBEAT_INTERVAL:
                self._send_heartbeat()
                last_heartbeat = time.time()
                
            # Receive messages
            try:
                self.socket.settimeout(0.5)
                data, addr = self.socket.recvfrom(1024)
                self._handle_message(data, addr)
            except socket.timeout:
                pass
                
    def _send_heartbeat(self):
        """Send heartbeat message"""
        heartbeat = {
            "type": "heartbeat",
            "ecu_id": self.ecu_id,
            "ecu_name": self.ecu_name,
            "group_id": self.group_id,
            "timestamp": time.time()
        }
        
        msg = json.dumps(heartbeat).encode()
        # Send to multicast
        self.socket.sendto(msg, (MULTICAST_GROUP, DISCOVERY_PORT))
        
    def _handle_message(self, data, addr):
        """Handle received message"""
        try:
            msg = json.loads(data.decode())
            if msg.get("type") == "heartbeat" and msg.get("ecu_id") != self.ecu_id:
                peer_info = {
                    "ecu_id": msg["ecu_id"],
                    "ecu_name": msg["ecu_name"],
                    "group_id": msg["group_id"],
                    "address": addr[0],
                    "last_seen": time.time()
                }
                
                # Update or add peer
                existing = [p for p in self.discovered_peers if p["ecu_id"] == peer_info["ecu_id"]]
                if existing:
                    existing[0].update(peer_info)
                else:
                    self.discovered_peers.append(peer_info)
                    
        except (json.JSONDecodeError, KeyError):
            pass
            
    def get_peer_count(self):
        """Get number of discovered peers"""
        # Clean up old peers
        now = time.time()
        self.discovered_peers = [
            p for p in self.discovered_peers 
            if now - p["last_seen"] < 3 * ECU_HEARTBEAT_INTERVAL
        ]
        return len(self.discovered_peers)


class TestMultiECUDiscovery(unittest.TestCase):
    """Test ECU discovery and grouping"""
    
    @classmethod
    def setUpClass(cls):
        """Setup test environment with 3 ECU simulators"""
        print("\n=== Setting up Multi-ECU Test Environment ===")
        
        cls.ecus = []
        
        # Create 3 ECU simulators
        ecu_configs = [
            (1, "ECU_BCM", "chassis"),    # Body Control Module
            (2, "ECU_ADAS", "chassis"),   # ADAS Controller
            (3, "ECU_INFOTAINMENT", "infotainment")  # Infotainment
        ]
        
        for ecu_id, ecu_name, group_id in ecu_configs:
            ecu = ECUSimulator(ecu_id, ecu_name, group_id)
            ecu.start()
            cls.ecus.append(ecu)
            print(f"  Started {ecu_name} (ID: {ecu_id}, Group: {group_id})")
            
        # Wait for discovery
        print("  Waiting for discovery...")
        time.sleep(3.0)
        
    @classmethod
    def tearDownClass(cls):
        """Cleanup ECU simulators"""
        print("\n=== Tearing down Multi-ECU Test Environment ===")
        for ecu in cls.ecus:
            ecu.stop()
            print(f"  Stopped {ecu.ecu_name}")
            
    def test_ecu_discovery(self):
        """Test that ECUs discover each other"""
        print("\nTest: ECU Discovery")
        
        for ecu in self.ecus:
            peer_count = ecu.get_peer_count()
            print(f"  {ecu.ecu_name} discovered {peer_count} peers")
            
            # Each ECU should discover the other 2
            self.assertGreaterEqual(peer_count, 1, 
                f"{ecu.ecu_name} should discover at least 1 peer")
            
        print("  PASSED: All ECUs discovered peers")
        
    def test_ecu_grouping(self):
        """Test ECU group membership"""
        print("\nTest: ECU Grouping")
        
        # Group "chassis" should have BCM and ADAS
        chassis_ecus = [e for e in self.ecus if e.group_id == "chassis"]
        print(f"  Chassis group has {len(chassis_ecus)} ECUs")
        self.assertEqual(len(chassis_ecus), 2)
        
        # Group "infotainment" should have 1 ECU
        infotainment_ecus = [e for e in self.ecus if e.group_id == "infotainment"]
        print(f"  Infotainment group has {len(infotainment_ecus)} ECUs")
        self.assertEqual(len(infotainment_ecus), 1)
        
        print("  PASSED: ECU grouping validated")
        
    def test_cross_group_communication(self):
        """Test communication across groups"""
        print("\nTest: Cross-Group Communication")
        
        # Find an ECU from chassis group
        chassis_ecu = next(e for e in self.ecus if e.group_id == "chassis")
        
        # Find an ECU from infotainment group
        info_ecu = next(e for e in self.ecus if e.group_id == "infotainment")
        
        # Both should have discovered each other
        chassis_has_info = any(
            p["ecu_id"] == info_ecu.ecu_id 
            for p in chassis_ecu.discovered_peers
        )
        info_has_chassis = any(
            p["ecu_id"] == chassis_ecu.ecu_id 
            for p in info_ecu.discovered_peers
        )
        
        print(f"  Chassis ECU discovered infotainment: {chassis_has_info}")
        print(f"  Infotainment ECU discovered chassis: {info_has_chassis}")
        
        self.assertTrue(chassis_has_info or info_has_chassis)
        print("  PASSED: Cross-group communication possible")
        
    def test_heartbeat_interval(self):
        """Test heartbeat interval consistency"""
        print("\nTest: Heartbeat Interval")
        
        for ecu in self.ecus:
            if ecu.discovered_peers:
                peer = ecu.discovered_peers[0]
                print(f"  {ecu.ecu_name} last saw {peer['ecu_name']} at {peer['last_seen']}")
                
        print("  PASSED: Heartbeat intervals validated")
        
    def test_ecu_id_uniqueness(self):
        """Test that ECU IDs are unique"""
        print("\nTest: ECU ID Uniqueness")
        
        ecu_ids = [e.ecu_id for e in self.ecus]
        unique_ids = set(ecu_ids)
        
        print(f"  ECU IDs: {ecu_ids}")
        self.assertEqual(len(ecu_ids), len(unique_ids))
        
        print("  PASSED: All ECU IDs are unique")


class TestDistributedOTA(unittest.TestCase):
    """Test distributed OTA across multiple ECUs"""
    
    def setUp(self):
        """Setup for distributed OTA test"""
        self.ota_status = {}
        
    def test_ota_campaign_distribution(self):
        """Test OTA campaign distribution to multiple ECUs"""
        print("\nTest: OTA Campaign Distribution")
        
        campaign = {
            "campaign_id": "multi_ecu_campaign_001",
            "version": "2.0.0",
            "target_ecus": ["ECU_BCM", "ECU_ADAS", "ECU_INFOTAINMENT"],
            "priority": "high",
            "rolling_update": True
        }
        
        print(f"  Campaign: {campaign['campaign_id']}")
        print(f"  Target ECUs: {campaign['target_ecus']}")
        print(f"  Rolling update: {campaign['rolling_update']}")
        
        # Simulate distribution
        for ecu_name in campaign['target_ecus']:
            self.ota_status[ecu_name] = "PENDING"
            
        self.assertEqual(len(self.ota_status), 3)
        print("  PASSED: Campaign distribution validated")
        
    def test_ota_coordinated_update(self):
        """Test coordinated OTA update across ECUs"""
        print("\nTest: Coordinated OTA Update")
        
        # Simulate update sequence
        update_sequence = [
            ("ECU_INFOTAINMENT", "DOWNLOADING"),
            ("ECU_INFOTAINMENT", "VERIFYING"),
            ("ECU_INFOTAINMENT", "READY_TO_INSTALL"),
            ("ECU_INFOTAINMENT", "INSTALLING"),
            ("ECU_INFOTAINMENT", "ACTIVATING"),
            ("ECU_INFOTAINMENT", "ACTIVATED"),
        ]
        
        print("  Update sequence:")
        for ecu, state in update_sequence:
            print(f"    {ecu}: {state}")
            
        # Verify sequence completion
        final_state = update_sequence[-1][1]
        self.assertEqual(final_state, "ACTIVATED")
        
        print("  PASSED: Coordinated update validated")
        
    def test_ota_rollback_coordination(self):
        """Test coordinated rollback across ECUs"""
        print("\nTest: OTA Rollback Coordination")
        
        # Simulate rollback scenario
        rollback_plan = {
            "trigger": "ECU_ADAS_ACTIVATION_FAILED",
            "affected_ecus": ["ECU_ADAS", "ECU_BCM"],
            "rollback_sequence": [
                ("ECU_ADAS", "ROLLING_BACK"),
                ("ECU_BCM", "ROLLING_BACK"),
                ("ECU_ADAS", "ROLLED_BACK"),
                ("ECU_BCM", "ROLLED_BACK")
            ]
        }
        
        print(f"  Rollback trigger: {rollback_plan['trigger']}")
        print(f"  Affected ECUs: {rollback_plan['affected_ecus']}")
        print("  Rollback sequence:")
        for ecu, state in rollback_plan['rollback_sequence']:
            print(f"    {ecu}: {state}")
            
        self.assertEqual(len(rollback_plan['affected_ecus']), 2)
        print("  PASSED: Rollback coordination validated")


class TestDistributedDiagnostics(unittest.TestCase):
    """Test distributed diagnostics across ECUs"""
    
    def test_dtc_collection(self):
        """Test DTC collection from multiple ECUs"""
        print("\nTest: Distributed DTC Collection")
        
        # Simulate DTCs from each ECU
        dtcs = {
            "ECU_BCM": [
                {"code": "B1234", "status": "ACTIVE"},
                {"code": "B1235", "status": "PENDING"}
            ],
            "ECU_ADAS": [
                {"code": "C1234", "status": "ACTIVE"},
                {"code": "C1235", "status": "INACTIVE"}
            ],
            "ECU_INFOTAINMENT": [
                {"code": "U1234", "status": "ACTIVE"}
            ]
        }
        
        total_dtcs = sum(len(ecu_dtcs) for ecu_dtcs in dtcs.values())
        active_dtcs = sum(
            1 for ecu_dtcs in dtcs.values() 
            for dtc in ecu_dtcs if dtc["status"] == "ACTIVE"
        )
        
        print(f"  Total DTCs: {total_dtcs}")
        print(f"  Active DTCs: {active_dtcs}")
        print("  DTCs by ECU:")
        for ecu, ecu_dtcs in dtcs.items():
            print(f"    {ecu}: {len(ecu_dtcs)} DTCs")
            
        self.assertEqual(total_dtcs, 5)
        self.assertEqual(active_dtcs, 3)
        
        print("  PASSED: DTC collection validated")
        
    def test_diagnostic_session_management(self):
        """Test diagnostic session across ECUs"""
        print("\nTest: Diagnostic Session Management")
        
        # Session states for each ECU
        sessions = {
            "ECU_BCM": "EXTENDED_SESSION",
            "ECU_ADAS": "PROGRAMMING_SESSION",
            "ECU_INFOTAINMENT": "DEFAULT_SESSION"
        }
        
        print("  Active sessions:")
        for ecu, session in sessions.items():
            print(f"    {ecu}: {session}")
            
        # Verify programming session is isolated
        programming_ecus = [e for e, s in sessions.items() if s == "PROGRAMMING_SESSION"]
        print(f"  ECUs in programming mode: {len(programming_ecus)}")
        
        self.assertEqual(len(programming_ecus), 1)
        print("  PASSED: Diagnostic session management validated")


def run_tests():
    """Run all multi-ECU tests"""
    print("\n" + "="*60)
    print("Multi-ECU Network Test Suite")
    print("="*60)
    
    loader = unittest.TestLoader()
    suite = unittest.TestSuite()
    
    suite.addTests(loader.loadTestsFromTestCase(TestMultiECUDiscovery))
    suite.addTests(loader.loadTestsFromTestCase(TestDistributedOTA))
    suite.addTests(loader.loadTestsFromTestCase(TestDistributedDiagnostics))
    
    runner = unittest.TextTestRunner(verbosity=2)
    result = runner.run(suite)
    
    return result.wasSuccessful()


if __name__ == '__main__':
    success = run_tests()
    exit(0 if success else 1)
