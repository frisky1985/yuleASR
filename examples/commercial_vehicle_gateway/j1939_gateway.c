/*
 * @file j1939_gateway.c
 * @brief J1939到CAN转发实现
 */

#include "J1939Tp.h"
#include "PduR.h"
#include "Com.h"

/* J1939 PDU映射到CAN信号 */
typedef struct {
    uint32 j1939Pgn;
    uint16 canSignalId;
} GatewayMappingType;

static const GatewayMappingType gatewayMap[] = {
    {0xF004, 0},  /* EEC1 -> Engine Speed */
    {0xF003, 1},  /* EEC2 -> Accelerator Pedal */
    {0xFEEA, 2},  /* CCVS1 -> Vehicle Speed */
    /* 更多映射... */
};

void ProcessJ1939ToCanGateway(void) {
    /* 处理J1939接收 */
    /* 转发到CAN信号 */
}
