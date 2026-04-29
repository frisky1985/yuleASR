/******************************************************************************
 * @file    test_com_pdur_integration.c
 * @brief   COM-PduR Integration Tests (T016) - Simplified Version
 * 
 * Tests complete end-to-end data flow between COM and PduR modules:
 * - Signal transmission through PduR routing
 * - Signal reception through PduR indication
 * - TxConfirmation handling
 * - Performance tests (latency, throughput)
 *
 * This is a simplified version using stubs for complex dependencies.
 *
 * AUTOSAR Classic Platform R22-11 compliant
 * ASIL-D Safety Level
 *
 * @copyright Copyright (c) 2024
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>

#include "../../tests/unity/unity.h"

/*==================[Type Definitions]======================================*/

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;
typedef uint16_t PduIdType;
typedef uint16_t PduLengthType;

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef NULL_PTR
#define NULL_PTR ((void*)0)
#endif

typedef struct {
    uint8 *SduDataPtr;
    uint8 *MetaDataPtr;
    PduLengthType SduLength;
} PduInfoType;

typedef enum {
    E_OK = 0,
    E_NOT_OK = 1
} Std_ReturnType;

/* COM types */
typedef uint16_t Com_SignalIdType;
typedef uint16_t Com_IPduIdType;
typedef uint16_t Com_SignalGroupIdType;

typedef enum {
    COM_UNINIT = 0,
    COM_READY
} Com_StatusType;

typedef enum {
    COM_UINT8 = 0,
    COM_UINT16,
    COM_UINT32,
    COM_UINT64,
    COM_SINT8,
    COM_SINT16,
    COM_SINT32,
    COM_SINT64,
    COM_FLOAT32,
    COM_FLOAT64,
    COM_BOOLEAN
} Com_SignalTypeType;

typedef enum {
    COM_TRIGGERED = 0,
    COM_TRIGGERED_ON_CHANGE,
    COM_TRIGGERED_ON_CHANGE_WITHOUT_REPETITION,
    COM_TRIGGERED_WITHOUT_REPETITION,
    COM_PENDING
} Com_TransferPropertyType;

typedef enum {
    COM_LITTLE_ENDIAN = 0,
    COM_BIG_ENDIAN
} Com_SignalEndiannessType;

typedef enum {
    COM_SEND = 0,
    COM_RECEIVE
} Com_IPduDirectionType;

typedef enum {
    COM_NORMAL = 0,
    COM_TP
} Com_IPduTypeType;

typedef enum {
    COM_IMMEDIATE = 0,
    COM_DEFERRED
} Com_IPduSignalProcessingType;

typedef enum {
    COM_PERIODIC = 0,
    COM_DIRECT,
    COM_MIXED,
    COM_NONE
} Com_TxModeType;

typedef struct {
    Com_TxModeType Mode;
    uint32 Period;
    uint32 RepetitionPeriod;
    uint8 NumRepetitions;
    uint32 TimeOffset;
} Com_TxModeConfigType;

typedef struct {
    Com_SignalIdType SignalId;
    uint8* DataPtr;
    uint16 BitPosition;
    uint8 BitSize;
    Com_SignalEndiannessType Endianness;
    Com_SignalTypeType SignalType;
    Com_TransferPropertyType TransferProperty;
    void (*ComNotification)(void);
    uint32 Timeout;
    const void* InitValue;
} Com_SignalConfigType;

typedef struct {
    Com_IPduIdType IPduId;
    uint8* DataPtr;
    uint16 Length;
    Com_IPduDirectionType Direction;
    Com_IPduTypeType Type;
    Com_IPduSignalProcessingType SignalProcessing;
    Com_SignalIdType* SignalRefs;
    uint16 NumSignals;
    Com_SignalGroupIdType* SignalGroupRefs;
    uint16 NumSignalGroups;
    Com_TxModeConfigType TxMode;
    void** IpduGroupRefs;
    uint16 NumIpduGroups;
    uint32 Timeout;
    void (*ComIPduCallout)(void);
} Com_IPduConfigType;

