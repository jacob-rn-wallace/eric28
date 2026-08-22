/*
 *   malloc.h
 *
 *   Part of the Emu28 macOS/Linux port's compat/ shim directory (see
 *   ../../CLAUDE.md). MSVC's malloc.h just re-exports the standard
 *   allocation functions that live in <stdlib.h> on POSIX (and, unlike
 *   MSVC, macOS/BSD has no top-level malloc.h at all - it's
 *   <malloc/malloc.h>) - so satisfy
 *   vendor/emu28-upstream/PCH.H's `#include <malloc.h>` by forwarding
 *   there instead.
 */

#ifndef SHIM_COMPAT_MALLOC_H
#define SHIM_COMPAT_MALLOC_H

#include <stdlib.h>

#endif /* SHIM_COMPAT_MALLOC_H */
