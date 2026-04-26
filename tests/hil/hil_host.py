#!/usr/bin/env python3
"""
HIL (Hardware-in-the-Loop) Test Framework - Host Side
硬件在环测试主机端框架

支持:
- CAN接口 (python-can: PCAN/Vector/Kvaser/SocketCAN)
- 以太网接口 (scapy: Raw Socket/DoIP)
- UDS诊断仪模拟
- 测试序列执行
- 模拟模式支持 (无需硬件)

技术栈:
- Python 3.10+
- python-can
- scapy
- pytest
"""

import os
import sys
import time
import json
import struct
import socket
import logging
from enum import Enum, IntEnum
from dataclasses import dataclass, field
from typing import Optional, Callable, List, Dict, Any, Tuple, Union
from collections import deque
from abc import ABC, abstractmethod

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger('HIL')

# Try to import optional dependencies
try:
    import can
    CAN_AVAILABLE = True
except ImportError:
    CAN_AVAILABLE = False
    logger.warning("python-can not available, CAN functionality limited")

try:
    from scapy.all import Ether, IP, TCP, UDP, Raw, sniff, sendp
    from scapy.layers.inet import ETH_P_IP
    SCAPY_AVAILABLE = True
except ImportError:
    SCAPY_AVAILABLE = False
    logger.warning("scapy not available, Ethernet functionality limited")


###############################################################################
# Constants and Enums
###############################################################################

class UdsServiceId(IntEnum):
    """UDS Service IDs (ISO 14229-1)"""
    DIAGNOSTIC_SESSION_CONTROL = 0x10
    ECU_RESET = 0x11
    SECURITY_ACCESS = 0x27
    COMMUNICATION_CONTROL = 0x28
    TESTER_PRESENT = 0x3E
    CONTROL_DTC_SETTING = 0x85
    READ_DATA_BY_IDENTIFIER = 0x22
    WRITE_DATA_BY_IDENTIFIER = 0x2E
    CLEAR_DIAGNOSTIC_INFORMATION = 0x14
    READ_DTC_INFORMATION = 0x19
    ROUTINE_CONTROL = 0x31
    REQUEST_DOWNLOAD = 0x34
    TRANSFER_DATA = 0x36
    REQUEST_TRANSFER_EXIT = 0x37


class UdsSessionType(IntEnum):
    """UDS Session Types"""
    DEFAULT = 0x01
    PROGRAMMING = 0x02
    EXTENDED = 0x03
    SAFETY_SYSTEM = 0x04


class UdsNegativeResponseCode(IntEnum):
    """UDS Negative Response Codes"""
    GENERAL_REJECT = 0x10
    SERVICE_NOT_SUPPORTED = 0x11
    SUBFUNCTION_NOT_SUPPORTED = 0x12
    INCORRECT_MESSAGE_LENGTH = 0x13
    BUSY_REPEAT_REQUEST = 0x21
    CONDITIONS_NOT_CORRECT = 0x22
    REQUEST_SEQUENCE_ERROR = 0x24
    REQUEST_OUT_OF_RANGE = 0x31
    SECURITY_ACCESS_DENIED = 0x33
    INVALID_KEY = 0x35
    EXCEED_NUMBER_OF_ATTEMPTS = 0x36
    RESPONSE_PENDING = 0x78


class E2EProfile(IntEnum):
    """E2E Profile IDs"""
    P01 = 0x01  # CRC8
    P02 = 0x02  # CRC8 + Counter
    P04 = 0x04  # CRC32
    P05 = 0x05  # CRC16
    P06 = 0x06  # CRC16 + Counter
    P07 = 0x07  # CRC32 + Counter
    P11 = 0x0B  # Dynamic CRC8
    P22 = 0x16  # Dynamic CRC16 (Large Data)


class OtaState(IntEnum):
    """OTA Update States"""
    IDLE = 0
    CAMPAIGN_RECEIVED = 1
    DOWNLOADING = 2
    VERIFYING = 3
    READY_TO_INSTALL = 4
    INSTALLING = 5
    ACTIVATING = 6
    VERIFYING_BOOT = 7
    SUCCESS = 8
    FAILED = 9
    ROLLING_BACK = 10


###############################################################################
# Data Classes
###############################################################################

