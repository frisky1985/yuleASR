/******************************************************************************
 * @file Compiler.h
 * @brief Compiler Abstraction (AutoSAR) - Simulated
 * @details Provides compiler-independent macros for AutoSAR implementation.
 *          This is a minimal stub for compilation purposes.
 ******************************************************************************/

#ifndef COMPILER_H
#define COMPILER_H

/*******************************************************************************
 * Standard compiler abstraction macros
 ******************************************************************************/

/* NULL pointer definition */
#ifndef NULL_PTR
    #define NULL_PTR    ((void*)0)
#endif

/* Function definition macro */
#define FUNC(rettype, memclass)          rettype

/* Variable definition macros */
#define P2VAR(ptrtype, memclass, ptrclass)    ptrtype *
#define P2CONST(ptrtype, memclass, ptrclass)  const ptrtype *
#define CONST(consttype, memclass)            const consttype
#define VAR(vartype, memclass)                vartype

/* Static and inline macros */
#ifndef STATIC
    #define STATIC      static
#endif

#ifndef INLINE
    #define INLINE      inline
#endif

/* Module definition macros */
#define MODULE_ID          0U

/* Vendor ID */
#define VENDOR_ID          0x0000U

/*******************************************************************************
 * Memory section abstraction
 ******************************************************************************/
#define AUTOMATIC
#define TYPEDEF

/* Boolean type */
#ifndef boolean
    #define boolean     unsigned char
#endif

#endif /* COMPILER_H */
