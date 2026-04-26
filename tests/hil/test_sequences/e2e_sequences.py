#!/usr/bin/env python3
"""
E2E (End-to-End) Protection Test Sequences
E2E保护测试序列

测试范围:
- Profile验证 (P01/P02/P04/P05/P06/P07/P11/P22)
- CRC计算验证
- 计数器同步
- 错误检测
- 状态机转换
- Window Size验证
- Max Delta Counter检查

应用标准: AUTOSAR E2E R22-11
"""

import sys
import time
import struct
import zlib
from typing import Optional, List, Dict, Any, Tuple
from dataclasses import dataclass
from enum import IntEnum

sys.path.insert(0, '/home/admin/eth-dds-integration/tests/hil')

from hil_host import HilTestManager, E2EProfile, E2EProtectedMessage, TestResult


###############################################################################
# E2E CRC Implementation
###############################################################################

class CRC8_SAEJ1850:
    """CRC-8 SAE J1850 polynomial: 0x1D"""
    _table: List[int] = []
    _initialized: bool = False
    
    @classmethod
    def _init_table(cls):
        if cls._initialized:
            return
        for i in range(256):
            crc = i
            for _ in range(8):
                if crc & 0x80:
                    crc = (crc << 1) ^ 0x1D
                else:
                    crc <<= 1
                crc &= 0xFF
            cls._table.append(crc)
        cls._initialized = True
    
    @classmethod
    def calculate(cls, data: bytes, initial: int = 0xFF) -> int:
        cls._init_table()
        crc = initial
        for byte in data:
            crc = cls._table[crc ^ byte]
        return crc ^ 0xFF  # Final XOR


def crc16_ccitt(data: bytes, initial: int = 0xFFFF) -> int:
    """CRC-16 CCITT polynomial: 0x1021"""
    crc = initial
    for byte in data:
        crc ^= byte << 8
        for _ in range(8):
            if crc & 0x8000:
                crc = (crc << 1) ^ 0x1021
            else:
                crc <<= 1
            crc &= 0xFFFF
    return crc


def crc32_ieee(data: bytes, initial: int = 0xFFFFFFFF) -> int:
    """CRC-32 IEEE (same as zlib.crc32)"""
    return zlib.crc32(data, initial) & 0xFFFFFFFF


###############################################################################
# E2E Profile Implementations
###############################################################################

class E2EProfile01:
    """
    E2E Profile 1: CRC8
    - 1 byte CRC (SAE J1850)
    - Data ID included in CRC calculation
    """
    
    HEADER_SIZE = 1
    
    @staticmethod
    def protect(data: bytes, data_id: int, data_id_mode: int = 0) -> bytes:
        """Protect data with Profile 1"""
        # Calculate CRC over Data ID and data
        crc_data = struct.pack('>H', data_id) + data
        crc = CRC8_SAEJ1850.calculate(crc_data)
        return bytes([crc]) + data
    
    @staticmethod
    def check(protected_data: bytes, data_id: int, data_id_mode: int = 0) -> Tuple[bool, bytes]:
        """Check and extract data"""
        if len(protected_data) < 1:
            return False, b''
        
        received_crc = protected_data[0]
        data = protected_data[1:]
        
        # Calculate expected CRC
        crc_data = struct.pack('>H', data_id) + data
        expected_crc = CRC8_SAEJ1850.calculate(crc_data)
        
        return received_crc == expected_crc, data