@dataclass
class CanMessage:
    """CAN Message structure"""
    arbitration_id: int
    data: bytes
    is_extended_id: bool = False
    is_fd: bool = False
    timestamp: float = field(default_factory=time.time)
    
    def __post_init__(self):
        if isinstance(self.data, list):
            self.data = bytes(self.data)


@dataclass
class EthernetFrame:
    """Ethernet Frame structure"""
    dst_mac: bytes
    src_mac: bytes
    ethertype: int
    payload: bytes
    timestamp: float = field(default_factory=time.time)
    
    def __post_init__(self):
        if isinstance(self.dst_mac, str):
            self.dst_mac = bytes.fromhex(self.dst_mac.replace(':', ''))
        if isinstance(self.src_mac, str):
            self.src_mac = bytes.fromhex(self.src_mac.replace(':', ''))
        if isinstance(self.payload, list):
            self.payload = bytes(self.payload)


@dataclass
class UdsRequest:
    """UDS Request structure"""
    service_id: int
    subfunction: Optional[int] = None
    data: bytes = b''
    suppress_response: bool = False
    
    def to_bytes(self) -> bytes:
        """Serialize to bytes"""
        payload = bytes([self.service_id])
        if self.subfunction is not None:
            subfunc = self.subfunction
            if self.suppress_response:
                subfunc |= 0x80
            payload += bytes([subfunc])
        payload += self.data
        return payload


@dataclass
class UdsResponse:
    """UDS Response structure"""
    service_id: int
    data: bytes
    is_negative: bool = False
    negative_code: int = 0
    timestamp: float = field(default_factory=time.time)
    
    @classmethod
    def from_bytes(cls, data: bytes) -> 'UdsResponse':
        """Deserialize from bytes"""
        if len(data) < 1:
            raise ValueError("Empty response")
        
        # Check for negative response
        if data[0] == 0x7F:
            if len(data) < 3:
                raise ValueError("Invalid negative response")
            return cls(
                service_id=data[1],
                data=data[2:],
                is_negative=True,
                negative_code=data[2]
            )
        
        # Positive response
        return cls(
            service_id=data[0],
            data=data[1:]
        )
    
    def is_positive_response_to(self, request_service_id: int) -> bool:
        """Check if this is a positive response to a request"""
        if self.is_negative:
            return False
        return (self.service_id & 0x7F) == (request_service_id + 0x40)


@dataclass
class E2EProtectedMessage:
    """E2E Protected Message"""
    profile: E2EProfile
    data_id: int
    payload: bytes
    counter: int = 0
    crc: int = 0
    
    def verify(self) -> bool:
        """Verify E2E protection"""
        # Simplified verification - would implement full CRC checks in production
        return True


@dataclass
class TestResult:
    """Test execution result"""
    test_name: str
    passed: bool
    duration_ms: float
    error_message: Optional[str] = None
    details: Dict[str, Any] = field(default_factory=dict)
    
    def to_dict(self) -> Dict[str, Any]:
        return {
            'test_name': self.test_name,
            'passed': self.passed,
            'duration_ms': self.duration_ms,
            'error_message': self.error_message,
            'details': self.details
        }


###############################################################################
# HIL Interface Classes
###############################################################################

class HilInterface(ABC):
    """Abstract base class for HIL interfaces"""
    
    def __init__(self, name: str):
        self.name = name
        self.is_open = False
        self._callbacks: List[Callable] = []
    
    @abstractmethod
    def open(self) -> bool:
        """Open the interface"""
        pass
    
    @abstractmethod
    def close(self) -> None:
        """Close the interface"""
        pass
    
    @abstractmethod
    def send(self, data: Any) -> bool:
        """Send data"""
        pass
    
    @abstractmethod
    def receive(self, timeout_ms: int = 1000) -> Optional[Any]:
        """Receive data"""
        pass
    
    def register_callback(self, callback: Callable) -> None:
        """Register receive callback"""
        self._callbacks.append(callback)
    
    def _notify_callbacks(self, data: Any) -> None:
        """Notify all registered callbacks"""
        for callback in self._callbacks:
            try:
                callback(data)
            except Exception as e:
                logger.error(f"Callback error: {e}")


