/**
 * @file stubs.c
 * @brief TcpIp / Det / missing SoAd API stubs for the SoAd unit test.
 *
 * The real SoAd.c drives TcpIp sockets (Create/Bind/Close/Send/Receive) and
 * calls Det_ReportError for dev-errors.  These stubs are deterministic and
 * configurable so the test can drive specific SoAd state transitions.
 */

#include "SoAd.h"
#include "SoAd_Cfg.h"
#include "TcpIp.h"
#include "Det.h"
#include "Std_Types.h"
#include "ComStack_Types.h"

#include <string.h>

/* ─── TcpIp stub state ─────────────────────────────────────────────────── */

#define SOAD_STUB_MAX_SOCKETS   (32U)

typedef struct {
    boolean         InUse;
    TcpIp_DomainType Domain;
    TcpIp_ProtocolType Protocol;
    TcpIp_SockAddrType LocalAddr;
} TcpIp_SlotType;

static TcpIp_SlotType g_tcpip_slots[SOAD_STUB_MAX_SOCKETS];
static uint8          g_tcpip_next_slot = 0U;

/* Per-socket return overrides — default is TCPIP_OK. */
static TcpIp_ReturnType g_create_override[SOAD_STUB_MAX_SOCKETS];
static TcpIp_ReturnType g_send_override [SOAD_STUB_MAX_SOCKETS];
static TcpIp_ReturnType g_recv_override [SOAD_STUB_MAX_SOCKETS];

/* Receive data ring — tests seed this so SoAd_Receive() reads a known payload. */
typedef struct {
    boolean HasData;
    uint8   Data[256];
    uint16  Len;
} TcpIp_RxSeedType;

static TcpIp_RxSeedType g_rx_seeds[SOAD_STUB_MAX_SOCKETS];

/* Send side — captures the last payload handed by SoAd. */
#define SOAD_STUB_MAX_CAPTURE (2048U)
typedef struct {
    boolean Captured;
    uint8   Data[SOAD_STUB_MAX_CAPTURE];
    uint16  Len;
} TcpIp_TxCaptureType;

static TcpIp_TxCaptureType g_tx_captures[SOAD_STUB_MAX_SOCKETS];

/* Call counters — exposed via extern for the test to assert on. */
uint32 stub_TcpIp_Create_Count  = 0U;
uint32 stub_TcpIp_Bind_Count    = 0U;
uint32 stub_TcpIp_Close_Count   = 0U;
uint32 stub_TcpIp_Send_Count    = 0U;
uint32 stub_TcpIp_Receive_Count = 0U;

/* Reset helpers — called from setUp(). */
extern uint32 stub_Det_ReportError_Count;
extern uint16 stub_Det_Last_ModuleId;
extern uint8  stub_Det_Last_InstanceId;
extern uint8  stub_Det_Last_ApiId;
extern uint8  stub_Det_Last_ErrorId;

void SoAdStub_Reset(void)
{
    memset(g_tcpip_slots, 0, sizeof(g_tcpip_slots));
    memset(g_create_override, 0, sizeof(g_create_override));
    memset(g_send_override,  0, sizeof(g_send_override));
    memset(g_recv_override,  0, sizeof(g_recv_override));
    memset(g_rx_seeds,      0, sizeof(g_rx_seeds));
    memset(g_tx_captures,   0, sizeof(g_tx_captures));
    g_tcpip_next_slot = 0U;

    stub_TcpIp_Create_Count  = 0U;
    stub_TcpIp_Bind_Count    = 0U;
    stub_TcpIp_Close_Count   = 0U;
    stub_TcpIp_Send_Count    = 0U;
    stub_TcpIp_Receive_Count = 0U;

    stub_Det_ReportError_Count = 0U;
    stub_Det_Last_ModuleId   = 0U;
    stub_Det_Last_InstanceId = 0U;
    stub_Det_Last_ApiId      = 0U;
    stub_Det_Last_ErrorId    = 0U;
}

void SoAdStub_SetCreateFail(TcpIp_SocketIdType socketId, TcpIp_ReturnType ret)
{
    g_create_override[socketId] = ret;
}

void SoAdStub_SetSendFail(TcpIp_SocketIdType socketId, TcpIp_ReturnType ret)
{
    g_send_override[socketId] = ret;
}

