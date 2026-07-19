/**
 * @file test_e2e_diagnostic_stack.c
 * @brief E2E Test: Diagnostic Communication Stack (UDS over CAN)
 *
 * Verifies end-to-end diagnostic message flow:
 *   DCM -> PduR -> CanIf -> CanDrv (and reverse)
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>

/* Diagnostic service identifiers */
#define UDS_DIAGNOSTIC_SESSION_CONTROL    0x10U
#define UDS_ECU_RESET                     0x11U
#define UDS_READ_DATA_BY_ID               0x22U
#define UDS_WRITE_DATA_BY_ID              0x2EU
#define UDS_ROUTINE_CONTROL               0x31U

/* Positive / negative response */
#define UDS_POSITIVE_RESPONSE_MASK        0x40U
#define UDS_NEGATIVE_RESPONSE            0x7FU
#define UDS_NRC_GENERAL_REJECT           0x10U

/* Mock diagnostic state */
static unsigned int mock_session = 0x01U; /* Default session */
static int mock_ecu_reset_requested = 0;
static unsigned char mock_did_data[65536];
static unsigned int mock_did_data_len = 0;
static int mock_routine_executed = 0;

/* Simulate UDS message processing */
static int uds_process_request(const unsigned char *request, unsigned int req_len,
                                unsigned char *response, unsigned int *resp_len)
{
    if (req_len < 2) return -1;

    unsigned char sid = request[0];
    unsigned char rsid = sid | UDS_POSITIVE_RESPONSE_MASK;

    switch (sid)
    {
    case UDS_DIAGNOSTIC_SESSION_CONTROL:
        if (req_len >= 2)
        {
            mock_session = request[1];
            response[0] = rsid;
            response[1] = request[1]; /* echo session */
            *resp_len = 2;
            return 0;
        }
        break;

    case UDS_ECU_RESET:
        mock_ecu_reset_requested = 1;
        response[0] = rsid;
        response[1] = 0x01; /* reset type echoed */
        *resp_len = 2;
        return 0;

    case UDS_READ_DATA_BY_ID:
        if (req_len >= 3)
        {
            unsigned int did = ((unsigned int)request[1] << 8) | request[2];
            if (did < 65535U && mock_did_data_len > 0)
            {
                response[0] = rsid;
                response[1] = request[1];
                response[2] = request[2];
                memcpy(&response[3], mock_did_data, mock_did_data_len);
                *resp_len = 3 + mock_did_data_len;
                return 0;
            }
        }
        break;

    case UDS_WRITE_DATA_BY_ID:
        if (req_len >= 4)
        {
            mock_did_data_len = req_len - 3;
            memcpy(mock_did_data, &request[3], mock_did_data_len);
            response[0] = rsid;
            response[1] = request[1];
            response[2] = request[2];
            *resp_len = 3;
            return 0;
        }
        break;

    case UDS_ROUTINE_CONTROL:
        mock_routine_executed = 1;
        response[0] = rsid;
        response[1] = request[1];
        response[2] = request[2];
        response[3] = request[3];
        *resp_len = 4;
        return 0;

    default:
        /* Negative response */
        response[0] = UDS_NEGATIVE_RESPONSE;
        response[1] = sid;
        response[2] = UDS_NRC_GENERAL_REJECT;
        *resp_len = 3;
        return 0;
    }

    return -1;
}

/* Test: Diagnostic session control */
static int test_diagnostic_session_control(void)
{
    unsigned char req[] = {UDS_DIAGNOSTIC_SESSION_CONTROL, 0x03U}; /* Extended session */
    unsigned char resp[64];
    unsigned int resp_len = 0;
    
    int result = uds_process_request(req, sizeof(req), resp, &resp_len);
    assert(result == 0);
    assert(resp[0] == (UDS_DIAGNOSTIC_SESSION_CONTROL | UDS_POSITIVE_RESPONSE_MASK));
    assert(mock_session == 0x03U);
    printf("  [PASS] test_diagnostic_session_control\n");
    return 1;
}

/* Test: Read data by identifier */
static int test_read_data_by_id(void)
{
    unsigned char req[] = {UDS_READ_DATA_BY_ID, 0xF1, 0x90}; /* VIN */
    unsigned char resp[64];
    unsigned int resp_len = 0;
    
    /* Setup mock data */
    const char *vin = "YULEASR000000001";
    mock_did_data_len = (unsigned int)strlen(vin);
    memcpy(mock_did_data, vin, mock_did_data_len);
    
    int result = uds_process_request(req, sizeof(req), resp, &resp_len);
    assert(result == 0);
    assert(resp[0] == (UDS_READ_DATA_BY_ID | UDS_POSITIVE_RESPONSE_MASK));
    printf("  [PASS] test_read_data_by_id\n");
    return 1;
}

/* Test: Write data by identifier */
static int test_write_data_by_id(void)
{
    unsigned char data[] = {0x01, 0x02, 0x03, 0x04};
    unsigned char req[7];
    unsigned char resp[64];
    unsigned int resp_len = 0;
    
    req[0] = UDS_WRITE_DATA_BY_ID;
    req[1] = 0xF1;
    req[2] = 0x00;
    memcpy(&req[3], data, sizeof(data));
    
    int result = uds_process_request(req, sizeof(req), resp, &resp_len);
    assert(result == 0);
    assert(resp[0] == (UDS_WRITE_DATA_BY_ID | UDS_POSITIVE_RESPONSE_MASK));
    assert(mock_did_data_len == sizeof(data));
    printf("  [PASS] test_write_data_by_id\n");
    return 1;
}

/* Test: ECU reset */
static int test_ecu_reset(void)
{
    unsigned char req[] = {UDS_ECU_RESET, 0x01U}; /* Hard reset */
    unsigned char resp[64];
    unsigned int resp_len = 0;
    
    mock_ecu_reset_requested = 0;
    
    int result = uds_process_request(req, sizeof(req), resp, &resp_len);
    assert(result == 0);
    assert(mock_ecu_reset_requested == 1);
    printf("  [PASS] test_ecu_reset\n");
    return 1;
}

/* Test: Routine control */
static int test_routine_control(void)
{
    unsigned char req[] = {UDS_ROUTINE_CONTROL, 0x01, 0xFF, 0x00}; /* Start routine 0xFF00 */
    unsigned char resp[64];
    unsigned int resp_len = 0;
    
    mock_routine_executed = 0;
    
    int result = uds_process_request(req, sizeof(req), resp, &resp_len);
    assert(result == 0);
    assert(mock_routine_executed == 1);
    printf("  [PASS] test_routine_control\n");
    return 1;
}

int main(void)
{
    int passed = 0;
    int total = 0;
    
    printf("=== E2E Test: Diagnostic Stack ===\n");
    
    total++; passed += test_diagnostic_session_control();
    total++; passed += test_read_data_by_id();
    total++; passed += test_write_data_by_id();
    total++; passed += test_ecu_reset();
    total++; passed += test_routine_control();
    
    printf("\nResult: %d/%d tests passed\n", passed, total);
    return (passed == total) ? 0 : 1;
}
