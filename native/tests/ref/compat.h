/* Driver-only ABI bridge. Algorithms are always linked from the real modules. */
#ifndef REF_COMPAT_H
#define REF_COMPAT_H
#ifdef REF_NATIVE
#include "dazzle.h"
#include "dzconst.h"
#define far
#define huge
#define farmalloc malloc
#define farfree free
typedef uint16_t RefWord;
typedef int16_t RefInt;
typedef uint32_t RefLong;
#else
#include <alloc.h>
#include "dazzle.h"
#include "dzconst.h"
typedef unsigned RefWord;
typedef int RefInt;
typedef unsigned long RefLong;
#endif
#endif