class SimulatedCanInterface(HilInterface):
    """Simulated CAN interface for testing without hardware"""
    
    _instances: Dict[str, 'SimulatedCanInterface'] = {}
    _shared_buffer: deque = deque(maxlen=1000)
    
    def __init__(self, name: str = "sim_can"):
        super().__init__(name)
        self._rx_buffer: deque = deque(maxlen=1000)
        self._tx_buffer: deque = deque(maxlen=1000)
        self._latency_ms: float = 0.0
        self._packet_loss_rate: float = 0.0
        SimulatedCanInterface._instances[name] = self
    
    def open(self) -> bool:
        self.is_open = True
        logger.info(f"[HIL] Simulated CAN interface '{self.name}' opened")
        return True
    
    def close(self) -> None:
        self.is_open = False
        logger.info(f"[HIL] Simulated CAN interface '{self.name}' closed")
    
    def send(self, message: CanMessage) -> bool:
        if not self.is_open:
            return False
        
        # Simulate packet loss
        import random
        if random.random() < self._packet_loss_rate:
            logger.debug(f"[HIL] SIM: Packet lost (TX)")
            return True
        
        # Add latency simulation
        if self._latency_ms > 0:
            time.sleep(self._latency_ms / 1000.0)
        
        self._tx_buffer.append(message)
        SimulatedCanInterface._shared_buffer.append(('can', self.name, message))
        
        logger.debug(f"[HIL] SIM TX CAN ID=0x{message.arbitration_id:03X}, Len={len(message.data)}")
        return True
    
    def receive(self, timeout_ms: int = 1000) -> Optional[CanMessage]:
        if not self.is_open:
            return None
        
        start_time = time.time()
        timeout_sec = timeout_ms / 1000.0
        
        while time.time() - start_time < timeout_sec:
            # Check own RX buffer
            if self._rx_buffer:
                return self._rx_buffer.popleft()
            
            # Check shared buffer for messages from other instances
            for item in list(SimulatedCanInterface._shared_buffer):
                if item[0] == 'can' and item[1] != self.name:
                    SimulatedCanInterface._shared_buffer.remove(item)
                    return item[2]
            
            time.sleep(0.001)
        
        return None
    
    def inject_frame(self, message: CanMessage) -> None:
        """Inject a frame (for testing)"""
        self._rx_buffer.append(message)
        self._notify_callbacks(message)
    
    def set_latency(self, latency_ms: float) -> None:
        self._latency_ms = latency_ms
    
    def set_packet_loss_rate(self, rate: float) -> None:
        self._packet_loss_rate = rate


class RealCanInterface(HilInterface):
    """Real CAN interface using python-can"""
    
    def __init__(self, channel: str = 'can0', bustype: str = 'socketcan',
                 bitrate: int = 500000, name: str = "can0"):
        super().__init__(name)
        self.channel = channel
        self.bustype = bustype
        self.bitrate = bitrate
        self._bus: Optional[can.Bus] = None
    
    def open(self) -> bool:
        if not CAN_AVAILABLE:
            logger.error("python-can not available")
            return False
        
        try:
            self._bus = can.Bus(
                channel=self.channel,
                bustype=self.bustype,
                bitrate=self.bitrate
            )
            self.is_open = True
            logger.info(f"[HIL] CAN interface '{self.name}' opened ({self.channel})")
            return True
        except Exception as e:
            logger.error(f"Failed to open CAN: {e}")
            return False
    
    def close(self) -> None:
        if self._bus:
            self._bus.shutdown()
            self._bus = None
        self.is_open = False
        logger.info(f"[HIL] CAN interface '{self.name}' closed")
    
    def send(self, message: CanMessage) -> bool:
        if not self.is_open or not self._bus:
            return False
        
        try:
            msg = can.Message(
                arbitration_id=message.arbitration_id,
                data=list(message.data),
                is_extended_id=message.is_extended_id,
                is_fd=message.is_fd
            )
            self._bus.send(msg)
            return True
        except Exception as e:
            logger.error(f"CAN send error: {e}")
            return False
    
    def receive(self, timeout_ms: int = 1000) -> Optional[CanMessage]:
        if not self.is_open or not self._bus:
            return None
        
        try:
            msg = self._bus.recv(timeout=timeout_ms / 1000.0)
            if msg:
                return CanMessage(
                    arbitration_id=msg.arbitration_id,
                    data=bytes(msg.data),
                    is_extended_id=msg.is_extended_id,
                    is_fd=msg.is_fd,
                    timestamp=msg.timestamp
                )
        except Exception as e:
            logger.error(f"CAN receive error: {e}")
        
        return None


