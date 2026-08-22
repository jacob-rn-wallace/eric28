/*
 *   win32_types.h
 *
 *   This file is part of the Emu28 macOS/Linux port (see ../CLAUDE.md).
 *
 *   Win32 types-and-primitives compatibility shim: lets Emu28's
 *   untouched engine sources build with plain gcc/clang. Scope is the
 *   Win32 *types* (DWORD, BOOL, HANDLE, ...) and small CRT-style
 *   *primitives* (ZeroMemory, GetTickCount, critical sections, ...)
 *   that vendor/emu28-upstream's non-GDI/window/registry/sound files
 *   assume are always available - see CLAUDE.md's "Win32 API surface"
 *   table for the category breakdown. Opaque handle types (HWND, HDC,
 *   HBITMAP, ...) are declared here so headers like EMU28.H parse, but
 *   the real objects behind them - and the GDI/window/registry/sound
 *   functions that operate on them - belong to later shim layers, not
 *   this one.
 *
 *   New code, not derived from any Win32 SDK header. GPL-2.0, as a
 *   derivative work of Emu28 (Copyright (C) 2002 Christoph
 *   Giesselink), since it exists solely to compile Emu28's source.
 */

#ifndef SHIM_WIN32_TYPES_H
#define SHIM_WIN32_TYPES_H

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif

/* NOTE: PCH.H needs _MSC_VER defined (see compat/winsock2.h for why
 * and the full explanation) - that define deliberately does NOT live
 * here. This header is included both via PCH.H's vendor-compile path
 * *and* directly by this project's own new (non-vendor) code, e.g.
 * the SDL2 platform layer - and third-party headers like SDL.h
 * genuinely check _MSC_VER to decide whether to pull in real
 * MSVC-only headers (SDL_stdinc.h's `#include <sal.h>`). Defining it
 * unconditionally here once broke exactly that for any new file
 * including this header outside the vendor-compile path. */

#include <assert.h>
#include <fcntl.h>
#include <locale.h>
#include <pthread.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <unistd.h>
#include <ctype.h>
#include <dirent.h>
#include <fnmatch.h>
#include <sys/stat.h>

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---- fixed-size integer types ----------------------------------------- */

typedef uint8_t  BYTE;
typedef uint16_t WORD;
typedef uint32_t DWORD;
typedef int64_t  LONGLONG;
typedef uint64_t ULONGLONG;

typedef int            INT;
typedef unsigned int   UINT;
/* Always 32-bit, per Win32's LLP64 model - NOT `long`, which is
 * 64-bit on LP64 macOS/Linux. Binary-format structs with strict
 * layout requirements (BITMAPINFOHEADER, ...) silently corrupt if
 * this is wrong; see CLAUDE.md's milestone 3 notes for how that was
 * caught. */
typedef int32_t  LONG;
typedef uint32_t ULONG;
typedef short            SHORT;
typedef unsigned short   USHORT;
typedef char CHAR;

typedef int BOOL;
#define TRUE  1
#define FALSE 0

#define VOID void
#define CONST const

typedef size_t    DWORD_PTR;
typedef size_t    ULONG_PTR;
typedef ptrdiff_t LONG_PTR;
typedef size_t    SIZE_T;
typedef size_t    UINT_PTR;
typedef ptrdiff_t INT_PTR;

/* LRESULT: return type of the (not-yet-ported) window procedure.
 * WPARAM/LPARAM: its message-parameter types. */
typedef LONG_PTR LRESULT;
typedef UINT_PTR WPARAM;
typedef LONG_PTR LPARAM;

/* CALLBACK/WINAPI: Win32 calling-convention tags (__stdcall). Not
 * meaningful outside x86 Windows, so they disappear here. */
#define CALLBACK
#define WINAPI

/* ---- pointer typedefs -------------------------------------------------- */

typedef BYTE*  LPBYTE;
typedef WORD*  LPWORD;
typedef DWORD* LPDWORD;
typedef void*  LPVOID;
typedef const void* LPCVOID;

/* ---- ANSI string/char types --------------------------------------------
 * No UNICODE build: TCHAR == char, matching the ANSI Win32 API set
 * (...A suffix) rather than the wide (...W) one. */

typedef char TCHAR;
#define _T(x) x

typedef char*       LPSTR;
typedef const char* LPCSTR;
typedef char*       LPTSTR;
typedef const char* LPCTSTR;

#define MAX_PATH 260

/* ---- misc value types --------------------------------------------------- */

typedef DWORD COLORREF;

typedef union {
	struct {
		DWORD LowPart;
		int32_t HighPart;
	};
	LONGLONG QuadPart;
} LARGE_INTEGER;

typedef struct {
	LONG left;
	LONG top;
	LONG right;
	LONG bottom;
} RECT, *LPRECT;

typedef struct {
	WORD wYear;
	WORD wMonth;
	WORD wDayOfWeek;
	WORD wDay;
	WORD wHour;
	WORD wMinute;
	WORD wSecond;
	WORD wMilliseconds;
} SYSTEMTIME, *LPSYSTEMTIME;

