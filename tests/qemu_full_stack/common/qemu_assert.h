#ifndef QEMU_ASSERT_H
#define QEMU_ASSERT_H

#include <stdint.h>
#include <stdbool.h>

#define QEMU_PASS_MARKER  "QEMU_FULL_STACK_PASS"
#define QEMU_FAIL_MARKER  "QEMU_FULL_STACK_FAIL"

void Qemu_ReportPass(void);
void Qemu_ReportFail(const char *msg);
void Qemu_Assert(bool cond, const char *msg);

#endif
