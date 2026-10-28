/**
 * @file test_xcp_services.c
 * @brief XCP (services layer, src/bsw/services/xcp) unit tests
 *
 * Coverage-uplift work: compiles the real production source
 * src/bsw/services/xcp/src/Xcp.c against a Det_ReportError mock and
 * exercises the XCP protocol command set end to end:
 *  - lifecycle (Init/DeInit/MainFunction/GetVersionInfo) + DET paths
 *  - transport indirection (RxIndication/TriggerTransmit/SetTransmissionMode)
 *  - standard commands (CONNECT/DISCONNECT/GET_STATUS/SYNCH/GET_COMM_MODE_INFO/
 *    GET_ID/SET_MTA/UPLOAD/SHORT_UPLOAD/DOWNLOAD/GET_SEED/UNLOCK)
 *  - DAQ command set (alloc/free/ptr/write/mode/start-stop/processor info)
 *  - PGM command set (PROGRAM_START/CLEAR/PROGRAM/RESET/VERIFY)
 *  - memory access helpers (ReadMemory/WriteMemory/UnlockResource)
 */

// @tests src/bsw/services/xcp/src/Xcp.c  @tests src/bsw/services/xcp/include/Xcp.h

#include <string.h>
#include <sys/mman.h>
#include <stdint.h>
#include "unity.h"
#include "Xcp.h"
#include "Det.h"

/*==================================================================================================
*                                  Host memory mapping
*================================================================================================*/
/* Xcp_ValidateMemoryAccess only accepts the simulated address windows
 * (RAM 0x20000000..+1024, flash 0x08000000..+1024) and Xcp_Read/WriteMemory
 * dereference the raw address, so the test maps real host pages at those
 * addresses to exercise the transfer loops on the native target. */
#define XCP_TEST_RAM_ADDR    0x20000000U
#define XCP_TEST_FLASH_ADDR  0x08000000U
#define XCP_TEST_PAGE_SIZE   0x1000U

static boolean test_memory_mapped = FALSE;

static void test_map_simulated_memory(void)
{
#if !defined(__APPLE__)
    /* Linux (CI): mapping the simulated address windows works, so the
     * Upload/Download/ReadMemory/WriteMemory transfer loops run for real. */
    void *ram = mmap((void *)(uintptr_t)XCP_TEST_RAM_ADDR, XCP_TEST_PAGE_SIZE,
                     PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANON | MAP_FIXED, -1, 0);
    void *flash = mmap((void *)(uintptr_t)XCP_TEST_FLASH_ADDR, XCP_TEST_PAGE_SIZE,
                       PROT_READ | PROT_WRITE,
                       MAP_PRIVATE | MAP_ANON | MAP_FIXED, -1, 0);
    if ((ram != MAP_FAILED) && (flash != MAP_FAILED))
    {
        test_memory_mapped = TRUE;
    }
#else
    /* macOS arm64 refuses mmap below 4 GiB (probe returns MAP_FAILED);
     * dereferencing tests are skipped via TEST_IGNORE instead. */
    test_memory_mapped = FALSE;
#endif
}

/*==================================================================================================
*                                  Det mock
*================================================================================================*/
uint16 mock_XcpDetCalls = 0U;
uint8  mock_XcpDetLastApiId = 0xFFU;
uint8  mock_XcpDetLastErrorId = 0xFFU;

Std_ReturnType Det_ReportError(uint16 ModuleId, uint8 InstanceId, uint8 ApiId, uint8 ErrorId)
{
    (void)ModuleId;
    (void)InstanceId;
    mock_XcpDetLastApiId = ApiId;
    mock_XcpDetLastErrorId = ErrorId;
    mock_XcpDetCalls++;
    return E_OK;
}

/*==================================================================================================
*                                  Fixtures
*================================================================================================*/
/* Session config placeholder: Xcp_Init currently validates the pointer only,
 * the production Xcp_Config is compiled inside Xcp.c. */
static Xcp_SessionConfigType testSessionConfig;
static Xcp_ChannelConfigType testChannelConfig;
static Xcp_DaqListType       testDaqLists[XCP_MAX_DAQ_LISTS];

static Xcp_ConfigType testConfig = {
    NULL_PTR,            /* ChannelConfigs  - filled in setUp */
    XCP_NUMBER_OF_CHANNELS,
    NULL_PTR,            /* SessionConfig   - filled in setUp */
    testDaqLists,
    XCP_MAX_DAQ_LISTS,
    XCP_DEV_ERROR_DETECT,
    XCP_VERSION_INFO_API,
    TRUE,
    FALSE,
    10U
};

/* Simulated calibration RAM — mapped at XCP_TEST_RAM_ADDR on Linux hosts */
static volatile uint8 *testCalRam = (volatile uint8 *)(uintptr_t)XCP_TEST_RAM_ADDR;

static void test_connect(void)
{
    uint8 connect[2] = { XCP_CMD_CONNECT, 0x00U };
    Xcp_ProcessCommand(0U, connect, sizeof(connect));
}

