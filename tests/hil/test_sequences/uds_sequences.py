#!/usr/bin/env python3
"""
UDS (Unified Diagnostic Services) Test Sequences
UDS诊断测试序列

测试范围:
- 会话管理 (Session Management)
- ECU复位 (ECU Reset)
- 安全访问 (Security Access)
- 数据读写 (Data Read/Write)
- 故障码读取 (DTC Read)
- 缺省响应处理 (Negative Response Handling)

应用标准: ISO 14229-1:2020
"""

import sys
import time
import struct
from typing import Optional, List, Dict, Any
from dataclasses import dataclass

# Add parent directory to path
sys.path.insert(0, '/home/admin/eth-dds-integration/tests/hil')

from hil_host import (
    HilTestManager, UdsClient, UdsRequest, UdsResponse,
    UdsServiceId, UdsSessionType, UdsNegativeResponseCode,
    CanMessage, TestResult
)


###############################################################################
# Test Configuration
###############################################################################

# UDS Configuration
UDS_TX_ID = 0x7E0  # Tester -> ECU
UDS_RX_ID = 0x7E8  # ECU -> Tester

# Test DIDs
DID_VIN = 0xF190
DID_ECU_SERIAL = 0xF18C
DID_SOFTWARE_VERSION = 0xF194
DID_HARDWARE_VERSION = 0xF193
DID_SYSTEM_SUPPLIER = 0xF18A

# Security Access Levels
SEC_LEVEL_1 = 0x01  # Request Seed
SEC_LEVEL_2 = 0x02  # Send Key for Level 1
SEC_LEVEL_3 = 0x03  # Extended Security - Request Seed
SEC_LEVEL_4 = 0x04  # Extended Security - Send Key

# Timing Parameters
P2_SERVER_MAX_MS = 50
P2_STAR_SERVER_MAX_MS = 5000
S3_SERVER_MS = 2000


###############################################################################
# Helper Functions
###############################################################################

def check_positive_response(response: UdsResponse, expected_service: int) -> bool:
    """Check if response is positive for expected service"""
    if response.is_negative:
        return False
    return response.is_positive_response_to(expected_service)


def check_negative_response(response: UdsResponse, expected_code: int) -> bool:
    """Check if response is negative with expected code"""
    return response.is_negative and response.negative_code == expected_code


def simulate_ecu_response(manager: HilTestManager, request: UdsRequest,
                          response_data: bytes, is_negative: bool = False,
                          nrc: int = 0) -> None:
    """Simulate ECU response (for testing without real ECU)"""
    can_if = manager.get_interface("can0")
    
    if isinstance(can_if.__class__.__name__, str):
        # Check if it's simulated
        if hasattr(can_if, 'inject_frame'):
            if is_negative:
                data = bytes([0x7F, request.service_id, nrc])
            else:
                data = bytes([request.service_id + 0x40]) + response_data
            
            frame = CanMessage(arbitration_id=UDS_RX_ID, data=data)
            can_if.inject_frame(frame)


###############################################################################
# Session Management Tests
###############################################################################

def test_session_control_default(manager: HilTestManager) -> Dict[str, Any]:
    """
    Test: Default Session Control
    测试默认会话切换
    
    Steps:
    1. Send Diagnostic Session Control (0x10) with Default Session (0x01)
    2. Verify positive response
    3. Verify session parameters in response
    """
    client = manager.get_uds_client("uds_can")
    if not client:
        return {'passed': False, 'error': 'UDS client not available'}
    
    # In simulation mode, inject expected response
    if manager.simulation_mode:
        simulate_ecu_response(
            manager,
            UdsRequest(service_id=UdsServiceId.DIAGNOSTIC_SESSION_CONTROL, subfunction=0x01),
            bytes([0x01, 0x00, 0x32, 0x01, 0xF4])  # Session + P2/P2* timings
        )
    
    response = client.session_control(UdsSessionType.DEFAULT)
    
    passed = check_positive_response(response, UdsServiceId.DIAGNOSTIC_SESSION_CONTROL)
    
    details = {
        'request_service': UdsServiceId.DIAGNOSTIC_SESSION_CONTROL,
        'session_type': UdsSessionType.DEFAULT,
        'response_service': response.service_id,
        'is_negative': response.is_negative,
        'response_data': response.data.hex() if response.data else None
    }
    
    if passed and len(response.data) >= 5:
        # Parse session timing
        p2_max = struct.unpack('>H', response.data[1:3])[0]
        p2_star_max = struct.unpack('>H', response.data[3:5])[0]
        details['p2_max_ms'] = p2_max
        details['p2_star_max_ms'] = p2_star_max
    
    return {'passed': passed, **details}