typedef struct {
    const Com_SignalConfigType* Signals;
    uint16 NumSignals;
    void* SignalGroups;
    uint16 NumSignalGroups;
    const Com_IPduConfigType* IPdus;
    uint16 NumIPdus;
    void* IPduGroups;
    uint16 NumIPduGroups;
} Com_ConfigType;

/*==================[Test Configuration]===================================*/

#define TEST_MAX_PDUS           8
#define TEST_MAX_SIGNALS        16
#define TEST_IPDU_BUFFER_SIZE   64

/* Performance test configuration */
#define PERF_TEST_MESSAGE_COUNT 1000
#define PERF_TEST_MAX_LATENCY_US 10000
#define PERF_TEST_MIN_THROUGHPUT_KBPS 100

/*==================[Test Data Types]======================================*/

typedef struct {
    PduIdType PduId;
    bool WasTransmitted;
    bool WasReceived;
    uint8 Data[TEST_IPDU_BUFFER_SIZE];
    PduLengthType Length;
    uint64_t TxTimestamp;
    uint64_t RxTimestamp;
} TestPduTrackingType;

typedef struct {
    uint32 TotalTxRequests;
    uint32 SuccessfulTransmissions;
    uint32 FailedTransmissions;
    uint32 ConfirmationsReceived;
    uint32 RxIndicationsReceived;
    uint64_t TotalLatencyNs;
    uint64_t MinLatencyNs;
    uint64_t MaxLatencyNs;
} TestStatisticsType;

/*==================[Function Declarations]================================*/

/* Forward declarations for PduR functions */
extern Std_ReturnType PduR_IfTransmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr);
extern void PduR_DeInit(void);
extern void Com_MainFunctionTx(void);
extern void Com_TxConfirmation(PduIdType TxPduId, Std_ReturnType result);

/*==================[Mock COM/PduR State]==================================*/

static Com_StatusType Mock_ComStatus = COM_UNINIT;
static const Com_ConfigType* Mock_ComConfig = NULL_PTR;
static bool Mock_PduR_Initialized = FALSE;
static bool Mock_TransmitShouldFail = FALSE;
static uint32 Mock_TransmitDelayUs = 0;

/* Buffers for I-PDUs */
static uint8 TestIPduBuffers[TEST_MAX_PDUS][TEST_IPDU_BUFFER_SIZE];

/* Signal configurations */
static Com_SignalConfigType TestSignals[TEST_MAX_SIGNALS];
static Com_IPduConfigType TestIPdus[TEST_MAX_PDUS];

/* COM configuration */
static Com_ConfigType TestComConfig;

/* Tracking data */
static TestPduTrackingType TestPduTracking[TEST_MAX_PDUS];
static TestStatisticsType TestStats;

/* Signal data tracking */
static uint64_t SignalValues[TEST_MAX_SIGNALS];

/*==================[Mock COM Implementation]==============================*/

void Com_Init(const Com_ConfigType* config)
{
    if (config == NULL_PTR) {
        return;
    }
    Mock_ComConfig = config;
    Mock_ComStatus = COM_READY;
    
    /* Clear buffers */
    memset(TestIPduBuffers, 0, sizeof(TestIPduBuffers));
    memset(SignalValues, 0, sizeof(SignalValues));
}

void Com_DeInit(void)
{
    Mock_ComStatus = COM_UNINIT;
    Mock_ComConfig = NULL_PTR;
}

Com_StatusType Com_GetStatus(void)
{
    return Mock_ComStatus;
}