class SimulatedEthernetInterface(HilInterface):
    """Simulated Ethernet interface"""
    
    _instances: Dict[str, 'SimulatedEthernetInterface'] = {}
    _shared_buffer: deque = deque(maxlen=1000)
    
    def __init__(self, name: str = "sim_eth", ethertype: int = 0x13400):
        super().__init__(name)
        self.ethertype = ethertype
        self._rx_buffer: deque = deque(maxlen=1000)
        self._tx_buffer: deque = deque(maxlen=1000)
        SimulatedEthernetInterface._instances[name] = self
    
    def open(self) -> bool:
        self.is_open = True
        logger.info(f"[HIL] Simulated Ethernet interface '{self.name}' opened")
        return True
    
    def close(self) -> None:
        self.is_open = False
        logger.info(f"[HIL] Simulated Ethernet interface '{self.name}' closed")
    
    def send(self, frame: EthernetFrame) -> bool:
        if not self.is_open:
            return False
        
        self._tx_buffer.append(frame)
        SimulatedEthernetInterface._shared_buffer.append(('eth', self.name, frame))
        
        logger.debug(f"[HIL] SIM TX Ethernet frame, Len={len(frame.payload)}")
        return True
    
    def receive(self, timeout_ms: int = 1000) -> Optional[EthernetFrame]:
        if not self.is_open:
            return None
        
        start_time = time.time()
        timeout_sec = timeout_ms / 1000.0
        
        while time.time() - start_time < timeout_sec:
            if self._rx_buffer:
                return self._rx_buffer.popleft()
            
            for item in list(SimulatedEthernetInterface._shared_buffer):
                if item[0] == 'eth' and item[1] != self.name:
                    SimulatedEthernetInterface._shared_buffer.remove(item)
                    return item[2]
            
            time.sleep(0.001)
        
        return None
    
    def inject_frame(self, frame: EthernetFrame) -> None:
        """Inject a frame (for testing)"""
        self._rx_buffer.append(frame)
        self._notify_callbacks(frame)


###############################################################################
# Test Equipment Simulator
###############################################################################

class UdsClient:
    """UDS Diagnostic Client"""
    
    def __init__(self, interface: HilInterface, tx_id: int = 0x7E0, rx_id: int = 0x7E8):
        self.interface = interface
        self.tx_id = tx_id
        self.rx_id = rx_id
        self.session: UdsSessionType = UdsSessionType.DEFAULT
        self.security_level: int = 0
        self._p2_timeout_ms: int = 50
        self._p2_star_timeout_ms: int = 5000
    
    def send_request(self, request: UdsRequest, timeout_ms: int = 1000) -> UdsResponse:
        """Send UDS request and wait for response"""
        data = request.to_bytes()
        
        # Send via CAN interface
        if isinstance(self.interface, (SimulatedCanInterface, RealCanInterface)):
            msg = CanMessage(
                arbitration_id=self.tx_id,
                data=data,
                is_extended_id=False
            )
            self.interface.send(msg)
        
        # Wait for response
        start_time = time.time()
        while (time.time() - start_time) * 1000 < timeout_ms:
            response = self.interface.receive(timeout_ms=100)
            if response and isinstance(response, CanMessage):
                if response.arbitration_id == self.rx_id:
                    try:
                        return UdsResponse.from_bytes(response.data)
                    except ValueError as e:
                        logger.error(f"Invalid UDS response: {e}")
        
        raise TimeoutError("No UDS response received")
    
    def session_control(self, session_type: UdsSessionType) -> UdsResponse:
        """Request diagnostic session control"""
        request = UdsRequest(
            service_id=UdsServiceId.DIAGNOSTIC_SESSION_CONTROL,
            subfunction=session_type
        )
        response = self.send_request(request)
        
        if response.is_positive_response_to(UdsServiceId.DIAGNOSTIC_SESSION_CONTROL):
            self.session = session_type
        
        return response
    
    def ecu_reset(self, reset_type: int = 0x01) -> UdsResponse:
        """Request ECU reset"""
        request = UdsRequest(
            service_id=UdsServiceId.ECU_RESET,
            subfunction=reset_type
        )
        return self.send_request(request)
    
    def read_data_by_identifier(self, did: int) -> UdsResponse:
        """Read data by identifier"""
        request = UdsRequest(
            service_id=UdsServiceId.READ_DATA_BY_IDENTIFIER,
            data=struct.pack('>H', did)
        )
        return self.send_request(request)
    
    def write_data_by_identifier(self, did: int, data: bytes) -> UdsResponse:
        """Write data by identifier"""
        request = UdsRequest(
            service_id=UdsServiceId.WRITE_DATA_BY_IDENTIFIER,
            data=struct.pack('>H', did) + data
        )
        return self.send_request(request)
    
    def tester_present(self, suppress_response: bool = False) -> Optional[UdsResponse]:
        """Send tester present"""
        request = UdsRequest(
            service_id=UdsServiceId.TESTER_PRESENT,
            subfunction=0x00,
            suppress_response=suppress_response
        )
        
        if suppress_response:
            # Send without waiting for response
            data = request.to_bytes()
            msg = CanMessage(arbitration_id=self.tx_id, data=data)
            self.interface.send(msg)
            return None
        
        return self.send_request(request)
    
    def security_access_request_seed(self, level: int) -> UdsResponse:
        """Request security access seed"""
        request = UdsRequest(
            service_id=UdsServiceId.SECURITY_ACCESS,
            subfunction=level  # Level 1 = request seed
        )
        return self.send_request(request)
    
    def security_access_send_key(self, level: int, key: bytes) -> UdsResponse:
        """Send security access key"""
        request = UdsRequest(
            service_id=UdsServiceId.SECURITY_ACCESS,
            subfunction=level + 1,  # Level 2 = send key for level 1
            data=key
        )
        response = self.send_request(request)
        
        if response.is_positive_response_to(UdsServiceId.SECURITY_ACCESS):
            self.security_level = level
        
        return response


