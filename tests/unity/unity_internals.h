/* Unity - Simple Unit Testing for C
 * Internal header file
 */

#ifndef UNITY_INTERNALS_H
#define UNITY_INTERNALS_H

#include <stdio.h>
#include <setjmp.h>
#include <stdint.h>
#include <stddef.h>

/* Unity配置 */
#ifndef UNITY_OUTPUT_CHAR
#define UNITY_OUTPUT_CHAR(a) putchar(a)
#endif

#ifndef UNITY_PRINT_EOL
#define UNITY_PRINT_EOL()    UNITY_OUTPUT_CHAR('\n')
#endif

#ifndef UNITY_DISPLAY_RANGE_UINT
#define UNITY_DISPLAY_RANGE_UINT 1
#endif

#ifndef UNITY_DISPLAY_RANGE_INT
#define UNITY_DISPLAY_RANGE_INT 1
#endif

#ifndef UNITY_DISPLAY_RANGE_HEX
#define UNITY_DISPLAY_RANGE_HEX 1
#endif

/* Unity测试状态结构 */
typedef struct _Unity {
    int TestFile;
    const char* CurrentTestName;
    const char* CurrentDetail1;
    const char* CurrentDetail2;
    unsigned char TestFailed;
    unsigned char TestIgnored;
    jmp_buf AbortFrame;
} Unity_t;

/* 全局Unity实例 */
extern Unity_t Unity;

/* 内部函数声明 */
void UnityPrint(const char* string);
void UnityPrintNumberByStyle(int number, char style);
void UnityPrintNumberInt(int number);
void UnityPrintNumberUnsigned(unsigned int number);
void UnityPrintNumberHex(unsigned int number, int nibbles);
void UnityPrintLen(const char* string, int length);
void UnityPrintMask(unsigned int mask, unsigned int number);
void UnityPrintFloat(float number);
void UnityPrintDouble(double number);

/* 断言实现 */
void UnityAssertBits(unsigned int mask, unsigned int expected, unsigned int actual, const char* msg, unsigned short line);
void UnityAssertEqualNumber(int expected, int actual, const char* msg, unsigned short line, char style);
void UnityAssertEqualIntArray(const int* expected, const int* actual, unsigned int num_elements, const char* msg, unsigned short line, char style);
void UnityAssertEqualString(const char* expected, const char* actual, const char* msg, unsigned short line);
void UnityAssertEqualMemory(const void* expected, const void* actual, unsigned int length, const char* msg, unsigned short line);
void UnityAssertEqualStringArray(const char** expected, const char** actual, unsigned int num_elements, const char* msg, unsigned short line);
void UnityAssertEqualStringLen(const char* expected, const char* actual, unsigned int length, const char* msg, unsigned short line);
void UnityAssertNull(const void* pointer, const char* msg, unsigned short line);
void UnityAssertNotNull(const void* pointer, const char* msg, unsigned short line);
void UnityFail(const char* message, unsigned short line);
void UnityIgnore(const char* message, unsigned short line);

/* 宏展开辅助 */
#define UNITY_TEST_ASSERT(condition, line, message) \
    do { if (!(condition)) { UnityFail((message), (line)); } } while (0)

#define UNITY_TEST_ASSERT_NULL(pointer, line, message) \
    UnityAssertNull((const void*)(pointer), (message), (line))

#define UNITY_TEST_ASSERT_NOT_NULL(pointer, line, message) \
    UnityAssertNotNull((const void*)(pointer), (message), (line))

#define UNITY_TEST_ASSERT_EQUAL_INT(expected, actual, line, message) \
    UnityAssertEqualNumber((int)(expected), (int)(actual), (message), (line), 'd')

#define UNITY_TEST_ASSERT_EQUAL_UINT(expected, actual, line, message) \
    UnityAssertEqualNumber((int)(expected), (int)(actual), (message), (line), 'u')

#define UNITY_TEST_ASSERT_EQUAL_HEX32(expected, actual, line, message) \
    UnityAssertEqualNumber((int)(expected), (int)(actual), (message), (line), 'x')

#define UNITY_TEST_ASSERT_EQUAL_STRING(expected, actual, line, message) \
    UnityAssertEqualString((const char*)(expected), (const char*)(actual), (message), (line))

#define UNITY_TEST_ASSERT_EQUAL_MEMORY(expected, actual, len, line, message) \
    UnityAssertEqualMemory((const void*)(expected), (const void*)(actual), (size_t)(len), (message), (line))

#define UNITY_TEST_ASSERT_EQUAL_INT_ARRAY(expected, actual, num_elements, line, message) \
    UnityAssertEqualIntArray((const int*)(expected), (const int*)(actual), (unsigned int)(num_elements), (message), (line), 'd')

#define UNITY_TEST_ASSERT_EQUAL_UINT_ARRAY(expected, actual, num_elements, line, message) \
    UnityAssertEqualIntArray((const int*)(expected), (const int*)(actual), (unsigned int)(num_elements), (message), (line), 'u')

#define UNITY_TEST_IGNORE(line, message) \
    UnityIgnore((message), (line))

#endif /* UNITY_INTERNALS_H */
