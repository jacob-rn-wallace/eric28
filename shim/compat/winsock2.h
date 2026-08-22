/*
 *   winsock2.h
 *
 *   Part of the Emu28 macOS/Linux port's compat/ shim directory (see
 *   ../../CLAUDE.md). Stands in for the real Win32 SDK header so that
 *   vendor/emu28-upstream/PCH.H's `#include <winsock2.h>` resolves on
 *   macOS/Linux (UDP.C's real socket use is implemented for real in
 *   ../win32_types.h).
 *
 *   Also where _MSC_VER gets defined - it has to happen somewhere on
 *   the vendor-compile path before PCH.H's own `#if _MSC_VER <= 1200`
 *   guards run (see ../win32_types.h's note on why it isn't defined
 *   there), and winsock2.h is the first angle-bracket header PCH.H
 *   includes, so here it is. Left undefined, `#if _MSC_VER <= 1200`
 *   evaluates as `#if 0 <= 1200` - true - which silently selects
 *   PCH.H's *oldest* fallback branches (e.g. the MSVC6-era `i64`
 *   integer-literal suffix, which gcc/clang reject outright) instead
 *   of skipping them. Claiming a recent MSVC version makes PCH.H's
 *   own guards pick its modern branches instead, exactly as they
 *   would under a current MSVC - without editing PCH.H. The couple of
 *   MSVC-only `#pragma comment`/`#pragma warning` lines this newly
 *   exposes are silently ignored by gcc/clang.
 */

#ifndef SHIM_COMPAT_WINSOCK2_H
#define SHIM_COMPAT_WINSOCK2_H

#ifndef _MSC_VER
#define _MSC_VER 1930
#endif

#include "../win32_types.h"

#endif /* SHIM_COMPAT_WINSOCK2_H */
