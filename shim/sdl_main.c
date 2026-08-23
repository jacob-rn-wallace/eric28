/*
 *   sdl_main.c
 *
 *   This file is part of the Emu28 macOS/Linux port (see ../CLAUDE.md).
 *
 *   SDL2 platform layer (milestone 4) - first slice. Proves the
 *   rendering pipeline built in milestones 1-3 works end to end in a
 *   real, on-screen SDL2 window: it calls DISPLAY.C's already-proven
 *   CreateMainBitmap/CreateLcdBitmap/UpdateMainDisplay directly
 *   against the real skin bitmap (skins/hp28c/REAL28C.BMP) and its
 *   real Lcd Zoom/Offset values (read out of REAL28C.KMI by hand for
 *   now), uploads the composited hWindowDC into an SDL2 texture, and
 *   presents it.
 *
 *   Deliberately NOT yet in scope (later slices of this same
 *   milestone):
 *     - Parsing a real .KML script end to end via KML.C's InitKML().
 *       InitKML's code path also touches FILES.C's ROM-loading/
 *       ROM-patch-checking functions (CheckForBeepPatch, MapRom, ...)
 *       for any skin that references a Rom line (REAL28CL.KML does) -
 *       that's FILES.C's real job, not this slice's. This file
 *       bypasses KML.C's parser and drives DISPLAY.C's bitmap/LCD
 *       setup directly instead.
 *     - The CPU-emulation worker thread (ENGINE.C's WorkerThread) and
 *       the real event/thread-sync primitives it needs
 *       (CreateThread/CreateEvent/WaitForSingleObject) - see
 *       win32_types.h's "thread/event synchronization" section for
 *       why those stayed declared-only through milestone 3: a real
 *       Win32 HANDLE is a polymorphic kernel object (WaitForSingleObject
 *       is called on both event and thread handles), which needs a
 *       tagged-handle design this file doesn't attempt yet.
 *     - Keyboard/mouse input (KEYBOARD.C's ScanKeyboard/KeyboardEvent,
 *       KML.C's MouseIsButton/MouseButtonDownAt/...).
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
#include "Emu28.h"
#include "kml.h"

#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>

/* ---- globals DISPLAY.C references via EMU28.H/KML.H --------------------------------
 * Normally defined in EMU28.C/KML.C, neither of which this slice
 * links (see the file header comment). */

HPALETTE hPalette = (HPALETTE)(intptr_t)1; /* dummy non-NULL: SelectPalette/RealizePalette are no-ops */
HWND     hWnd;                              /* set to a real opaque token in main(), once g_window exists */
HDC      hWindowDC;
CRITICAL_SECTION csGDILock;
CRITICAL_SECTION csLcdLock;
CHIPSET  Chipset; /* zero-initialized: display renders correctly in its "off" state without a ROM/CPU thread */
KmlAnnunciator pAnnunciator[7]; /* zeroed: no annunciators without KML.C's real InitAnnunciator */
BOOL     bAlwaysOnTop = FALSE;

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

HMENU GetMenu(HWND hwnd)
{
	(void)hwnd;
	return NULL; /* no native menu - not the plan for this port at all, per CLAUDE.md */
}

VOID DrawAnnunciator(UINT nId, BOOL bOn, DWORD dwColor)
{
	(void)nId;
	(void)bOn;
	(void)dwColor; /* KML.C's real implementation - unreachable this slice, pAnnunciator is all zeros */
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
	const char *bmpPath = argc > 1 ? argv[1] : "skins/hp28c/REAL28C.BMP";

	InitializeCriticalSection(&csGDILock);
	InitializeCriticalSection(&csLcdLock);

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

	if (!CreateMainBitmap(bmpPath)) {
		fprintf(stderr, "CreateMainBitmap(%s) failed\n", bmpPath);
		return 1;
	}

	nBackgroundW = winW;
	nBackgroundH = winH;
	nBackgroundX = 0;
	nBackgroundY = 0;
	nLcdX = 474; /* skins/hp28c/REAL28C.KMI: "Lcd { Offset 474 177  Zoom 2 }" */
	nLcdY = 177;
	nLcdZoom = 2;

	/* paint the full calculator body once, like EMU28.C's OnPaint would */
	BitBlt(hWindowDC, 0, 0, winW, winH, hMainDC, 0, 0, SRCCOPY);

	CreateLcdBitmap();   /* sets up the LCD compositing buffers, draws the initial (display-off) state */
	UpdateMainDisplay(); /* composites current LCD state onto hWindowDC (all-zero Chipset -> display off) */

	printf("Emu28: window open (close it to exit).\n");

	while (running) {
		SDL_Event e;
		void *pixels;
		int pitch, x, y;

		while (SDL_PollEvent(&e)) {
			if (e.type == SDL_QUIT)
				running = FALSE;
		}

		/* re-upload every frame: cheap at this resolution/frame rate,
		 * and side-steps needing a "did anything change" dirty flag
		 * before there's an actual CPU thread updating the display */
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