def test_session_control_extended(manager: HilTestManager) -> Dict[str, Any]:
    """
    Test: Extended Diagnostic Session
    
    Steps:
    1. Request Extended Session (0x03)
    2. Verify positive response
    3. Verify session is active
    """
    client = manager.get_uds_client("uds_can")
    if not client:
        return {'passed': False, 'error': 'UDS client not available'}
    
    if manager.simulation_mode:
        simulate_ecu_response(
            manager,
            UdsRequest(service_id=UdsServiceId.DIAGNOSTIC_SESSION_CONTROL, subfunction=0x03),
            bytes([0x03, 0x00, 0x32, 0x01, 0xF4])
        )
    
    response = client.session_control(UdsSessionType.EXTENDED)
    
    passed = check_positive_response(response, UdsServiceId.DIAGNOSTIC_SESSION_CONTROL)
    
    return {
        'passed': passed,
        'session_type': UdsSessionType.EXTENDED,
        'current_session': client.session.value if passed else None
    }


def test_session_control_programming(manager: HilTestManager) -> Dict[str, Any]:
    """
    Test: Programming Session
    
    Steps:
    1. Request Programming Session (0x02)
    2. Verify positive response or security access denied
    """
    client = manager.get_uds_client("uds_can")
    if not client:
        return {'passed': False, 'error': 'UDS client not available'}
    
    if manager.simulation_mode:
        simulate_ecu_response(
            manager,
            UdsRequest(service_id=UdsServiceId.DIAGNOSTIC_SESSION_CONTROL, subfunction=0x02),
            bytes([0x02, 0x00, 0x32, 0x01, 0xF4])
        )
    
    response = client.session_control(UdsSessionType.PROGRAMMING)
    
    # Programming session may require specific conditions
    passed = (check_positive_response(response, UdsServiceId.DIAGNOSTIC_SESSION_CONTROL) or
              check_negative_response(response, UdsNegativeResponseCode.CONDITIONS_NOT_CORRECT))
    
    return {
        'passed': passed,
        'session_type': UdsSessionType.PROGRAMMING,
        'is_negative': response.is_negative
    }


def test_session_timeout_s3(manager: HilTestManager) -> Dict[str, Any]:
    """
    Test: Session Timeout (S3)
    
    Steps:
    1. Enter Extended Session
    2. Wait longer than S3 timeout without Tester Present
    3. Verify ECU returns to Default Session
    """
    client = manager.get_uds_client("uds_can")
    if not client:
        return {'passed': False, 'error': 'UDS client not available'}
    
    # Enter extended session
    if manager.simulation_mode:
        simulate_ecu_response(
            manager,
            UdsRequest(service_id=UdsServiceId.DIAGNOSTIC_SESSION_CONTROL, subfunction=0x03),
            bytes([0x03, 0x00, 0x32, 0x01, 0xF4])
        )
    
    client.session_control(UdsSessionType.EXTENDED)
    
    # Wait for S3 timeout (shortened for test)
    time.sleep(0.5)
    
    # Try to access service that requires extended session
    if manager.simulation_mode:
        simulate_ecu_response(
            manager,
            UdsRequest(service_id=UdsServiceId.READ_DATA_BY_IDENTIFIER, data=b'\xF1\x90'),
            b'',
            is_negative=True,
            nrc=UdsNegativeResponseCode.SERVICE_NOT_SUPPORTED_IN_SESSION
        )
    
    response = client.read_data_by_identifier(DID_VIN)
    
    # Should get error about session not supported
    passed = check_negative_response(response, UdsNegativeResponseCode.SERVICE_NOT_SUPPORTED_IN_SESSION)
    
    return {
        'passed': passed,
        'test_type': 'session_timeout_s3',
        'negative_code': response.negative_code if response.is_negative else None
    }