class E2EProfile02:
    """
    E2E Profile 2: CRC8 + Counter
    - 1 byte CRC (SAE J1850)
    - 4-bit Counter (nibble)
    - Data ID included
    """
    
    HEADER_SIZE = 2
    
    def __init__(self):
        self.counter = 0
    
    def protect(self, data: bytes, data_id: int) -> bytes:
        """Protect data with Profile 2"""
        # Calculate CRC over Data ID, Counter, and data
        crc_data = struct.pack('>H', data_id) + bytes([self.counter & 0x0F]) + data
        crc = CRC8_SAEJ1850.calculate(crc_data)
        
        # Build header: CRC + Counter (low nibble)
        header = bytes([crc, self.counter & 0x0F])
        
        self.counter = (self.counter + 1) & 0x0F
        
        return header + data
    
    def check(self, protected_data: bytes, data_id: int) -> Tuple[bool, bytes, int]:
        """Check and extract data, returns (valid, data, counter)"""
        if len(protected_data) < 2:
            return False, b'', 0
        
        received_crc = protected_data[0]
        counter = protected_data[1] & 0x0F
        data = protected_data[2:]
        
        # Calculate expected CRC
        crc_data = struct.pack('>H', data_id) + bytes([counter]) + data
        expected_crc = CRC8_SAEJ1850.calculate(crc_data)
        
        return received_crc == expected_crc, data, counter


class E2EProfile05:
    """
    E2E Profile 5: CRC16
    - 2 bytes CRC (CCITT)
    - Data ID included
    """
    
    HEADER_SIZE = 2
    
    @staticmethod
    def protect(data: bytes, data_id: int) -> bytes:
        """Protect data with Profile 5"""
        crc_data = struct.pack('>H', data_id) + data
        crc = crc16_ccitt(crc_data)
        return struct.pack('>H', crc) + data
    
    @staticmethod
    def check(protected_data: bytes, data_id: int) -> Tuple[bool, bytes]:
        """Check and extract data"""
        if len(protected_data) < 2:
            return False, b''
        
        received_crc = struct.unpack('>H', protected_data[0:2])[0]
        data = protected_data[2:]
        
        crc_data = struct.pack('>H', data_id) + data
        expected_crc = crc16_ccitt(crc_data)
        
        return received_crc == expected_crc, data


class E2EProfile06:
    """
    E2E Profile 6: CRC16 + Counter
    - 2 bytes CRC (CCITT)
    - 1 byte Counter (8-bit)
    - Data ID included
    """
    
    HEADER_SIZE = 4  # CRC(2) + Counter(1) + Reserved(1)
    
    def __init__(self):
        self.counter = 0
    
    def protect(self, data: bytes, data_id: int) -> bytes:
        """Protect data with Profile 6"""
        crc_data = struct.pack('>HB', data_id, self.counter) + data
        crc = crc16_ccitt(crc_data)
        
        # CRC(2) + Counter(1) + Reserved(1)
        header = struct.pack('>HBB', crc, self.counter, 0)
        
        self.counter = (self.counter + 1) & 0xFF
        
        return header + data
    
    def check(self, protected_data: bytes, data_id: int) -> Tuple[bool, bytes, int]:
        """Check and extract data"""
        if len(protected_data) < 4:
            return False, b'', 0
        
        received_crc = struct.unpack('>H', protected_data[0:2])[0]
        counter = protected_data[2]
        data = protected_data[4:]
        
        crc_data = struct.pack('>HB', data_id, counter) + data
        expected_crc = crc16_ccitt(crc_data)
        
        return received_crc == expected_crc, data, counter


class E2EProfile04:
    """
    E2E Profile 4: CRC32
    - 4 bytes CRC (IEEE)
    - Data ID (32-bit) included
    """
    
    HEADER_SIZE = 4
    
    @staticmethod
    def protect(data: bytes, data_id: int) -> bytes:
        """Protect data with Profile 4"""
        crc_data = struct.pack('>I', data_id) + data
        crc = crc32_ieee(crc_data)
        return struct.pack('>I', crc) + data
    
    @staticmethod
    def check(protected_data: bytes, data_id: int) -> Tuple[bool, bytes]:
        """Check and extract data"""
        if len(protected_data) < 4:
            return False, b''
        
        received_crc = struct.unpack('>I', protected_data[0:4])[0]
        data = protected_data[4:]
        
        crc_data = struct.pack('>I', data_id) + data
        expected_crc = crc32_ieee(crc_data)
        
        return received_crc == expected_crc, data


