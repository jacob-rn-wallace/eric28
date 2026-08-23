/*
 *   sdl_main.c
 *
 *   This file is part of the Emu28 macOS/Linux port (see ../CLAUDE.md).
 *
 *   SDL2 platform layer. Milestone 4's first slice proved the
 *   rendering pipeline end to end against hand-set values; milestone 9
 *   replaced that with the real thing: KML.C's InitKML() parses the
 *   real skin script (skins/hp28c/REAL28CL.KML), whose Global/
 *   Background/Lcd blocks call FILES.C's real MapRom/CreateMainBitmap
 *   and set nBackgroundX/Y/W/H and nLcdX/Y/nLcdZoom for real - no more
 *   hand-read KMI values - and ENGINE.C's real WorkerThread runs as an
 *   actual CPU-emulation thread against the real ROM, with
 *   UpdateMainDisplay() polled once per frame from the main thread
 *   (cross-thread safe via csGDILock) since the winmm periodic-timer
 *   callback StartDisplay() would normally use is still a no-op stub
 *   (see timeSetEvent below) - see CLAUDE.md's milestone 9 notes for
 *   the full writeup, including the real "Memory Lost" boot screen
 *   this produces against a virgin ROM image.
 *
 *   Still NOT in scope:
 *     - Keyboard/mouse input (KEYBOARD.C's ScanKeyboard/KeyboardEvent,
 *       KML.C's MouseIsButton/MouseButtonDownAt/...) - the emulator
 *       runs and boots, but nothing can be typed at it yet.
 *     - A real winmm periodic timer (see timeSetEvent's own comment
 *       below) - StartDisplay()'s intended periodic redraw never
 *       fires; this file's own per-frame UpdateMainDisplay() poll is
 *       what actually keeps the LCD live instead.
 *     - Clean thread shutdown - hThread is never joined on exit (the
 *       process just exits with the worker thread mid-flight); fine
 *       for now, not fine for a real "File > Exit."
 *     - Window resizing, menus, dialogs - see win32_types.h's "window
 *       management" section; the stub functions below are the
 *       minimum needed to satisfy DISPLAY.C's *other* functions
 *       (StartDisplay/StopDisplay/ResizeWindow/UpdateAnnunciators -
 *       not called by this slice, but still compiled into DISPLAY.C's
 *       object file, so still needing to resolve at link time) - most
 *       are honest no-ops for this architecture (e.g.
 *       InvalidateRect: this slice redraws the whole frame every
 *       loop iteration already, so a dirty-rect invalidation call has
 *       nothing useful to do), not placeholders for something more.
 *
 *   New code, not derived from any Win32 SDK header. GPL-2.0, as a
 *   derivative work of Emu28 (Copyright (C) 2002 Christoph
 *   Giesselink), since it exists solely to run Emu28's source.
 */

#include "win32_types.h"
#include "gdi.h"
#include "win32_handle.h"
#include "Emu28.h"
#include "kml.h"

#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* ---- globals DISPLAY.C references via EMU28.H/KML.H --------------------------------
 * Normally defined in EMU28.C/KML.C, neither of which this slice
 * links (see the file header comment). */

HPALETTE hPalette; /* starts real-Win32-correct NULL, not a dummy sentinel: milestone 9's KML.C now calls the real
                     * CreatePalette/DeleteObject on this (FILES.C's DecodeBmp creates one on demand when
                     * hPalette == NULL, KillKML deletes it when non-NULL) - a fake non-NULL placeholder value
                     * here made KillKML's real DeleteObject(hPalette) segfault the first time this file
                     * actually linked against KML.C's real KillKML() instead of nothing calling it at all. */
HWND     hWnd;                              /* set to a real opaque token in main(), once g_window exists */
HDC      hWindowDC;
CRITICAL_SECTION csGDILock;
CRITICAL_SECTION csLcdLock;
CHIPSET  Chipset; /* zero-initialized: display renders correctly in its "off" state without a ROM/CPU thread */
KmlAnnunciator pAnnunciator[7]; /* zeroed: no annunciators without KML.C's real InitAnnunciator */
BOOL     bAlwaysOnTop = FALSE;

