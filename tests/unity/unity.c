/* Unity - Simple Unit Testing for C
 * Implementation file
 */

#include "unity.h"
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>

/* 全局Unity实例 */
Unity_t Unity;

/* 内部辅助函数 */
static void UnityPrintChar(const char ch)
{
    UNITY_OUTPUT_CHAR(ch);
}

void UnityPrint(const char* string)
{
    if (string == NULL) {
        UnityPrint("(null)");
        return;
    }
    while (*string) {
        UNITY_OUTPUT_CHAR(*string++);
    }
}

void UnityPrintLen(const char* string, int length)
{
    while (*string && (length-- > 0)) {
        UNITY_OUTPUT_CHAR(*string++);
    }
}

void UnityPrintNumberInt(int number)
{
    char buffer[16];
    int i = 14;
    int is_negative = (number < 0);
    unsigned int n = is_negative ? -number : number;

    buffer[15] = '\0';
    do {
        buffer[i--] = (char)('0' + (n % 10));
        n /= 10;
    } while (n > 0);

    if (is_negative) {
        buffer[i--] = '-';
    }

    UnityPrint(&buffer[i + 1]);
}

void UnityPrintNumberUnsigned(unsigned int number)
{
    char buffer[16];
    int i = 14;

    buffer[15] = '\0';
    do {
        buffer[i--] = (char)('0' + (number % 10));
        number /= 10;
    } while (number > 0);

    UnityPrint(&buffer[i + 1]);
}

void UnityPrintNumberHex(unsigned int number, int nibbles)
{
    char buffer[9];
    int i = 7;
    const char hex_digits[] = "0123456789ABCDEF";

    buffer[8] = '\0';
    do {
        buffer[i--] = hex_digits[number & 0x0F];
        number >>= 4;
    } while ((number > 0) || (i >= (8 - nibbles)));

    UnityPrint("0x");
    UnityPrint(&buffer[i + 1]);
}

/* 测试框架核心函数 */
void UnityBegin(const char* filename)
{
    Unity.TestFile = 1;
    Unity.CurrentTestName = NULL;
    Unity.CurrentDetail1 = NULL;
    Unity.CurrentDetail2 = NULL;
    Unity.TestFailed = 0;
    Unity.TestIgnored = 0;

    UnityPrint("\n========== Unity Test Framework ==========\n");
    UnityPrint("Running tests from: ");
    UnityPrint(filename);
    UNITY_PRINT_EOL();
    UNITY_PRINT_EOL();
}

static int UnityTestResults = 0;

int UnityEnd(void)
{
    UNITY_PRINT_EOL();
    UnityPrint("==========================================\n");
    if (UnityTestResults == 0) {
        UnityPrint("All tests passed!\n");
    } else {
        UnityPrintNumberInt(UnityTestResults);
        UnityPrint(" test(s) failed.\n");
    }
    UnityPrint("==========================================\n");
    return UnityTestResults;
}

void UnityDefaultTestRun(void (*func)(void), const char* name, int line)
{
    Unity.CurrentTestName = name;
    Unity.TestFailed = 0;
    Unity.TestIgnored = 0;

    UnityPrint("TEST: ");
    UnityPrint(name);
    UnityPrint(" ... ");

    setUp();

    if (setjmp(Unity.AbortFrame) == 0) {
        func();
    }

    tearDown();

    if (Unity.TestIgnored) {
        UnityPrint("IGNORED");
        UNITY_PRINT_EOL();
    } else if (Unity.TestFailed) {
        UnityTestResults++;
        UnityPrint("FAILED");
        UNITY_PRINT_EOL();
    } else {
        UnityPrint("PASSED");
        UNITY_PRINT_EOL();
    }
}

/* 断言实现 */
void UnityFail(const char* message, unsigned short line)
{
    UNITY_PRINT_EOL();
    UnityPrint("  FAIL at line ");
    UnityPrintNumberInt(line);
    if (message != NULL) {
        UnityPrint(": ");
        UnityPrint(message);
    }
    UNITY_PRINT_EOL();
    Unity.TestFailed = 1;
    longjmp(Unity.AbortFrame, 1);
}

void UnityIgnore(const char* message, unsigned short line)
{
    UNITY_PRINT_EOL();
    UnityPrint("  IGNORED at line ");
    UnityPrintNumberInt(line);
    if (message != NULL) {
        UnityPrint(": ");
        UnityPrint(message);
    }
    UNITY_PRINT_EOL();
    Unity.TestIgnored = 1;
    longjmp(Unity.AbortFrame, 1);
}