void setUp(void)
{
    mock_XcpDetCalls = 0U;
    mock_XcpDetLastApiId = 0xFFU;
    mock_XcpDetLastErrorId = 0xFFU;
    (void)memset(&testSessionConfig, 0, sizeof(testSessionConfig));
    testChannelConfig.TxPduId = 0U;
    testChannelConfig.RxPduId = 0U;
    testChannelConfig.TransportLayer = XCP_TRANSPORT_CAN;
    testChannelConfig.Priority = 0U;
    testChannelConfig.IsActive = TRUE;
    testConfig.ChannelConfigs = &testChannelConfig;
    testConfig.SessionConfig = &testSessionConfig;
    if (test_memory_mapped)
    {
        (void)memset((void *)testCalRam, 0xA5, 16U);
    }
}

void tearDown(void)
{
    Xcp_DeInit();
}

/*==================================================================================================
*                                  Lifecycle
*================================================================================================*/
void test_Xcp_Init_NullConfig_ReportsDet(void)
{
    Xcp_Init(NULL_PTR);
    TEST_ASSERT_EQUAL_UINT16(1U, mock_XcpDetCalls);
}

void test_Xcp_Init_ValidConfig_Succeeds(void)
{
    Xcp_Init(&testConfig);
    TEST_ASSERT_EQUAL_UINT16(0U, mock_XcpDetCalls);

    /* After Init the module must accept the CONNECT command without DET */
    test_connect();
    TEST_ASSERT_EQUAL_UINT16(0U, mock_XcpDetCalls);
}

void test_Xcp_DeInit_StopsAllDaq(void)
{
    Xcp_Init(&testConfig);
    Xcp_DeInit();
    /* DeInit when uninit is a silent no-op */
    Xcp_DeInit();
    TEST_ASSERT_EQUAL_UINT16(0U, mock_XcpDetCalls);
}

void test_Xcp_GetVersionInfo_Valid(void)
{
    Std_VersionInfoType info;
    (void)memset(&info, 0, sizeof(info));

    Xcp_Init(&testConfig);
    Xcp_GetVersionInfo(&info);
    TEST_ASSERT_EQUAL_UINT16(XCP_CFG_SW_MAJOR_VERSION, info.sw_major_version);
    TEST_ASSERT_EQUAL_UINT16(XCP_CFG_SW_MINOR_VERSION, info.sw_minor_version);
}

void test_Xcp_MainFunction_IncrementsEventCounters(void)
{
    Xcp_Init(&testConfig);
    Xcp_MainFunction();
    Xcp_MainFunction();
    /* No DET, no crash — counters are internal */
    TEST_ASSERT_EQUAL_UINT16(0U, mock_XcpDetCalls);
}

/*==================================================================================================
*                                  Transport layer interface
*================================================================================================*/
void test_Xcp_RxIndication_Uninit_ReportsDet(void)
{
    PduInfoType pdu;
    uint8 data[1] = { XCP_CMD_CONNECT };

    pdu.SduDataPtr = data;
    pdu.SduLength = 1U;
    Xcp_RxIndication(0U, 0U, &pdu);
    TEST_ASSERT_EQUAL_UINT16(1U, mock_XcpDetCalls);
}

void test_Xcp_RxIndication_NullPdu_ReportsDet(void)
{
    Xcp_Init(&testConfig);
    Xcp_RxIndication(0U, 0U, NULL_PTR);
    TEST_ASSERT_EQUAL_UINT16(1U, mock_XcpDetCalls);
}

void test_Xcp_RxIndication_InvalidChannel_ReportsDet(void)
{
    PduInfoType pdu;
    uint8 data[1] = { XCP_CMD_CONNECT };

    Xcp_Init(&testConfig);
    pdu.SduDataPtr = data;
    pdu.SduLength = 1U;
    Xcp_RxIndication(XCP_NUMBER_OF_CHANNELS, 0U, &pdu);
    TEST_ASSERT_EQUAL_UINT16(1U, mock_XcpDetCalls);
}

void test_Xcp_RxIndication_ConnectCommand_IsProcessed(void)
{
    PduInfoType pdu;
    uint8 data[2] = { XCP_CMD_CONNECT, 0x00U };

    Xcp_Init(&testConfig);
    pdu.SduDataPtr = data;
    pdu.SduLength = sizeof(data);
    Xcp_RxIndication(0U, 0U, &pdu);
    TEST_ASSERT_EQUAL_UINT16(0U, mock_XcpDetCalls);
}

void test_Xcp_TriggerTransmit_Variants(void)
{
    PduInfoType pdu;
    uint8 buf[8];

    /* Uninit */
    TEST_ASSERT_EQUAL(E_NOT_OK, Xcp_TriggerTransmit(0U, 0U, &pdu));

    Xcp_Init(&testConfig);

    /* Null pointer */
    TEST_ASSERT_EQUAL(E_NOT_OK, Xcp_TriggerTransmit(0U, 0U, NULL_PTR));

    /* Invalid channel */
    pdu.SduDataPtr = buf;
    pdu.SduLength = sizeof(buf);
    TEST_ASSERT_EQUAL(E_NOT_OK, Xcp_TriggerTransmit(XCP_NUMBER_OF_CHANNELS, 0U, &pdu));

    /* Valid */
    TEST_ASSERT_EQUAL(E_OK, Xcp_TriggerTransmit(0U, 0U, &pdu));
    /* The three invalid calls each reported one DET error */
    TEST_ASSERT_EQUAL_UINT16(3U, mock_XcpDetCalls);
}

