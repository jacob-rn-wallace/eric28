/*
 *   win32_handle.h
 *
 *   This file is part of the Emu28 macOS/Linux port (see ../CLAUDE.md).
 *
 *   A real Win32 HANDLE is a polymorphic kernel-object reference: the
 *   same opaque type identifies a file, an event, a thread, a mutex,
 *   and more, and generic functions (CloseHandle, WaitForSingleObject)
 *   work correctly regardless of which. vendor/emu28-upstream's
 *   KEYMACRO.C calls CloseHandle on both a file handle (hMacroFile)
 *   and event/thread handles (hThreadEv, hEventPlay) in the same
 *   function, and ENGINE.C calls WaitForSingleObject on both event
 *   handles (hEventDebug, hEventShutdn) and a thread handle (hThread) -
 *   so this shim's HANDLE needs that same polymorphism, not three
 *   independent implementations that happen to share a type name.
 *
 *   Implemented here as a tagged dispatch, the same shape as gdi.c's
 *   SelectObject/DeleteObject tell bitmaps/brushes/pens apart through
 *   one HGDIOBJ: every HANDLE this file hands out points at a small
 *   struct whose first field says what kind of kernel object it
 *   actually is (file/event/thread), and every function here reads
 *   that tag before deciding what to do. win32_types.h's own
 *   CreateFile-shaped HANDLE (a bare fd smuggled through the pointer
 *   value) predates this file and got folded in here once CloseHandle
 *   needed to dispatch across all three kinds - see git history if
 *   the difference matters.
 *
 *   File I/O: HANDLE-based Win32 file I/O maps directly onto POSIX
 *   file descriptors, so this is real CRT-primitive territory, same
 *   as win32_types.h's memory/string functions.
 *
 *   Events: a pthread mutex + condvar + a signaled flag, with
 *   auto-reset events consuming their signal on a successful wait
 *   (matching real Win32 - manual-reset events don't).
 *
 *   Threads: pthread_create behind a trampoline that records the
 *   thread's result and signals completion through the same
 *   mutex/condvar shape as an event, so WaitForSingleObject can treat
 *   "wait for this thread to finish" identically to "wait for this
 *   event to be signaled" - which is exactly what real Win32 does,
 *   and exactly why ENGINE.C's code is allowed to call the same
 *   WaitForSingleObject on both without knowing which it has.
 *
 *   New code, not derived from any Win32 SDK header. GPL-2.0, as a
 *   derivative work of Emu28 (Copyright (C) 2002 Christoph
 *   Giesselink), since it exists solely to run Emu28's source.
 */

#ifndef SHIM_WIN32_HANDLE_H
#define SHIM_WIN32_HANDLE_H

#include "win32_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ---- file I/O ------------------------------------------------------------------- */

extern HANDLE CreateFile(LPCTSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode,
                          LPVOID lpSecurityAttributes, DWORD dwCreationDisposition,
                          DWORD dwFlagsAndAttributes, HANDLE hTemplateFile);
extern BOOL   ReadFile(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead,
                        LPDWORD lpNumberOfBytesRead, LPVOID lpOverlapped);
extern BOOL   WriteFile(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite,
                         LPDWORD lpNumberOfBytesWritten, LPVOID lpOverlapped);
extern DWORD  SetFilePointer(HANDLE hFile, LONG lDistanceToMove, LONG *lpDistanceToMoveHigh, DWORD dwMoveMethod);
extern DWORD  GetFileSize(HANDLE hFile, LPDWORD lpFileSizeHigh);
extern BOOL   SetEndOfFile(HANDLE hFile);
extern BOOL   FlushFileBuffers(HANDLE hFile);

/* ---- file mapping (FILES.C's LoadBitmapFile/MapRom-adjacent code) ----------------
 * A fourth HANDLE kind (see this file's header comment for the tagged-
 * dispatch design): FILES.C always maps a whole read-only file into
 * memory once and reads straight out of the mapping, never writes
 * through it, so this is a thin, real wrapper over POSIX mmap - not
 * a general-purpose reimplementation of Windows' shared-memory-section
 * semantics (no PAGE_READWRITE/copy-on-write path exists here because
 * nothing in this codebase's call sites needs one). MapViewOfFile
 * returns a bare LPVOID (matching real Win32), not a HANDLE, so
 * UnmapViewOfFile can't read a type tag back out of it the way
 * CloseHandle does - see win32_handle.c for how it recovers the
 * mapping's length instead. */

#define PAGE_READONLY  0x02
#define FILE_MAP_READ  0x0004

extern HANDLE CreateFileMapping(HANDLE hFile, LPVOID lpAttributes, DWORD flProtect,
                                 DWORD dwMaximumSizeHigh, DWORD dwMaximumSizeLow, LPCTSTR lpName);
extern LPVOID MapViewOfFile(HANDLE hFileMappingObject, DWORD dwDesiredAccess,
                             DWORD dwFileOffsetHigh, DWORD dwFileOffsetLow, SIZE_T dwNumberOfBytesToMap);
extern BOOL   UnmapViewOfFile(LPCVOID lpBaseAddress);

/* ---- events --------------------------------------------------------------------- */

#define INFINITE 0xFFFFFFFFu

#define WAIT_OBJECT_0 0x00000000u
#define WAIT_TIMEOUT  0x00000102u
#define WAIT_FAILED   0xFFFFFFFFu

extern HANDLE CreateEvent(LPVOID lpEventAttributes, BOOL bManualReset, BOOL bInitialState, LPCTSTR lpName);
extern BOOL   SetEvent(HANDLE hEvent);
extern BOOL   ResetEvent(HANDLE hEvent);

/* ---- threads and the shared wait primitive ---------------------------------------- */

typedef DWORD (*LPTHREAD_START_ROUTINE)(LPVOID);

extern HANDLE CreateThread(LPVOID lpThreadAttributes, SIZE_T dwStackSize,
                            LPTHREAD_START_ROUTINE lpStartAddress, LPVOID lpParameter,
                            DWORD dwCreationFlags, LPDWORD lpThreadId);
extern DWORD  WaitForSingleObject(HANDLE hHandle, DWORD dwMilliseconds);

/* ---- common to all three kinds ----------------------------------------------------- */

extern BOOL CloseHandle(HANDLE hObject);

#ifdef __cplusplus
}
#endif

#endif /* SHIM_WIN32_HANDLE_H */