/* ---- the rest of EMU28.C's globals -------------------------------------------------
 * Milestone 7's whole-program link needs every global EMU28.C would
 * normally define, not just the handful DISPLAY.C's own slice needed.
 * Settings flags default FALSE (SETTINGS.C's ReadSettings overwrites
 * them with real values off Emu28.ini at startup - these are only the
 * pre-ReadSettings defaults); dialog/thread/lock handles are real but
 * empty, since nothing has created an actual dialog or CPU thread yet
 * (that's still ahead - see the file header comment); szAppName/
 * szTopic/szTitle/uCF_HpObj only matter to DDE, which is declared-only
 * everywhere else in this shim too, so their exact values are inert. */

BOOL   bAutoSave = FALSE;
BOOL   bAutoSaveOnExit = FALSE;
BOOL   bSaveDefConfirm = FALSE;
BOOL   bStartupBackup = FALSE;
BOOL   bAlwaysDisplayLog = FALSE;
BOOL   bLoadObjectWarning = FALSE;
BOOL   bShowTitle = TRUE;
BOOL   bShowMenu = TRUE;
BOOL   bActFollowsMouse = FALSE;
BOOL   bClientWinMove = FALSE;
BOOL   bSingleInstance = FALSE;

CRITICAL_SECTION csKeyLock;
CRITICAL_SECTION csTLock;
CRITICAL_SECTION csSlowLock;
CRITICAL_SECTION csDbgLock;

DWORD  dwTColor;
DWORD  dwTColorTol;
DWORD  dwWakeupDelay;
DWORD  dwWaveTime;
DWORD  dwWaveVol;

HINSTANCE hApp = (HINSTANCE)(intptr_t)1; /* dummy non-NULL, same reasoning as hPalette - only ever passed to declared-only functions */
HWND   hDlgDebug;
HWND   hDlgFind;
HWND   hDlgProfile;
HWND   hDlgRplObjView;
HANDLE hEventShutdn;
HPALETTE hOldPalette;
HRGN   hRgn;
HCURSOR hCursorArrow;
HCURSOR hCursorHand;
HANDLE hThread;
DWORD  idDdeInst;
LARGE_INTEGER lFreq;
static CHAR szAppNameBuf[] = "Emu28";
static CHAR szTopicBuf[] = "System";
static CHAR szTitleBuf[] = "Emu28";
LPTSTR szAppName = szAppNameBuf;
LPTSTR szTopic = szTopicBuf;
LPTSTR szTitle = szTitleBuf;
UINT   uCF_HpObj;
UINT   uWaveDevId;

static SDL_Window *g_window;

/* ---- window-management stubs (see file header comment) ----------------------------- */

BOOL AdjustWindowRect(LPRECT lpRect, DWORD dwStyle, BOOL bMenu)
{
	(void)dwStyle;
	(void)bMenu;
	return lpRect != NULL; /* no window-decoration accounting yet - not needed until resizing is */
}

BOOL GetClientRect(HWND hwnd, LPRECT lpRect)
{
	int w = 0, h = 0;

	(void)hwnd;
	if (g_window)
		SDL_GetWindowSize(g_window, &w, &h);
	if (lpRect)
		SetRect(lpRect, 0, 0, w, h);
	return TRUE;
}

BOOL SetWindowPos(HWND hwnd, HWND hWndInsertAfter, INT x, INT y, INT cx, INT cy, UINT uFlags)
{
	(void)hwnd;
	(void)hWndInsertAfter;
	(void)x;
	(void)y;
	(void)cx;
	(void)cy;
	(void)uFlags;
	return TRUE; /* no resizable window yet - ResizeWindow isn't called by this slice */
}

LONG_PTR GetWindowLongPtr(HWND hwnd, INT nIndex)
{
	(void)hwnd;
	(void)nIndex; /* GWL_STYLE et al. are native window-style bits with no SDL2 equivalent */
	return 0;
}

BOOL InvalidateRect(HWND hwnd, CONST RECT *lpRect, BOOL bErase)
{
	(void)hwnd;
	(void)lpRect;
	(void)bErase;
	return TRUE; /* this slice redraws the whole frame every loop iteration already */
}

BOOL SetWindowOrgEx(HDC hdc, INT x, INT y, LPVOID lpPoint)
{
	(void)hdc;
	(void)x;
	(void)y;
	(void)lpPoint; /* no menu-bar-height DC origin offset to account for yet */
	return TRUE;
}

BOOL GetWindowPlacement(HWND hwnd, WINDOWPLACEMENT *lpwndpl)
{
	int x = 0, y = 0, w = 0, h = 0;

	(void)hwnd;
	if (g_window) {
		SDL_GetWindowPosition(g_window, &x, &y);
		SDL_GetWindowSize(g_window, &w, &h);
	}
	lpwndpl->flags = 0;
	lpwndpl->showCmd = 0;
	lpwndpl->ptMinPosition.x = 0;
	lpwndpl->ptMinPosition.y = 0;
	lpwndpl->ptMaxPosition.x = 0;
	lpwndpl->ptMaxPosition.y = 0;
	SetRect(&lpwndpl->rcNormalPosition, x, y, x + w, y + h);
	return TRUE;
}