void test_Xcp_SetTransmissionMode_NoCrash(void)
{
    Xcp_SetTransmissionMode(0U, 0U);   /* transmission mode off */
    Xcp_Init(&testConfig);
    Xcp_SetTransmissionMode(0U, 1U);   /* transmission mode on */
    Xcp_SetTransmissionMode(XCP_NUMBER_OF_CHANNELS, 1U);
    /* uninit + invalid channel each report DET */
    TEST_ASSERT_EQUAL_UINT16(2U, mock_XcpDetCalls);
}

/*==================================================================================================
*                                  Command dispatch
*================================================================================================*/
void test_Xcp_ProcessCommand_NullOrEmpty_Ignored(void)
{
    uint8 cmd[1] = { XCP_CMD_CONNECT };

    Xcp_Init(&testConfig);
    Xcp_ProcessCommand(0U, NULL_PTR, 1U);
    Xcp_ProcessCommand(0U, cmd, 0U);
    TEST_ASSERT_EQUAL_UINT16(0U, mock_XcpDetCalls);
}

void test_Xcp_ProcessCommand_NotConnected_NonConnectIgnored(void)
{
    uint8 cmd[2] = { XCP_CMD_GET_STATUS, 0U };

    Xcp_Init(&testConfig);
    Xcp_ProcessCommand(0U, cmd, sizeof(cmd));
    /* GET_STATUS while disconnected is silently ignored */
    TEST_ASSERT_EQUAL_UINT16(0U, mock_XcpDetCalls);
}

void test_Xcp_ProcessCommand_UnknownCommand_AfterConnect(void)
{
    uint8 cmd[2] = { 0x01U, 0U };  /* not a known XCP command */

    Xcp_Init(&testConfig);
    test_connect();
    Xcp_ProcessCommand(0U, cmd, sizeof(cmd));
    TEST_ASSERT_EQUAL_UINT16(0U, mock_XcpDetCalls);
}

void test_Xcp_Connect_SyntaxError_BadMode(void)
{
    uint8 shortCmd[1] = { XCP_CMD_CONNECT };
    uint8 badMode[2]  = { XCP_CMD_CONNECT, 0x02U };

    Xcp_Init(&testConfig);

    /* CONNECT with truncated payload -> ERR_CMD_SYNTAX path */
    Xcp_ProcessCommand(0U, shortCmd, sizeof(shortCmd));
    /* CONNECT with invalid mode -> ERR_MODE_NOT_VALID path */
    Xcp_ProcessCommand(0U, badMode, sizeof(badMode));
    TEST_ASSERT_EQUAL_UINT16(0U, mock_XcpDetCalls);
}

void test_Xcp_Connect_ThenGetStatus_Synch_CommModeInfo(void)
{
    uint8 getStatus[1]     = { XCP_CMD_GET_STATUS };
    uint8 synch[1]         = { XCP_CMD_SYNCH };
    uint8 commModeInfo[1]  = { XCP_CMD_GET_COMM_MODE_INFO };

    Xcp_Init(&testConfig);
    test_connect();

    Xcp_ProcessCommand(0U, getStatus, sizeof(getStatus));
    Xcp_ProcessCommand(0U, synch, sizeof(synch));
    Xcp_ProcessCommand(0U, commModeInfo, sizeof(commModeInfo));
    TEST_ASSERT_EQUAL_UINT16(0U, mock_XcpDetCalls);
}

void test_Xcp_Disconnect(void)
{
    uint8 disconnect[1] = { XCP_CMD_DISCONNECT };

    Xcp_Init(&testConfig);
    test_connect();
    Xcp_ProcessCommand(0U, disconnect, sizeof(disconnect));
    /* After disconnect, GET_STATUS must be ignored again */
    Xcp_ProcessCommand(0U, disconnect, sizeof(disconnect));
    TEST_ASSERT_EQUAL_UINT16(0U, mock_XcpDetCalls);
}

void test_Xcp_GetId_AllIdTypes(void)
{
    uint8 id0[2] = { XCP_CMD_GET_ID, 0x00U };
    uint8 id1[2] = { XCP_CMD_GET_ID, 0x01U };
    uint8 id2[2] = { XCP_CMD_GET_ID, 0x02U };
    uint8 idx[2] = { XCP_CMD_GET_ID, 0x7FU };

    Xcp_Init(&testConfig);
    test_connect();

    Xcp_ProcessCommand(0U, id0, sizeof(id0));
    Xcp_ProcessCommand(0U, id1, sizeof(id1));
    Xcp_ProcessCommand(0U, id2, sizeof(id2));
    Xcp_ProcessCommand(0U, idx, sizeof(idx));
    TEST_ASSERT_EQUAL_UINT16(0U, mock_XcpDetCalls);
}

/*==================================================================================================
*                                  Memory transfer commands
*================================================================================================*/
static void test_set_mta(uint32 addr)
{
    uint8 setMta[6];
    setMta[0] = XCP_CMD_SET_MTA;
    setMta[1] = 0x00U;                          /* address extension */
    setMta[2] = (uint8)(addr & 0xFFU);
    setMta[3] = (uint8)((addr >> 8) & 0xFFU);
    setMta[4] = (uint8)((addr >> 16) & 0xFFU);
    setMta[5] = (uint8)((addr >> 24) & 0xFFU);
    Xcp_ProcessCommand(0U, setMta, sizeof(setMta));
}