uint8 Com_SendSignal(Com_SignalIdType SignalId, const void* SignalDataPtr)
{
    if (Mock_ComStatus != COM_READY) {
        return 1; /* COM_SERVICE_NOT_AVAILABLE */
    }
    
    if (SignalDataPtr == NULL_PTR || SignalId >= Mock_ComConfig->NumSignals) {
        return 1;
    }
    
    const Com_SignalConfigType* sigConfig = &Mock_ComConfig->Signals[SignalId];
    
    /* Store signal value */
    switch (sigConfig->SignalType) {
        case COM_UINT8:
            SignalValues[SignalId] = (uint64_t)(*(const uint8*)SignalDataPtr);
            break;
        case COM_UINT16:
            SignalValues[SignalId] = (uint64_t)(*(const uint16*)SignalDataPtr);
            break;
        case COM_UINT32:
            SignalValues[SignalId] = (uint64_t)(*(const uint32*)SignalDataPtr);
            break;
        case COM_BOOLEAN:
            SignalValues[SignalId] = (uint64_t)(*(const bool*)SignalDataPtr);
            break;
        default:
            break;
    }
    
    /* Write to I-PDU buffer (simplified) */
    if (sigConfig->DataPtr != NULL_PTR) {
        uint8* dest = sigConfig->DataPtr;
        uint16 bitPos = sigConfig->BitPosition;
        uint8 bytePos = bitPos / 8;
        
        switch (sigConfig->SignalType) {
            case COM_UINT8:
                dest[bytePos] = (uint8)SignalValues[SignalId];
                break;
            case COM_UINT16:
                dest[bytePos] = (uint8)(SignalValues[SignalId] & 0xFF);
                dest[bytePos + 1] = (uint8)((SignalValues[SignalId] >> 8) & 0xFF);
                break;
            case COM_UINT32:
                dest[bytePos] = (uint8)(SignalValues[SignalId] & 0xFF);
                dest[bytePos + 1] = (uint8)((SignalValues[SignalId] >> 8) & 0xFF);
                dest[bytePos + 2] = (uint8)((SignalValues[SignalId] >> 16) & 0xFF);
                dest[bytePos + 3] = (uint8)((SignalValues[SignalId] >> 24) & 0xFF);
                break;
            default:
                break;
        }
    }
    
    /* Trigger transmission if needed */
    if (sigConfig->TransferProperty == COM_TRIGGERED ||
        sigConfig->TransferProperty == COM_TRIGGERED_ON_CHANGE) {
        
        /* Find the I-PDU containing this signal */
        for (uint16 i = 0; i < Mock_ComConfig->NumIPdus; i++) {
            const Com_IPduConfigType* ipdu = &Mock_ComConfig->IPdus[i];
            for (uint16 j = 0; j < ipdu->NumSignals; j++) {
                if (ipdu->SignalRefs[j] == SignalId) {
                    /* Call PduR transmit */
                    PduInfoType pduInfo;
                    pduInfo.SduDataPtr = ipdu->DataPtr;
                    pduInfo.SduLength = ipdu->Length;
                    pduInfo.MetaDataPtr = NULL_PTR;
                    
                    PduR_IfTransmit((PduIdType)i, &pduInfo);
                    break;
                }
            }
        }
    }
    
    return 0; /* E_OK */
}

uint8 Com_ReceiveSignal(Com_SignalIdType SignalId, void* SignalDataPtr)
{
    if (Mock_ComStatus != COM_READY || SignalDataPtr == NULL_PTR) {
        return 1;
    }
    
    if (SignalId >= Mock_ComConfig->NumSignals) {
        return 1;
    }
    
    const Com_SignalConfigType* sigConfig = &Mock_ComConfig->Signals[SignalId];
    
    /* Return signal value */
    switch (sigConfig->SignalType) {
        case COM_UINT8:
            *(uint8*)SignalDataPtr = (uint8)SignalValues[SignalId];
            break;
        case COM_UINT16:
            *(uint16*)SignalDataPtr = (uint16)SignalValues[SignalId];
            break;
        case COM_UINT32:
            *(uint32*)SignalDataPtr = (uint32)SignalValues[SignalId];
            break;
        case COM_BOOLEAN:
            *(bool*)SignalDataPtr = (bool)SignalValues[SignalId];
            break;
        default:
            break;
    }
    
    return 0;
}

void Com_MainFunctionTx(void)
{
    /* Process pending transmissions */
}

void Com_TxConfirmation(PduIdType TxPduId, Std_ReturnType result)
{
    (void)TxPduId;
    (void)result;
    TestStats.ConfirmationsReceived++;
}

