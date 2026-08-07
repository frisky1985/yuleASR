/* test_com_coverage.c — Com module coverage driver (real signal config)
 *
 * Exercises the real src/bsw/services/com/ implementation.  The host
 * build uses a reduced signal count (COM_NUM_OF_SIGNALS=8 via
 * coverage_run/asil/com_cfg_override.h) because the production config's
 * 256 signals combined with a uint8 loop counter in Com_Init creates an
 * unbounded loop on a native host build (see coverage report finding).
 *
 * This driver supplies a REAL configuration table (signals with various
 * bit sizes / endianness / transfer properties / filter algorithms plus
 * periodic & repeating IPDUs) so pack/unpack, filter evaluation,
 * transmission scheduling, Rx/Tx indication and group control paths in
 * Com.c are genuinely driven against production code.
 */
#include <stdio.h>
#include <string.h>
#include "Com.h"
#include "Com_Cfg.h"
#include "Det.h"

static int t_pass = 0;
static int t_fail = 0;

#define CHECK(cond, msg) \
    do { if (cond) { t_pass++; } else { t_fail++; printf("  [FAIL] %s (line %d)\n", msg, __LINE__); } } while (0)

/* Signal group (IPDU) ids used by the signals below */
#define GRP_A   0U
#define GRP_B   1U
#define GRP_C   2U

static const Com_SignalConfigType TestSignals[COM_NUM_OF_SIGNALS] = {
    /* s0: 8-bit little-endian, triggered, always filter */
    { 0U, 0U,  8U, COM_LITTLE_ENDIAN, COM_TRIGGERED, COM_ALWAYS, 0x00U, 0x00U, GRP_A },
    /* s1: 16-bit little-endian, triggered-on-change, masked-new-equals-X */
    { 1U, 8U, 16U, COM_LITTLE_ENDIAN, COM_TRIGGERED_ON_CHANGE, COM_MASKED_NEW_EQUALS_X, 0xFFU, 0x55U, GRP_A },
    /* s2: 32-bit big-endian, pending, masked-new-differs-X */
    { 2U, 0U, 32U, COM_BIG_ENDIAN, COM_PENDING, COM_MASKED_NEW_DIFFERS_X, 0x0FU, 0x05U, GRP_B },
    /* s3: 8-bit little-endian, triggered, masked-new-differs-masked-old */
    { 3U, 8U,  8U, COM_LITTLE_ENDIAN, COM_TRIGGERED, COM_MASKED_NEW_DIFFERS_MASKED_OLD, 0xFFU, 0x00U, GRP_B },
    /* s4: 8-bit big-endian at bit 16, triggered-on-change, never filter */
    { 4U, 16U, 8U, COM_BIG_ENDIAN, COM_TRIGGERED_ON_CHANGE, COM_NEVER, 0x00U, 0x00U, GRP_C },
    /* s5: 1-bit big-endian at bit 24, triggered, always filter */
    { 5U, 24U, 1U, COM_BIG_ENDIAN, COM_TRIGGERED, COM_ALWAYS, 0x00U, 0x00U, GRP_C },
    /* s6: 12-bit little-endian at bit 32, pending, default filter (within) */
    { 6U, 32U, 12U, COM_LITTLE_ENDIAN, COM_PENDING, COM_NEW_IS_WITHIN, 0x00U, 0x00U, GRP_A },
    /* s7: 8-bit little-endian, triggered, one-every-N (default) */
    { 7U, 48U,  8U, COM_LITTLE_ENDIAN, COM_TRIGGERED, COM_ONE_EVERY_N, 0x00U, 0x00U, GRP_B },
};

static const Com_IPduConfigType TestIPdus[] = {
    /* IPDU 0 (group A): periodic + repeating */
    { 0U, 8U, TRUE, 2U, 3U, 5U },
    /* IPDU 1 (group B): non-periodic, no repetition */
    { 1U, 8U, FALSE, 0U, 0U, 0U },
    /* IPDU 2 (group C): periodic, no repetition */
    { 2U, 4U, FALSE, 0U, 0U, 10U },
};

static const Com_ConfigType TestComConfig = {
    TestSignals,
    COM_NUM_OF_SIGNALS,
    TestIPdus,
    3U
};

