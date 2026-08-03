/**
 * @file rtps_wire.h
 * @brief RTPS 报文 wire 层 — 底层收发桥接
 * @version 1.0
 * @date 2026-08-04
 *
 * rtps_wire 位于 RTPS 状态机与传输层之间:
 * - send: 将 rtps_message_builder_t 组好的报文字节流交给传输层发送
 * - recv: 从传输层回调接收报文, 交给上层解析分发
 * 传输后端为 dds_eth_transport (dds_eth_send_rtps / 数据回调)。
 */
#ifndef RTPS_WIRE_H
#define RTPS_WIRE_H

#include <stdint.h>
#include <stdbool.h>
#include "../../common/types/eth_types.h"
#include "rtps_message.h"
#include "rtps_state.h"

#ifdef __cplusplus
extern "C" {
#endif

/** wire 层接收回调 (上层解析入口) */
typedef void (*rtps_wire_rx_cb_t)(const uint8_t *data, uint32_t len, void *user_data);

/** wire 层上下文 */
typedef struct rtps_wire_context {
    bool initialized;
    rtps_wire_rx_cb_t rx_callback;
    void *rx_user_data;
    /* 统计 */
    uint32_t tx_count;
    uint32_t rx_count;
    uint64_t tx_bytes;
    uint64_t rx_bytes;
} rtps_wire_context_t;

/**
 * @brief 初始化 wire 层
 * @param ctx wire 上下文
 * @param rx_callback 接收回调 (可为 NULL)
 * @param user_data 回调用户数据
 */
eth_status_t rtps_wire_init(rtps_wire_context_t *ctx,
                            rtps_wire_rx_cb_t rx_callback,
                            void *user_data);

/** 反初始化 */
void rtps_wire_deinit(rtps_wire_context_t *ctx);

/**
 * @brief 发送 RTPS 报文 (由 builder 生成)
 * @param ctx wire 上下文
 * @param data 报文数据
 * @param len 报文长度
 */
eth_status_t rtps_wire_send(rtps_wire_context_t *ctx,
                            const uint8_t *data,
                            uint32_t len);

/**
 * @brief 处理接收到的报文 (传输层回调入口)
 * @param ctx wire 上下文
 * @param data 报文数据
 * @param len 报文长度
 */
eth_status_t rtps_wire_handle_rx(rtps_wire_context_t *ctx,
                                 const uint8_t *data,
                                 uint32_t len);

/** 获取 wire 层统计 */
eth_status_t rtps_wire_get_stats(rtps_wire_context_t *ctx,
                                 uint32_t *tx_count,
                                 uint32_t *rx_count,
                                 uint64_t *tx_bytes,
                                 uint64_t *rx_bytes);

#ifdef __cplusplus
}
#endif

#endif /* RTPS_WIRE_H */