/* ---- opaque handles ------------------------------------------------------
 * These identify Windows kernel/GDI/user/DDE objects. The files this
 * header targets only ever pass them around as opaque values (extern
 * globals and function signatures declared in EMU28.H) - the code that
 * actually creates and operates on them belongs to the GDI/window/DDE
 * shim layers, not here. */

typedef void* HANDLE;
typedef void* HWND;
typedef void* HDC;
typedef void* HBITMAP;
typedef void* HMENU;
typedef void* HCURSOR;
typedef void* HINSTANCE;
typedef void* HRGN;
typedef void* HPALETTE;
typedef void* HBRUSH;
typedef void* HCONV;
typedef void* HSZ;
typedef void* HDDEDATA;

/* ---- message-box stand-ins ------------------------------------------------
 * EMU28.H's InfoMessage()/AbortMessage()/etc. inline wrappers call
 * MessageBox() directly. Declared, not implemented, here - a real
 * implementation (an SDL2 dialog or equivalent) belongs to the window
 * shim layer; until that exists, linking anything that calls these
 * wrappers will fail at link time, not compile time. */

#define MB_OK              0x00000000L
#define MB_YESNO           0x00000004L
#define MB_YESNOCANCEL     0x00000003L
#define MB_ICONSTOP        0x00000010L
#define MB_ICONEXCLAMATION 0x00000030L
#define MB_ICONINFORMATION 0x00000040L
#define MB_APPLMODAL       0x00000000L
#define MB_SETFOREGROUND   0x00010000L

extern int MessageBox(HWND hWnd, LPCTSTR lpText, LPCTSTR lpCaption, UINT uType);

/* ---- critical sections (pthread mutex-backed) ---------------------------- */

typedef struct {
	pthread_mutex_t mutex;
} CRITICAL_SECTION;

static inline void InitializeCriticalSection(CRITICAL_SECTION *cs)
{
	pthread_mutex_init(&cs->mutex, NULL);
}

static inline void DeleteCriticalSection(CRITICAL_SECTION *cs)
{
	pthread_mutex_destroy(&cs->mutex);
}

static inline void EnterCriticalSection(CRITICAL_SECTION *cs)
{
	pthread_mutex_lock(&cs->mutex);
}

static inline void LeaveCriticalSection(CRITICAL_SECTION *cs)
{
	pthread_mutex_unlock(&cs->mutex);
}

/* MSVC's spelling of C99 inline, and its "really, inline this" hint -
 * the latter has no portable equivalent, so it's dropped: the function
 * keeps ordinary external linkage (matching its `extern` declaration
 * in EMU28.H) and the compiler is free to inline it or not. */
#ifndef __inline
#define __inline inline
#endif
#ifndef __forceinline
#define __forceinline
#endif

/* ---- debug/CRT macros ----------------------------------------------------- */

#define _ASSERT(expr) assert(expr)
#define UNREFERENCED_PARAMETER(p) ((void)(p))

#if !defined VERIFY
#if defined _DEBUG
#define VERIFY(f) _ASSERT(f)
#else
#define VERIFY(f) ((VOID)(f))
#endif
#endif

/* ---- memory/string primitives (Win32 CRT extensions) ---------------------- */

#define ZeroMemory(dst, len)      memset((dst), 0, (len))
#define CopyMemory(dst, src, len) memcpy((dst), (src), (len))
#define MoveMemory(dst, src, len) memmove((dst), (src), (len))
#define FillMemory(dst, len, val) memset((dst), (val), (len))

#define lstrlen(s)            ((INT)strlen(s))
#define lstrcpy(dst, src)     strcpy((dst), (src))
#define lstrcpyn(dst, src, n) strncpy((dst), (src), (n))
#define lstrcat(dst, src)     strcat((dst), (src))
#define lstrcmp(a, b)         strcmp((a), (b))
#define lstrcmpi(a, b)        strcasecmp((a), (b))

/* tchar.h's generic-text CRT functions - TCHAR == char here, so these
 * are just the ANSI (str*) forms under their _tcs* names. */
#define _tcsspn(s, set)       strspn((s), (set))
#define _tcschr(s, c)         strchr((s), (c))
#define _tcsncmp(a, b, n)     strncmp((a), (b), (n))
#define _tcstoul(s, end, base) strtoul((s), (end), (base))

static inline int wvsprintf(char *buf, const char *fmt, va_list args)
{
	return vsprintf(buf, fmt, args);
}

static inline int wsprintf(char *buf, const char *fmt, ...)
{
	va_list args;
	int n;

	va_start(args, fmt);
	n = vsprintf(buf, fmt, args);
	va_end(args);
	return n;
}

/* ---- bit-manipulation macros ----------------------------------------------- */

#define LOBYTE(w) ((BYTE)((DWORD_PTR)(w) & 0xff))
#define HIBYTE(w) ((BYTE)(((DWORD_PTR)(w) >> 8) & 0xff))
#define LOWORD(l) ((WORD)((DWORD_PTR)(l) & 0xffff))
#define HIWORD(l) ((WORD)(((DWORD_PTR)(l) >> 16) & 0xffff))
#define MAKEWORD(lo, hi) ((WORD)(((BYTE)(lo)) | (((WORD)((BYTE)(hi))) << 8)))
#define MAKELONG(lo, hi) ((LONG)(((WORD)(lo)) | (((DWORD)((WORD)(hi))) << 16)))

