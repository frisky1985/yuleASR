/**
 * @file tsn_stack.h
 * @brief 最小桩 — 供 dds_tsn_transport 单元测试使用 (tests/unit/ddstrans)
 *
 * dds_tsn_transport.c 中对 TSN 协议栈的调用均为注释占位
 * ("注: 实际实现需要调用..."), 未引用任何 tsn_stack 符号。
 * 真实 src/tsn/tsn_stack.h 会拖入 gptp/tas/cbs/srp 整栈依赖，
 * host 单测无需；通过 include-dir 重定向使
 * "../../../tsn/tsn_stack.h" 命中本桩而非真实头。
 */

#ifndef DDS_TEST_TSN_STACK_STUB_H
#define DDS_TEST_TSN_STACK_STUB_H

/* 空桩: 被测代码未引用任何符号 */

#endif /* DDS_TEST_TSN_STACK_STUB_H */