BOOL SetWindowPlacement(HWND hwnd, CONST WINDOWPLACEMENT *lpwndpl)
{
	(void)hwnd;
	if (g_window)
		SDL_SetWindowPosition(g_window, lpwndpl->rcNormalPosition.left, lpwndpl->rcNormalPosition.top);
	return TRUE;
}

VOID SetWindowTitle(LPCTSTR szString)
{
	if (g_window)
		SDL_SetWindowTitle(g_window, szString);
}

VOID ForceForegroundWindow(HWND hWnd)
{
	(void)hWnd; /* real activation-stealing trick for a minimized/background window - not meaningful without native window chrome */
}

VOID CopyItemsToClipboard(HWND hWnd)
{
	(void)hWnd; /* clipboard access is declared-only (see win32_types.h's clipboard section) - nothing to copy into */
}

HMENU GetMenu(HWND hwnd)
{
	(void)hwnd;
	return NULL; /* no native menu - not the plan for this port at all, per CLAUDE.md */
}

UINT timeSetEvent(UINT uDelay, UINT uResolution, LPTIMECALLBACK lpTimeProc, DWORD_PTR dwUser, UINT fuEvent)
{
	(void)uDelay;
	(void)uResolution;
	(void)lpTimeProc;
	(void)dwUser;
	(void)fuEvent;
	return 1; /* periodic display-refresh timer - deferred with the CPU thread work */
}

UINT timeKillEvent(UINT uTimerID)
{
	(void)uTimerID;
	return 0;
}

/* ---- main -------------------------------------------------------------------------- */