#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif
#ifndef max
#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif
#ifndef __min
#define __min(a, b) min((a), (b))
#endif
#ifndef __max
#define __max(a, b) max((a), (b))
#endif

/* ---- timing ----------------------------------------------------------------- */

static inline DWORD GetTickCount(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (DWORD)((uint64_t)ts.tv_sec * 1000 + (uint64_t)ts.tv_nsec / 1000000);
}

static inline ULONGLONG GetTickCount64(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (ULONGLONG)ts.tv_sec * 1000 + (ULONGLONG)ts.tv_nsec / 1000000;
}

/* winmm's high-resolution millisecond counter - same clock as
 * GetTickCount here, since clock_gettime(CLOCK_MONOTONIC) already has
 * sub-millisecond resolution on macOS/Linux. */
static inline DWORD timeGetTime(void)
{
	return GetTickCount();
}

static inline void Sleep(DWORD dwMilliseconds)
{
	struct timespec ts;
	ts.tv_sec = dwMilliseconds / 1000;
	ts.tv_nsec = (long)(dwMilliseconds % 1000) * 1000000L;
	nanosleep(&ts, NULL);
}

static inline BOOL QueryPerformanceCounter(LARGE_INTEGER *lpCount)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	lpCount->QuadPart = (LONGLONG)ts.tv_sec * 1000000000LL + ts.tv_nsec;
	return TRUE;
}

static inline BOOL QueryPerformanceFrequency(LARGE_INTEGER *lpFrequency)
{
	lpFrequency->QuadPart = 1000000000LL;
	return TRUE;
}

static inline void GetLocalTime(LPSYSTEMTIME lpst)
{
	time_t t = time(NULL);
	struct tm tmv;
	struct timespec ts;

	localtime_r(&t, &tmv);
	clock_gettime(CLOCK_REALTIME, &ts);

	lpst->wYear = (WORD)(tmv.tm_year + 1900);
	lpst->wMonth = (WORD)(tmv.tm_mon + 1);
	lpst->wDayOfWeek = (WORD)tmv.tm_wday;
	lpst->wDay = (WORD)tmv.tm_mday;
	lpst->wHour = (WORD)tmv.tm_hour;
	lpst->wMinute = (WORD)tmv.tm_min;
	lpst->wSecond = (WORD)tmv.tm_sec;
	lpst->wMilliseconds = (WORD)(ts.tv_nsec / 1000000);
}

/* ---- locale ------------------------------------------------------------------
 * Only the one call STACK.C actually makes - GetLocaleInfo(...,
 * LOCALE_SDECIMAL, ...) to read the decimal-point character - is
 * covered; Win32's real GetLocaleInfo is far more general than this. */

#define LOCALE_USER_DEFAULT 0x0400
#define LOCALE_SDECIMAL     0x0000000E

static inline int GetLocaleInfo(DWORD Locale, DWORD LCType, LPTSTR lpLCData, int cchData)
{
	const char *decimal_point;

	(void)Locale;

	if (LCType != LOCALE_SDECIMAL)
		return 0;

	decimal_point = localeconv()->decimal_point;
	if (cchData == 0)
		return (int)strlen(decimal_point) + 1;

	strncpy(lpLCData, decimal_point, (size_t)cchData - 1);
	lpLCData[cchData - 1] = '\0';
	return (int)strlen(lpLCData) + 1;
}

/* ---- power status --------------------------------------------------------------
 * MOPS.C's low-battery auto-shutdown check reads this. Real battery
 * state (IOKit on macOS, /sys/class/power_supply on Linux) is a later
 * milestone - declared, not implemented, here. */

typedef struct {
	BYTE  ACLineStatus;
	BYTE  BatteryFlag;
	BYTE  BatteryLifePercent;
	BYTE  Reserved1;
	DWORD BatteryLifeTime;
	DWORD BatteryFullLifeTime;
} SYSTEM_POWER_STATUS, *LPSYSTEM_POWER_STATUS;

#define AC_LINE_OFFLINE        0x00
#define AC_LINE_ONLINE         0x01
#define BATTERY_FLAG_LOW       0x02
#define BATTERY_FLAG_CRITICAL  0x04

extern BOOL GetSystemPowerStatus(LPSYSTEM_POWER_STATUS lpSystemPowerStatus);

/* ---- thread/event synchronization -----------------------------------------------
 * Real implementation in win32_handle.h/.c now, not here: ENGINE.C's
 * worker-thread state machine waits on both event handles
 * (hEventDebug, hEventShutdn) and a thread handle (hThread) through
 * the same WaitForSingleObject() call, which is only well-defined
 * because real Win32 HANDLEs are polymorphic kernel objects - giving
 * this shim's opaque HANDLE that same polymorphism needed its own
 * tagged-dispatch design, the same shape as gdi.c's
 * bitmap/brush/pen SelectObject trick, so it lives with the file-I/O
 * HANDLE implementation (CloseHandle needs to dispatch on all three
 * kinds too) rather than as more inline functions here. */

