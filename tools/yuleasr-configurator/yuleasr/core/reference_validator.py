"""
AUTOSAR BSW 跨模块引用关系定义

基于 AUTOSAR Classic Platform 规范，定义模块间的引用依赖关系。
用于跨模块配置一致性校验。
"""

from typing import Dict, List, Tuple

# 引用关系定义：(源模块, 源字段路径, 目标模块, 目标字段路径)
# 语义：源模块的源字段引用了目标模块的目标字段

CROSS_MODULE_REFERENCES: List[Tuple[str, str, str, str]] = [
    # CanIf → Can (CanIf 的控制器必须引用已启用的 Can 控制器)
    ("CanIf", "controllers", "Can", "general.num_controllers"),

    # CanIf → Can (CanIf 的 HRH/HHT 必须引用 Can 的 HardwareObject)
    ("CanIf", "hrh", "Can", "general.num_hardware_objects_rx"),
    ("CanIf", "hth", "Can", "general.num_hardware_objects_tx"),

    # CanTp → CanIf (CanTp 的 PDU ID 必须在 CanIf 中存在)
    ("CanTp", "canif_pdu_ids", "CanIf", "tx_pdus"),

    # PduR → Com (PduR 路由路径的源/目标 PDU 必须与 Com 的 IPDU 匹配)
    ("PduR", "routing_paths", "Com", "ipdus"),

    # PduR → CanIf (PduR 的下层路由必须引用 CanIf 的 PDU)
    ("PduR", "routing_paths", "CanIf", "tx_pdus"),
    ("PduR", "routing_paths", "CanIf", "rx_pdus"),

    # Com → PduR (Com 的 IPDU 必须通过 PduR 路由)
    ("Com", "ipdus", "PduR", "routing_paths"),

    # NvM → Fee (NvM block 必须引用 Fee 的 memory area)
    ("NvM", "blocks", "Fee", "memory_areas"),

    # NvM → MemIf (NvM 底层必须引用 MemIf 驱动)
    ("NvM", "general.mem_if_driver", "MemIf", "name"),

    # Fee → MemIf (Fee 必须引用 MemIf 驱动)
    ("Fee", "general.mem_if_driver", "MemIf", "name"),

    # Ea → MemIf (Ea 必须引用 MemIf 驱动)
    ("Ea", "general.mem_if_driver", "MemIf", "name"),

    # Dcm → PduR (Dcm 的 Rx/Tx PDU 必须在 PduR 中路由)
    ("Dcm", "rx_pdu_ids", "PduR", "routing_paths"),
    ("Dcm", "tx_pdu_ids", "PduR", "routing_paths"),

    # Dem → Dcm (Dem 的 DTC 可能与 Dcm 的诊断服务关联)
    ("Dem", "dtcs", "Dcm", "dtc_settings"),

    # SecOC → Com (SecOC 保护的 PDU 必须是 Com 的 IPDU)
    ("SecOC", "tx_pdu_ids", "Com", "ipdus"),
    ("SecOC", "rx_pdu_ids", "Com", "ipdus"),

    # SecOC → Csm (SecOC 的加密操作必须引用 Csm 的密钥)
    ("SecOC", "general.csm_key_id", "Csm", "keys"),

    # BswM → 所有模块 (BswM 的规则引用各模块的 mode)
    ("BswM", "rules", "EcuM", "modes"),
    ("BswM", "rules", "ComM", "modes"),
    ("BswM", "rules", "CanSM", "modes"),

    # EcuM → BswM (EcuM 的 state 转换可能触发 BswM 规则)
    ("EcuM", "state_transitions", "BswM", "rules"),

    # IoHwAb → MCAL (IoHwAb 的 channel 必须引用 MCAL 驱动)
    ("IoHwAb", "channels", "Dio", "channels"),
    ("IoHwAb", "channels", "Adc", "groups"),
    ("IoHwAb", "channels", "Pwm", "channels"),

    # EthIf → Eth (EthIf 必须引用 Eth 驱动)
    ("EthIf", "controllers", "Eth", "controllers"),

    # SoAd → EthIf (SoAd 必须引用 EthIf 控制器)
    ("SoAd", "socket_connections", "EthIf", "controllers"),

    # SomeIpSd → SoAd (SomeIpSd 必须引用 SoAd 的 socket)
    ("SomeIpSd", "services", "SoAd", "socket_connections"),

    # CanNm → CanIf (CanNm 必须引用 CanIf 的 PDU)
    ("CanNm", "general.canif_pdu_id", "CanIf", "tx_pdus"),

    # LinIf → Lin (LinIf 必须引用 Lin 驱动)
    ("LinIf", "channels", "Lin", "channels"),

    # FrIf → Fr (FrIf 必须引用 Fr 驱动)
    ("FrIf", "controllers", "Fr", "controllers"),

    # Spi → Mcu (Spi 的时钟配置必须与 Mcu 一致)
    ("Spi", "channels", "Mcu", "clock_settings"),

    # Gpt → Mcu (Gpt 的时钟源必须来自 Mcu)
    ("Gpt", "channels", "Mcu", "clock_settings"),

    # Adc → Mcu (Adc 的时钟源必须来自 Mcu)
    ("Adc", "groups", "Mcu", "clock_settings"),

    # Pwm → Mcu (Pwm 的时钟源必须来自 Mcu)
    ("Pwm", "channels", "Mcu", "clock_settings"),

    # Icu → Mcu (Icu 的时钟源必须来自 Mcu)
    ("Icu", "channels", "Mcu", "clock_settings"),

    # WdgM → WdgIf (WdgM 必须引用 WdgIf 驱动)
    ("WdgM", "supervisions", "WdgIf", "devices"),

    # Os → Mcu (Os 的定时器必须引用 Gpt/Mcu)
    ("Os", "counters", "Gpt", "channels"),
]


def get_reference_graph() -> Dict[str, List[Dict[str, str]]]:
    """
    构建引用图：模块 → [它引用的目标模块列表]

    Returns:
        {
            "CanIf": [{"target": "Can", "field": "controllers"}],
            "PduR": [{"target": "Com", "field": "routing_paths"}, ...],
            ...
        }
    """
    graph: Dict[str, List[Dict[str, str]]] = {}
    for src_module, src_field, tgt_module, tgt_field in CROSS_MODULE_REFERENCES:
        if src_module not in graph:
            graph[src_module] = []
        graph[src_module].append({
            "target": tgt_module,
            "src_field": src_field,
            "tgt_field": tgt_field,
        })
    return graph


def get_reverse_reference_graph() -> Dict[str, List[Dict[str, str]]]:
    """
    构建反向引用图：模块 → [引用它的源模块列表]

    用于检查"禁用某模块时，哪些模块会受影响"。

    Returns:
        {
            "Can": [{"source": "CanIf", "field": "controllers"}],
            "Com": [{"source": "PduR", "field": "routing_paths"}, ...],
            ...
        }
    """
    graph: Dict[str, List[Dict[str, str]]] = {}
    for src_module, src_field, tgt_module, tgt_field in CROSS_MODULE_REFERENCES:
        if tgt_module not in graph:
            graph[tgt_module] = []
        graph[tgt_module].append({
            "source": src_module,
            "src_field": src_field,
            "tgt_field": tgt_field,
        })
    return graph


# 预构建的图（模块加载时可用）
REFERENCE_GRAPH = get_reference_graph()
REVERSE_REFERENCE_GRAPH = get_reverse_reference_graph()
