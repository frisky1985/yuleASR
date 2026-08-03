/**
 * @file rtps_wire.c
 * @brief RTPS 报文 wire 层实现
 * @version 1.0
 * @date 2026-08-04
 */
#include "rtps_wire.h"
#include <string.h>

/* ============================================================================
 * 实现
 * ============================================================================ */

eth_status_t rtps_wire_init(rtps_wire_context_t *ctx,
                            rtps_wire_rx_cb_t rx_callback,
                            void *user_data)
{
    if (ctx == NULL) {
        return ETH_INVALID_PARAM;
    }
    memset(ctx, 0, sizeof(*ctx));
    ctx->initialized = true;
    ctx->rx_callback = rx_callback;
    ctx->rx_user_data = user_data;
    return ETH_OK;
}

void rtps_wire_deinit(rtps_wire_context_t *ctx)
{
    if (ctx == NULL) {
        return;
    }
    ctx->initialized = false;
    ctx->rx_callback = NULL;
    ctx->rx_user_data = NULL;
}

eth_status_t rtps_wire_send(rtps_wire_context_t *ctx,
                            const uint8_t *data,
                            uint32_t len)
{
    if (ctx == NULL || !ctx->initialized || data == NULL || len == 0) {
        return ETH_INVALID_PARAM;
    }

    /* 上层通过 dds_eth_transport 发送 — 此处仅记录统计。
     * 实际发送由 runtime 层在发送回调中调用 dds_eth_send_rtps。
     * 本函数作为 wire 层入口保留统计与校验。 */
    ctx->tx_count++;
    ctx->tx_bytes += len;
    return ETH_OK;
}

eth_status_t rtps_wire_handle_rx(rtps_wire_context_t *ctx,
                                 const uint8_t *data,
                                 uint32_t len)
{
    if (ctx == NULL || !ctx->initialized || data == NULL || len == 0) {
        return ETH_INVALID_PARAM;
    }

    ctx->rx_count++;
    ctx->rx_bytes += len;

    /* 转发给上层解析回调 */
    if (ctx->rx_callback != NULL) {
        ctx->rx_callback(data, len, ctx->rx_user_data);
    }
    return ETH_OK;
}

eth_status_t rtps_wire_get_stats(rtps_wire_context_t *ctx,
                                 uint32_t *tx_count,
                                 uint32_t *rx_count,
                                 uint64_t *tx_bytes,
                                 uint64_t *rx_bytes)
{
    if (ctx == NULL) {
        return ETH_INVALID_PARAM;
    }
    if (tx_count != NULL) {
        *tx_count = ctx->tx_count;
    }
    if (rx_count != NULL) {
        *rx_count = ctx->rx_count;
    }
    if (tx_bytes != NULL) {
        *tx_bytes = ctx->tx_bytes;
    }
    if (rx_bytes != NULL) {
        *rx_bytes = ctx->rx_bytes;
    }
    return ETH_OK;
}
