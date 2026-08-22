/*
 *   shlobj.h
 *
 *   Part of the Emu28 macOS/Linux port's compat/ shim directory (see
 *   ../../CLAUDE.md). Stands in for the real Win32 SDK header so that
 *   vendor/emu28-upstream/PCH.H's `#include <shlobj.h>` resolves on
 *   macOS/Linux. Nothing in the current build scope (the CPU/RPL
 *   engine) uses shell folder APIs; this only satisfies PCH.H's
 *   include.
 */

#ifndef SHIM_COMPAT_SHLOBJ_H
#define SHIM_COMPAT_SHLOBJ_H

#include "../win32_types.h"

#endif /* SHIM_COMPAT_SHLOBJ_H */
