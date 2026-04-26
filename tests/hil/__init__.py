# HIL Test Framework
# Hardware-in-the-Loop Testing Framework

__version__ = '1.0.0'
__author__ = 'HIL Test Framework Team'

from .hil_host import (
    HilTestManager,
    HilInterface,
    SimulatedCanInterface,
    RealCanInterface,
    SimulatedEthernetInterface,
    UdsClient,
    DoipClient,
    CanMessage,
    EthernetFrame,
    UdsRequest,
    UdsResponse,
    TestResult,
    create_hil_manager
)

__all__ = [
    'HilTestManager',
    'HilInterface',
    'SimulatedCanInterface',
    'RealCanInterface',
    'SimulatedEthernetInterface',
    'UdsClient',
    'DoipClient',
    'CanMessage',
    'EthernetFrame',
    'UdsRequest',
    'UdsResponse',
    'TestResult',
    'create_hil_manager'
]