/* ---- clipboard -------------------------------------------------------------------
 * STACK.C's stack-copy/paste commands go through the Windows clipboard
 * and its GlobalAlloc-backed memory objects. Clipboard access is
 * inherently window-system-integrated (NSPasteboard on macOS, X11/
 * Wayland selections via SDL2 on Linux) - that belongs to the SDL2
 * platform layer, not this types header. Declared, not implemented,
 * here. */

#define CF_TEXT        1
#define GMEM_MOVEABLE  0x0002

extern HANDLE GlobalAlloc(UINT uFlags, SIZE_T dwBytes);
extern LPVOID GlobalLock(HANDLE hMem);
extern BOOL   GlobalUnlock(HANDLE hMem);
extern HANDLE GlobalFree(HANDLE hMem);

extern BOOL   OpenClipboard(HWND hWndNewOwner);
extern BOOL   CloseClipboard(VOID);
extern BOOL   EmptyClipboard(VOID);
extern HANDLE SetClipboardData(UINT uFormat, HANDLE hMem);
extern HANDLE GetClipboardData(UINT uFormat);
extern BOOL   IsClipboardFormatAvailable(UINT format);

/* ---- misc UI feedback -------------------------------------------------------------
 * A simple audio cue - belongs with the sound/window shim, not here. */

extern BOOL MessageBeep(UINT uType);

/* ---- file I/O ------------------------------------------------------------------
 * HANDLE-based Win32 file I/O maps directly onto POSIX file
 * descriptors - real CRT-primitive territory, like the memory/string
 * functions above. CreateFile/ReadFile/WriteFile/CloseHandle/
 * SetFilePointer/GetFileSize themselves live in win32_handle.h/.c
 * now, not here: CloseHandle is also called on event and thread
 * handles (see win32_handle.h), so all three HANDLE flavors need to
 * funnel through one tagged-dispatch implementation, the same
 * SelectObject-style trick gdi.c uses for bitmaps/brushes/pens. */

#define GENERIC_READ  0x80000000u
#define GENERIC_WRITE 0x40000000u

#define FILE_SHARE_READ  0x00000001u
#define FILE_SHARE_WRITE 0x00000002u

#define CREATE_ALWAYS 2u
#define OPEN_EXISTING 3u

#define FILE_ATTRIBUTE_NORMAL     0x00000080u
#define FILE_FLAG_SEQUENTIAL_SCAN 0x08000000u

#define FILE_BEGIN   0u
#define FILE_CURRENT 1u
#define FILE_END     2u

#define INVALID_HANDLE_VALUE ((HANDLE)(intptr_t)-1)

#ifndef INVALID_SET_FILE_POINTER
#define INVALID_SET_FILE_POINTER ((DWORD)-1)
#endif

static inline BOOL SetCurrentDirectory(LPCTSTR lpPathName)
{
	return chdir(lpPathName) == 0;
}

/* ---- file enumeration (KML.C's *.KML skin-directory scan) -------------------------
 * FindFirstFile/FindNextFile's glob-pattern matching maps directly
 * onto POSIX fnmatch(); only cFileName is populated (the only field
 * KML.C reads) - the timestamp/attribute fields real Win32 fills in
 * are zeroed, since nothing here reads them. */

#define FILE_ATTRIBUTE_DIRECTORY 0x00000010u

typedef struct {
	DWORD dwFileAttributes;
	DWORD ftCreationTime[2];
	DWORD ftLastAccessTime[2];
	DWORD ftLastWriteTime[2];
	DWORD nFileSizeHigh;
	DWORD nFileSizeLow;
	DWORD dwReserved0;
	DWORD dwReserved1;
	TCHAR cFileName[260];
	TCHAR cAlternateFileName[14];
} WIN32_FIND_DATA, *LPWIN32_FIND_DATA;

typedef struct {
	DIR   *dir;
	char   pattern[260];
} ShimFindHandle;

static inline BOOL shim_find_next(ShimFindHandle *h, LPWIN32_FIND_DATA lpData)
{
	struct dirent *de;

	while ((de = readdir(h->dir)) != NULL) {
		/* Case-sensitive: FNM_CASEFOLD needs different feature-test
		 * macros on macOS vs. glibc to stay visible under the strict
		 * _POSIX_C_SOURCE this file sets, and every real .KML skin
		 * file (see skins/hp28c/) is already uppercase, matching the
		 * "*.KML" pattern KML.C actually searches for. */
		if (fnmatch(h->pattern, de->d_name, 0) != 0)
			continue;
		memset(lpData, 0, sizeof(*lpData));
		strncpy(lpData->cFileName, de->d_name, sizeof(lpData->cFileName) - 1);
		return TRUE;
	}
	return FALSE;
}

