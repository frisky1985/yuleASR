/**
 * @file Compiler.h
 * @brief AUTOSAR Compiler Abstraction (Native Stub)
 *
 * Minimal stub for native (x86_64/Darwin) compilation.
 * All macros guarded with #ifndef to avoid Std_Types.h conflicts.
 */
#ifndef COMPILER_H
#define COMPILER_H

#ifndef NULL_PTR
#define NULL_PTR ((void *)0)
#endif

#ifndef AUTOMATIC
#define AUTOMATIC
#endif

#ifndef TYPEDEF
#define TYPEDEF
#endif

#ifndef FUNC
#define FUNC(ret, memclass) ret
#endif

#ifndef P2VAR
#define P2VAR(ptr, memclass, ptrclass) ptr *
#endif

#ifndef P2CONST
#define P2CONST(ptr, memclass, ptrclass) const ptr *
#endif

#ifndef CONST
#define CONST(consttype, memclass) const consttype
#endif

#ifndef VAR
#define VAR(vartype, memclass) vartype
#endif

#ifndef STATIC
#define STATIC static
#endif

#ifndef INLINE
#define INLINE inline
#endif

#ifndef LOCAL_INLINE
#define LOCAL_INLINE static inline
#endif

#ifndef MEMMAP_ERROR
#define MEMMAP_ERROR
#endif

#ifndef MODULE_ID
#define MODULE_ID 0
#endif

/* Memory-mapped register I/O macros (native stub - real HW access) */
#ifndef REG_READ32
#define REG_READ32(addr)                (*((volatile uint32*)(addr)))
#endif
#ifndef REG_WRITE32
#define REG_WRITE32(addr, val)          (*((volatile uint32*)(addr)) = (val))
#endif
#ifndef REG_READ8
#define REG_READ8(addr)                 (*((volatile uint8*)(addr)))
#endif
#ifndef REG_WRITE8
#define REG_WRITE8(addr, val)           (*((volatile uint8*)(addr)) = (val))
#endif

#endif /* COMPILER_H */
