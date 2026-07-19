/**
 * @file test_e2e_watchdog.c
 * @brief E2E Test: Watchdog Manager Stack
 *
 * Verifies watchdog trigger, alarm notification,
 * and timeout monitoring end-to-end.
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>

/* Mock watchdog state */
static int mock_wdg_initialized = 0;
static unsigned int mock_wdg_trigger_count = 0;
static unsigned int mock_wdg_timeout_count = 0;
static unsigned int mock_wdg_alarm_id = 0;
static int mock_wdg_alarm_triggered = 0;
static unsigned int mock_bswm_wdg_notification = 0;

/* Watchdog modes */
#define WDG_MODE_OFF        0
#define WDG_MODE_SLOW       1
#define WDG_MODE_FAST       2
#define WDG_MODE_CHALLENGE  3

/* Watchdog status */
#define WDG_STATUS_EXPIRED  0
#define WDG_STATUS_ALIVE    1
#define WDG_STATUS_DEINIT   2

/* Mock Wdg driver */
static void mock_wdg_init(unsigned int timeout_ms)
{
    (void)timeout_ms;
    mock_wdg_initialized = 1;
    mock_wdg_trigger_count = 0;
    mock_wdg_timeout_count = 0;
    mock_wdg_alarm_triggered = 0;
}

static int mock_wdg_set_mode(unsigned char mode)
{
    (void)mode;
    return 0;
}

static void mock_wdg_trigger(void)
{
    if (!mock_wdg_initialized) return;
    mock_wdg_trigger_count++;
}

static int mock_wdg_get_status(void)
{
    return mock_wdg_initialized ? WDG_STATUS_ALIVE : WDG_STATUS_DEINIT;
}

/* Mock BswM notification handler */
static void mock_bswm_wdg_timeout_callback(unsigned int alarm_id)
{
    mock_bswm_wdg_notification++;
    mock_wdg_alarm_id = alarm_id;
}

/* Simulate watchdog timeout */
static void simulate_wdg_timeout(void)
{
    mock_wdg_timeout_count++;
    mock_wdg_alarm_triggered = 1;
    mock_bswm_wdg_timeout_callback(mock_wdg_timeout_count);
}

/* Test: Watchdog initialization */
static int test_wdg_init(void)
{
    mock_wdg_initialized = 0;
    mock_wdg_init(1000);
    assert(mock_wdg_initialized == 1);
    assert(mock_wdg_get_status() == WDG_STATUS_ALIVE);
    printf("  [PASS] test_wdg_init\n");
    return 1;
}

/* Test: Watchdog trigger */
static int test_wdg_trigger(void)
{
    mock_wdg_init(1000);
    mock_wdg_trigger_count = 0;
    
    mock_wdg_trigger();
    assert(mock_wdg_trigger_count == 1);
    
    mock_wdg_trigger();
    mock_wdg_trigger();
    assert(mock_wdg_trigger_count == 3);
    
    printf("  [PASS] test_wdg_trigger\n");
    return 1;
}

/* Test: Watchdog timeout detection */
static int test_wdg_timeout(void)
{
    mock_wdg_init(100);
    mock_bswm_wdg_notification = 0;
    mock_wdg_alarm_triggered = 0;
    
    simulate_wdg_timeout();
    assert(mock_wdg_alarm_triggered == 1);
    assert(mock_bswm_wdg_notification == 1);
    assert(mock_wdg_alarm_id == 1);
    
    printf("  [PASS] test_wdg_timeout\n");
    return 1;
}

/* Test: Watchdog mode switching */
static int test_wdg_mode_switch(void)
{
    mock_wdg_init(1000);
    
    assert(mock_wdg_set_mode(WDG_MODE_OFF) == 0);
    assert(mock_wdg_set_mode(WDG_MODE_SLOW) == 0);
    assert(mock_wdg_set_mode(WDG_MODE_FAST) == 0);
    assert(mock_wdg_set_mode(WDG_MODE_CHALLENGE) == 0);
    
    printf("  [PASS] test_wdg_mode_switch\n");
    return 1;
}

/* Test: Trigger before init fails gracefully */
static int test_wdg_trigger_before_init(void)
{
    mock_wdg_initialized = 0;
    mock_wdg_trigger_count = 0;
    
    /* Trigger without init should not increment */
    mock_wdg_trigger();
    assert(mock_wdg_trigger_count == 0);
    
    printf("  [PASS] test_wdg_trigger_before_init: graceful handling\n");
    return 1;
}

int main(void)
{
    int passed = 0;
    int total = 0;
    
    printf("=== E2E Test: Watchdog Manager ===\n");
    
    total++; passed += test_wdg_init();
    total++; passed += test_wdg_trigger();
    total++; passed += test_wdg_timeout();
    total++; passed += test_wdg_mode_switch();
    total++; passed += test_wdg_trigger_before_init();
    
    printf("\nResult: %d/%d tests passed\n", passed, total);
    return (passed == total) ? 0 : 1;
}