static inline HANDLE FindFirstFile(LPCTSTR lpFileName, LPWIN32_FIND_DATA lpFindFileData)
{
	ShimFindHandle *h = (ShimFindHandle *)calloc(1, sizeof(ShimFindHandle));
	CONST CHAR *slash = strrchr(lpFileName, '/');

	h->dir = opendir(".");
	strncpy(h->pattern, slash ? slash + 1 : lpFileName, sizeof(h->pattern) - 1);
	if (h->dir == NULL || !shim_find_next(h, lpFindFileData)) {
		if (h->dir)
			closedir(h->dir);
		free(h);
		return INVALID_HANDLE_VALUE;
	}
	return (HANDLE)h;
}

static inline BOOL FindNextFile(HANDLE hFindFile, LPWIN32_FIND_DATA lpFindFileData)
{
	return shim_find_next((ShimFindHandle *)hFindFile, lpFindFileData);
}

static inline BOOL FindClose(HANDLE hFindFile)
{
	ShimFindHandle *h = (ShimFindHandle *)hFindFile;

	closedir(h->dir);
	free(h);
	return TRUE;
}

/* ---- small arithmetic/struct helpers (MulDiv, RECT, HRESULT) ----------------------- */

static inline INT MulDiv(INT nNumber, INT nNumerator, INT nDenominator)
{
	/* Matches Win32's documented rounding: round the true quotient to
	 * the nearest integer, .5 away from zero. */
	int64_t result = (int64_t)nNumber * nNumerator;
	int64_t half = nDenominator / 2;

	if (nDenominator == 0)
		return -1;
	if ((result < 0) != (nDenominator < 0))
		half = -half;
	return (INT)((result + half) / nDenominator);
}

static inline VOID SetRect(LPRECT lprc, INT xLeft, INT yTop, INT xRight, INT yBottom)
{
	lprc->left = xLeft;
	lprc->top = yTop;
	lprc->right = xRight;
	lprc->bottom = yBottom;
}

static inline VOID SetRectEmpty(LPRECT lprc)
{
	lprc->left = lprc->top = lprc->right = lprc->bottom = 0;
}

typedef LONG HRESULT;
#define SUCCEEDED(hr) ((HRESULT)(hr) >= 0)

/* Standard Win32 LANGID sub-field macros (documented bit layout, not
 * platform-specific behavior) plus the one KML.C compares against. */
typedef WORD LANGID;
#define PRIMARYLANGID(lgid) ((WORD)(lgid) & 0x3ffu)
#define SUBLANGID(lgid)     ((WORD)(lgid) >> 10)
#define LANG_NEUTRAL        0x00
#define SUBLANG_NEUTRAL     0x00

#define _istdigit(c) isdigit((unsigned char)(c))
typedef unsigned char _TUCHAR;

/* ---- Winsock (UDP.C) ------------------------------------------------------------
 * Winsock's scalar API (socket/htons/inet_addr/gethostbyname) is
 * deliberately BSD-socket-shaped, so those need no renaming at all -
 * <sys/socket.h>/<netinet/in.h>/<arpa/inet.h>/<netdb.h> (included
 * above) already declare POSIX functions of those exact names with
 * compatible signatures.
 *
 * SOCKADDR_IN is a different story: UDP.C brace-initializes
 * `sServer`'s address as `{ 255, 255, 255, 255 }` (Winsock's
 * `struct in_addr` is a union whose first member is a 4-byte
 * sub-struct, so this is well-defined there) and reads/writes it back
 * through a bare `.s_addr` - neither of which POSIX's plain-scalar
 * `struct in_addr { in_addr_t s_addr; }` supports. So SOCKADDR_IN/
 * IN_ADDR here are genuinely our own types, matching Winsock's actual
 * layout, and sendto() is a real wrapper that copies them into a
 * proper native `struct sockaddr_in` (needed since macOS's version of
 * that struct - unlike Linux's - carries a leading `sin_len` byte
 * that a raw pointer-cast across our own struct would get wrong)
 * before calling the real libc sendto(). */

typedef int SOCKET;

typedef union {
	struct {
		BYTE s_b1, s_b2, s_b3, s_b4;
	} S_un_b;
	DWORD s_addr;
} IN_ADDR, *PIN_ADDR;

typedef struct {
	SHORT   sin_family;
	WORD    sin_port;
	IN_ADDR sin_addr;
	CHAR    sin_zero[8];
} SOCKADDR_IN;

typedef struct {
	SHORT sa_family;
	CHAR  sa_data[14];
} SOCKADDR, *LPSOCKADDR;

typedef struct hostent HOSTENT, *PHOSTENT;
typedef const char    *LPCCH;

#define AF_INET        2
#define SOCK_DGRAM     2
#define INVALID_SOCKET ((SOCKET)-1)
#define SOCKET_ERROR   (-1)

/* Some platforms' <arpa/inet.h> already define this (correctly, as
 * in_addr_t); on those that don't (recent macOS SDKs dropped it),
 * fall back to the same value with the same, correctly-32-bit-sized
 * type. Do NOT define this as `unsigned long`, as Win32 itself does -
 * `unsigned long` is 64-bit on LP64 macOS/Linux, and silently makes
 * every `== INADDR_NONE` comparison against the (32-bit) address
 * field always false. */