void SoAdStub_SetReceiveFail(TcpIp_SocketIdType socketId, TcpIp_ReturnType ret)
{
    g_recv_override[socketId] = ret;
}

void SoAdStub_SeedReceive(TcpIp_SocketIdType socketId, const uint8 *data, uint16 len)
{
    if (len > sizeof(g_rx_seeds[socketId].Data)) {
        len = (uint16)sizeof(g_rx_seeds[socketId].Data);
    }
    g_rx_seeds[socketId].HasData = TRUE;
    g_rx_seeds[socketId].Len     = len;
    if (data != NULL_PTR && len > 0U) {
        memcpy(g_rx_seeds[socketId].Data, data, len);
    }
}

const uint8 *SoAdStub_GetTxCapture(TcpIp_SocketIdType socketId, uint16 *lenOut)
{
    if (lenOut != NULL_PTR) {
        *lenOut = g_tx_captures[socketId].Len;
    }
    return g_tx_captures[socketId].Captured ? g_tx_captures[socketId].Data : NULL_PTR;
}

/* ─── TcpIp stub implementations ────────────────────────────────────────── */

TcpIp_ReturnType TcpIp_Create(TcpIp_DomainType Domain,
                              TcpIp_ProtocolType Protocol,
                              TcpIp_SocketIdType *SocketIdPtr)
{
    stub_TcpIp_Create_Count++;

    if (SocketIdPtr == NULL_PTR) {
        return TCPIP_E_NOT_OK;
    }

    /* Look for a free slot (wrap around to allow reuse after Close). */
    uint8 start = g_tcpip_next_slot;
    uint8 found = 0xFFU;
    uint8 i;
    for (i = 0U; i < SOAD_STUB_MAX_SOCKETS; i++) {
        uint8 idx = (uint8)((start + i) % SOAD_STUB_MAX_SOCKETS);
        if (!g_tcpip_slots[idx].InUse) {
            found = idx;
            break;
        }
    }

    if (found == 0xFFU) {
        return TCPIP_E_NOT_OK;
    }

    /* If a failure override is armed for this slot it trumps success. */
    if (g_create_override[found] != TCPIP_OK) {
        return g_create_override[found];
    }

    g_tcpip_slots[found].InUse   = TRUE;
    g_tcpip_slots[found].Domain  = Domain;
    g_tcpip_slots[found].Protocol = Protocol;
    g_tcpip_next_slot = (uint8)((found + 1U) % SOAD_STUB_MAX_SOCKETS);

    *SocketIdPtr = (TcpIp_SocketIdType)found;
    return TCPIP_OK;
}

TcpIp_ReturnType TcpIp_Bind(TcpIp_SocketIdType SocketId,
                            const TcpIp_SockAddrType *Addr)
{
    stub_TcpIp_Bind_Count++;
    if (SocketId < SOAD_STUB_MAX_SOCKETS && Addr != NULL_PTR) {
        g_tcpip_slots[SocketId].LocalAddr = *Addr;
    }
    return TCPIP_OK;
}

TcpIp_ReturnType TcpIp_Close(TcpIp_SocketIdType SocketId, boolean Abort)
{
    (void)Abort;
    stub_TcpIp_Close_Count++;
    if (SocketId < SOAD_STUB_MAX_SOCKETS) {
        g_tcpip_slots[SocketId].InUse = FALSE;
    }
    return TCPIP_OK;
}

TcpIp_ReturnType TcpIp_Send(TcpIp_SocketIdType SocketId,
                            const uint8 *DataPtr,
                            uint16 Length)
{
    stub_TcpIp_Send_Count++;

    if (SocketId < SOAD_STUB_MAX_SOCKETS) {
        if (g_send_override[SocketId] != TCPIP_OK) {
            return g_send_override[SocketId];
        }

        uint16 capLen = Length;
        if (capLen > SOAD_STUB_MAX_CAPTURE) {
            capLen = SOAD_STUB_MAX_CAPTURE;
        }
        g_tx_captures[SocketId].Len = capLen;
        if (DataPtr != NULL_PTR && capLen > 0U) {
            memcpy(g_tx_captures[SocketId].Data, DataPtr, capLen);
        }
        g_tx_captures[SocketId].Captured = TRUE;
    }
    return TCPIP_OK;
}