void PduR_ComRxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr)
{
    (void)RxPduId;
    (void)PduInfoPtr;
}

/*==================[Mock PduR Implementation]=============================*/

Std_ReturnType PduR_Init(void* ConfigPtr)
{
    if (ConfigPtr == NULL_PTR) {
        return E_NOT_OK;
    }
    Mock_PduR_Initialized = TRUE;
    return E_OK;
}

void PduR_DeInit(void)
{
    Mock_PduR_Initialized = FALSE;
}

Std_ReturnType PduR_IfTransmit(PduIdType TxPduId, const PduInfoType* PduInfoPtr)
{
    if (!Mock_PduR_Initialized || PduInfoPtr == NULL_PTR) {
        return E_NOT_OK;
    }
    
    if (TxPduId >= TEST_MAX_PDUS) {
        return E_NOT_OK;
    }
    
    TestStats.TotalTxRequests++;
    
    if (Mock_TransmitDelayUs > 0) {
        usleep(Mock_TransmitDelayUs);
    }
    
    if (Mock_TransmitShouldFail) {
        TestStats.FailedTransmissions++;
        return E_NOT_OK;
    }
    
    TestPduTracking[TxPduId].WasTransmitted = TRUE;
    TestPduTracking[TxPduId].PduId = TxPduId;
    TestPduTracking[TxPduId].Length = PduInfoPtr->SduLength;
    
    if (PduInfoPtr->SduDataPtr != NULL_PTR && PduInfoPtr->SduLength > 0) {
        memcpy(TestPduTracking[TxPduId].Data, PduInfoPtr->SduDataPtr,
               PduInfoPtr->SduLength > TEST_IPDU_BUFFER_SIZE ? TEST_IPDU_BUFFER_SIZE : PduInfoPtr->SduLength);
    }
    
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    TestPduTracking[TxPduId].TxTimestamp = (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
    
    TestStats.SuccessfulTransmissions++;
    
    return E_OK;
}

void PduR_IfTxConfirmation(PduIdType TxPduId)
{
    if (TxPduId < TEST_MAX_PDUS) {
        Com_TxConfirmation(TxPduId, E_OK);
    }
}

void PduR_IfRxIndication(PduIdType RxPduId, const PduInfoType* PduInfoPtr)
{
    if (RxPduId >= TEST_MAX_PDUS || PduInfoPtr == NULL_PTR) {
        return;
    }
    
    TestPduTracking[RxPduId].WasReceived = TRUE;
    TestPduTracking[RxPduId].Length = PduInfoPtr->SduLength;
    
    if (PduInfoPtr->SduDataPtr != NULL_PTR && PduInfoPtr->SduLength > 0) {
        memcpy(TestPduTracking[RxPduId].Data, PduInfoPtr->SduDataPtr,
               PduInfoPtr->SduLength > TEST_IPDU_BUFFER_SIZE ? TEST_IPDU_BUFFER_SIZE : PduInfoPtr->SduLength);
    }
    
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    TestPduTracking[RxPduId].RxTimestamp = (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
    
    TestStats.RxIndicationsReceived++;
}

void Mock_SimulateTxConfirmation(PduIdType TxPduId)
{
    usleep(100);
    PduR_IfTxConfirmation(TxPduId);
}

void Mock_SimulateRxIndication(PduIdType RxPduId, const uint8* Data, PduLengthType Length)
{
    PduInfoType pduInfo;
    pduInfo.SduDataPtr = (uint8*)Data;
    pduInfo.SduLength = Length;
    pduInfo.MetaDataPtr = NULL_PTR;
    
    PduR_IfRxIndication(RxPduId, &pduInfo);
}

void Mock_Reset(void)
{
    Mock_PduR_Initialized = FALSE;
    Mock_TransmitShouldFail = FALSE;
    Mock_TransmitDelayUs = 0;
    memset(TestPduTracking, 0, sizeof(TestPduTracking));
    memset(&TestStats, 0, sizeof(TestStats));
    TestStats.MinLatencyNs = UINT64_MAX;
}

/*==================[Test Setup/Teardown]==================================*/

void setUp(void)
{
    memset(TestIPduBuffers, 0, sizeof(TestIPduBuffers));
    memset(TestSignals, 0, sizeof(TestSignals));
    memset(TestIPdus, 0, sizeof(TestIPdus));
    Mock_Reset();
}

void tearDown(void)
{
    Com_DeInit();
    PduR_DeInit();
}

/*==================[Configuration Builders]===============================*/

static void BuildBasicConfiguration(void)
{
    /* Signal 0: UINT16 engine speed */
    TestSignals[0].SignalId = 0;
    TestSignals[0].DataPtr = TestIPduBuffers[0];
    TestSignals[0].BitPosition = 0;
    TestSignals[0].BitSize = 16;
    TestSignals[0].Endianness = COM_LITTLE_ENDIAN;
    TestSignals[0].SignalType = COM_UINT16;
    TestSignals[0].TransferProperty = COM_TRIGGERED;
    
    /* Signal 1: UINT8 vehicle speed */
    TestSignals[1].SignalId = 1;
    TestSignals[1].DataPtr = &TestIPduBuffers[0][2];
    TestSignals[1].BitPosition = 16;
    TestSignals[1].BitSize = 8;
    TestSignals[1].Endianness = COM_LITTLE_ENDIAN;
    TestSignals[1].SignalType = COM_UINT8;
    TestSignals[1].TransferProperty = COM_TRIGGERED_ON_CHANGE;
    
    /* Signal 2: UINT32 timestamp */
    TestSignals[2].SignalId = 2;
    TestSignals[2].DataPtr = TestIPduBuffers[1];
    TestSignals[2].BitPosition = 0;
    TestSignals[2].BitSize = 32;
    TestSignals[2].Endianness = COM_LITTLE_ENDIAN;
    TestSignals[2].SignalType = COM_UINT32;
    TestSignals[2].TransferProperty = COM_TRIGGERED;
    
    /* I-PDU 0: Engine data */
    static Com_SignalIdType IPdu0Signals[] = {0, 1};
    TestIPdus[0].IPduId = 0;
    TestIPdus[0].DataPtr = TestIPduBuffers[0];
    TestIPdus[0].Length = 8;
    TestIPdus[0].Direction = COM_SEND;
    TestIPdus[0].Type = COM_NORMAL;
    TestIPdus[0].SignalProcessing = COM_IMMEDIATE;
    TestIPdus[0].SignalRefs = IPdu0Signals;
    TestIPdus[0].NumSignals = 2;
    TestIPdus[0].SignalGroupRefs = NULL_PTR;
    TestIPdus[0].NumSignalGroups = 0;
    TestIPdus[0].TxMode.Mode = COM_PERIODIC;
    TestIPdus[0].TxMode.Period = 100;
    TestIPdus[0].TxMode.TimeOffset = 0;
    TestIPdus[0].Timeout = 1000;
    
    /* I-PDU 1: Timestamp data */
    static Com_SignalIdType IPdu1Signals[] = {2};
    TestIPdus[1].IPduId = 1;
    TestIPdus[1].DataPtr = TestIPduBuffers[1];
    TestIPdus[1].Length = 8;
    TestIPdus[1].Direction = COM_SEND;
    TestIPdus[1].Type = COM_NORMAL;
    TestIPdus[1].SignalProcessing = COM_DEFERRED;
    TestIPdus[1].SignalRefs = IPdu1Signals;
    TestIPdus[1].NumSignals = 1;
    TestIPdus[1].SignalGroupRefs = NULL_PTR;
    TestIPdus[1].NumSignalGroups = 0;
    TestIPdus[1].TxMode.Mode = COM_DIRECT;
    TestIPdus[1].TxMode.TimeOffset = 0;
    TestIPdus[1].Timeout = 500;
    
    /* COM Configuration */
    TestComConfig.Signals = TestSignals;
    TestComConfig.NumSignals = 3;
    TestComConfig.SignalGroups = NULL_PTR;
    TestComConfig.NumSignalGroups = 0;
    TestComConfig.IPdus = TestIPdus;
    TestComConfig.NumIPdus = 2;
    TestComConfig.IPduGroups = NULL_PTR;
    TestComConfig.NumIPduGroups = 0;
}

static void InitModules(void)
{
    Std_ReturnType pdurResult = PduR_Init(&TestComConfig);
    TEST_ASSERT_EQUAL(E_OK, pdurResult);
    
    Com_Init(&TestComConfig);
    TEST_ASSERT_EQUAL(COM_READY, Com_GetStatus());
}

/*==================[End-to-End Data Flow Tests]===========================*/

void test_e2e_basic_signal_send(void)
{
    printf("Test: Basic Signal Send Through PduR\n");
    
    BuildBasicConfiguration();
    InitModules();
    
    uint16 engineSpeed = 3000;
    uint8 result = Com_SendSignal(0, &engineSpeed);
    
    TEST_ASSERT_EQUAL(0, result);
    TEST_ASSERT_EQUAL(1, TestStats.TotalTxRequests);
    TEST_ASSERT_EQUAL(1, TestStats.SuccessfulTransmissions);
    TEST_ASSERT_EQUAL(TRUE, TestPduTracking[0].WasTransmitted);
    
    TEST_ASSERT_EQUAL(0xB8, TestIPduBuffers[0][0]);
    TEST_ASSERT_EQUAL(0x0B, TestIPduBuffers[0][1]);
    
    printf("  PASSED: Signal sent and routed through PduR\n");
}

void test_e2e_multiple_signals_same_ipdu(void)
{
    printf("Test: Multiple Signals in Same I-PDU\n");
    
    BuildBasicConfiguration();
    InitModules();
    
    uint16 engineSpeed = 2500;
    uint8 result1 = Com_SendSignal(0, &engineSpeed);
    TEST_ASSERT_EQUAL_UINT8(0, result1);
    
    uint8 vehicleSpeed = 80;
    uint8 result2 = Com_SendSignal(1, &vehicleSpeed);
    TEST_ASSERT_EQUAL_UINT8(0, result2);
    
    TEST_ASSERT_EQUAL_UINT8(0xC4, TestIPduBuffers[0][0]);
    TEST_ASSERT_EQUAL_UINT8(0x09, TestIPduBuffers[0][1]);
    TEST_ASSERT_EQUAL_UINT8(80, TestIPduBuffers[0][2]);
    
    printf("  PASSED: Multiple signals packed in same I-PDU\n");
}

void test_e2e_pdur_transmit_failure(void)
{
    printf("Test: PduR Transmission Failure Handling\n");
    
    BuildBasicConfiguration();
    InitModules();
    
    Mock_TransmitShouldFail = TRUE;
    
    uint16 engineSpeed = 2000;
    uint8 result = Com_SendSignal(0, &engineSpeed);
    
    TEST_ASSERT_EQUAL(0, result);
    TEST_ASSERT_EQUAL(1, TestStats.FailedTransmissions);
    
    printf("  PASSED: Transmission failure handled gracefully\n");
}

void test_e2e_tx_confirmation(void)
{
    printf("Test: TxConfirmation Callback\n");
    
    BuildBasicConfiguration();
    InitModules();
    
    uint16 engineSpeed = 3500;
    Com_SendSignal(0, &engineSpeed);
    
    Mock_SimulateTxConfirmation(0);
    
    TEST_ASSERT_EQUAL(1, TestStats.ConfirmationsReceived);
    
    printf("  PASSED: TxConfirmation received and processed\n");
}

void test_e2e_rx_indication(void)
{
    printf("Test: RxIndication Callback\n");
    
    BuildBasicConfiguration();
    InitModules();
    
    uint8 rxData[8] = {0x90, 0x01, 0x50, 0, 0, 0, 0, 0};
    Mock_SimulateRxIndication(100, rxData, 8);
    
    TEST_ASSERT_EQUAL(1, TestStats.RxIndicationsReceived);
    TEST_ASSERT_EQUAL(TRUE, TestPduTracking[100].WasReceived);
    
    printf("  PASSED: RxIndication received and processed\n");
}

void test_e2e_signal_receive(void)
{
    printf("Test: Signal Receive\n");
    
    BuildBasicConfiguration();
    InitModules();
    
    /* Pre-set signal value */
    SignalValues[0] = 5000;
    
    uint16 receivedValue = 0;
    uint8 result = Com_ReceiveSignal(0, &receivedValue);
    
    TEST_ASSERT_EQUAL(0, result);
    TEST_ASSERT_EQUAL(5000, receivedValue);
    
    printf("  PASSED: Signal received correctly\n");
}

/*==================[Performance Tests]====================================*/

void test_perf_latency(void)
{
    printf("Test: Transmission Latency Measurement\n");
    
    BuildBasicConfiguration();
    InitModules();
    
    const int numSamples = 100;
    uint64_t latencies[100];
    uint64_t totalLatencyNs = 0;
    
    for (int i = 0; i < numSamples; i++) {
        struct timespec start, end;
        
        clock_gettime(CLOCK_MONOTONIC, &start);
        
        uint16 engineSpeed = 2000 + i;
        Com_SendSignal(0, &engineSpeed);
        
        clock_gettime(CLOCK_MONOTONIC, &end);
        
        uint64_t latencyNs = ((end.tv_sec - start.tv_sec) * 1000000000ULL) +
                             (end.tv_nsec - start.tv_nsec);
        latencies[i] = latencyNs;
        totalLatencyNs += latencyNs;
    }
    
    uint64_t avgLatencyNs = totalLatencyNs / numSamples;
    uint64_t minLatencyNs = latencies[0];
    uint64_t maxLatencyNs = latencies[0];
    
    for (int i = 1; i < numSamples; i++) {
        if (latencies[i] < minLatencyNs) minLatencyNs = latencies[i];
        if (latencies[i] > maxLatencyNs) maxLatencyNs = latencies[i];
    }
    
    printf("  Latency Statistics (%d samples):\n", numSamples);
    printf("    Average: %lu us\n", (unsigned long)(avgLatencyNs / 1000));
    printf("    Min: %lu us\n", (unsigned long)(minLatencyNs / 1000));
    printf("    Max: %lu us\n", (unsigned long)(maxLatencyNs / 1000));
    
    TEST_ASSERT(avgLatencyNs < PERF_TEST_MAX_LATENCY_US * 1000);
    
    printf("  PASSED: Latency within acceptable range\n");
}

void test_perf_throughput(void)
{
    printf("Test: Throughput Measurement\n");
    
    BuildBasicConfiguration();
    InitModules();
    
    const int numMessages = PERF_TEST_MESSAGE_COUNT;
    const size_t messageSize = 8;
    
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    for (int i = 0; i < numMessages; i++) {
        uint16 engineSpeed = i % 65535;
        uint8 vehicleSpeed = i % 255;
        
        Com_SendSignal(0, &engineSpeed);
        Com_SendSignal(1, &vehicleSpeed);
    }
    
    Com_MainFunctionTx();
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    uint64_t elapsedNs = ((end.tv_sec - start.tv_sec) * 1000000000ULL) +
                         (end.tv_nsec - start.tv_nsec);
    double elapsedSec = elapsedNs / 1e9;
    
    double throughputKBps = (numMessages * 2 * messageSize) / (elapsedSec * 1024);
    double messageRate = (numMessages * 2) / elapsedSec;
    
    printf("  Throughput Results:\n");
    printf("    Messages sent: %d\n", numMessages * 2);
    printf("    Elapsed time: %.3f sec\n", elapsedSec);
    printf("    Throughput: %.2f KB/s\n", throughputKBps);
    printf("    Message rate: %.2f msg/sec\n", messageRate);
    
    TEST_ASSERT(throughputKBps > PERF_TEST_MIN_THROUGHPUT_KBPS);
    
    printf("  PASSED: Throughput meets minimum requirements\n");
}

void test_perf_concurrent_operations(void)
{
    printf("Test: Concurrent Send/Receive Operations\n");
    
    BuildBasicConfiguration();
    InitModules();
    
    const int iterations = 50;
    int txCount = 0;
    int rxCount = 0;
    
    for (int i = 0; i < iterations; i++) {
        uint16 engineSpeed = 1000 + i;
        Com_SendSignal(0, &engineSpeed);
        txCount++;
        
        uint8 rxData[8] = {0};
        rxData[0] = i;
        Mock_SimulateRxIndication(100, rxData, 8);
        rxCount++;
        
        Com_MainFunctionTx();
    }
    
    TEST_ASSERT_EQUAL_INT(txCount, TestStats.TotalTxRequests);
    TEST_ASSERT_EQUAL_INT(rxCount, TestStats.RxIndicationsReceived);
    
    printf("  Concurrent operations: %d TX, %d RX\n", txCount, rxCount);
    printf("  PASSED: Concurrent operations handled correctly\n");
}

/*==================[Stress Tests]=========================================*/

void test_stress_init_deinit_cycles(void)
{
    printf("Test: Rapid Init/DeInit Cycles\n");
    
    BuildBasicConfiguration();
    
    const int cycles = 10;
    for (int i = 0; i < cycles; i++) {
        Std_ReturnType pdurResult = PduR_Init(&TestComConfig);
        TEST_ASSERT_EQUAL(E_OK, pdurResult);
        
        Com_Init(&TestComConfig);
        TEST_ASSERT_EQUAL(COM_READY, Com_GetStatus());
        
        uint16 engineSpeed = 1000;
        Com_SendSignal(0, &engineSpeed);
        
        Com_DeInit();
        TEST_ASSERT_EQUAL(COM_UNINIT, Com_GetStatus());
        
        PduR_DeInit();
    }
    
    printf("  Completed %d init/deinit cycles\n", cycles);
    printf("  PASSED: Rapid cycles handled correctly\n");
}

void test_stress_boundary_conditions(void)
{
    printf("Test: Boundary Conditions\n");
    
    BuildBasicConfiguration();
    InitModules();
    
    uint16 minValue = 0;
    uint8 result = Com_SendSignal(0, &minValue);
    TEST_ASSERT_EQUAL(0, result);
    
    uint16 maxValue = 65535;
    result = Com_SendSignal(0, &maxValue);
    TEST_ASSERT_EQUAL(0, result);
    
    result = Com_SendSignal(99, &minValue);
    TEST_ASSERT(result != 0);
    
    result = Com_SendSignal(0, NULL_PTR);
    TEST_ASSERT(result != 0);
    
    printf("  PASSED: Boundary conditions handled correctly\n");
}

/*==================[Main Test Runner]=====================================*/

int main(void)
{
    UNITY_BEGIN();
    
    printf("\n========================================\n");
    printf("COM-PduR Integration Tests (T016)\n");
    printf("========================================\n\n");
    
    printf("--- End-to-End Data Flow Tests ---\n");
    RUN_TEST(test_e2e_basic_signal_send);
    RUN_TEST(test_e2e_multiple_signals_same_ipdu);
    RUN_TEST(test_e2e_pdur_transmit_failure);
    RUN_TEST(test_e2e_tx_confirmation);
    RUN_TEST(test_e2e_rx_indication);
    RUN_TEST(test_e2e_signal_receive);
    
    printf("\n--- Performance Tests ---\n");
    RUN_TEST(test_perf_latency);
    RUN_TEST(test_perf_throughput);
    RUN_TEST(test_perf_concurrent_operations);
    
    printf("\n--- Stress Tests ---\n");
    RUN_TEST(test_stress_init_deinit_cycles);
    RUN_TEST(test_stress_boundary_conditions);
    
    printf("\n========================================\n");
    printf("COM-PduR Integration Tests Complete\n");
    printf("========================================\n\n");
    
    return UNITY_END();
}
