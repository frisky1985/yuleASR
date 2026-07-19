/**
 * @file hello.c
 * @brief ARM SIL test executable for QEMU lm3s6965evb
 *
 * Minimal ARM test that outputs via UART and runs basic
 * algorithmic validation tests.
 * No standard library dependencies — fully freestanding.
 *
 * UART0 on lm3s6965evb at 0x4000C000.
 */

typedef unsigned char  uint8_t;
typedef unsigned int   uint32_t;

/* UART0 registers (QEMU lm3s6965evb) */
#define UART0_DATA       (*(volatile uint32_t *)0x4000C000U)
#define UART0_FLAG       (*(volatile uint32_t *)0x4000C018U)
#define UART0_IBRD       (*(volatile uint32_t *)0x4000C024U)
#define UART0_FBRD       (*(volatile uint32_t *)0x4000C028U)
#define UART0_LCRH       (*(volatile uint32_t *)0x4000C02CU)
#define UART0_CTL        (*(volatile uint32_t *)0x4000C030U)

/* Flag register bits */
#define UART_FLAG_TXFF   (1U << 5)

/* Test result indicators */
#define TEST_PASS          0U
#define TEST_FAIL          1U

/* Initialize UART0 (115200 baud @ 20MHz system clock) */
static void uart_init(void)
{
    /* Disable UART during config */
    UART0_CTL = 0U;

    /* Set baud rate: 20MHz / (16 * 115200) = 10.85 → IBRD=10, FBRD=14 */
    UART0_IBRD = 10U;
    UART0_FBRD = 14U;

    /* 8-bit, 1 stop, no parity, FIFO enabled */
    UART0_LCRH = (0x3U << 5) | (1U << 4);

    /* Enable UART with TX enabled */
    UART0_CTL = (1U << 0) | (1U << 8);
}

/* Send a single character via UART */
static void putchar_uart(char c)
{
    /* Wait for TX FIFO not full */
    while (UART0_FLAG & UART_FLAG_TXFF)
    {
        /* spin */
    }
    UART0_DATA = (uint32_t)(unsigned char)c;

    /* Handle newline carriage return */
    if (c == '\n')
    {
        while (UART0_FLAG & UART_FLAG_TXFF)
        {
            /* spin */
        }
        UART0_DATA = (uint32_t)'\r';
    }
}

/* Send a null-terminated string */
static void puts_uart(const char *str)
{
    while (*str)
    {
        putchar_uart(*str++);
    }
}

/* Memory compare (freestanding) */
static uint32_t my_memcmp(const void *s1, const void *s2, uint32_t n)
{
    const uint8_t *p1 = (const uint8_t *)s1;
    const uint8_t *p2 = (const uint8_t *)s2;
    uint32_t i;
    for (i = 0U; i < n; i++)
    {
        if (p1[i] != p2[i])
            return 1U;
    }
    return 0U;
}

/* Memory copy (freestanding) */
static void my_memcpy(void *dst, const void *src, uint32_t n)
{
    uint8_t *d = (uint8_t *)dst;
    const uint8_t *s = (const uint8_t *)src;
    uint32_t i;
    for (i = 0U; i < n; i++)
    {
        d[i] = s[i];
    }
}

/* Test: basic arithmetic */
static uint32_t test_arithmetic(void)
{
    uint32_t a = 42U;
    uint32_t b = 58U;
    return ((a + b) == 100U) ? TEST_PASS : TEST_FAIL;
}

/* Test: memory */
static uint32_t test_memory(void)
{
    const char expected[] = "Hello SIL World!";
    char actual[32];
    uint32_t i;
    for (i = 0U; i < sizeof(actual); i++) actual[i] = 0U;
    my_memcpy(actual, expected, sizeof(expected));
    return (my_memcmp(expected, actual, sizeof(expected)) == 0U) ? TEST_PASS : TEST_FAIL;
}

/* Test: bit manipulation */
static uint32_t test_bitops(void)
{
    uint32_t value = 0x0F0F0F0FU;
    value = (value << 4) | (value >> 28);
    return (value == 0xF0F0F0F0U) ? TEST_PASS : TEST_FAIL;
}

/* Test: loop counter */
static uint32_t test_loop(void)
{
    uint32_t count = 0U;
    uint32_t i;
    for (i = 0U; i < 100U; i++)
    {
        count++;
    }
    return (count == 100U) ? TEST_PASS : TEST_FAIL;
}

/* Test: alignment */
static uint32_t test_alignment(void)
{
    uint8_t buffer[16];
    uint32_t *ptr;
    uint32_t i;
    for (i = 0U; i < 16U; i++) buffer[i] = 0U;
    ptr = (uint32_t *)(buffer + 4);
    *ptr = 0xDEADBEEFU;
    return (*ptr == 0xDEADBEEFU) ? TEST_PASS : TEST_FAIL;
}

int main(void)
{
    uint32_t all_pass = 1U;

    uart_init();

    puts_uart("Hello from yuleOSH cross-compilation test!\n");

    all_pass = all_pass && (test_arithmetic() == TEST_PASS);
    all_pass = all_pass && (test_memory() == TEST_PASS);
    all_pass = all_pass && (test_bitops() == TEST_PASS);
    all_pass = all_pass && (test_loop() == TEST_PASS);
    all_pass = all_pass && (test_alignment() == TEST_PASS);

    if (all_pass)
    {
        puts_uart("SIL Test Suite: ALL PASS\n");
    }
    else
    {
        puts_uart("SIL Test Suite: SOME TESTS FAILED\n");
    }

    /* Indefinite loop after tests */
    while (1U)
    {
        __asm volatile("wfi");
    }

    return 0U;
}