#ifndef INADDR_NONE
#define INADDR_NONE ((in_addr_t)0xffffffff)
#endif

static inline int closesocket(SOCKET s)
{
	return close(s);
}

static inline int win32_sendto(SOCKET s, LPCCH buf, int len, int flags, LPSOCKADDR to, int tolen)
{
	SOCKADDR_IN *win_addr = (SOCKADDR_IN *)to;
	struct sockaddr_in real_addr;

	(void)tolen;
	memset(&real_addr, 0, sizeof(real_addr));
#ifdef __APPLE__
	real_addr.sin_len = sizeof(real_addr);
#endif
	real_addr.sin_family = AF_INET;
	real_addr.sin_port = (in_port_t)win_addr->sin_port;
	real_addr.sin_addr.s_addr = (in_addr_t)win_addr->sin_addr.s_addr;

	return (int)sendto(s, buf, (size_t)len, flags, (struct sockaddr *)&real_addr, sizeof(real_addr));
}
#define sendto(s, buf, len, flags, to, tolen) win32_sendto((s), (buf), (len), (flags), (to), (tolen))

/* ---- winmm multimedia timer (TIMER.C) ----------------------------------------------
 * timeSetEvent runs its callback on its own OS-managed thread at a
 * periodic interval - the same kind of async-primitive design
 * decision as thread/event sync above (which mechanism backs it:
 * POSIX interval timers + signal handler, or a dedicated thread), so
 * it belongs with that work, not folded in here. Declared, not
 * implemented. */

typedef struct {
	UINT wPeriodMin;
	UINT wPeriodMax;
} TIMECAPS, *LPTIMECAPS;

typedef VOID (CALLBACK *LPTIMECALLBACK)(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser,
                                         DWORD_PTR dw1, DWORD_PTR dw2);

#define TIME_ONESHOT    0
#define TIME_PERIODIC   1
#define TIMERR_NOERROR  0

extern UINT timeBeginPeriod(UINT uPeriod);
extern UINT timeEndPeriod(UINT uPeriod);
extern UINT timeGetDevCaps(LPTIMECAPS ptc, UINT cbtc);
extern UINT timeSetEvent(UINT uDelay, UINT uResolution, LPTIMECALLBACK lpTimeProc,
                          DWORD_PTR dwUser, UINT fuEvent);
extern UINT timeKillEvent(UINT uTimerID);

/* ---- cursor creation (CURSOR.C) -----------------------------------------------------
 * GDI, per the API-surface table above - deferred to the GDI shim
 * layer (milestone 3), not implemented here. */

extern HCURSOR CreateCursor(HINSTANCE hInst, INT xHotSpot, INT yHotSpot, INT nWidth, INT nHeight,
                             CONST VOID *pvANDPlane, CONST VOID *pvXORPlane);

/* ---- DDE (DDESERV.C) -----------------------------------------------------------------
 * Dynamic Data Exchange is a legacy Windows-only IPC mechanism with
 * no macOS/Linux equivalent and nothing in SDL2 that replaces it -
 * unlike every other category here, this one may never get a real
 * implementation in this port. Declared only so DDESERV.C parses. */

#define XTYP_CONNECT     0x0060
#define XTYP_POKE        0x0090
#define XTYP_REQUEST     0x00B0
#define DDE_FACK         0x8000
#define DDE_FNOTPROCESSED 0x0000

extern DWORD    DdeQueryString(DWORD idInst, HSZ hsz, LPTSTR psz, DWORD cchMax, INT iCodePage);
extern LPBYTE   DdeAccessData(HDDEDATA hData, LPDWORD pcbDataSize);
extern BOOL     DdeUnaccessData(HDDEDATA hData);
extern HDDEDATA DdeCreateDataHandle(DWORD idInst, LPBYTE pSrc, DWORD cb, DWORD cbOff,
                                     HSZ hszItem, UINT wFmt, UINT afCmd);

/* ---- dialog UI (KEYMACRO.C) -------------------------------------------------------------
 * CLAUDE.md is explicit that this port won't reproduce Emu28's
 * Windows-native dialog-based UI as literal dialogs, so - like DDE -
 * this may never get a real implementation; declared only so
 * KEYMACRO.C's macro-settings dialog parses. */

typedef INT_PTR (CALLBACK *DLGPROC)(HWND, UINT, WPARAM, LPARAM);

#define WM_INITDIALOG 0x0110
#define WM_COMMAND    0x0111

#define IDOK     1
#define IDCANCEL 2

#define BST_CHECKED 1

#define TBM_GETPOS       (0x0400 + 0)
#define TBM_SETTICFREQ   (0x0400 + 20)
#define TBM_SETRANGE     (0x0400 + 6)
#define TBM_SETPOS       (0x0400 + 5)

#define MAKEINTRESOURCE(i) ((LPTSTR)(DWORD_PTR)((WORD)(i)))