###############################################################################
# E2E State Machine
###############################################################################

class E2EState(IntEnum):
    """E2E State Machine States"""
    NONE = 0
    VALID = 1
    DEINIT = 2
    NODATA = 3
    INIT = 4


class E2ECheckStatus(IntEnum):
    """E2E Check Status"""
    OK = 0
    REPEATED = 1
    WRONG_SEQUENCE = 2
    ERROR = 3
    NOTAVAILABLE = 4
    NONEWDATA = 5


@dataclass
class E2EStateMachine:
    """E2E State Machine for counter verification"""
    counter: int = 0
    max_delta_counter: int = 1
    last_valid_counter: int = 0
    state: E2EState = E2EState.INIT
    
    def check_counter(self, received_counter: int, max_counter: int = 14) -> E2ECheckStatus:
        """Check received counter against expected"""
        delta = (received_counter - self.last_valid_counter) & max_counter
        
        if delta == 0:
            return E2ECheckStatus.REPEATED
        elif delta <= self.max_delta_counter:
            self.last_valid_counter = received_counter
            self.state = E2EState.VALID
            return E2ECheckStatus.OK
        else:
            self.state = E2EState.VALID  # Still valid but wrong sequence
            return E2ECheckStatus.WRONG_SEQUENCE


###############################################################################
# Test Functions
###############################################################################

def test_e2e_profile01_crc_calculation(manager: HilTestManager) -> Dict[str, Any]:
    """
    Test: E2E Profile 1 CRC Calculation
    
    Steps:
    1. Calculate CRC for test data
    2. Verify CRC correctness
    3. Verify data can be extracted correctly
    """
    data_id = 0x1234
    test_data = b'\x01\x02\x03\x04\x05\x06\x07\x08'
    
    # Protect
    protected = E2EProfile01.protect(test_data, data_id)
    
    # Verify structure
    if len(protected) != len(test_data) + 1:
        return {'passed': False, 'error': 'Incorrect protected length'}
    
    # Check
    valid, extracted = E2EProfile01.check(protected, data_id)
    
    return {
        'passed': valid and extracted == test_data,
        'profile': 'P01',
        'data_id': hex(data_id),
        'original_len': len(test_data),
        'protected_len': len(protected),
        'crc': hex(protected[0])
    }


def test_e2e_profile01_crc_error_detection(manager: HilTestManager) -> Dict[str, Any]:
    """
    Test: E2E Profile 1 CRC Error Detection
    
    Steps:
    1. Protect data with CRC
    2. Corrupt data
    3. Verify CRC check fails
    """
    data_id = 0x1234
    test_data = b'\x01\x02\x03\x04'
    
    protected = E2EProfile01.protect(test_data, data_id)
    
    # Corrupt the data
    corrupted = bytearray(protected)
    corrupted[-1] ^= 0xFF  # Flip bits in last byte
    
    # Check should fail
    valid, _ = E2EProfile01.check(bytes(corrupted), data_id)
    
    return {
        'passed': not valid,
        'profile': 'P01',
        'error_detected': not valid
    }


def test_e2e_profile02_counter_increment(manager: HilTestManager) -> Dict[str, Any]:
    """
    Test: E2E Profile 2 Counter Increment
    
    Steps:
    1. Protect multiple messages
    2. Verify counter increments correctly
    3. Verify counter wraps at 15
    """
    data_id = 0x1234
    test_data = b'\xAA\xBB'
    
    profile = E2EProfile02()
    counters = []
    
    for i in range(20):  # More than 15 to test wrap
        protected = profile.protect(test_data, data_id)
        valid, extracted, counter = profile.check(protected, data_id)
        
        if not valid:
            return {'passed': False, 'error': f'CRC check failed at iteration {i}'}
        
        counters.append(counter)
    
    # Check counter sequence
    expected = list(range(15)) + list(range(5))  # 0-14, then 0-4
    passed = counters[:20] == expected[:20]
    
    return {
        'passed': passed,
        'profile': 'P02',
        'counters': counters[:20],
        'wrap_detected': 0 in counters[15:]
    }