int main(int argc, char **argv)
{
	const INT winW = 790, winH = 662; /* skins/hp28c/REAL28C.BMP's real dimensions */
	SDL_Renderer *renderer;
	SDL_Texture *texture;
	HBITMAP hWindowBitmap;
	BOOL running = TRUE;
	const char *kmlName = argc > 1 ? argv[1] : "REAL28CL.KML";
	char cwd[MAX_PATH];

	InitializeCriticalSection(&csGDILock);
	InitializeCriticalSection(&csLcdLock);
	InitializeCriticalSection(&csKeyLock);
	InitializeCriticalSection(&csTLock);
	InitializeCriticalSection(&csSlowLock);
	InitializeCriticalSection(&csDbgLock);
	QueryPerformanceFrequency(&lFreq);

	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
		return 1;
	}

	g_window = SDL_CreateWindow("Emu28", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
	                             winW, winH, SDL_WINDOW_SHOWN);
	hWnd = (HWND)(intptr_t)1; /* opaque non-NULL token - nothing dereferences it structurally yet */
	if (g_window == NULL) {
		fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
		return 1;
	}

	renderer = SDL_CreateRenderer(g_window, -1, SDL_RENDERER_ACCELERATED);
	texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING, winW, winH);
	if (renderer == NULL || texture == NULL) {
		fprintf(stderr, "SDL renderer/texture setup failed: %s\n", SDL_GetError());
		return 1;
	}

	/* hWindowDC: a plain shim DC, same as any other - see gdi.h's
	 * header comment on why real device contexts aren't a special
	 * case there. What makes this one "the window" is purely that
	 * this file uploads its pixel buffer into the SDL2 texture below. */
	hWindowDC = CreateCompatibleDC(NULL);
	hWindowBitmap = CreateCompatibleBitmap(NULL, winW, winH);
	SelectObject(hWindowDC, hWindowBitmap);

	/* szEmuDirectory/szCurrentDirectory bracket every relative file
	 * access InitKML/MapRom/CreateMainBitmap make (each does
	 * SetCurrentDirectory(szEmuDirectory) ... SetCurrentDirectory(
	 * szCurrentDirectory) around its own CreateFile calls) - real
	 * EMU28.C sets these once at startup to the install directory and
	 * the launch-time CWD respectively; this port's equivalent is the
	 * skin directory and wherever the process was actually launched
	 * from. getcwd(), not a relative "." literal, because the second
	 * SetCurrentDirectory(szEmuDirectory) call (MapRom's, inside
	 * InitKML's own Rom-line handling) needs an absolute path to
	 * return to - by then the process's real CWD is already
	 * skins/hp28c, not wherever it started. */
	if (getcwd(cwd, sizeof(cwd)) == NULL) {
		fprintf(stderr, "getcwd failed\n");
		return 1;
	}
	lstrcpyn(szCurrentDirectory, cwd, ARRAYSIZEOF(szCurrentDirectory));
	lstrcpyn(szEmuDirectory, cwd, ARRAYSIZEOF(szEmuDirectory));
	lstrcat(szEmuDirectory, "/skins/hp28c");

	/* InitKML (KML.C) parses the real .KML script end to end: its
	 * Global block's Rom/Bitmap lines call FILES.C's real MapRom/
	 * CreateMainBitmap, its Background/Lcd blocks set
	 * nBackgroundX/Y/W/H and nLcdX/Y/nLcdZoom for real (no more hand-
	 * read values), and it calls DISPLAY.C's ResizeMainBitmap/
	 * CreateLcdBitmap internally too - see KML.C's InitGlobal/
	 * InitBackground/InitLcd. bNoLog=TRUE skips the "always show the
	 * parse log" path on success (win32_ui_stub.c's DialogBoxParam
	 * stub would otherwise make that path look like the user hit
	 * Cancel - see CLAUDE.md's milestone 9 notes for why that's safe
	 * either way, just noisier than needed here). */
	if (!InitKML((LPTSTR)kmlName, TRUE)) {
		fprintf(stderr, "InitKML(%s) failed - is skins/hp28c/HP28C.ROM present (see CLAUDE.md)?\n", kmlName);
		return 1;
	}

	/* paint the full calculator body once, like EMU28.C's OnPaint would */
	BitBlt(hWindowDC, 0, 0, winW, winH, hMainDC, 0, 0, SRCCOPY);
	UpdateMainDisplay(); /* composites current LCD state onto hWindowDC (all-zero Chipset -> display off) */

	/* ENGINE.C's own nNextState starts at SM_RUN (its static
	 * initializer, not something this file sets), so WorkerThread
	 * begins executing real opcodes against the just-loaded ROM the
	 * instant this thread starts - no separate SwitchToState(SM_RUN)
	 * call needed. hEventShutdn has to be a real event before that
	 * happens: the very first opcode loop that hits a SHUTDN
	 * (real hardware's own "go to sleep waiting for input" state -
	 * expected quickly, since nothing can be typed at it yet) calls
	 * WaitForSingleObject(hEventShutdn, INFINITE), and this shim's
	 * WaitForSingleObject dereferences its HANDLE argument
	 * unconditionally - a NULL hEventShutdn here would crash the
	 * worker thread within its first few hundred opcodes, not at
	 * startup, which is what made this worth calling out explicitly. */
	hEventShutdn = CreateEvent(NULL, FALSE, FALSE, NULL);
	hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)WorkerThread, NULL, 0, NULL);

	printf("Emu28: window open (close it to exit).\n");

	while (running) {
		SDL_Event e;
		void *pixels;
		int pitch, x, y;

		while (SDL_PollEvent(&e)) {
			if (e.type == SDL_QUIT)
				running = FALSE;
		}

		/* the worker thread changes Chipset/the LCD state continuously
		 * once running; re-composite from it every frame rather than
		 * waiting for a StartDisplay()-driven timer callback that
		 * doesn't really fire yet (see this file's header comment).
		 * Cross-thread safe: UpdateMainDisplay() takes csGDILock
		 * internally, same as the worker thread's own writes do. */
		UpdateMainDisplay();

		/* re-upload every frame: cheap at this resolution/frame rate,
		 * and side-steps needing a "did anything change" dirty flag */
		SDL_LockTexture(texture, NULL, &pixels, &pitch);
		for (y = 0; y < winH; ++y) {
			Uint32 *row = (Uint32 *)((BYTE *)pixels + y * pitch);
			for (x = 0; x < winW; ++x)
				row[x] = 0xFF000000u | (GetPixel(hWindowDC, x, y) & 0x00FFFFFFu);
		}
		SDL_UnlockTexture(texture);

		SDL_RenderClear(renderer);
		SDL_RenderCopy(renderer, texture, NULL, NULL);
		SDL_RenderPresent(renderer);
		SDL_Delay(16);
	}

	DestroyLcdBitmap();
	DestroyMainBitmap();
	SDL_DestroyTexture(texture);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(g_window);
	SDL_Quit();
	return 0;
}