typedef struct {
	DWORD   lStructSize;
	HWND    hwndOwner;
	HINSTANCE hInstance;
	LPCTSTR lpstrFilter;
	LPTSTR  lpstrCustomFilter;
	DWORD   nMaxCustFilter;
	DWORD   nFilterIndex;
	LPTSTR  lpstrFile;
	DWORD   nMaxFile;
	LPTSTR  lpstrFileTitle;
	DWORD   nMaxFileTitle;
	LPCTSTR lpstrInitialDir;
	LPCTSTR lpstrTitle;
	DWORD   Flags;
	WORD    nFileOffset;
	WORD    nFileExtension;
	LPCTSTR lpstrDefExt;
	LPARAM  lCustData;
	LPVOID  lpfnHook;
	LPCTSTR lpTemplateName;
} OPENFILENAME, *LPOPENFILENAME;

#define OFN_HIDEREADONLY    0x00000004
#define OFN_OVERWRITEPROMPT 0x00000002
#define OFN_FILEMUSTEXIST   0x00001000
#define OFN_PATHMUSTEXIST   0x00000800
#define OFN_CREATEPROMPT    0x00002000
#define OFN_EXPLORER        0x00080000

extern INT_PTR DialogBox(HINSTANCE hInstance, LPCTSTR lpTemplate, HWND hWndParent, DLGPROC lpDialogFunc);
extern BOOL    EndDialog(HWND hDlg, INT_PTR nResult);
extern HWND    GetDlgItem(HWND hDlg, INT nIDDlgItem);
extern LRESULT SendDlgItemMessage(HWND hDlg, INT nIDDlgItem, UINT Msg, WPARAM wParam, LPARAM lParam);
extern BOOL    CheckDlgButton(HWND hDlg, INT nIDButton, UINT uCheck);
extern UINT    IsDlgButtonChecked(HWND hDlg, INT nIDButton);
extern BOOL    EnableWindow(HWND hWnd, BOOL bEnable);
extern BOOL    GetOpenFileName(LPOPENFILENAME lpofn);
extern BOOL    GetSaveFileName(LPOPENFILENAME lpofn);
extern BOOL    PostMessage(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);

/* ---- menu API + MRU path helpers (MRU.C) --------------------------------------------------
 * MRU.C's whole job is populating a native Win32 menu with a recent-
 * files list - out of scope until this port has some menu
 * replacement to populate instead (native menus aren't part of the
 * plan any more than native dialogs are, per CLAUDE.md). Its
 * GetCurrentDirectory/GetFullPathName calls are declared alongside
 * rather than given real POSIX implementations, since without the
 * menu half MRU.C can't do anything useful yet either. */

#define MF_STRING     0x00000000
#define MF_GRAYED     0x00000001
#define MF_SEPARATOR  0x00000800
#define MF_BYCOMMAND  0x00000000
#define MF_BYPOSITION 0x00000400

extern HMENU GetMenu(HWND hWnd);
extern HMENU GetSubMenu(HMENU hMenu, INT nPos);
extern INT   GetMenuItemCount(HMENU hMenu);
extern UINT  GetMenuItemID(HMENU hMenu, INT nPos);
extern UINT  GetMenuState(HMENU hMenu, UINT uId, UINT uFlags);
extern INT   GetMenuString(HMENU hMenu, UINT uIDItem, LPTSTR lpString, INT cchMax, UINT uFlag);
extern BOOL  InsertMenu(HMENU hMenu, UINT uPosition, UINT uFlags, UINT_PTR uIDNewItem, LPCTSTR lpNewItem);
extern BOOL  DeleteMenu(HMENU hMenu, UINT uPosition, UINT uFlags);
extern BOOL  IsMenu(HMENU hMenu);
extern DWORD GetCurrentDirectory(DWORD nBufferLength, LPTSTR lpBuffer);
extern DWORD GetFullPathName(LPCTSTR lpFileName, DWORD nBufferLength, LPTSTR lpBuffer, LPTSTR *lpFilePart);

/* ---- window management (DISPLAY.C's ResizeWindow/StopDisplay) --------------------------------
 * Real window sizing/positioning/repaint-invalidation, genuinely
 * dependent on an actual SDL2 window that doesn't exist until
 * milestone 4 (the SDL2 event loop replacing EMU28.C's window/
 * message-pump section) - declared only so DISPLAY.C type-checks as a
 * unit, same as the dialog/menu/DDE groups above. This is distinct
 * from the GDI *drawing* shim (shim/gdi.h) - that one is a real
 * implementation, because unlike window placement it doesn't need a
 * live window to be correct. */

#define GWL_STYLE 0xFFFFFFF0 /* wraps to -16, matching real Win32 */

#define SWP_NOMOVE   0x0002
#define SWP_NOZORDER 0x0004

#define HWND_TOPMOST   ((HWND)(intptr_t)-1)
#define HWND_NOTOPMOST ((HWND)(intptr_t)-2)