TcpIp_ReturnType TcpIp_Receive(TcpIp_SocketIdType SocketId,
                              uint8 *Buffer,
                              uint16 MaxLen,
                              uint16 *ReceivedLen)
{
    stub_TcpIp_Receive_Count++;

    if (ReceivedLen != NULL_PTR) {
        *ReceivedLen = 0U;
    }

    if (SocketId < SOAD_STUB_MAX_SOCKETS) {
        if (g_recv_override[SocketId] != TCPIP_OK) {
            return g_recv_override[SocketId];
        }

        if (g_rx_seeds[SocketId].HasData && Buffer != NULL_PTR && MaxLen > 0U) {
            uint16 copyLen = g_rx_seeds[SocketId].Len;
            if (copyLen > MaxLen) {
                copyLen = MaxLen;
            }
            memcpy(Buffer, g_rx_seeds[SocketId].Data, copyLen);
            if (ReceivedLen != NULL_PTR) {
                *ReceivedLen = copyLen;
            }
            g_rx_seeds[SocketId].HasData = FALSE;
            return TCPIP_OK;
        }
    }
    return TCPIP_E_NOT_OK;
}

/* ─── Det stub ─────────────────────────────────────────────────────────── */

uint32 stub_Det_ReportError_Count = 0U;
uint16 stub_Det_Last_ModuleId  = 0U;
uint8  stub_Det_Last_InstanceId = 0U;
uint8  stub_Det_Last_ApiId      = 0U;
uint8  stub_Det_Last_ErrorId    = 0U;

Std_ReturnType Det_ReportError(uint16 ModuleId,
                               uint8 InstanceId,
                               uint8 ApiId,
                               uint8 ErrorId)
{
    stub_Det_ReportError_Count++;
    stub_Det_Last_ModuleId   = ModuleId;
    stub_Det_Last_InstanceId = InstanceId;
    stub_Det_Last_ApiId      = ApiId;
    stub_Det_Last_ErrorId    = ErrorId;
    return E_OK;
}

/* Det_Init is also referenced in Det.h — provide a stub so tests that
 * link the real service_det lib alongside this TU don't conflict. */
void Det_Init(const Det_ConfigType *ConfigPtr) { (void)ConfigPtr; }
void Det_Start(void) { }

/* ─── SoAd config object referenced by SoAd.h extern ──────────────────── */

SoAd_ConnectionConfigType g_conn_cfgs[SOAD_NUMBER_OF_CONNECTIONS];
SoAd_SocketConfigType     g_sock_cfgs [SOAD_NUMBER_OF_SOCKETS];
SoAd_PduRouteConfigType  g_route_cfgs[SOAD_NUMBER_OF_PDU_ROUTES];

const SoAd_ConfigType SoAd_Config = {
    g_sock_cfgs,         SOAD_NUMBER_OF_SOCKETS,
    NULL_PTR,            0U,  /* ConnGrpConfigs */
    g_conn_cfgs,         SOAD_NUMBER_OF_CONNECTIONS,
    g_route_cfgs,        SOAD_NUMBER_OF_PDU_ROUTES,
    STD_ON,              /* DevErrorDetect  */
    STD_ON,              /* VersionInfoApi  */
    STD_ON               /* EnablePduHeader */
};

/* ─── SoAd_IfTransmit / SoAd_CloseConnection (declared in SoAd.h but not
 *     present in SoAd.c) — provide thin stubs so the header does not pull
 *     in undefined symbols for tests that #include SoAd.h directly. ───── */

Std_ReturnType SoAd_IfTransmit(PduIdType TxPduId, const PduInfoType *PduInfoPtr)
{
    (void)TxPduId;
    (void)PduInfoPtr;
    return E_OK;
}

Std_ReturnType SoAd_CloseConnection(SoAd_SoConIdType SoConId)
{
    return SoAd_CloseTcpConnection((uint16)SoConId, TRUE);
}

/* ─── MemMap.h sections are no-op macros on native builds (see
 *     include/autosar/Std_Types.h).  No additional definitions needed. ─── */