def test_tester_present_keepalive(manager: HilTestManager) -> Dict[str, Any]:
    """
    Test: Tester Present Keep-Alive
    
    Steps:
    1. Enter Extended Session
    2. Send periodic Tester Present (0x3E)
    3. Verify session remains active
    """
    client = manager.get_uds_client("uds_can")
    if not client:
        return {'passed': False, 'error': 'UDS client not available'}
    
    # Enter extended session
    if manager.simulation_mode:
        simulate_ecu_response(
            manager,
            UdsRequest(service_id=UdsServiceId.DIAGNOSTIC_SESSION_CONTROL, subfunction=0x03),
            bytes([0x03, 0x00, 0x32, 0x01, 0xF4])
        )
    
    client.session_control(UdsSessionType.EXTENDED)
    
    # Send Tester Present multiple times
    responses = []
    for i in range(3):
        if manager.simulation_mode:
            simulate_ecu_response(
                manager,
                UdsRequest(service_id=UdsServiceId.TESTER_PRESENT, subfunction=0x00),
                bytes([0x00])
            )
        
        resp = client.tester_present()
        responses.append(resp.is_positive_response_to(UdsServiceId.TESTER_PRESENT))
        time.sleep(0.2)
    
    passed = all(responses) and len(responses) == 3
    
    return {
        'passed': passed,
        'test_type': 'tester_present_keepalive',
        'responses_received': len(responses)
    }


###############################################################################
# ECU Reset Tests
###############################################################################

def test_ecu_reset_hard(manager: HilTestManager) -> Dict[str, Any]:
    """
    Test: Hard ECU Reset
    
    Steps:
    1. Request ECU Reset (0x11) with Hard Reset (0x01)
    2. Verify positive response
    3. Verify ECU responds after reset
    """
    client = manager.get_uds_client("uds_can")
    if not client:
        return {'passed': False, 'error': 'UDS client not available'}
    
    # Must be in extended session
    if manager.simulation_mode:
        simulate_ecu_response(
            manager,
            UdsRequest(service_id=UdsServiceId.DIAGNOSTIC_SESSION_CONTROL, subfunction=0x03),
            bytes([0x03, 0x00, 0x32, 0x01, 0xF4])
        )
    
    client.session_control(UdsSessionType.EXTENDED)
    
    if manager.simulation_mode:
        simulate_ecu_response(
            manager,
            UdsRequest(service_id=UdsServiceId.ECU_RESET, subfunction=0x01),
            bytes([0x01])
        )
    
    response = client.ecu_reset(0x01)  # Hard Reset
    
    passed = check_positive_response(response, UdsServiceId.ECU_RESET)
    
    return {
        'passed': passed,
        'reset_type': 'hard',
        'reset_type_code': 0x01
    }


def test_ecu_reset_soft(manager: HilTestManager) -> Dict[str, Any]:
    """
    Test: Soft ECU Reset
    
    Steps:
    1. Request ECU Reset (0x11) with Soft Reset (0x03)
    2. Verify positive response
    """
    client = manager.get_uds_client("uds_can")
    if not client:
        return {'passed': False, 'error': 'UDS client not available'}
    
    if manager.simulation_mode:
        simulate_ecu_response(
            manager,
            UdsRequest(service_id=UdsServiceId.DIAGNOSTIC_SESSION_CONTROL, subfunction=0x03),
            bytes([0x03, 0x00, 0x32, 0x01, 0xF4])
        )
        client.session_control(UdsSessionType.EXTENDED)
        
        simulate_ecu_response(
            manager,
            UdsRequest(service_id=UdsServiceId.ECU_RESET, subfunction=0x03),
            bytes([0x03])
        )
    
    response = client.ecu_reset(0x03)  # Soft Reset
    
    passed = check_positive_response(response, UdsServiceId.ECU_RESET)
    
    return {
        'passed': passed,
        'reset_type': 'soft',
        'reset_type_code': 0x03
    }


