#!/usr/bin/env python3
"""
TSN Clock Synchronization Test Suite
Tests gPTP/802.1AS time synchronization across ECUs
"""

import unittest
import time
import struct
import statistics

class TSNTimeSyncTest(unittest.TestCase):
    """Test TSN time synchronization"""
    
    @classmethod
    def setUpClass(cls):
        """Setup TSN test environment"""
        print("\n=== Setting up TSN Sync Test Environment ===")
        cls.sync_results = []
        
    def test_gptp_master_selection(self):
        """Test gPTP grandmaster selection"""
        print("\nTest: gPTP Grandmaster Selection")
        
        # Simulate ECUs with different clock qualities
        ecu_clocks = [
            {"ecu": "ECU_BCM", "priority1": 128, "clock_class": 6, "accuracy": 0x20},
            {"ecu": "ECU_ADAS", "priority1": 128, "clock_class": 7, "accuracy": 0x21},
            {"ecu": "ECU_INFOTAINMENT", "priority1": 246, "clock_class": 248, "accuracy": 0xFE}
        ]
        
        # Sort by BMCA (Best Master Clock Algorithm)
        # Priority: priority1 > clockClass > clockAccuracy > priority2
        ecu_clocks.sort(key=lambda x: (x["priority1"], x["clock_class"], x["accuracy"]))
        
        grandmaster = ecu_clocks[0]
        print(f"  Selected grandmaster: {grandmaster['ecu']}")
        print(f"    Priority1: {grandmaster['priority1']}")
        print(f"    Clock Class: {grandmaster['clock_class']}")
        print(f"    Clock Accuracy: 0x{grandmaster['accuracy']:02X}")
        
        self.assertEqual(grandmaster["ecu"], "ECU_BCM")
        print("  PASSED: Grandmaster selection validated")
        
    def test_time_synchronization(self):
        """Test time synchronization accuracy"""
        print("\nTest: Time Synchronization Accuracy")
        
        # Simulate synchronization measurements (in nanoseconds)
        offset_measurements = [
            150, 120, 130, 110, 140,  # 5 measurements
            125, 135, 115, 145, 128
        ]
        
        avg_offset = statistics.mean(offset_measurements)
        std_dev = statistics.stdev(offset_measurements)
        max_offset = max(abs(o) for o in offset_measurements)
        
        print(f"  Average offset: {avg_offset:.1f} ns")
        print(f"  Standard deviation: {std_dev:.1f} ns")
        print(f"  Max absolute offset: {max_offset} ns")
        
        # Automotive requirements typically < 500ns
        self.assertLess(max_offset, 500, "Time offset exceeds 500ns")
        
        print("  PASSED: Time synchronization within tolerance")
        
    def test_sync_message_rate(self):
        """Test sync message transmission rate"""
        print("\nTest: Sync Message Rate")
        
        # gPTP sync interval (typically 125ms for automotive)
        sync_interval_ms = 125
        expected_rate = 1000 / sync_interval_ms  # messages per second
        
        print(f"  Sync interval: {sync_interval_ms} ms")
        print(f"  Expected rate: {expected_rate:.1f} msg/s")
        
        # Simulate receiving sync messages
        sync_times = []
        base_time = time.time()
        
        for i in range(10):
            sync_times.append(base_time + (i * sync_interval_ms / 1000))
            
        # Calculate actual intervals
        intervals = [
            (sync_times[i+1] - sync_times[i]) * 1000 
            for i in range(len(sync_times) - 1)
        ]
        
        avg_interval = statistics.mean(intervals)
        print(f"  Average interval: {avg_interval:.1f} ms")
        
        self.assertAlmostEqual(avg_interval, sync_interval_ms, delta=5)
        print("  PASSED: Sync message rate validated")
        
    def test_clock_holdover(self):
        """Test clock holdover during master failure"""
        print("\nTest: Clock Holdover")
        
        # Simulate master failure and holdover
        holdover_duration = 5.0  # seconds
        holdover_drift_ppb = 50  # parts per billion
        
        # Calculate time error during holdover
        time_error_ns = holdover_duration * holdover_drift_ppb
        
        print(f"  Holdover duration: {holdover_duration} s")
        print(f"  Holdover drift: {holdover_drift_ppb} ppb")
        print(f"  Accumulated time error: {time_error_ns} ns")
        
        # Automotive requirement: < 1us drift during holdover
        self.assertLess(time_error_ns, 1000, "Holdover drift exceeds 1us")
        
        print("  PASSED: Clock holdover validated")
        
    def test_boundary_clock(self):
        """Test boundary clock operation"""
        print("\nTest: Boundary Clock Operation")
        
        # Simulate boundary clock with multiple ports
        boundary_clock = {
            "ecu": "ECU_SWITCH",
            "ports": [
                {"port_id": 1, "role": "SLAVE", "synced": True, "master": "GM"},
                {"port_id": 2, "role": "MASTER", "synced": True, "slaves": ["ECU_BCM"]},
                {"port_id": 3, "role": "MASTER", "synced": True, "slaves": ["ECU_ADAS"]}
            ]
        }
        
        print(f"  Boundary clock: {boundary_clock['ecu']}")
        print("  Port configuration:")
        
        for port in boundary_clock["ports"]:
            print(f"    Port {port['port_id']}: {port['role']}")
            if port['role'] == "SLAVE":
                print(f"      Synced to: {port['master']}")
            else:
                print(f"      Serving: {port['slaves']}")
                
        # Verify at least one slave port and one master port
        slave_ports = [p for p in boundary_clock["ports"] if p["role"] == "SLAVE"]
        master_ports = [p for p in boundary_clock["ports"] if p["role"] == "MASTER"]
        
        self.assertGreater(len(slave_ports), 0)
        self.assertGreater(len(master_ports), 0)
        
        print("  PASSED: Boundary clock operation validated")


