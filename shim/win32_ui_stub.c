/*
 *   win32_ui_stub.c
 *
 *   This file is part of the Emu28 macOS/Linux port (see ../CLAUDE.md).
 *
 *   Milestone 7's whole-program link: every remaining Win32 function
 *   this shim declares (across win32_types.h and gdi.h's several
 *   "no SDL2 equivalent, CLAUDE.md already rules out reproducing this"
 *   sections - native dialogs/menus/common controls, resource
 *   loading, clipboard, DDE, GDI font/text output, and the handful of
 *   window-management calls DEBUGGER.C/KML.C reach for that
 *   shim/sdl_main.c's own window-management stubs don't already cover)
 *   but has never had a body anywhere, because nothing needed a real
 *   *link* of the files that call them until now. Every function here
 *   is a real, honest "this operation is unavailable" stub - same
 *   idiom as sound_stub.c's SoundOut and stegano_stub.c's
 *   SteganoDecodeHBm, just covering a much larger, more heterogeneous
 *   surface in one place since none of it shares sound_stub.c's or
 *   stegano_stub.c's narrow single-category focus. None of these are
 *   ever actually reached at runtime by anything this port currently
 *   drives (no code path creates a real dialog, menu, or clipboard
 *   transfer), so the exact failure semantics chosen below mostly
 *   only matter for "doesn't crash if something unexpected calls
 *   this," not for correctness of any real feature.
 *
 *   New code, not derived from any Win32 SDK header. GPL-2.0, as a
 *   derivative work of Emu28 (Copyright (C) 2002 Christoph
 *   Giesselink), since it exists solely to run Emu28's source.
 */

#include "win32_types.h"
#include "gdi.h"

#include <stdlib.h>

/* ---- menus ------------------------------------------------------------------------ */

BOOL AppendMenu(HMENU hMenu, UINT uFlags, UINT_PTR uIDNewItem, LPCTSTR lpNewItem)
{
	(void)hMenu; (void)uFlags; (void)uIDNewItem; (void)lpNewItem;
	return FALSE;
}

BOOL DestroyMenu(HMENU hMenu) { (void)hMenu; return TRUE; }
HMENU LoadMenu(HINSTANCE hInstance, LPCTSTR lpMenuName) { (void)hInstance; (void)lpMenuName; return NULL; }
HMENU GetSystemMenu(HWND hWnd, BOOL bRevert) { (void)hWnd; (void)bRevert; return NULL; }

BOOL TrackPopupMenu(HMENU hMenu, UINT uFlags, INT x, INT y, INT nReserved, HWND hWnd, CONST RECT *prcRect)
{
	(void)hMenu; (void)uFlags; (void)x; (void)y; (void)nReserved; (void)hWnd; (void)prcRect;
	return FALSE;
}

BOOL CheckMenuItem(HMENU hMenu, UINT uIDCheckItem, UINT uCheck) { (void)hMenu; (void)uIDCheckItem; (void)uCheck; return FALSE; }
BOOL EnableMenuItem(HMENU hMenu, UINT uIDEnableItem, UINT uEnable) { (void)hMenu; (void)uIDEnableItem; (void)uEnable; return FALSE; }
HMENU GetSubMenu(HMENU hMenu, INT nPos) { (void)hMenu; (void)nPos; return NULL; }
INT GetMenuItemCount(HMENU hMenu) { (void)hMenu; return -1; }
UINT GetMenuItemID(HMENU hMenu, INT nPos) { (void)hMenu; (void)nPos; return (UINT)-1; }
UINT GetMenuState(HMENU hMenu, UINT uId, UINT uFlags) { (void)hMenu; (void)uId; (void)uFlags; return (UINT)-1; }
INT GetMenuString(HMENU hMenu, UINT uIDItem, LPTSTR lpString, INT cchMax, UINT uFlag)
{
	(void)hMenu; (void)uIDItem; (void)uFlag;
	if (lpString && cchMax > 0)
		lpString[0] = '\0';
	return 0;
}
BOOL InsertMenu(HMENU hMenu, UINT uPosition, UINT uFlags, UINT_PTR uIDNewItem, LPCTSTR lpNewItem)
{
	(void)hMenu; (void)uPosition; (void)uFlags; (void)uIDNewItem; (void)lpNewItem;
	return FALSE;
}
BOOL DeleteMenu(HMENU hMenu, UINT uPosition, UINT uFlags) { (void)hMenu; (void)uPosition; (void)uFlags; return FALSE; }
BOOL IsMenu(HMENU hMenu) { (void)hMenu; return FALSE; }