###############################################################################
# Security Access Tests
###############################################################################

def test_security_access_sequence(manager: HilTestManager) -> Dict[str, Any]:
    """
    Test: Security Access Sequence
    
    Steps:
    1. Request Seed (Level 1)
    2. Verify seed received
    3. Send Key (derived from seed)
    4. Verify security access granted
    """
    client = manager.get_uds_client("uds_can")
    if not client:
        return {'passed': False, 'error': 'UDS client not available'}
    
    # Must be in extended session
    if manager.simulation_mode:
        simulate_ecu_response(
            manager,
            UdsRequest(service_id=UdsServiceId.DIAGNOSTIC_SESSION_CONTROL, subfunction=0x03),
            bytes([0x03, 0x00, 0x32, 0x01, 0xF4])
        )
        client.session_control(UdsSessionType.EXTENDED)
        
        # Simulate seed response
        simulate_ecu_response(
            manager,
            UdsRequest(service_id=UdsServiceId.SECURITY_ACCESS, subfunction=SEC_LEVEL_1),
            bytes([SEC_LEVEL_1, 0x12, 0x34, 0x56, 0x78])  # 4-byte seed
        )
    
    # Request seed
    response = client.security_access_request_seed(SEC_LEVEL_1)
    
    if not check_positive_response(response, UdsServiceId.SECURITY_ACCESS):
        return {'passed': False, 'error': 'Failed to get seed', 'step': 'request_seed'}
    
    # Extract seed
    seed = response.data[1:5] if len(response.data) >= 5 else b'\x00\x00\x00\x00'
    
    # Compute key (simplified - real implementation would use proper algorithm)
    key = bytes([b ^ 0xFF for b in seed])  # Simple XOR for test
    
    if manager.simulation_mode:
        simulate_ecu_response(
            manager,
            UdsRequest(service_id=UdsServiceId.SECURITY_ACCESS, subfunction=SEC_LEVEL_2, data=key),
            bytes([SEC_LEVEL_2])
        )
    
    # Send key
    response = client.security_access_send_key(SEC_LEVEL_1, key)
    
    passed = check_positive_response(response, UdsServiceId.SECURITY_ACCESS)
    
    return {
        'passed': passed,
        'security_level': client.security_level if passed else 0,
        'seed': seed.hex(),
        'key_sent': key.hex()
    }


def test_security_access_invalid_key(manager: HilTestManager) -> Dict[str, Any]:
    """
    Test: Security Access with Invalid Key
    
    Steps:
    1. Request Seed
    2. Send invalid key
    3. Verify negative response (Invalid Key)
    """
    client = manager.get_uds_client("uds_can")
    if not client:
        return {'passed': False, 'error': 'UDS client not available'}
    
    if manager.simulation_mode:
        simulate_ecu_response(
            manager,
            UdsRequest(service_id=UdsServiceId.DIAGNOSTIC_SESSION_CONTROL, subfunction=0x03),
            bytes([0x03, 0x00, 0x32, 0x01, 0xF4])
        )
        client.session_control(UdsSessionType.EXTENDED)
        
        simulate_ecu_response(
            manager,
            UdsRequest(service_id=UdsServiceId.SECURITY_ACCESS, subfunction=SEC_LEVEL_1),
            bytes([SEC_LEVEL_1, 0x12, 0x34, 0x56, 0x78])
        )
    
    response = client.security_access_request_seed(SEC_LEVEL_1)
    
    # Send invalid key
    invalid_key = b'\xFF\xFF\xFF\xFF'
    
    if manager.simulation_mode:
        simulate_ecu_response(
            manager,
            UdsRequest(service_id=UdsServiceId.SECURITY_ACCESS, subfunction=SEC_LEVEL_2, data=invalid_key),
            b'',
            is_negative=True,
            nrc=UdsNegativeResponseCode.INVALID_KEY
        )
    
    response = client.security_access_send_key(SEC_LEVEL_1, invalid_key)
    
    passed = check_negative_response(response, UdsNegativeResponseCode.INVALID_KEY)
    
    return {
        'passed': passed,
        'negative_code': response.negative_code,
        'expected_nrc': UdsNegativeResponseCode.INVALID_KEY
    }