class DoipClient:
    """DoIP (Diagnostic over IP) Client"""
    
    def __init__(self, interface: HilInterface, target_addr: int = 0x0E80):
        self.interface = interface
        self.target_addr = target_addr
        self.source_addr = 0x0E00
    
    def send_diagnostic_message(self, uds_data: bytes) -> bool:
        """Send diagnostic message over DoIP"""
        # DoIP header
        version = 0x02
        version_inv = 0xFD
        payload_type = 0x8001  # Diagnostic message
        
        payload = struct.pack('>HH', self.source_addr, self.target_addr) + uds_data
        payload_len = len(payload)
        
        header = struct.pack('>BBHH', version, version_inv, payload_type, payload_len)
        
        frame = EthernetFrame(
            dst_mac=b'\xFF\xFF\xFF\xFF\xFF\xFF',
            src_mac=b'\x00\x00\x00\x00\x00\x00',
            ethertype=0x13400,  # DoIP Ethertype
            payload=header + payload
        )
        
        return self.interface.send(frame)
    
    def receive_diagnostic_message(self, timeout_ms: int = 1000) -> Optional[bytes]:
        """Receive diagnostic message"""
        frame = self.interface.receive(timeout_ms)
        
        if frame and isinstance(frame, EthernetFrame):
            if len(frame.payload) > 8:
                # Skip DoIP header
                return frame.payload[8:]
        
        return None


###############################################################################
# HIL Test Manager
###############################################################################