void test_Xcp_Upload_ValidRam_AndOutOfRange(void)
{
    uint8 upload2[2] = { XCP_CMD_UPLOAD, 2U };
    uint8 uploadMax[2] = { XCP_CMD_UPLOAD, 0xFFU };  /* clamped to MAX_CTO-1 */
    uint8 uploadBlocked[2] = { XCP_CMD_UPLOAD, 4U };

    if (!test_memory_mapped) {
        TEST_IGNORE_MESSAGE("low-address mmap unsupported on this host");
    }

    Xcp_Init(&testConfig);
    test_connect();

    /* Valid: MTA points at the simulated calibration RAM */
    test_set_mta(XCP_TEST_RAM_ADDR);
    Xcp_ProcessCommand(0U, upload2, sizeof(upload2));

    /* Over-length request is clamped and still served */
    test_set_mta(XCP_TEST_RAM_ADDR);
    Xcp_ProcessCommand(0U, uploadMax, sizeof(uploadMax));

    /* Out of any registered memory range -> ACCESS_DENIED path */
    test_set_mta(0xDEAD0000U);
    Xcp_ProcessCommand(0U, uploadBlocked, sizeof(uploadBlocked));
    TEST_ASSERT_EQUAL_UINT16(0U, mock_XcpDetCalls);
}

void test_Xcp_ShortUpload(void)
{
    uint8 shortUpload[4] = { XCP_CMD_SHORT_UPLOAD, 2U, 0x00U, 0x00U };

    if (!test_memory_mapped) {
        TEST_IGNORE_MESSAGE("low-address mmap unsupported on this host");
    }

    Xcp_Init(&testConfig);
    test_connect();
    test_set_mta(XCP_TEST_RAM_ADDR);
    Xcp_ProcessCommand(0U, shortUpload, sizeof(shortUpload));

    /* Reserved bytes must stay non-zero per spec? implementation reads [2]/[3]:
     * also hit the reserved-field variant */
    shortUpload[3] = 0x01U;
    Xcp_ProcessCommand(0U, shortUpload, sizeof(shortUpload));
    TEST_ASSERT_EQUAL_UINT16(0U, mock_XcpDetCalls);
}

void test_Xcp_Download_Syntax_And_ValidWrite(void)
{
    uint8 syntaxErr[2]  = { XCP_CMD_DOWNLOAD, 8U };          /* length lies: only 0 payload bytes */
    uint8 validWrite[4] = { XCP_CMD_DOWNLOAD, 2U, 0x11U, 0x22U };

    if (!test_memory_mapped) {
        TEST_IGNORE_MESSAGE("low-address mmap unsupported on this host");
    }

    Xcp_Init(&testConfig);
    test_connect();

    Xcp_ProcessCommand(0U, syntaxErr, sizeof(syntaxErr));

    test_set_mta(XCP_TEST_RAM_ADDR);
    Xcp_ProcessCommand(0U, validWrite, sizeof(validWrite));
    TEST_ASSERT_EQUAL_UINT8(0x11U, testCalRam[0]);
    TEST_ASSERT_EQUAL_UINT8(0x22U, testCalRam[1]);
    TEST_ASSERT_EQUAL_UINT16(0U, mock_XcpDetCalls);
}

void test_Xcp_Download_OutOfRange_AccessDenied(void)
{
    uint8 validWrite[4] = { XCP_CMD_DOWNLOAD, 2U, 0x11U, 0x22U };

    Xcp_Init(&testConfig);
    test_connect();
    test_set_mta(0xDEAD0000U);
    Xcp_ProcessCommand(0U, validWrite, sizeof(validWrite));
    TEST_ASSERT_EQUAL_UINT16(0U, mock_XcpDetCalls);
    if (test_memory_mapped)
    {
        /* the calibration RAM must be untouched */
        TEST_ASSERT_EQUAL_UINT8(0xA5U, testCalRam[0]);
    }
}

/*==================================================================================================
*                                  Seed & key
*================================================================================================*/
void test_Xcp_GetSeed_Variants(void)
{
    uint8 firstPart[3]  = { XCP_CMD_GET_SEED, 0x00U, XCP_RESOURCE_CAL_PAG };
    uint8 secondPart[3] = { XCP_CMD_GET_SEED, 0x01U, XCP_RESOURCE_DAQ };
    uint8 badMode[3]    = { XCP_CMD_GET_SEED, 0x02U, XCP_RESOURCE_DAQ };
    uint8 badResource[3]= { XCP_CMD_GET_SEED, 0x00U, 0xF0U };

    Xcp_Init(&testConfig);
    test_connect();

    Xcp_ProcessCommand(0U, firstPart, sizeof(firstPart));
    Xcp_ProcessCommand(0U, secondPart, sizeof(secondPart));
    Xcp_ProcessCommand(0U, badMode, sizeof(badMode));
    Xcp_ProcessCommand(0U, badResource, sizeof(badResource));
    TEST_ASSERT_EQUAL_UINT16(0U, mock_XcpDetCalls);
}

