/*
 *   direct.h
 *
 *   Part of the Emu28 macOS/Linux port's compat/ shim directory (see
 *   ../../CLAUDE.md). Stands in for the MSVC CRT header so that
 *   vendor/emu28-upstream/PCH.H's `#include <direct.h>` resolves on
 *   macOS/Linux. Nothing in the current build scope (the CPU/RPL
 *   engine) calls its directory functions (those belong to FILES.C,
 *   not yet in scope) - empty until something needs it.
 */

#ifndef SHIM_COMPAT_DIRECT_H
#define SHIM_COMPAT_DIRECT_H

#endif /* SHIM_COMPAT_DIRECT_H */