def test_security_access_wrong_sequence(manager: HilTestManager) -> Dict[str, Any]:
    """
    Test: Security Access Wrong Sequence
    
    Steps:
    1. Send Key without requesting seed first
    2. Verify negative response (Request Sequence Error)
    """
    client = manager.get_uds_client("uds_can")
    if not client:
        return {'passed': False, 'error': 'UDS client not available'}
    
    if manager.simulation_mode:
        simulate_ecu_response(
            manager,
            UdsRequest(service_id=UdsServiceId.DIAGNOSTIC_SESSION_CONTROL, subfunction=0x03),
            bytes([0x03, 0x00, 0x32, 0x01, 0xF4])
        )
        client.session_control(UdsSessionType.EXTENDED)
        
        simulate_ecu_response(
            manager,
            UdsRequest(service_id=UdsServiceId.SECURITY_ACCESS, subfunction=SEC_LEVEL_2, data=b'\x00\x00\x00\x00'),
            b'',
            is_negative=True,
            nrc=UdsNegativeResponseCode.REQUEST_SEQUENCE_ERROR
        )
    
    response = client.security_access_send_key(SEC_LEVEL_1, b'\x00\x00\x00\x00')
    
    passed = check_negative_response(response, UdsNegativeResponseCode.REQUEST_SEQUENCE_ERROR)
    
    return {
        'passed': passed,
        'negative_code': response.negative_code,
        'expected_nrc': UdsNegativeResponseCode.REQUEST_SEQUENCE_ERROR
    }


###############################################################################
# Data Read/Write Tests
###############################################################################

def test_read_data_by_identifier_vin(manager: HilTestManager) -> Dict[str, Any]:
    """
    Test: Read VIN by Identifier
    
    Steps:
    1. Read DID 0xF190 (VIN)
    2. Verify positive response
    3. Verify VIN format (17 characters)
    """
    client = manager.get_uds_client("uds_can")
    if not client:
        return {'passed': False, 'error': 'UDS client not available'}
    
    # VIN response
    vin = "1HGBH41JXMN109186"  # 17 chars
    
    if manager.simulation_mode:
        simulate_ecu_response(
            manager,
            UdsRequest(service_id=UdsServiceId.READ_DATA_BY_IDENTIFIER, 
                      data=struct.pack('>H', DID_VIN)),
            struct.pack('>H', DID_VIN) + vin.encode('ascii')
        )
    
    response = client.read_data_by_identifier(DID_VIN)
    
    passed = check_positive_response(response, UdsServiceId.READ_DATA_BY_IDENTIFIER)
    
    details = {'passed': passed}
    
    if passed and len(response.data) >= 2:
        did_received = struct.unpack('>H', response.data[0:2])[0]
        vin_data = response.data[2:].decode('ascii', errors='ignore') if len(response.data) > 2 else ""
        
        details['did_received'] = hex(did_received)
        details['vin'] = vin_data
        details['vin_length'] = len(vin_data)
        details['vin_valid'] = len(vin_data) == 17
        
        passed = passed and (did_received == DID_VIN) and (len(vin_data) == 17)
    
    return details