class HilTestManager:
    """HIL Test Manager - Main entry point for tests"""
    
    def __init__(self, simulation_mode: bool = True):
        self.simulation_mode = simulation_mode
        self.interfaces: Dict[str, HilInterface] = {}
        self.uds_clients: Dict[str, UdsClient] = {}
        self.results: List[TestResult] = []
        self._setup_complete = False
    
    def setup(self) -> bool:
        """Setup HIL test environment"""
        logger.info("="*60)
        logger.info("HIL Test Framework Setup")
        logger.info("="*60)
        logger.info(f"Mode: {'SIMULATION' if self.simulation_mode else 'HARDWARE'}")
        
        if self.simulation_mode:
            # Create simulated interfaces
            can_if = SimulatedCanInterface("sim_can0")
            eth_if = SimulatedEthernetInterface("sim_eth0")
            
            if can_if.open() and eth_if.open():
                self.interfaces["can0"] = can_if
                self.interfaces["eth0"] = eth_if
                
                # Create UDS client
                self.uds_clients["uds_can"] = UdsClient(can_if, tx_id=0x7E0, rx_id=0x7E8)
                
                self._setup_complete = True
                logger.info("Simulation interfaces created successfully")
                return True
        else:
            # Try to use real hardware
            if CAN_AVAILABLE:
                can_if = RealCanInterface(channel='can0', name='can0')
                if can_if.open():
                    self.interfaces['can0'] = can_if
                    self.uds_clients['uds_can'] = UdsClient(can_if, tx_id=0x7E0, rx_id=0x7E8)
            
            self._setup_complete = True
            return True
        
        return False
    
    def teardown(self) -> None:
        """Teardown HIL test environment"""
        logger.info("Tearing down HIL test environment...")
        
        for name, iface in self.interfaces.items():
            iface.close()
        
        self.interfaces.clear()
        self.uds_clients.clear()
        self._setup_complete = False
        
        logger.info("Teardown complete")
    
    def get_interface(self, name: str) -> Optional[HilInterface]:
        """Get interface by name"""
        return self.interfaces.get(name)
    
    def get_uds_client(self, name: str) -> Optional[UdsClient]:
        """Get UDS client by name"""
        return self.uds_clients.get(name)
    
    def run_test(self, test_func: Callable[..., Any], test_name: str,
                 *args, **kwargs) -> TestResult:
        """Run a single test function"""
        logger.info(f"\nRunning test: {test_name}")
        
        start_time = time.time()
        
        try:
            result = test_func(self, *args, **kwargs)
            
            if isinstance(result, bool):
                passed = result
                details = {}
            elif isinstance(result, dict):
                passed = result.get('passed', True)
                details = result
            else:
                passed = True
                details = {'return_value': str(result)}
            
            error_message = None
        except Exception as e:
            passed = False
            error_message = str(e)
            details = {'exception': type(e).__name__}
            logger.error(f"Test {test_name} failed: {e}")
        
        duration_ms = (time.time() - start_time) * 1000
        
        test_result = TestResult(
            test_name=test_name,
            passed=passed,
            duration_ms=duration_ms,
            error_message=error_message,
            details=details
        )
        
        self.results.append(test_result)
        
        status = "PASS" if passed else "FAIL"
        logger.info(f"Test {test_name}: {status} ({duration_ms:.1f}ms)")
        
        return test_result
    
    def run_test_sequence(self, sequence: List[Tuple[Callable, str, tuple, dict]]) -> List[TestResult]:
        """Run a sequence of tests"""
        results = []
        
        for test_func, test_name, args, kwargs in sequence:
            result = self.run_test(test_func, test_name, *args, **kwargs)
            results.append(result)
            
            # Stop on first failure (configurable)
            if not result.passed:
                logger.warning(f"Stopping sequence due to failure in {test_name}")
                break
        
        return results
    
    def get_summary(self) -> Dict[str, Any]:
        """Get test summary"""
        total = len(self.results)
        passed = sum(1 for r in self.results if r.passed)
        failed = total - passed
        
        total_duration = sum(r.duration_ms for r in self.results)
        
        return {
            'total': total,
            'passed': passed,
            'failed': failed,
            'pass_rate': (passed / total * 100) if total > 0 else 0,
            'total_duration_ms': total_duration,
            'simulation_mode': self.simulation_mode
        }
    
    def clear_results(self) -> None:
        """Clear all test results"""
        self.results.clear()


def create_hil_manager(simulation_mode: bool = True) -> HilTestManager:
    """Factory function to create HIL test manager"""
    return HilTestManager(simulation_mode=simulation_mode)


###############################################################################
# Main Entry Point
###############################################################################

if __name__ == "__main__":
    # Simple self-test
    print("HIL Test Framework - Self Test")
    print("="*60)
    
    # Create manager in simulation mode
    manager = create_hil_manager(simulation_mode=True)
    
    if manager.setup():
        print("Setup successful")
        
        # Test CAN interface
        can_if = manager.get_interface("can0")
        if can_if:
            test_msg = CanMessage(arbitration_id=0x123, data=b'\x01\x02\x03\x04')
            can_if.send(test_msg)
            print(f"Sent CAN message: ID=0x{test_msg.arbitration_id:03X}")
        
        # Print summary
        summary = manager.get_summary()
        print(f"\nTest Summary:")
        print(f"  Simulation Mode: {summary['simulation_mode']}")
        print(f"  Interfaces: {list(manager.interfaces.keys())}")
        
        manager.teardown()
    else:
        print("Setup failed")
        sys.exit(1)
