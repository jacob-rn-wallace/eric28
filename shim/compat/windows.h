/*
 *   windows.h
 *
 *   Part of the Emu28 macOS/Linux port's compat/ shim directory (see
 *   ../../CLAUDE.md). Stands in for the real Win32 SDK header so that
 *   vendor/emu28-upstream/PCH.H's `#include <windows.h>` resolves to
 *   something on macOS/Linux without editing that vendored file.
 *   Never hand-edit vendor/ to route around this - add types/
 *   primitives to ../win32_types.h, GDI drawing calls to ../gdi.h,
 *   file/event/thread HANDLE calls to ../win32_handle.h, or INI-file
 *   calls to ../ini_file.h, instead - real windows.h exposes all
 *   four, so this does too.
 */

#ifndef SHIM_COMPAT_WINDOWS_H
#define SHIM_COMPAT_WINDOWS_H

#include "../win32_types.h"
#include "../gdi.h"
#include "../win32_handle.h"
#include "../ini_file.h"

#endif /* SHIM_COMPAT_WINDOWS_H */