extern BOOL AdjustWindowRect(LPRECT lpRect, DWORD dwStyle, BOOL bMenu);
extern BOOL GetClientRect(HWND hWnd, LPRECT lpRect);
extern BOOL SetWindowPos(HWND hWnd, HWND hWndInsertAfter, INT X, INT Y, INT cx, INT cy, UINT uFlags);
extern LONG_PTR GetWindowLongPtr(HWND hWnd, INT nIndex);
extern BOOL InvalidateRect(HWND hWnd, CONST RECT *lpRect, BOOL bErase);
extern BOOL SetWindowOrgEx(HDC hdc, INT X, INT Y, LPVOID lpPoint);
extern INT  SetWindowRgn(HWND hWnd, HRGN hRgn, BOOL bRedraw);
extern BOOL DestroyWindow(HWND hWnd);
extern BOOL DragAcceptFiles(HWND hWnd, BOOL fAccept);
extern LONG_PTR GetClassLongPtr(HWND hWnd, INT nIndex);
extern HCURSOR SetCursor(HCURSOR hCursor);
extern LRESULT SendMessage(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);

#define GCL_HCURSOR   -12
#define WM_SYSCOMMAND 0x0112
#define MK_LBUTTON    0x0001

static inline BOOL IsRectEmpty(CONST RECT *lprc)
{
	return lprc->left >= lprc->right || lprc->top >= lprc->bottom;
}

/* ---- keyboard layout (KML.C's per-locale key mapping) ----------------------------
 * Reading the OS's active keyboard layout identifier is genuinely
 * platform-specific (Windows registry-shaped layout IDs have no
 * macOS/Linux equivalent - SDL2 exposes layout differently, if this
 * ever gets revisited it'll be through SDL2's keyboard API, not this
 * string). Declared only. */

#define KL_NAMELENGTH 9

extern BOOL GetKeyboardLayoutName(LPTSTR pwszKLID);

/* ---- dialog UI, continued (KML.C's skin-selection combo box) ---------------------
 * Combo-box messages are dispatched through SendMessage, which is
 * itself declared-only above - see win32_types.h's first dialog UI
 * section for why (CLAUDE.md already rules out literal native
 * dialogs for this port). Values are the real Win32 constants; only
 * meaningful once something actually implements SendMessage. */

#define CB_ERR          (-1)
#define CB_GETCURSEL    0x0147
#define CB_RESETCONTENT 0x014B
#define CB_ADDSTRING    0x0143
#define CB_SETCURSEL    0x014E
#define CB_GETITEMDATA  0x0150
#define CB_SETITEMDATA  0x0151

extern INT_PTR DialogBoxParam(HINSTANCE hInstance, LPCTSTR lpTemplateName, HWND hWndParent,
                               DLGPROC lpDialogFunc, LPARAM dwInitParam);
extern UINT    GetDlgItemText(HWND hDlg, INT nIDDlgItem, LPTSTR lpString, INT cchMax);
extern BOOL    SetDlgItemText(HWND hDlg, INT nIDDlgItem, LPCTSTR lpString);

/* ---- shell folder browsing (KML.C's "choose a skin folder" dialog) ---------------
 * The Shell "browse for folder" dialog and its COM-flavored support
 * types (IMalloc, ITEMIDLIST) have no macOS/Linux equivalent and
 * nothing in SDL2 replaces them - like DDE and native dialogs, this
 * may simply never get a real implementation in this port. Declared
 * only so KML.C type-checks as a unit. */

typedef VOID* LPITEMIDLIST;

/* IMalloc is a COM interface (a vtable-pointer struct) in real Win32;
 * KML.C calls two of its methods through `->lpVtbl->`. Shaped to match
 * that call syntax so it type-checks - never actually invoked, since
 * SHGetMalloc (the only thing that could produce a real instance) is
 * declared-only, same as the rest of this shell-browsing group. */
typedef struct IMallocVtbl {
	VOID (*Free)(VOID *pThis, LPVOID pv);
	VOID (*Release)(VOID *pThis);
} IMallocVtbl;

typedef struct {
	CONST IMallocVtbl *lpVtbl;
} IMalloc, *LPMALLOC;

typedef struct {
	HWND    hwndOwner;
	LPVOID  pidlRoot;
	LPTSTR  pszDisplayName;
	LPCTSTR lpszTitle;
	UINT    ulFlags;
	LPVOID  lpfn;
	LPARAM  lParam;
	INT     iImage;
} BROWSEINFO, *LPBROWSEINFO;

#define BIF_RETURNONLYFSDIRS 0x0001
#define BIF_STATUSTEXT       0x0004

#define BFFM_INITIALIZED    1
#define BFFM_SELCHANGED     2
#define BFFM_SETSTATUSTEXT  (0x0400 + 100)
#define BFFM_SETSELECTION   (0x0400 + 102)

extern LPITEMIDLIST SHBrowseForFolder(LPBROWSEINFO lpbi);
extern HRESULT       SHGetMalloc(LPMALLOC *ppMalloc);
extern BOOL           SHGetPathFromIDList(LPITEMIDLIST pidl, LPTSTR pszPath);

#ifdef __cplusplus
}
#endif

#endif /* SHIM_WIN32_TYPES_H */