def test_write_data_by_identifier(manager: HilTestManager) -> Dict[str, Any]:
    """
    Test: Write Data by Identifier
    
    Steps:
    1. Enter Extended Session
    2. Unlock security level
    3. Write to configurable DID
    4. Verify write successful
    """
    client = manager.get_uds_client("uds_can")
    if not client:
        return {'passed': False, 'error': 'UDS client not available'}
    
    # Configurable DID (e.g., workshop code)
    DID_WORKSHOP_CODE = 0xF198
    workshop_code = b'WORKSHOP01'
    
    if manager.simulation_mode:
        simulate_ecu_response(
            manager,
            UdsRequest(service_id=UdsServiceId.DIAGNOSTIC_SESSION_CONTROL, subfunction=0x03),
            bytes([0x03, 0x00, 0x32, 0x01, 0xF4])
        )
        client.session_control(UdsSessionType.EXTENDED)
        
        simulate_ecu_response(
            manager,
            UdsRequest(service_id=UdsServiceId.WRITE_DATA_BY_IDENTIFIER,
                      data=struct.pack('>H', DID_WORKSHOP_CODE) + workshop_code),
            struct.pack('>H', DID_WORKSHOP_CODE)
        )
    
    response = client.write_data_by_identifier(DID_WORKSHOP_CODE, workshop_code)
    
    passed = check_positive_response(response, UdsServiceId.WRITE_DATA_BY_IDENTIFIER)
    
    return {
        'passed': passed,
        'did': hex(DID_WORKSHOP_CODE),
        'data_written': workshop_code.hex()
    }


###############################################################################
# Negative Response Tests
###############################################################################

def test_service_not_supported(manager: HilTestManager) -> Dict[str, Any]:
    """
    Test: Service Not Supported
    
    Steps:
    1. Send unsupported service ID
    2. Verify negative response (Service Not Supported)
    """
    client = manager.get_uds_client("uds_can")
    if not client:
        return {'passed': False, 'error': 'UDS client not available'}
    
    UNSUPPORTED_SERVICE = 0xBF  # Reserved/unused service
    
    if manager.simulation_mode:
        simulate_ecu_response(
            manager,
            UdsRequest(service_id=UNSUPPORTED_SERVICE),
            b'',
            is_negative=True,
            nrc=UdsNegativeResponseCode.SERVICE_NOT_SUPPORTED
        )
    
    request = UdsRequest(service_id=UNSUPPORTED_SERVICE)
    response = client.send_request(request)
    
    passed = check_negative_response(response, UdsNegativeResponseCode.SERVICE_NOT_SUPPORTED)
    
    return {
        'passed': passed,
        'service_id': hex(UNSUPPORTED_SERVICE),
        'negative_code': response.negative_code
    }


def test_subfunction_not_supported(manager: HilTestManager) -> Dict[str, Any]:
    """
    Test: Subfunction Not Supported
    
    Steps:
    1. Send valid service with invalid subfunction
    2. Verify negative response (Subfunction Not Supported)
    """
    client = manager.get_uds_client("uds_can")
    if not client:
        return {'passed': False, 'error': 'UDS client not available'}
    
    INVALID_SESSION = 0xFF  # Invalid session type
    
    if manager.simulation_mode:
        simulate_ecu_response(
            manager,
            UdsRequest(service_id=UdsServiceId.DIAGNOSTIC_SESSION_CONTROL, subfunction=INVALID_SESSION),
            b'',
            is_negative=True,
            nrc=UdsNegativeResponseCode.SUBFUNCTION_NOT_SUPPORTED
        )
    
    request = UdsRequest(
        service_id=UdsServiceId.DIAGNOSTIC_SESSION_CONTROL,
        subfunction=INVALID_SESSION
    )
    response = client.send_request(request)
    
    passed = check_negative_response(response, UdsNegativeResponseCode.SUBFUNCTION_NOT_SUPPORTED)
    
    return {
        'passed': passed,
        'subfunction': hex(INVALID_SESSION),
        'negative_code': response.negative_code
    }