/* ---- dialogs ------------------------------------------------------------------------ */

INT_PTR DialogBox(HINSTANCE hInstance, LPCTSTR lpTemplate, HWND hWndParent, DLGPROC lpDialogFunc)
{
	(void)hInstance; (void)lpTemplate; (void)hWndParent; (void)lpDialogFunc;
	return -1; /* real Win32's own documented failure return */
}
INT_PTR DialogBoxParam(HINSTANCE hInstance, LPCTSTR lpTemplate, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam)
{
	(void)hInstance; (void)lpTemplate; (void)hWndParent; (void)lpDialogFunc; (void)dwInitParam;
	return -1;
}
BOOL EndDialog(HWND hDlg, INT_PTR nResult) { (void)hDlg; (void)nResult; return FALSE; }
HWND GetDlgItem(HWND hDlg, INT nIDDlgItem) { (void)hDlg; (void)nIDDlgItem; return NULL; }
LRESULT SendDlgItemMessage(HWND hDlg, INT nIDDlgItem, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	(void)hDlg; (void)nIDDlgItem; (void)Msg; (void)wParam; (void)lParam;
	return 0;
}
BOOL CheckDlgButton(HWND hDlg, INT nIDButton, UINT uCheck) { (void)hDlg; (void)nIDButton; (void)uCheck; return FALSE; }
UINT IsDlgButtonChecked(HWND hDlg, INT nIDButton) { (void)hDlg; (void)nIDButton; return 0; }
BOOL EnableWindow(HWND hWnd, BOOL bEnable) { (void)hWnd; (void)bEnable; return FALSE; }
BOOL GetOpenFileName(LPOPENFILENAME lpofn) { (void)lpofn; return FALSE; }
BOOL GetSaveFileName(LPOPENFILENAME lpofn) { (void)lpofn; return FALSE; }
BOOL PostMessage(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) { (void)hWnd; (void)Msg; (void)wParam; (void)lParam; return FALSE; }
LRESULT SendMessage(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) { (void)hWnd; (void)Msg; (void)wParam; (void)lParam; return 0; }
UINT GetDlgItemText(HWND hDlg, INT nIDDlgItem, LPTSTR lpString, INT cchMax)
{
	(void)hDlg; (void)nIDDlgItem;
	if (lpString && cchMax > 0)
		lpString[0] = '\0';
	return 0;
}
BOOL SetDlgItemText(HWND hDlg, INT nIDDlgItem, LPCTSTR lpString) { (void)hDlg; (void)nIDDlgItem; (void)lpString; return FALSE; }
HWND CreateDialog(HINSTANCE hInstance, LPCTSTR lpTemplate, HWND hWndParent, DLGPROC lpDialogFunc)
{
	(void)hInstance; (void)lpTemplate; (void)hWndParent; (void)lpDialogFunc;
	return NULL;
}

/* ---- common controls (toolbar/tooltips) ---------------------------------------------- */

VOID InitCommonControls(VOID) { }
HWND CreateToolbarEx(HWND hwnd, DWORD ws, UINT wID, INT nBitmaps, HINSTANCE hBMInst, UINT_PTR wBMID,
                      LPTBBUTTON lpButtons, INT iNumButtons, INT dxButton, INT dyButton,
                      INT dxBitmap, INT dyBitmap, UINT uStructSize)
{
	(void)hwnd; (void)ws; (void)wID; (void)nBitmaps; (void)hBMInst; (void)wBMID;
	(void)lpButtons; (void)iNumButtons; (void)dxButton; (void)dyButton;
	(void)dxBitmap; (void)dyBitmap; (void)uStructSize;
	return NULL;
}

/* ---- resource loading ------------------------------------------------------------------ */