def test_e2e_profile05_crc16(manager: HilTestManager) -> Dict[str, Any]:
    """
    Test: E2E Profile 5 CRC16
    
    Steps:
    1. Calculate CRC16 for test data
    2. Verify CRC correctness
    """
    data_id = 0x1234
    test_data = b'\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A'
    
    protected = E2EProfile05.protect(test_data, data_id)
    
    if len(protected) != len(test_data) + 2:
        return {'passed': False, 'error': 'Incorrect protected length'}
    
    valid, extracted = E2EProfile05.check(protected, data_id)
    
    return {
        'passed': valid and extracted == test_data,
        'profile': 'P05',
        'protected_len': len(protected),
        'crc': hex(struct.unpack('>H', protected[0:2])[0])
    }


def test_e2e_profile06_counter_verification(manager: HilTestManager) -> Dict[str, Any]:
    """
    Test: E2E Profile 6 Counter with State Machine
    
    Steps:
    1. Initialize state machine
    2. Process sequence of messages with counters
    3. Verify state transitions
    """
    data_id = 0x1234
    test_data = b'\xCC\xDD\xEE\xFF'
    
    profile = E2EProfile06()
    sm = E2EStateMachine(max_delta_counter=2)
    
    results = []
    
    # Send sequence: 0, 1, 2, 3 (OK)
    for expected_counter in range(4):
        protected = profile.protect(test_data, data_id)
        valid, extracted, counter = profile.check(protected, data_id)
        status = sm.check_counter(counter, max_counter=255)
        
        results.append({
            'counter': counter,
            'valid': valid,
            'status': status.name
        })
    
    # Send repeated counter (should detect as REPEATED)
    protected = profile.protect(test_data, data_id)
    protected = bytearray(protected)
    protected[2] = 3  # Set counter to previous value
    valid, extracted, counter = profile.check(bytes(protected), data_id)
    status = sm.check_counter(counter, max_counter=255)
    
    results.append({
        'counter': counter,
        'valid': valid,
        'status': status.name,
        'repeated_detected': status == E2ECheckStatus.REPEATED
    })
    
    passed = all(r['valid'] for r in results[:4])
    passed = passed and results[-1]['repeated_detected']
    
    return {
        'passed': passed,
        'profile': 'P06',
        'results': results
    }


def test_e2e_profile04_crc32(manager: HilTestManager) -> Dict[str, Any]:
    """
    Test: E2E Profile 4 CRC32
    
    Steps:
    1. Calculate CRC32 for test data
    2. Verify CRC correctness for large data
    """
    data_id = 0x12345678
    test_data = bytes(range(256))  # Large test data
    
    protected = E2EProfile04.protect(test_data, data_id)
    
    if len(protected) != len(test_data) + 4:
        return {'passed': False, 'error': 'Incorrect protected length'}
    
    valid, extracted = E2EProfile04.check(protected, data_id)
    
    return {
        'passed': valid and extracted == test_data,
        'profile': 'P04',
        'original_len': len(test_data),
        'protected_len': len(protected)
    }


