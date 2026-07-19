/**
 * @file Compiler.h
 * @brief AUTOSAR Compiler Abstraction (Native Stub)
 *
 * Minimal stub for native (x86_64/Darwin) compilation.
 */
#ifndef COMPILER_H
#define COMPILER_H

/* NULL_PTR */
#ifndef NULL_PTR
#define NULL_PTR ((void *)0)
#endif

/* AUTOSAR compiler keywords (no-ops for GCC/Clang) */
#define AUTOMATIC
#define TYPEDEF
#define FUNC(ret, memclass) ret
#define P2VAR(ptr, memclass, ptrclass) ptr *
#define P2CONST(ptr, memclass, ptrclass) const ptr *
#define CONST(consttype, memclass) const consttype
#define VAR(vartype, memclass) vartype
#define STATIC static
#define INLINE inline
#define LOCAL_INLINE static inline

/* Memory mapping macros (no-ops for native) */
#define MEMMAP_ERROR

/* Module ID types */
#define MODULE_ID 0

#endif /* COMPILER_H */