HRSRC FindResource(HINSTANCE hModule, LPCTSTR lpName, LPCTSTR lpType) { (void)hModule; (void)lpName; (void)lpType; return NULL; }
HGLOBAL LoadResource(HINSTANCE hModule, HRSRC hResInfo) { (void)hModule; (void)hResInfo; return NULL; }
LPVOID LockResource(HGLOBAL hResData) { (void)hResData; return NULL; }
BOOL FreeResource(HGLOBAL hResData) { (void)hResData; return TRUE; }
HBITMAP LoadBitmap(HINSTANCE hInstance, LPCTSTR lpBitmapName) { (void)hInstance; (void)lpBitmapName; return NULL; }
HICON LoadIcon(HINSTANCE hInstance, LPCTSTR lpIconName) { (void)hInstance; (void)lpIconName; return NULL; }
HANDLE LoadImage(HINSTANCE hInst, LPCTSTR name, UINT type, INT cx, INT cy, UINT fuLoad)
{
	(void)hInst; (void)name; (void)type; (void)cx; (void)cy; (void)fuLoad;
	return NULL;
}

/* ---- clipboard -------------------------------------------------------------------------
 * Real NSPasteboard/X11-selection access belongs to a real window-
 * system integration this port doesn't have yet (see win32_types.h's
 * clipboard section) - every call here fails cleanly, exactly as if
 * the clipboard were simply never available. */

HANDLE GlobalAlloc(UINT uFlags, SIZE_T dwBytes) { (void)uFlags; return dwBytes ? malloc(dwBytes) : NULL; }
LPVOID GlobalLock(HANDLE hMem) { return hMem; } /* this shim's GlobalAlloc hands back a plain pointer already */
BOOL GlobalUnlock(HANDLE hMem) { (void)hMem; return FALSE; }
HANDLE GlobalFree(HANDLE hMem) { free(hMem); return NULL; }
BOOL OpenClipboard(HWND hWndNewOwner) { (void)hWndNewOwner; return FALSE; }
BOOL CloseClipboard(VOID) { return FALSE; }
BOOL EmptyClipboard(VOID) { return FALSE; }
HANDLE SetClipboardData(UINT uFormat, HANDLE hMem) { (void)uFormat; (void)hMem; return NULL; }
HANDLE GetClipboardData(UINT uFormat) { (void)uFormat; return NULL; }
BOOL IsClipboardFormatAvailable(UINT format) { (void)format; return FALSE; }
BOOL MessageBeep(UINT uType) { (void)uType; return FALSE; }

/* ---- DDE ---------------------------------------------------------------------------------
 * No macOS/Linux/SDL2 equivalent at all - see win32_types.h's DDE
 * section. DdeCallback (DDESERV.C) checks each of these for failure
 * before using their result, so honest failure returns are enough for
 * it to bail out cleanly if ever reached. */

DWORD DdeQueryString(DWORD idInst, HSZ hsz, LPTSTR psz, DWORD cchMax, INT iCodePage)
{
	(void)idInst; (void)hsz; (void)iCodePage;
	if (psz && cchMax > 0)
		psz[0] = '\0';
	return 0;
}
LPBYTE DdeAccessData(HDDEDATA hData, LPDWORD pcbDataSize) { (void)hData; if (pcbDataSize) *pcbDataSize = 0; return NULL; }
BOOL DdeUnaccessData(HDDEDATA hData) { (void)hData; return FALSE; }
HDDEDATA DdeCreateDataHandle(DWORD idInst, LPBYTE pSrc, DWORD cb, DWORD cbOff, HSZ hszItem, UINT wFmt, UINT afCmd)
{
	(void)idInst; (void)pSrc; (void)cb; (void)cbOff; (void)hszItem; (void)wFmt; (void)afCmd;
	return NULL;
}

/* ---- fonts and text output (gdi.h) -------------------------------------------------------
 * Real text rendering is a much bigger undertaking than this shim's
 * flat-color GDI subset, and would only ever be exercised by the
 * debugger window this project doesn't render - see gdi.h's own
 * comment on this section. */