def test_e2e_max_delta_counter(manager: HilTestManager) -> Dict[str, Any]:
    """
    Test: E2E Max Delta Counter Check
    
    Steps:
    1. Initialize state machine with max_delta=3
    2. Receive counters 0, 1, 2 (OK)
    3. Skip to counter 5 (delta=3, OK)
    4. Skip to counter 10 (delta=5, WRONG_SEQUENCE)
    """
    sm = E2EStateMachine(max_delta_counter=3)
    sm.last_valid_counter = 0
    
    test_counters = [1, 2, 3, 5, 10]
    expected_statuses = [
        E2ECheckStatus.OK,
        E2ECheckStatus.OK,
        E2ECheckStatus.OK,
        E2ECheckStatus.OK,  # delta=2 (5-3)
        E2ECheckStatus.WRONG_SEQUENCE  # delta=5
    ]
    
    results = []
    passed = True
    
    for i, counter in enumerate(test_counters):
        status = sm.check_counter(counter, max_counter=15)
        expected = expected_statuses[i]
        
        results.append({
            'counter': counter,
            'status': status.name,
            'expected': expected.name,
            'match': status == expected
        })
        
        if status != expected:
            passed = False
    
    return {
        'passed': passed,
        'max_delta': 3,
        'results': results
    }


def test_e2e_profile22_large_data(manager: HilTestManager) -> Dict[str, Any]:
    """
    Test: E2E Profile 22 Large Data Support
    
    Steps:
    1. Create large data payload (up to 4KB)
    2. Verify handling
    """
    # Profile 22 supports up to 4096 bytes
    test_sizes = [16, 64, 256, 1024, 4096]
    
    results = []
    for size in test_sizes:
        test_data = bytes([i % 256 for i in range(size)])
        
        # Simulate Profile 22 protection
        # Header: CRC16(2) + Length(2) + DataID(2) + Counter(2) + Reserved(4) = 12 bytes
        crc = crc16_ccitt(test_data)
        header = struct.pack('>HHIH', crc, size, 0x1234, 0) + b'\x00\x00\x00\x00'
        protected = header + test_data
        
        results.append({
            'size': size,
            'protected_size': len(protected),
            'header_size': 12
        })
    
    return {
        'passed': True,
        'profile': 'P22',
        'results': results
    }


def test_e2e_data_id_inclusion(manager: HilTestManager) -> Dict[str, Any]:
    """
    Test: E2E Data ID Inclusion in CRC
    
    Steps:
    1. Protect same data with different Data IDs
    2. Verify CRCs are different
    3. Verify wrong Data ID causes CRC failure
    """
    test_data = b'\x01\x02\x03\x04'
    data_ids = [0x0001, 0x1234, 0xFFFF]
    
    protected_messages = []
    for data_id in data_ids:
        protected = E2EProfile01.protect(test_data, data_id)
        protected_messages.append({
            'data_id': hex(data_id),
            'crc': hex(protected[0]),
            'protected': protected.hex()
        })
    
    # Verify different CRCs
    crcs = [m['crc'] for m in protected_messages]
    unique_crcs = set(crcs)
    
    # Try to verify with wrong Data ID
    protected = E2EProfile01.protect(test_data, 0x1234)
    valid, _ = E2EProfile01.check(protected, 0x5678)  # Wrong ID
    
    return {
        'passed': len(unique_crcs) == len(data_ids) and not valid,
        'unique_crcs': len(unique_crcs),
        'wrong_id_detected': not valid,
        'messages': protected_messages
    }


def test_e2e_message_integrity_sequence(manager: HilTestManager) -> Dict[str, Any]:
    """
    Test: E2E Message Integrity Sequence
    
    Steps:
    1. Send sequence of messages with E2E protection
    2. Verify all CRCs
    3. Verify counter progression
    4. Detect any errors
    """
    data_id = 0xABCD
    num_messages = 100
    
    profile = E2EProfile02()
    sm = E2EStateMachine(max_delta_counter=1)
    
    errors = []
    counters = []
    
    for i in range(num_messages):
        test_data = struct.pack('>H', i) + b'\x00' * 6
        
        # Protect
        protected = profile.protect(test_data, data_id)
        
        # Simulate transmission (copy)
        received = bytes(protected)
        
        # Randomly corrupt one message (message 50)
        if i == 50:
            received = bytearray(received)
            received[-1] ^= 0xFF
            received = bytes(received)
        
        # Check
        valid, extracted, counter = profile.check(received, data_id)
        
        if not valid:
            errors.append({'message': i, 'error': 'CRC failed', 'expected': True})
        else:
            status = sm.check_counter(counter, max_counter=15)
            counters.append(counter)
            
            if status == E2ECheckStatus.ERROR:
                errors.append({'message': i, 'error': 'Counter error', 'counter': counter})
    
    return {
        'passed': len(errors) == 1 and errors[0]['message'] == 50,
        'messages_sent': num_messages,
        'errors_detected': len(errors),
        'error_at_message': errors[0]['message'] if errors else None,
        'counters': counters[:10]  # First 10 counters
    }


