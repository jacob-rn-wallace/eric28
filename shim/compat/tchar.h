/*
 *   tchar.h
 *
 *   Part of the Emu28 macOS/Linux port's compat/ shim directory (see
 *   ../../CLAUDE.md). Stands in for the real CRT header so that
 *   vendor/emu28-upstream/PCH.H's `#include <tchar.h>` resolves on
 *   macOS/Linux. TCHAR/_T() themselves live in ../win32_types.h,
 *   which this just forwards to.
 */

#ifndef SHIM_COMPAT_TCHAR_H
#define SHIM_COMPAT_TCHAR_H

#include "../win32_types.h"

#endif /* SHIM_COMPAT_TCHAR_H */
