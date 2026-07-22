#ifndef COMPILER_STUB_H
#define COMPILER_STUB_H

/* Minimal compiler abstraction for testing */
#ifndef NULL_PTR
#define NULL_PTR ((void*)0)
#endif

#define FUNC(rettype, memclass)         rettype
#define VAR(vartype, memclass)          vartype
#define CONST(consttype, memclass)      const consttype
#define P2VAR(ptrtype, memclass, ...)   ptrtype*
#define P2CONST(ptrtype, memclass, ...) const ptrtype*

#endif /* COMPILER_STUB_H */