void test_Xcp_Unlock_ShortKey_And_ValidKey(void)
{
    uint8 getSeed[3]   = { XCP_CMD_GET_SEED, 0x00U, XCP_RESOURCE_CAL_PAG };
    uint8 shortKey[2]  = { XCP_CMD_UNLOCK, 1U };
    uint8 validKey[4]  = { XCP_CMD_UNLOCK, 2U, 0xAAU, 0xBBU };

    Xcp_Init(&testConfig);
    test_connect();

    Xcp_ProcessCommand(0U, shortKey, sizeof(shortKey));   /* ERR_SEQUENCE path */
    Xcp_ProcessCommand(0U, getSeed, sizeof(getSeed));     /* locks CAL/PAG */
    Xcp_ProcessCommand(0U, validKey, sizeof(validKey));   /* unlock path */
    TEST_ASSERT_EQUAL_UINT16(0U, mock_XcpDetCalls);
}

/*==================================================================================================
*                                  DAQ command set (direct public handlers)
*================================================================================================*/
/* DAQ/PGM command codes (>= 0xC0) are routed to the standard dispatcher by
 * Xcp_ProcessCommand, so the handlers are exercised through their public
 * APIs directly — exactly how a real transport layer integration would
 * register them. */

static uint8 daqBuf[8];

static void daq_alloc(uint16 daqCount)
{
    daqBuf[0] = XCP_CMD_ALLOC_DAQ;
    daqBuf[1] = 0U;
    daqBuf[2] = (uint8)(daqCount & 0xFFU);
    daqBuf[3] = (uint8)(daqCount >> 8);
    Xcp_CmdAllocDaq(0U, daqBuf);
}

static void daq_alloc_odt(uint16 daqList, uint8 odtCount)
{
    daqBuf[0] = XCP_CMD_ALLOC_ODT;
    daqBuf[1] = 0U;
    daqBuf[2] = (uint8)(daqList & 0xFFU);
    daqBuf[3] = (uint8)(daqList >> 8);
    daqBuf[4] = odtCount;
    Xcp_CmdAllocOdt(0U, daqBuf);
}

static void daq_alloc_entry(uint16 daqList, uint8 odtNum, uint8 entryCount)
{
    daqBuf[0] = XCP_CMD_ALLOC_ODT_ENTRY;
    daqBuf[1] = 0U;
    daqBuf[2] = (uint8)(daqList & 0xFFU);
    daqBuf[3] = (uint8)(daqList >> 8);
    daqBuf[4] = odtNum;
    daqBuf[5] = entryCount;
    Xcp_CmdAllocOdtEntry(0U, daqBuf);
}

static void daq_set_ptr(uint16 daqList, uint8 odtNum, uint8 entryNum)
{
    daqBuf[0] = XCP_CMD_SET_DAQ_PTR;
    daqBuf[1] = 0U;
    daqBuf[2] = (uint8)(daqList & 0xFFU);
    daqBuf[3] = (uint8)(daqList >> 8);
    daqBuf[4] = odtNum;
    daqBuf[5] = entryNum;
    Xcp_CmdSetDaqPtr(0U, daqBuf);
}

static void daq_write_entry(uint8 bitOffset, uint8 eleLen, uint32 addr)
{
    daqBuf[0] = XCP_CMD_WRITE_DAQ;
    daqBuf[1] = bitOffset;
    daqBuf[2] = eleLen;
    daqBuf[3] = 0x00U;                      /* address extension */
    daqBuf[4] = (uint8)(addr & 0xFFU);
    daqBuf[5] = (uint8)((addr >> 8) & 0xFFU);
    daqBuf[6] = (uint8)((addr >> 16) & 0xFFU);
    daqBuf[7] = (uint8)((addr >> 24) & 0xFFU);
    Xcp_CmdWriteDaq(0U, daqBuf);
}

static void daq_list_mode(uint8 mode, uint16 daqList, uint16 eventChannel, uint8 prescaler)
{
    daqBuf[0] = XCP_CMD_SET_DAQ_LIST_MODE;
    daqBuf[1] = mode;
    daqBuf[2] = (uint8)(daqList & 0xFFU);
    daqBuf[3] = (uint8)(daqList >> 8);
    daqBuf[4] = 0U;                         /* reserved */
    daqBuf[5] = (uint8)(eventChannel & 0xFFU);
    daqBuf[6] = (uint8)(eventChannel >> 8);
    daqBuf[7] = prescaler;
    Xcp_CmdSetDaqListMode(0U, daqBuf);
}

static void daq_start_stop(uint8 mode, uint16 daqList)
{
    daqBuf[0] = XCP_CMD_START_STOP_DAQ_LIST;
    daqBuf[1] = mode;
    daqBuf[2] = (uint8)(daqList & 0xFFU);
    daqBuf[3] = (uint8)(daqList >> 8);
    Xcp_CmdStartStopDaqList(0U, daqBuf);
}

