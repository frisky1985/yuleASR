/**
 * @file eth_manager.h
 * @brief 最小桩 — 供 dds_eth_transport.c 单元测试使用 (tests/unit/ddstrans)
 *
 * dds_eth_transport.c 仅 include 该头但未使用任何 Eth_Manager API。
 * 真实 src/ethernet/eth_manager.h 会拖入 eth_mac_driver/eth_dma 驱动链，
 * host 单测无需；通过 include-dir 重定向使
 * "../../../ethernet/eth_manager.h" 命中本桩而非真实头。
 */

#ifndef DDS_TEST_ETH_MANAGER_STUB_H
#define DDS_TEST_ETH_MANAGER_STUB_H

/* 空桩: 被测代码未引用任何符号 */

#endif /* DDS_TEST_ETH_MANAGER_STUB_H */