void UnityAssertNull(const void* pointer, const char* msg, unsigned short line)
{
    if (pointer != NULL) {
        UnityPrint("  Expected NULL, but was ");
        UnityPrintNumberHex((unsigned int)(uintptr_t)pointer, 8);
        UnityFail(msg, line);
    }
}

void UnityAssertNotNull(const void* pointer, const char* msg, unsigned short line)
{
    if (pointer == NULL) {
        UnityPrint("  Expected Non-NULL");
        UnityFail(msg, line);
    }
}

void UnityAssertEqualNumber(int expected, int actual, const char* msg, unsigned short line, char style)
{
    if (expected != actual) {
        UnityPrint("  Expected: ");
        if (style == 'x') {
            UnityPrintNumberHex((unsigned int)expected, 8);
        } else if (style == 'u') {
            UnityPrintNumberUnsigned((unsigned int)expected);
        } else {
            UnityPrintNumberInt(expected);
        }
        UnityPrint(" Actual: ");
        if (style == 'x') {
            UnityPrintNumberHex((unsigned int)actual, 8);
        } else if (style == 'u') {
            UnityPrintNumberUnsigned((unsigned int)actual);
        } else {
            UnityPrintNumberInt(actual);
        }
        UnityFail(msg, line);
    }
}

void UnityAssertEqualIntArray(const int* expected, const int* actual, unsigned int num_elements, const char* msg, unsigned short line, char style)
{
    unsigned int i;
    for (i = 0; i < num_elements; i++) {
        if (expected[i] != actual[i]) {
            UnityPrint("  Element ");
            UnityPrintNumberUnsigned(i);
            UnityPrint(" Expected: ");
            if (style == 'x') {
                UnityPrintNumberHex((unsigned int)expected[i], 8);
            } else if (style == 'u') {
                UnityPrintNumberUnsigned((unsigned int)expected[i]);
            } else {
                UnityPrintNumberInt(expected[i]);
            }
            UnityPrint(" Actual: ");
            if (style == 'x') {
                UnityPrintNumberHex((unsigned int)actual[i], 8);
            } else if (style == 'u') {
                UnityPrintNumberUnsigned((unsigned int)actual[i]);
            } else {
                UnityPrintNumberInt(actual[i]);
            }
            UnityFail(msg, line);
            return;
        }
    }
}

void UnityAssertEqualString(const char* expected, const char* actual, const char* msg, unsigned short line)
{
    const char* exp = expected;
    const char* act = actual;

    if (expected == NULL) {
        UnityFail("Expected string is NULL", line);
    }
    if (actual == NULL) {
        UnityFail("Actual string is NULL", line);
    }

    while (*exp && (*exp == *act)) {
        exp++;
        act++;
    }

    if (*exp || *act) {
        UnityPrint("  Expected: \"");
        UnityPrint(expected);
        UnityPrint("\" Actual: \"");
        UnityPrint(actual);
        UnityPrint("\"");
        UnityFail(msg, line);
    }
}

void UnityAssertEqualMemory(const void* expected, const void* actual, unsigned int length, const char* msg, unsigned short line)
{
    const unsigned char* exp = (const unsigned char*)expected;
    const unsigned char* act = (const unsigned char*)actual;
    unsigned int i;

    if (expected == NULL || actual == NULL) {
        UnityFail("Memory pointer is NULL", line);
    }

    for (i = 0; i < length; i++) {
        if (exp[i] != act[i]) {
            UnityPrint("  Byte ");
            UnityPrintNumberUnsigned(i);
            UnityPrint(" Expected: 0x");
            UnityPrintNumberHex(exp[i], 2);
            UnityPrint(" Actual: 0x");
            UnityPrintNumberHex(act[i], 2);
            UnityFail(msg, line);
            return;
        }
    }
}

void UnityAssertBits(unsigned int mask, unsigned int expected, unsigned int actual, const char* msg, unsigned short line)
{
    if ((expected & mask) != (actual & mask)) {
        UnityPrint("  Mask: 0x");
        UnityPrintNumberHex(mask, 8);
        UnityPrint(" Expected: 0x");
        UnityPrintNumberHex(expected & mask, 8);
        UnityPrint(" Actual: 0x");
        UnityPrintNumberHex(actual & mask, 8);
        UnityFail(msg, line);
    }
}

/* 默认的setUp和tearDown */
__attribute__((weak)) void setUp(void) {}
__attribute__((weak)) void tearDown(void) {}