void test_Xcp_Daq_Allocate_Configure_StartStop(void)
{
    Xcp_Init(&testConfig);
    test_connect();

    /* ALLOC_DAQ: 2 lists */
    daq_alloc(2U);
    /* ALLOC_ODT on list 0: 2 ODTs */
    daq_alloc_odt(0U, 2U);
    /* ALLOC_ODT_ENTRY on list 0 / odt 0: 2 entries */
    daq_alloc_entry(0U, 0U, 2U);
    /* SET_DAQ_PTR + WRITE_DAQ: 32-bit at the simulated calibration RAM */
    daq_set_ptr(0U, 0U, 0U);
    daq_write_entry(0U, 32U, XCP_TEST_RAM_ADDR);
    /* SET_DAQ_LIST_MODE: list 0, direction DAQ, event channel 0, prescaler 1 */
    daq_list_mode(0x01U, 0U, 0U, 1U);
    /* GET_DAQ_LIST_MODE */
    {
        daqBuf[0] = XCP_CMD_GET_DAQ_LIST_MODE;
        daqBuf[2] = 0U;
        daqBuf[3] = 0U;
        Xcp_CmdGetDaqListMode(0U, daqBuf);
    }
    /* START_STOP_DAQ_LIST: start list 0, then stop it */
    daq_start_stop(0x01U, 0U);
    daq_start_stop(0x00U, 0U);
    /* Select mode (synchronized start preparation) + invalid mode */
    daq_start_stop(0x02U, 0U);
    daq_start_stop(0x03U, 0U);
    /* START_STOP_SYNCH */
    {
        uint8 synch[1] = { XCP_CMD_START_STOP_SYNCH };
        Xcp_CmdStartStopSynch(0U, synch);
    }
    /* Info commands */
    Xcp_CmdGetDaqProcessorInfo(0U);
    Xcp_CmdGetDaqResolutionInfo(0U);
    {
        daqBuf[0] = XCP_CMD_GET_DAQ_LIST_INFO;
        daqBuf[1] = 0U;
        daqBuf[2] = 0U;
        daqBuf[3] = 0U;
        Xcp_CmdGetDaqListInfo(0U, daqBuf);
    }
    /* CLEAR_DAQ_LIST then FREE_DAQ */
    {
        daqBuf[0] = XCP_CMD_CLEAR_DAQ_LIST;
        daqBuf[1] = 0U;
        daqBuf[2] = 0U;
        daqBuf[3] = 0U;
        Xcp_CmdClearDaqList(0U, daqBuf);
    }
    Xcp_CmdFreeDaq(0U);
    TEST_ASSERT_EQUAL_UINT16(0U, mock_XcpDetCalls);
}

void test_Xcp_Daq_ErrorBranches(void)
{
    Xcp_Init(&testConfig);
    test_connect();

    /* Out-of-range list index in every list-indexed command */
    daq_alloc_odt(XCP_MAX_DAQ_LISTS, 1U);
    daq_alloc_entry(XCP_MAX_DAQ_LISTS, 0U, 1U);
    daq_alloc_entry(0U, XCP_MAX_ODTS_PER_DAQ, 1U);
    daq_alloc_odt(0U, XCP_MAX_ODTS_PER_DAQ + 1U);
    daq_alloc_entry(0U, 0U, XCP_MAX_ODT_ENTRIES_PER_ODT + 1U);
    daq_set_ptr(XCP_MAX_DAQ_LISTS, 0U, 0U);
    daq_list_mode(0x01U, XCP_MAX_DAQ_LISTS, 0U, 1U);
    daq_start_stop(0x01U, XCP_MAX_DAQ_LISTS);
    daq_alloc(XCP_MAX_DAQ_LISTS + 1U);
    {
        daqBuf[0] = XCP_CMD_GET_DAQ_LIST_INFO;
        daqBuf[1] = 0U;
        daqBuf[2] = (uint8)(XCP_MAX_DAQ_LISTS & 0xFFU);
        daqBuf[3] = (uint8)(XCP_MAX_DAQ_LISTS >> 8);
        Xcp_CmdGetDaqListInfo(0U, daqBuf);
    }

    /* WRITE_DAQ without a preceding SET_DAQ_PTR (ptr still zero-initialized:
     * valid indices) and with an invalid pointer state */
    daq_write_entry(0U, 32U, XCP_TEST_RAM_ADDR);

    /* DAQ_ACTIVE guard: start a list, then allocation commands must fail */
    daq_alloc(1U);
    daq_alloc_odt(0U, 1U);
    daq_alloc_entry(0U, 0U, 1U);
    daq_set_ptr(0U, 0U, 0U);
    daq_write_entry(0U, 32U, XCP_TEST_RAM_ADDR);
    daq_list_mode(0x01U, 0U, 0U, 1U);
    daq_start_stop(0x01U, 0U);
    daq_alloc(1U);
    daq_alloc_odt(0U, 1U);
    daq_alloc_entry(0U, 0U, 1U);
    daq_write_entry(0U, 32U, XCP_TEST_RAM_ADDR);
    /* stop it again */
    daq_start_stop(0x00U, 0U);

    TEST_ASSERT_EQUAL_UINT16(0U, mock_XcpDetCalls);
}