HFONT CreateFont(INT cHeight, INT cWidth, INT cEscapement, INT cOrientation, INT cWeight,
                  DWORD bItalic, DWORD bUnderline, DWORD bStrikeOut, DWORD iCharSet,
                  DWORD iOutPrecision, DWORD iClipPrecision, DWORD iQuality, DWORD iPitchAndFamily,
                  LPCTSTR pszFaceName)
{
	(void)cHeight; (void)cWidth; (void)cEscapement; (void)cOrientation; (void)cWeight;
	(void)bItalic; (void)bUnderline; (void)bStrikeOut; (void)iCharSet;
	(void)iOutPrecision; (void)iClipPrecision; (void)iQuality; (void)iPitchAndFamily; (void)pszFaceName;
	return NULL;
}
BOOL ExtTextOut(HDC hdc, INT x, INT y, UINT fuOptions, CONST RECT *lprc,
                LPCTSTR lpString, UINT cbCount, CONST INT *lpDx)
{
	(void)hdc; (void)x; (void)y; (void)fuOptions; (void)lprc; (void)lpString; (void)cbCount; (void)lpDx;
	return FALSE;
}
BOOL GetTextMetrics(HDC hdc, LPTEXTMETRIC lptm)
{
	(void)hdc;
	if (lptm)
		ZeroMemory(lptm, sizeof(*lptm));
	return FALSE;
}
INT GetDeviceCaps(HDC hdc, INT nIndex) { (void)hdc; (void)nIndex; return 0; }
BOOL DrawFocusRect(HDC hdc, CONST RECT *lprc) { (void)hdc; (void)lprc; return FALSE; }
COLORREF GetTextColor(HDC hdc) { (void)hdc; return 0; }
COLORREF SetTextColor(HDC hdc, COLORREF color) { (void)hdc; (void)color; return 0; }
COLORREF GetBkColor(HDC hdc) { (void)hdc; return 0; }

/* ---- window regions (FILES.C's CreateRgnFromBitmap, KML.C's shaped window) ---------------- */

HRGN ExtCreateRegion(LPVOID lpXform, DWORD nCount, CONST RGNDATA *lpRgnData)
{
	(void)lpXform; (void)nCount; (void)lpRgnData;
	return NULL;
}
INT SetWindowRgn(HWND hWnd, HRGN hRgn, BOOL bRedraw) { (void)hWnd; (void)hRgn; (void)bRedraw; return 0; }

/* ---- cursor creation (CURSOR.C) ------------------------------------------------------------ */

HCURSOR CreateCursor(HINSTANCE hInst, INT xHotSpot, INT yHotSpot, INT nWidth, INT nHeight,
                      CONST VOID *pvANDPlane, CONST VOID *pvXORPlane)
{
	(void)hInst; (void)xHotSpot; (void)yHotSpot; (void)nWidth; (void)nHeight; (void)pvANDPlane; (void)pvXORPlane;
	return NULL;
}
HCURSOR SetCursor(HCURSOR hCursor) { (void)hCursor; return NULL; }

/* ---- power status (MOPS.C's low-battery check) ------------------------------------------- */

BOOL GetSystemPowerStatus(LPSYSTEM_POWER_STATUS lpSystemPowerStatus)
{
	/* honest "always on AC power, battery not applicable" - the safe
	 * default for a low-battery auto-shutdown check that should never
	 * fire on a desktop/laptop dev machine. */
	if (lpSystemPowerStatus) {
		ZeroMemory(lpSystemPowerStatus, sizeof(*lpSystemPowerStatus));
		lpSystemPowerStatus->ACLineStatus = 1;
		lpSystemPowerStatus->BatteryFlag = 128; /* BATTERY_FLAG_NO_BATTERY */
	}
	return TRUE;
}

/* ---- winmm periodic timer, remaining bits (TIMER.C) ---------------------------------------
 * timeGetTime itself is real (win32_types.h aliases it to
 * GetTickCount); the periodic-callback family below never got a real
 * implementation - see win32_types.h's winmm section. */

UINT timeBeginPeriod(UINT uPeriod) { (void)uPeriod; return 0; }
UINT timeEndPeriod(UINT uPeriod) { (void)uPeriod; return 0; }
UINT timeGetDevCaps(LPTIMECAPS ptc, UINT cbtc)
{
	(void)cbtc;
	if (ptc) {
		ptc->wPeriodMin = 1;
		ptc->wPeriodMax = 1000;
	}
	return 0;
}