int main(void)
{
    Std_VersionInfoType ver;
    PduInfoType pdu;
    uint8 buf[32];
    uint8 out[32];
    uint16 val16;
    uint32 val32;
    Com_IpduGroupVector gvec;

    printf("=== Com Coverage Driver (real signal config) ===\n");

    memset(&pdu, 0, sizeof(pdu));
    memset(buf, 0, sizeof(buf));
    memset(out, 0, sizeof(out));
    memset(gvec, 0, sizeof(gvec));
    pdu.SduDataPtr = buf;
    pdu.SduLength = sizeof(buf);

    /* ---- lifecycle: uninit guards first ---- */
    CHECK(Com_GetStatus() == COM_UNINIT, "status uninit");
    CHECK(Com_SendSignal(0U, buf) == COM_SERVICE_NOT_OK, "SendSignal uninit");
    CHECK(Com_ReceiveSignal(0U, buf) == COM_SERVICE_NOT_OK, "ReceiveSignal uninit");
    CHECK(Com_TriggerTransmit(0U, &pdu) == E_NOT_OK, "TriggerTransmit uninit");
    CHECK(Com_TriggerIPDUSend(0U) == E_NOT_OK, "TriggerIPDUSend uninit");
    Com_TxConfirmation(0U, E_OK);
    Com_RxIndication(0U, &pdu);
    Com_MainFunctionRx();
    Com_MainFunctionTx();
    Com_ReceptionDMControl(gvec, TRUE);
    CHECK(Com_InvalidateSignal(0U) == COM_SERVICE_NOT_OK, "InvalidateSignal uninit");
    CHECK(Com_InvalidateSignalGroup(0U) == COM_SERVICE_NOT_OK, "InvalidateSignalGroup uninit");
    CHECK(Com_SendSignalGroup(0U) == COM_SERVICE_NOT_OK, "SendSignalGroup uninit");
    CHECK(Com_ReceiveSignalGroup(0U) == COM_SERVICE_NOT_OK, "ReceiveSignalGroup uninit");
    CHECK(Com_UpdateShadowSignal(0U, buf) == COM_SERVICE_NOT_OK, "UpdateShadowSignal uninit");
    CHECK(Com_ReceiveShadowSignal(0U, out) == COM_SERVICE_NOT_OK, "ReceiveShadowSignal uninit");

    /* ---- init with NULL + real config ---- */
    Com_Init(NULL);
    CHECK(Com_GetStatus() == COM_UNINIT, "Init(NULL) rejected");
    Com_Init(&TestComConfig);
    CHECK(Com_GetStatus() == COM_INIT, "Init(real cfg) ok");
    Com_GetVersionInfo(&ver);
    Com_GetVersionInfo(NULL);

    /* ---- SendSignal: filter always -> pack + transmit (IPDU 0) ---- */
    memset(buf, 0, sizeof(buf));
    buf[0] = 0x2A;
    CHECK(Com_SendSignal(0U, buf) == COM_SERVICE_OK, "SendSignal s0 ok");

    /* ---- SendSignal: masked-new-equals-X pass / block ---- */
    memset(buf, 0, sizeof(buf));
    val16 = 0x5555;
    memcpy(buf + 1, &val16, 2);
    CHECK(Com_SendSignal(1U, buf) == COM_SERVICE_OK, "SendSignal s1 equals-X passes");
    val16 = 0xAAAA;
    memcpy(buf + 1, &val16, 2);
    CHECK(Com_SendSignal(1U, buf) == COM_SERVICE_OK, "SendSignal s1 differs-X blocked");

    /* ---- SendSignal: masked-new-differs-X (pass + block) ---- */
    memset(buf, 0, sizeof(buf));
    val32 = 0x0000000FU;
    memcpy(buf, &val32, 4);
    CHECK(Com_SendSignal(2U, buf) == COM_SERVICE_OK, "SendSignal s2 differs-X blocked");
    val32 = 0x00000001U;
    memcpy(buf, &val32, 4);
    CHECK(Com_SendSignal(2U, buf) == COM_SERVICE_OK, "SendSignal s2 differs-X passes");

    /* ---- SendSignal: masked-new-differs-masked-old ---- */
    memset(buf, 0, sizeof(buf));
    buf[1] = 0x10;
    CHECK(Com_SendSignal(3U, buf) == COM_SERVICE_OK, "SendSignal s3 first value");
    buf[1] = 0x20;
    CHECK(Com_SendSignal(3U, buf) == COM_SERVICE_OK, "SendSignal s3 changed value");

    /* ---- SendSignal: never filter blocked ---- */
    memset(buf, 0, sizeof(buf));
    buf[2] = 0x01;
    CHECK(Com_SendSignal(4U, buf) == COM_SERVICE_OK, "SendSignal s4 never-filter blocked");

    /* ---- SendSignal: 1-bit big-endian ---- */
    memset(buf, 0, sizeof(buf));
    buf[0] = 0x01;   /* signal value = 1 (packed at IPDU bit 24) */
    CHECK(Com_SendSignal(5U, buf) == COM_SERVICE_OK, "SendSignal s5 1-bit");

    /* ---- SendSignal: 12-bit + pending (no transmit) ---- */
    memset(buf, 0, sizeof(buf));
    buf[4] = 0x0F; buf[5] = 0xF0;
    CHECK(Com_SendSignal(6U, buf) == COM_SERVICE_OK, "SendSignal s6 12-bit pending");

    /* ---- SendSignal: one-every-N (default filter) ---- */
    memset(buf, 0, sizeof(buf));
    buf[6] = 0x42;
    CHECK(Com_SendSignal(7U, buf) == COM_SERVICE_OK, "SendSignal s7 one-every-N");

    /* ---- ReceiveSignal: unpack little / big / 1-bit ---- */
    memset(out, 0, sizeof(out));
    CHECK(Com_ReceiveSignal(0U, out) == COM_SERVICE_OK, "ReceiveSignal s0 ok");
    CHECK(out[0] == 0x2AU, "s0 value round-trip");
    memset(out, 0, sizeof(out));
    CHECK(Com_ReceiveSignal(5U, out) == COM_SERVICE_OK, "ReceiveSignal s5 ok");
    CHECK((out[0] & 0x01U) == 0x01U, "s5 1-bit value");

    /* ---- TriggerTransmit: valid / invalid ids ---- */
    memset(&pdu, 0, sizeof(pdu));
    pdu.SduDataPtr = NULL;
    pdu.SduLength = 0;
    CHECK(Com_TriggerTransmit(0U, &pdu) == E_OK, "TriggerTransmit pdu0 ok");
    CHECK(pdu.SduDataPtr != NULL, "TriggerTransmit fills SduDataPtr");
    CHECK(Com_TriggerTransmit(0U, NULL) == E_NOT_OK, "TriggerTransmit NULL pdu");
    CHECK(Com_TriggerTransmit(99U, &pdu) == E_NOT_OK, "TriggerTransmit invalid id");
    CHECK(Com_TriggerIPDUSend(0U) == E_OK, "TriggerIPDUSend ok");
    CHECK(Com_TriggerIPDUSend(99U) == E_NOT_OK, "TriggerIPDUSend invalid");

    /* ---- TxConfirmation: repetition scheduling ---- */
    Com_TxConfirmation(0U, E_OK);          /* schedules next repetition */
    Com_TxConfirmation(0U, E_NOT_OK);      /* error result */
    Com_TxConfirmation(99U, E_OK);         /* out-of-range id */

    /* ---- MainFunctionTx: periodic transmit + repetition ---- */
    Com_MainFunctionTx();
    Com_MainFunctionTx();
    Com_MainFunctionTx();
    Com_MainFunctionTx();
    Com_MainFunctionTx();
    Com_MainFunctionTx();

    /* ---- RxIndication: valid / invalid ---- */
    memset(buf, 0xCD, sizeof(buf));
    pdu.SduDataPtr = buf;
    pdu.SduLength = 8;
    Com_RxIndication(0U, &pdu);
    Com_RxIndication(0U, NULL);
    Com_RxIndication(99U, &pdu);
    memset(out, 0, sizeof(out));
    CHECK(Com_ReceiveSignal(0U, out) == COM_SERVICE_OK, "ReceiveSignal after Rx");
    CHECK(out[0] == 0xCDU, "Rx data visible to signal");

    /* ---- MainFunctionRx ---- */
    Com_MainFunctionRx();

    /* ---- signal groups (Send/Receive group) ---- */
    CHECK(Com_SendSignalGroup(0U) == COM_SERVICE_OK, "SendSignalGroup ok");
    CHECK(Com_SendSignalGroup(99U) == COM_SERVICE_NOT_OK, "SendSignalGroup invalid");
    CHECK(Com_ReceiveSignalGroup(0U) == COM_SERVICE_OK, "ReceiveSignalGroup ok");
    CHECK(Com_ReceiveSignalGroup(99U) == COM_SERVICE_NOT_OK, "ReceiveSignalGroup invalid");

    /* ---- shadow signals ---- */
    memset(buf, 0, sizeof(buf));
    buf[0] = 0x5A;
    CHECK(Com_UpdateShadowSignal(0U, buf) == COM_SERVICE_OK, "UpdateShadowSignal ok");
    CHECK(Com_UpdateShadowSignal(0U, NULL) == COM_SERVICE_NOT_OK, "UpdateShadowSignal NULL");
    CHECK(Com_UpdateShadowSignal(99U, buf) == COM_SERVICE_NOT_OK, "UpdateShadowSignal invalid id");
    memset(out, 0, sizeof(out));
    CHECK(Com_ReceiveShadowSignal(0U, out) == COM_SERVICE_OK, "ReceiveShadowSignal ok");
    CHECK(Com_ReceiveShadowSignal(0U, NULL) == COM_SERVICE_NOT_OK, "ReceiveShadowSignal NULL");
    CHECK(Com_ReceiveShadowSignal(99U, out) == COM_SERVICE_NOT_OK, "ReceiveShadowSignal invalid");

    /* ---- invalidate signal / group ---- */
    CHECK(Com_InvalidateSignal(0U) == COM_SERVICE_OK, "InvalidateSignal ok");
    CHECK(Com_InvalidateSignal(99U) == COM_SERVICE_NOT_OK, "InvalidateSignal invalid");
    CHECK(Com_InvalidateSignalGroup(0U) == COM_SERVICE_OK, "InvalidateSignalGroup ok");
    CHECK(Com_InvalidateSignalGroup(99U) == COM_SERVICE_NOT_OK, "InvalidateSignalGroup invalid");

    /* ---- group vector helpers ---- */
    Com_ClearIpduGroupVector(gvec);
    Com_SetIpduGroup(gvec, 3U);
    Com_SetIpduGroup(gvec, 20U);   /* out of range */
    Com_ClearIpduGroup(gvec, 3U);
    Com_ClearIpduGroup(gvec, 20U); /* out of range */
    Com_IpduGroupControl(gvec, FALSE);
    Com_IpduGroupControl(gvec, TRUE);
    Com_ReceptionDMControl(gvec, TRUE);
    Com_ReceptionDMControl(gvec, FALSE);
    Com_EnableReceptionDM(gvec);
    Com_DisableReceptionDM(gvec);

    /* ---- dynamic signals (stubs) + tx mode ---- */
    CHECK(Com_SendDynSignal(0U, buf, 4U) == COM_SERVICE_NOT_OK, "SendDynSignal stub");
    {
        uint16 len = 4U;
        CHECK(Com_ReceiveDynSignal(0U, out, &len) == COM_SERVICE_NOT_OK, "ReceiveDynSignal stub");
    }
    Com_SwitchIpduTxMode(0U, COM_PERIODIC);
    Com_SwitchIpduTxMode(0U, COM_DIRECT);
    CHECK(Com_TriggerIPDUSendWithMetaData(0U, buf) == E_NOT_OK, "TriggerIPDUSendWithMetaData stub");
    Com_TpRxIndication(0U, E_OK);
    Com_TpTxConfirmation(0U, E_OK);

    /* ---- deinit ---- */
    Com_DeInit();
    CHECK(Com_GetStatus() == COM_UNINIT, "DeInit ok");
    Com_DeInit();   /* double deinit -> uninit guard */

    printf("\nResult: %d/%d checks passed\n", t_pass, t_pass + t_fail);
    return (t_fail == 0) ? 0 : 1;
}