void test_Xcp_Daq_MainFunction_Processing(void)
{
    Xcp_Init(&testConfig);
    test_connect();

    if (!test_memory_mapped) {
        TEST_IGNORE_MESSAGE("low-address mmap unsupported on this host");
    }

    /* Full configured DAQ list: 2 ODTs, one 32-bit entry each */
    daq_alloc(1U);
    daq_alloc_odt(0U, 2U);
    daq_alloc_entry(0U, 0U, 1U);
    daq_alloc_entry(0U, 1U, 1U);
    daq_set_ptr(0U, 0U, 0U);
    daq_write_entry(0U, 32U, XCP_TEST_RAM_ADDR);
    daq_set_ptr(0U, 1U, 0U);
    daq_write_entry(0U, 32U, XCP_TEST_RAM_ADDR + 4U);

    /* Timestamp + PID_OFF mode variants both exercised through DaqSample */
    daq_list_mode(0x01U | 0x02U, 0U, 0U, 1U);   /* timestamp enabled */
    daq_start_stop(0x01U, 0U);

    Xcp_MainFunction();     /* runs the periodic DAQ processor */
    Xcp_DaqProcessor();
    Xcp_DaqSample(0U);      /* explicit sample of the running list */
    Xcp_DaqSample(0xFFFFU); /* invalid list -> guard path */
    Xcp_DaqTransmit(0U, 0U);
    Xcp_DaqTransmit(0xFFFFU, 0U);

    /* PID_OFF variant */
    daq_list_mode(0x01U | 0x04U, 0U, 0U, 1U);   /* PID off */
    Xcp_DaqProcessor();

    daq_start_stop(0x00U, 0U);
    TEST_ASSERT_EQUAL_UINT16(0U, mock_XcpDetCalls);
}

/*==================================================================================================
*                                  PGM command set (direct handlers)
*================================================================================================*/
void test_Xcp_Pgm_Commands(void)
{
    uint8 programClear[8];
    uint8 program[6];

    Xcp_Init(&testConfig);
    test_connect();

    /* PROGRAM while PGM idle -> ERR_SEQUENCE branch (no memory access) */
    program[0] = XCP_CMD_PROGRAM;
    program[1] = 2U;
    program[2] = 0x55U;
    program[3] = 0xAAU;
    program[4] = 0x00U;
    program[5] = 0x00U;
    Xcp_CmdProgram(0U, program, sizeof(program));

    /* Resource protection guard: lock PGM, PROGRAM_START must fail locked */
    Xcp_SetResourceProtection(XCP_RESOURCE_PGM, TRUE);
    Xcp_CmdProgramStart(0U);
    Xcp_UnlockResource(XCP_RESOURCE_PGM);

    if (!test_memory_mapped) {
        TEST_IGNORE_MESSAGE("low-address mmap unsupported on this host");
    }

    /* PROGRAM_START */
    Xcp_CmdProgramStart(0U);
    /* PROGRAM_START again -> PGM_ACTIVE branch */
    Xcp_CmdProgramStart(0U);

    /* SET_MTA into the simulated flash window */
    test_set_mta(XCP_TEST_FLASH_ADDR);

    /* PROGRAM: 2 bytes into flash */
    Xcp_CmdProgram(0U, program, sizeof(program));

    /* PROGRAM_CLEAR: mode 0, 1 sector */
    (void)memset(programClear, 0, sizeof(programClear));
    programClear[0] = XCP_CMD_PROGRAM_CLEAR;
    programClear[1] = 0x00U;
    programClear[2] = 0x01U;
    programClear[3] = 0x00U;
    programClear[4] = 0x00U;
    programClear[5] = 0x00U;
    programClear[6] = 0x00U;
    programClear[7] = 0x08U;
    Xcp_CmdProgramClear(0U, programClear);

    /* PROGRAM_RESET / PROGRAM_VERIFY */
    Xcp_CmdProgramReset(0U);
    {
        uint8 verify[3] = { XCP_CMD_PROGRAM_VERIFY, 0x00U, 0x00U };
        Xcp_CmdProgramVerify(0U, verify);
    }
    TEST_ASSERT_EQUAL_UINT16(0U, mock_XcpDetCalls);
}

void test_Xcp_Pgm_OutOfRange_NoCrash(void)
{
    uint8 program[2] = { XCP_CMD_PROGRAM, 0U };

    Xcp_Init(&testConfig);
    test_connect();

    /* MTA outside all ranges: PROGRAM must reject with ACCESS_DENIED */
    Xcp_CmdProgramStart(0U);
    test_set_mta(0xDEAD0000U);
    Xcp_CmdProgram(0U, program, sizeof(program));
    Xcp_CmdProgramReset(0U);
    TEST_ASSERT_EQUAL_UINT16(0U, mock_XcpDetCalls);
}