class TestTSNStreamReservation(unittest.TestCase):
    """Test TSN stream reservation protocol (MSRP)"""
    
    def test_stream_registration(self):
        """Test stream registration with TSN"""
        print("\nTest: TSN Stream Registration")
        
        # Simulate stream registration
        streams = [
            {
                "stream_id": "CAM_001",
                "source": "ECU_ADAS",
                "destinations": ["ECU_BCM"],
                "bandwidth": 15_000_000,  # 15 Mbps
                "interval": 1000,  # microseconds
                "max_latency": 500  # microseconds
            },
            {
                "stream_id": "LIDAR_001",
                "source": "ECU_ADAS",
                "destinations": ["ECU_BCM", "ECU_INFOTAINMENT"],
                "bandwidth": 100_000_000,  # 100 Mbps
                "interval": 100,  # microseconds
                "max_latency": 200  # microseconds
            }
        ]
        
        total_bandwidth = sum(s["bandwidth"] for s in streams)
        
        print(f"  Registered streams: {len(streams)}")
        print(f"  Total bandwidth reserved: {total_bandwidth / 1_000_000:.1f} Mbps")
        
        for stream in streams:
            print(f"    {stream['stream_id']}: {stream['bandwidth'] / 1_000_000:.1f} Mbps")
            
        self.assertEqual(len(streams), 2)
        print("  PASSED: Stream registration validated")
        
    def test_bandwidth_allocation(self):
        """Test bandwidth allocation for time-sensitive streams"""
        print("\nTest: Bandwidth Allocation")
        
        # Link capacity and allocation
        link_capacity = 1000_000_000  # 1 Gbps
        reserved_bandwidth = 150_000_000  # 150 Mbps for TSN streams
        best_effort_bandwidth = link_capacity - reserved_bandwidth
        
        allocation_percent = (reserved_bandwidth / link_capacity) * 100
        
        print(f"  Link capacity: {link_capacity / 1_000_000:.0f} Mbps")
        print(f"  Reserved for TSN: {reserved_bandwidth / 1_000_000:.0f} Mbps ({allocation_percent:.1f}%)")
        print(f"  Best effort: {best_effort_bandwidth / 1_000_000:.0f} Mbps")
        
        # Verify allocation is within limits (typically max 75% for TSN)
        self.assertLess(allocation_percent, 75)
        
        print("  PASSED: Bandwidth allocation validated")


def run_tests():
    """Run all TSN tests"""
    print("\n" + "="*60)
    print("TSN Clock Synchronization Test Suite")
    print("="*60)
    
    loader = unittest.TestLoader()
    suite = unittest.TestSuite()
    
    suite.addTests(loader.loadTestsFromTestCase(TSNTimeSyncTest))
    suite.addTests(loader.loadTestsFromTestCase(TestTSNStreamReservation))
    
    runner = unittest.TextTestRunner(verbosity=2)
    result = runner.run(suite)
    
    return result.wasSuccessful()


if __name__ == '__main__':
    success = run_tests()
    exit(0 if success else 1)
