/*
 *   crtdbg.h
 *
 *   Part of the Emu28 macOS/Linux port's compat/ shim directory (see
 *   ../../CLAUDE.md). Stands in for the MSVC CRT debug header so that
 *   vendor/emu28-upstream/PCH.H's `#include <crtdbg.h>` resolves on
 *   macOS/Linux. _ASSERT itself is defined unconditionally in
 *   ../win32_types.h, which this forwards to.
 */

#ifndef SHIM_COMPAT_CRTDBG_H
#define SHIM_COMPAT_CRTDBG_H

#include "../win32_types.h"

#endif /* SHIM_COMPAT_CRTDBG_H */