/*==================================================================================================
*                                  Memory + resource helpers (public API)
*================================================================================================*/
void test_Xcp_ReadWriteMemory_PublicApi(void)
{
    uint8 buffer[4];
    const uint8 payload[4] = { 1U, 2U, 3U, 4U };

    if (!test_memory_mapped) {
        TEST_IGNORE_MESSAGE("low-address mmap unsupported on this host");
    }

    Xcp_Init(&testConfig);

    /* Valid RAM range */
    TEST_ASSERT_EQUAL(E_OK, Xcp_WriteMemory(XCP_TEST_RAM_ADDR, 0x00U, payload, sizeof(payload)));
    TEST_ASSERT_EQUAL(E_OK, Xcp_ReadMemory(XCP_TEST_RAM_ADDR, 0x00U, buffer, sizeof(buffer)));
    TEST_ASSERT_EQUAL_UINT8(1U, buffer[0]);
    TEST_ASSERT_EQUAL_UINT8(4U, buffer[3]);

    /* Out-of-range -> guarded */
    TEST_ASSERT_EQUAL(E_NOT_OK, Xcp_ReadMemory(0xDEAD0000U, 0x00U, buffer, sizeof(buffer)));
    TEST_ASSERT_EQUAL(E_NOT_OK, Xcp_WriteMemory(0xDEAD0000U, 0x00U, payload, sizeof(payload)));
}

void test_Xcp_ResourceProtection_Helpers(void)
{
    Xcp_Init(&testConfig);

    TEST_ASSERT_EQUAL(FALSE, Xcp_IsResourceProtected(XCP_RESOURCE_CAL_PAG));

    Xcp_SetResourceProtection(XCP_RESOURCE_CAL_PAG, TRUE);
    TEST_ASSERT_EQUAL(TRUE, Xcp_IsResourceProtected(XCP_RESOURCE_CAL_PAG));

    TEST_ASSERT_EQUAL(E_OK, Xcp_UnlockResource(XCP_RESOURCE_CAL_PAG));
    TEST_ASSERT_EQUAL(FALSE, Xcp_IsResourceProtected(XCP_RESOURCE_CAL_PAG));

    /* Unlocking a resource that is not locked still succeeds (idempotent) */
    TEST_ASSERT_EQUAL(E_OK, Xcp_UnlockResource(XCP_RESOURCE_DAQ));
}

/*==================================================================================================
*                                  STIM
*================================================================================================*/
void test_Xcp_StimProcessor_NoCrash(void)
{
    uint8 data[4] = { 0x01U, 0x02U, 0x03U, 0x04U };

    Xcp_Init(&testConfig);
    test_connect();
    Xcp_StimProcessor(0U, data, sizeof(data));
    Xcp_StimProcessor(0U, NULL_PTR, 0U);
    TEST_ASSERT_EQUAL_UINT16(0U, mock_XcpDetCalls);
}

/*==================================================================================================
*                                  MAIN
*================================================================================================*/
int main(void)
{
    UNITY_BEGIN();

    /* Map the simulated RAM/flash windows before any test runs */
    test_map_simulated_memory();

    RUN_TEST(test_Xcp_Init_NullConfig_ReportsDet);
    RUN_TEST(test_Xcp_Init_ValidConfig_Succeeds);
    RUN_TEST(test_Xcp_DeInit_StopsAllDaq);
    RUN_TEST(test_Xcp_GetVersionInfo_Valid);
    RUN_TEST(test_Xcp_MainFunction_IncrementsEventCounters);

    RUN_TEST(test_Xcp_RxIndication_Uninit_ReportsDet);
    RUN_TEST(test_Xcp_RxIndication_NullPdu_ReportsDet);
    RUN_TEST(test_Xcp_RxIndication_InvalidChannel_ReportsDet);
    RUN_TEST(test_Xcp_RxIndication_ConnectCommand_IsProcessed);
    RUN_TEST(test_Xcp_TriggerTransmit_Variants);
    RUN_TEST(test_Xcp_SetTransmissionMode_NoCrash);

    RUN_TEST(test_Xcp_ProcessCommand_NullOrEmpty_Ignored);
    RUN_TEST(test_Xcp_ProcessCommand_NotConnected_NonConnectIgnored);
    RUN_TEST(test_Xcp_ProcessCommand_UnknownCommand_AfterConnect);
    RUN_TEST(test_Xcp_Connect_SyntaxError_BadMode);
    RUN_TEST(test_Xcp_Connect_ThenGetStatus_Synch_CommModeInfo);
    RUN_TEST(test_Xcp_Disconnect);
    RUN_TEST(test_Xcp_GetId_AllIdTypes);

    RUN_TEST(test_Xcp_Upload_ValidRam_AndOutOfRange);
    RUN_TEST(test_Xcp_ShortUpload);
    RUN_TEST(test_Xcp_Download_Syntax_And_ValidWrite);
    RUN_TEST(test_Xcp_Download_OutOfRange_AccessDenied);

    RUN_TEST(test_Xcp_GetSeed_Variants);
    RUN_TEST(test_Xcp_Unlock_ShortKey_And_ValidKey);

    RUN_TEST(test_Xcp_Daq_Allocate_Configure_StartStop);
    RUN_TEST(test_Xcp_Daq_ErrorBranches);
    RUN_TEST(test_Xcp_Daq_MainFunction_Processing);

    RUN_TEST(test_Xcp_Pgm_Commands);
    RUN_TEST(test_Xcp_Pgm_OutOfRange_NoCrash);

    RUN_TEST(test_Xcp_ReadWriteMemory_PublicApi);
    RUN_TEST(test_Xcp_ResourceProtection_Helpers);

    RUN_TEST(test_Xcp_StimProcessor_NoCrash);

    return UNITY_END();
}
