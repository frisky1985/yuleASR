/* test_com_coverage.c — Com module coverage driver (extended)
 * 
 * Exercises the real src/bsw/services/com/ implementation.  The host
 * build uses a reduced signal count (COM_NUM_OF_SIGNALS=8 via
 * coverage_run/asil/com_cfg_override.h) because the production config's
 * 256 signals combined with a uint8 loop counter in Com_Init creates an
 * unbounded loop on a native host build (see coverage report finding).
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

int main(void)
{
    static Com_ConfigType cfg;
    Std_VersionInfoType ver;
    PduInfoType pdu;
    uint8 buf[32];

    printf("=== Com Coverage Driver ===\n");

    memset(&cfg, 0, sizeof(cfg));
    memset(&pdu, 0, sizeof(pdu));
    memset(buf, 0, sizeof(buf));
    pdu.SduDataPtr = buf;
    pdu.SduLength = sizeof(buf);

    /* Lifecycle */
    Com_Init(&cfg);
    Com_Init(NULL);
    Com_GetVersionInfo(&ver);
    Com_GetVersionInfo(NULL);

    /* Signal I/O paths (empty config -> guarded early paths) */
    (void)Com_SendSignal(0U, buf);
    (void)Com_SendSignal(0U, NULL);
    (void)Com_ReceiveSignal(0U, buf);
    (void)Com_ReceiveSignal(0U, NULL);

    /* IPDU group control / DM control */
    Com_IpduGroupControl(0U, TRUE);
    Com_IpduGroupControl(0U, FALSE);
    Com_ReceptionDMControl(0U, TRUE);
    Com_EnableReceptionDM(0U);
    Com_DisableReceptionDM(0U);

    /* PDU / transmission paths */
    (void)Com_TriggerIPDUSend(0U);
    (void)Com_TriggerTransmit(0U, &pdu);
    (void)Com_TriggerTransmit(0U, NULL);
    Com_RxIndication(0U, &pdu);
    Com_RxIndication(0U, NULL);
    Com_TxConfirmation(0U, E_OK);
    Com_TxConfirmation(0U, E_NOT_OK);
    Com_SwitchIpduTxMode(0U, 0U);
    Com_SwitchIpduTxMode(0U, 1U);

    /* Cyclic functions */
    Com_MainFunctionRx();
    Com_MainFunctionTx();
    Com_MainFunctionRouteSignals();

    /* Deinit */
    Com_DeInit();

    printf("\nResult: %d/%d checks passed\n", t_pass, t_pass + t_fail);
    return (t_fail == 0) ? 0 : 1;
}
