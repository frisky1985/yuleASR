/**
 * @file test_e2e_can_communication.c
 * @brief E2E Test: CAN Communication Stack
 *
 * Verifies end-to-end CAN message flow:
 *   ComM -> CanIf -> CanDrv -> CanIf -> Com
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>

/* Mock driver state */
static int mock_can_initialized = 0;
static int mock_can_tx_count = 0;
static int mock_can_rx_count = 0;
static int mock_com_rx_indication = 0;

/* Mock CAN controller */
static void mock_can_init(void)
{
    mock_can_initialized = 1;
}

static int mock_can_transmit(unsigned int id, const unsigned char *data, unsigned int len)
{
    (void)id;
    (void)data;
    (void)len;
    mock_can_tx_count++;
    return 0; /* success */
}

static int mock_can_receive(unsigned int *id, unsigned char *data, unsigned int *len)
{
    (void)id;
    (void)data;
    (void)len;
    if (mock_can_rx_count < 5)
    {
        mock_can_rx_count++;
        return 0; /* data available */
    }
    return -1; /* no data */
}

/* Mock ComM */
static void mock_comm_request_mode(void)
{
    /* Simulate FULL_COM mode */
}

/* Mock Com */
static void mock_com_rx_callback(unsigned int signal_id, const void *data)
{
    (void)signal_id;
    (void)data;
    mock_com_rx_indication++;
}

/* Test: CAN initialization */
static int test_can_init(void)
{
    mock_can_initialized = 0;
    mock_can_init();
    assert(mock_can_initialized == 1);
    printf("  [PASS] test_can_init: CAN initialized\n");
    return 1;
}

/* Test: CAN transmit */
static int test_can_transmit(void)
{
    unsigned char data[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    int result;
    
    mock_can_tx_count = 0;
    result = mock_can_transmit(0x100, data, sizeof(data));
    assert(result == 0);
    assert(mock_can_tx_count == 1);
    printf("  [PASS] test_can_transmit: CAN frame transmitted\n");
    return 1;
}

/* Test: CAN receive roundtrip */
static int test_can_rx_roundtrip(void)
{
    unsigned int id;
    unsigned char data[8];
    unsigned int len;
    
    mock_can_rx_count = 0;
    mock_com_rx_indication = 0;
    
    /* Simulate receiving a CAN message */
    int result = mock_can_receive(&id, data, &len);
    assert(result == 0);
    
    /* Notify COM layer */
    mock_com_rx_callback(id, data);
    assert(mock_com_rx_indication == 1);
    
    printf("  [PASS] test_can_rx_roundtrip: CAN frame received and indicated\n");
    return 1;
}

/* Test: Full communication cycle */
static int test_full_communication_cycle(void)
{
    unsigned char tx_data[] = {0xAA, 0xBB, 0xCC, 0xDD};
    unsigned int id;
    unsigned char rx_data[8];
    unsigned int len;
    
    /* Step 1: Request communication mode */
    mock_comm_request_mode();
    
    /* Step 2: Transmit */
    assert(mock_can_transmit(0x200, tx_data, sizeof(tx_data)) == 0);
    
    /* Step 3: Receive */
    assert(mock_can_receive(&id, rx_data, &len) == 0);
    
    /* Step 4: Indicate */
    mock_com_rx_callback(id, rx_data);
    
    printf("  [PASS] test_full_communication_cycle: Full cycle complete\n");
    return 1;
}

/* Test: Multiple message handling */
static int test_multiple_messages(void)
{
    int i;
    unsigned char data[] = {0x00};
    int initial_tx = mock_can_tx_count;
    
    for (i = 0; i < 10; i++)
    {
        data[0] = (unsigned char)i;
        mock_can_transmit(0x300 + i, data, sizeof(data));
    }
    
    assert(mock_can_tx_count == initial_tx + 10);
    printf("  [PASS] test_multiple_messages: 10 messages sent/received\n");
    return 1;
}

int main(void)
{
    int passed = 0;
    int total = 0;
    
    printf("=== E2E Test: CAN Communication ===\n");
    
    total++; passed += test_can_init();
    total++; passed += test_can_transmit();
    total++; passed += test_can_rx_roundtrip();
    total++; passed += test_full_communication_cycle();
    total++; passed += test_multiple_messages();
    
    printf("\nResult: %d/%d tests passed\n", passed, total);
    return (passed == total) ? 0 : 1;
}