/* ---- shell folder browsing (KML.C's "choose a skin folder" dialog) ------------------------ */

LPITEMIDLIST SHBrowseForFolder(LPBROWSEINFO lpbi) { (void)lpbi; return NULL; }
HRESULT SHGetMalloc(LPMALLOC *ppMalloc) { (void)ppMalloc; return -1; /* E_FAIL */ }
BOOL SHGetPathFromIDList(LPITEMIDLIST pidl, LPTSTR pszPath) { (void)pidl; if (pszPath) pszPath[0] = '\0'; return FALSE; }

/* ---- message boxes ------------------------------------------------------------------------- */

int MessageBox(HWND hWnd, LPCTSTR lpText, LPCTSTR lpCaption, UINT uType)
{
	(void)hWnd; (void)lpText; (void)lpCaption; (void)uType;
	return IDCANCEL; /* the one answer that's always safe regardless of which button set the caller expects */
}

/* ---- miscellaneous window-management calls not already covered by sdl_main.c -------------- */

HWND ChildWindowFromPointEx(HWND hwndParent, POINT pt, UINT uFlags) { (void)hwndParent; (void)pt; (void)uFlags; return NULL; }
INT GetDlgCtrlID(HWND hWnd) { (void)hWnd; return 0; }
HWND GetParent(HWND hWnd) { (void)hWnd; return NULL; }
HWND GetActiveWindow(VOID) { return NULL; }
HWND GetLastActivePopup(HWND hWndOwner) { return hWndOwner; }
BOOL GetWindowRect(HWND hWnd, LPRECT lpRect) { (void)hWnd; if (lpRect) SetRect(lpRect, 0, 0, 0, 0); return FALSE; }
INT GetWindowText(HWND hWnd, LPTSTR lpString, INT nMaxCount) { (void)hWnd; if (lpString && nMaxCount > 0) lpString[0] = '\0'; return 0; }
INT GetWindowTextLength(HWND hWnd) { (void)hWnd; return 0; }
BOOL SetWindowText(HWND hWnd, LPCTSTR lpString) { (void)hWnd; (void)lpString; return FALSE; }
LONG_PTR SetWindowLongPtr(HWND hWnd, INT nIndex, LONG_PTR dwNewLong) { (void)hWnd; (void)nIndex; (void)dwNewLong; return 0; }
BOOL ShowWindow(HWND hWnd, INT nCmdShow) { (void)hWnd; (void)nCmdShow; return FALSE; }
HDC GetDC(HWND hWnd) { (void)hWnd; return NULL; }
INT ReleaseDC(HWND hWnd, HDC hDC) { (void)hWnd; (void)hDC; return 0; }
HWND SetFocus(HWND hWnd) { (void)hWnd; return NULL; }
UINT GetSysColor(INT nIndex) { (void)nIndex; return 0; }
LONG_PTR GetClassLongPtr(HWND hWnd, INT nIndex) { (void)hWnd; (void)nIndex; return 0; }
BOOL DestroyWindow(HWND hWnd) { (void)hWnd; return FALSE; }
BOOL DragAcceptFiles(HWND hWnd, BOOL fAccept) { (void)hWnd; (void)fAccept; return FALSE; }

/* ---- MRU.C's path helpers (declared alongside its menu API, win32_types.h) ----------------- */

DWORD GetCurrentDirectory(DWORD nBufferLength, LPTSTR lpBuffer)
{
	if (lpBuffer && nBufferLength > 0)
		lpBuffer[0] = '\0';
	return 0;
}

DWORD GetFullPathName(LPCTSTR lpFileName, DWORD nBufferLength, LPTSTR lpBuffer, LPTSTR *lpFilePart)
{
	(void)lpFileName;
	if (lpBuffer && nBufferLength > 0)
		lpBuffer[0] = '\0';
	if (lpFilePart)
		*lpFilePart = lpBuffer;
	return 0;
}

/* ---- keyboard layout (KML.C's per-locale key mapping) -------------------------------------- */

BOOL GetKeyboardLayoutName(LPTSTR pwszKLID)
{
	if (pwszKLID)
		pwszKLID[0] = '\0';
	return FALSE;
}