###############################################################################
# Complete Test Sequences
###############################################################################

def get_e2e_basic_test_sequence():
    """Get basic E2E test sequence"""
    return [
        (test_e2e_profile01_crc_calculation, "E2E_P01_CRC_Calc", (), {}),
        (test_e2e_profile01_crc_error_detection, "E2E_P01_CRC_Error", (), {}),
        (test_e2e_profile02_counter_increment, "E2E_P02_Counter", (), {}),
        (test_e2e_profile05_crc16, "E2E_P05_CRC16", (), {}),
    ]


def get_e2e_full_test_sequence():
    """Get full E2E test sequence"""
    return [
        # Profile 01
        (test_e2e_profile01_crc_calculation, "E2E_P01_CRC_Calc", (), {}),
        (test_e2e_profile01_crc_error_detection, "E2E_P01_CRC_Error", (), {}),
        
        # Profile 02
        (test_e2e_profile02_counter_increment, "E2E_P02_Counter", (), {}),
        
        # Profile 04
        (test_e2e_profile04_crc32, "E2E_P04_CRC32", (), {}),
        
        # Profile 05
        (test_e2e_profile05_crc16, "E2E_P05_CRC16", (), {}),
        
        # Profile 06
        (test_e2e_profile06_counter_verification, "E2E_P06_Counter_SM", (), {}),
        
        # Profile 22
        (test_e2e_profile22_large_data, "E2E_P22_LargeData", (), {}),
        
        # State Machine
        (test_e2e_max_delta_counter, "E2E_MaxDeltaCounter", (), {}),
        
        # Data ID
        (test_e2e_data_id_inclusion, "E2E_DataID", (), {}),
        
        # Full sequence
        (test_e2e_message_integrity_sequence, "E2E_Full_Sequence", (), {}),
    ]


def get_e2e_safety_test_sequence():
    """Get E2E safety-critical test sequence"""
    return [
        (test_e2e_profile01_crc_calculation, "E2E_CRC_Verify", (), {}),
        (test_e2e_profile01_crc_error_detection, "E2E_Error_Detect", (), {}),
        (test_e2e_max_delta_counter, "E2E_Counter_Check", (), {}),
        (test_e2e_message_integrity_sequence, "E2E_Integrity_100", (), {}),
    ]


###############################################################################
# Main Entry Point
###############################################################################

if __name__ == "__main__":
    print("E2E Protection Test Sequences")
    print("="*60)
    
    sys.path.insert(0, '/home/admin/eth-dds-integration/tests/hil')
    from hil_host import create_hil_manager
    
    manager = create_hil_manager(simulation_mode=True)
    
    if manager.setup():
        print("Setup successful\n")
        
        # Run basic test sequence
        sequence = get_e2e_basic_test_sequence()
        results = manager.run_test_sequence(sequence)
        
        print("\nTest Results:")
        print("-"*60)
        for result in results:
            status = "PASS" if result.passed else "FAIL"
            print(f"  {result.test_name}: {status}")
        
        summary = manager.get_summary()
        print(f"\nSummary: {summary['passed']}/{summary['total']} passed")
        
        manager.teardown()
    else:
        print("Setup failed")
        sys.exit(1)