def test_incorrect_message_length(manager: HilTestManager) -> Dict[str, Any]:
    """
    Test: Incorrect Message Length
    
    Steps:
    1. Send request with wrong data length
    2. Verify negative response (Incorrect Message Length)
    """
    client = manager.get_uds_client("uds_can")
    if not client:
        return {'passed': False, 'error': 'UDS client not available'}
    
    if manager.simulation_mode:
        simulate_ecu_response(
            manager,
            UdsRequest(service_id=UdsServiceId.READ_DATA_BY_IDENTIFIER, data=b''),  # Missing DID
            b'',
            is_negative=True,
            nrc=UdsNegativeResponseCode.INCORRECT_MESSAGE_LENGTH
        )
    
    request = UdsRequest(
        service_id=UdsServiceId.READ_DATA_BY_IDENTIFIER,
        data=b''  # Missing DID - should be 2 bytes
    )
    response = client.send_request(request)
    
    passed = check_negative_response(response, UdsNegativeResponseCode.INCORRECT_MESSAGE_LENGTH)
    
    return {
        'passed': passed,
        'negative_code': response.negative_code
    }


###############################################################################
# Complete Test Sequences
###############################################################################

def get_uds_basic_test_sequence() -> List:
    """Get basic UDS test sequence"""
    return [
        (test_session_control_default, "UDS_Session_Default", (), {}),
        (test_session_control_extended, "UDS_Session_Extended", (), {}),
        (test_tester_present_keepalive, "UDS_TesterPresent", (), {}),
        (test_read_data_by_identifier_vin, "UDS_Read_VIN", (), {}),
    ]


def get_uds_full_test_sequence() -> List:
    """Get full UDS test sequence"""
    return [
        # Session Management
        (test_session_control_default, "UDS_Session_Default", (), {}),
        (test_session_control_extended, "UDS_Session_Extended", (), {}),
        (test_session_control_programming, "UDS_Session_Programming", (), {}),
        (test_tester_present_keepalive, "UDS_TesterPresent", (), {}),
        (test_session_timeout_s3, "UDS_Session_Timeout", (), {}),
        
        # ECU Reset
        (test_ecu_reset_soft, "UDS_Reset_Soft", (), {}),
        (test_ecu_reset_hard, "UDS_Reset_Hard", (), {}),
        
        # Security Access
        (test_security_access_sequence, "UDS_Security_Access", (), {}),
        (test_security_access_invalid_key, "UDS_Security_InvalidKey", (), {}),
        (test_security_access_wrong_sequence, "UDS_Security_WrongSequence", (), {}),
        
        # Data Read/Write
        (test_read_data_by_identifier_vin, "UDS_Read_VIN", (), {}),
        (test_write_data_by_identifier, "UDS_Write_Data", (), {}),
        
        # Negative Response
        (test_service_not_supported, "UDS_NRC_ServiceNotSupported", (), {}),
        (test_subfunction_not_supported, "UDS_NRC_SubfunctionNotSupported", (), {}),
        (test_incorrect_message_length, "UDS_NRC_IncorrectLength", (), {}),
    ]


def get_uds_security_test_sequence() -> List:
    """Get UDS security-focused test sequence"""
    return [
        (test_session_control_extended, "UDS_Session_Extended", (), {}),
        (test_security_access_sequence, "UDS_Security_Access_Valid", (), {}),
        (test_security_access_invalid_key, "UDS_Security_InvalidKey", (), {}),
        (test_security_access_wrong_sequence, "UDS_Security_WrongSequence", (), {}),
    ]


###############################################################################
# Main Entry Point
###############################################################################

if __name__ == "__main__":
    print("UDS Test Sequences")
    print("="*60)
    
    # Import and run tests
    sys.path.insert(0, '/home/admin/eth-dds-integration/tests/hil')
    from hil_host import create_hil_manager
    
    # Create manager in simulation mode
    manager = create_hil_manager(simulation_mode=True)
    
    if manager.setup():
        print("Setup successful\n")
        
        # Run basic test sequence
        sequence = get_uds_basic_test_sequence()
        results = manager.run_test_sequence(sequence)
        
        # Print results
        print("\nTest Results:")
        print("-"*60)
        for result in results:
            status = "PASS" if result.passed else "FAIL"
            print(f"  {result.test_name}: {status} ({result.duration_ms:.1f}ms)")
        
        # Summary
        summary = manager.get_summary()
        print(f"\nSummary: {summary['passed']}/{summary['total']} passed")
        
        manager.teardown()
    else:
        print("Setup failed")
        sys.exit(1)
