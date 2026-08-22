/*
 *   ini_file.h
 *
 *   This file is part of the Emu28 macOS/Linux port (see ../CLAUDE.md).
 *
 *   SETTINGS.C's own `#if !defined REGISTRY` split (REGISTRY is
 *   commented out, so this is the branch that actually compiles)
 *   already avoids the Windows registry in favor of Win32's
 *   INI-file API (GetPrivateProfileString/GetPrivateProfileInt/
 *   WritePrivateProfileString, all against a plain "Emu28.ini" next
 *   to the executable) - so milestone 5's "stub SETTINGS.C to a flat
 *   config file" is already what upstream's own default build does;
 *   this just needs to give those three Win32 functions real,
 *   portable bodies, not design a registry replacement.
 *
 *   Real implementation, not a stub: on every call the whole file is
 *   read, parsed into sections/key=value pairs, the read or write
 *   applied, and (for writes) the whole file rewritten - appropriate
 *   for a settings file touched a few dozen times at startup/exit,
 *   not a hot path. Section/key matching is case-insensitive,
 *   matching real Win32's documented behavior.
 *
 *   New code, not derived from any Win32 SDK header. GPL-2.0, as a
 *   derivative work of Emu28 (Copyright (C) 2002 Christoph
 *   Giesselink), since it exists solely to run Emu28's source.
 */

#ifndef SHIM_INI_FILE_H
#define SHIM_INI_FILE_H

#include "win32_types.h"

#ifdef __cplusplus
extern "C" {
#endif

extern DWORD GetPrivateProfileString(LPCTSTR lpAppName, LPCTSTR lpKeyName, LPCTSTR lpDefault,
                                      LPTSTR lpReturnedString, DWORD nSize, LPCTSTR lpFileName);
extern UINT  GetPrivateProfileInt(LPCTSTR lpAppName, LPCTSTR lpKeyName, INT nDefault, LPCTSTR lpFileName);
extern BOOL  WritePrivateProfileString(LPCTSTR lpAppName, LPCTSTR lpKeyName, LPCTSTR lpString, LPCTSTR lpFileName);

#ifdef __cplusplus
}
#endif

#endif /* SHIM_INI_FILE_H */
