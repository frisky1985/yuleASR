/* Compiler.h - AUTOSAR compiler abstraction stub */
#ifndef COMPILER_H
#define COMPILER_H

#define FUNC(returnType, memclass) returnType
#define VAR(type, memclass) type
#define CONST(type, memclass) const type
#define P2VAR(ptrType, memclass, ptrclass) ptrType*
#define P2CONST(ptrType, memclass, ptrclass) const ptrType*
#define P2FUNC(returnType, memclass, funcname) returnType (*funcname)

#define NULL_PTR ((void*)0)

#endif
