/*
 *   winsock2.h
 *
 *   Part of the Emu28 macOS/Linux port's compat/ shim directory (see
 *   ../../CLAUDE.md). Stands in for the real Win32 SDK header so that
 *   vendor/emu28-upstream/PCH.H's `#include <winsock2.h>` resolves on
 *   macOS/Linux. UDP.C (the only file that actually uses sockets) is
 *   out of scope for now - see ../win32_types.h's header comment -
 *   this stub only needs to satisfy PCH.H's unconditional include.
 */

#ifndef SHIM_COMPAT_WINSOCK2_H
#define SHIM_COMPAT_WINSOCK2_H

#include "../win32_types.h"

#endif /* SHIM_COMPAT_WINSOCK2_H */
