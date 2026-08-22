/*
 *   gdi.h
 *
 *   This file is part of the Emu28 macOS/Linux port (see ../CLAUDE.md).
 *
 *   GDI-drawing shim (milestone 3): a small software rasterizer
 *   implementing the specific subset of Win32 GDI that
 *   vendor/emu28-upstream/DISPLAY.C uses to composite the LCD/
 *   annunciator display - CreateCompatibleDC/CreateDIBSection-style
 *   memory device contexts backed by an owned pixel buffer, BitBlt/
 *   PatBlt against them (including DISPLAY.C's own two custom
 *   ternary ROPs - BitBlt evaluates any ROP DWORD generally, see
 *   gdi.c's rop3(), so it doesn't need to know their names), and
 *   solid-color brushes. Real device contexts (a live on-screen
 *   window) and window-management calls (SetWindowPos,
 *   InvalidateRect, ...) are explicitly NOT here - see
 *   win32_types.h's "window management" section - they need an
 *   actual SDL2 window (milestone 4) to mean anything, and are
 *   declared-only there.
 *
 *   KML.C needs a superset of this (StretchBlt, GetObject,
 *   CreateBitmap, GetPixel, SetBkColor, plus several window/shell
 *   APIs entirely outside GDI) - not attempted here; see CLAUDE.md's
 *   milestone 3 notes for why this pass scoped down to DISPLAY.C.
 *
 *   Design note: every bitmap this shim creates stores pixels as
 *   packed 32-bit words numerically equal to their COLORREF value
 *   (word == RGB(r,g,b), i.e. R in the low byte) - chosen so
 *   CreateSolidBrush's COLORREF argument needs no conversion at the
 *   API boundary. The one exception is an 8-bit paletted DIB section
 *   (DISPLAY.C's `bmiLcd` mask bitmap): BitBlt does the palette
 *   lookup into 32-bit color when blitting from it, exactly mirroring
 *   what real GDI does when blitting between DCs of different color
 *   depths.
 *
 *   New code, not derived from any Win32 SDK header. GPL-2.0, as a
 *   derivative work of Emu28 (Copyright (C) 2002 Christoph
 *   Giesselink), since it exists solely to run Emu28's source.
 */

#ifndef SHIM_GDI_H
#define SHIM_GDI_H

#include "win32_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ---- Windows bitmap-file/DIB format types ------------------------------------- */

typedef struct {
	BYTE rgbBlue;
	BYTE rgbGreen;
	BYTE rgbRed;
	BYTE rgbReserved;
} RGBQUAD;

typedef struct {
	DWORD biSize;
	LONG  biWidth;
	LONG  biHeight;
	WORD  biPlanes;
	WORD  biBitCount;
	DWORD biCompression;
	DWORD biSizeImage;
	LONG  biXPelsPerMeter;
	LONG  biYPelsPerMeter;
	DWORD biClrUsed;
	DWORD biClrImportant;
} BITMAPINFOHEADER;

typedef struct {
	BITMAPINFOHEADER bmiHeader;
	RGBQUAD           bmiColors[1];
} BITMAPINFO;

#pragma pack(push, 2)
typedef struct {
	WORD  bfType;
	DWORD bfSize;
	WORD  bfReserved1;
	WORD  bfReserved2;
	DWORD bfOffBits;
} BITMAPFILEHEADER;
#pragma pack(pop)

#define BI_RGB         0
#define DIB_RGB_COLORS 0

/* Generic GDI object handle - what SelectObject actually traffics in,
 * since a DC can have a bitmap, brush, pen, etc. selected into it.
 * Callers always cast the result back to the concrete handle type
 * they know it is (HBITMAP, HBRUSH, ...), same as real Win32 code
 * does. */
typedef void* HGDIOBJ;

#define RGB(r, g, b) ((COLORREF)(((BYTE)(r)) | (((WORD)((BYTE)(g))) << 8) | (((DWORD)((BYTE)(b))) << 16)))

/* ---- ternary raster-operation codes (the standard Win32 ones) ------------------- */

#define SRCCOPY     0x00CC0020
#define SRCPAINT    0x00EE0086
#define SRCAND      0x008800C6
#define SRCINVERT   0x00660046
#define SRCERASE    0x00440328
#define NOTSRCCOPY  0x00330008
#define NOTSRCERASE 0x001100A6
#define MERGECOPY   0x00C000CA
#define MERGEPAINT  0x00BB0226
#define PATCOPY     0x00F00021
#define PATPAINT    0x00FB0A09
#define PATINVERT   0x005A0049
#define DSTINVERT   0x00550009
#define BLACKNESS   0x00000042
#define WHITENESS   0x00FF0062

/* ---- device contexts / bitmaps / brushes ----------------------------------------- */

extern HDC     CreateCompatibleDC(HDC hdc);
extern HBITMAP CreateCompatibleBitmap(HDC hdc, INT nWidth, INT nHeight);
extern HBITMAP CreateDIBSection(HDC hdc, CONST BITMAPINFO *pbmi, DWORD usage,
                                 LPVOID *ppvBits, HANDLE hSection, DWORD offset);
extern HGDIOBJ SelectObject(HDC hdc, HGDIOBJ h);
extern BOOL    DeleteObject(HGDIOBJ h);
extern BOOL    DeleteDC(HDC hdc);
extern HBRUSH  CreateSolidBrush(COLORREF color);
extern VOID    GdiFlush(VOID);
extern BOOL    SelectPalette(HDC hdc, HPALETTE hPal, BOOL bForceBkgd);
extern UINT    RealizePalette(HDC hdc);

extern BOOL BitBlt(HDC hdcDest, INT xDest, INT yDest, INT w, INT h,
                    HDC hdcSrc, INT xSrc, INT ySrc, DWORD rop);
extern BOOL PatBlt(HDC hdc, INT x, INT y, INT w, INT h, DWORD rop);

/* Not called by DISPLAY.C (KML.C uses it) - exposed anyway since it's
 * the natural way to read back a composited pixel for verification,
 * and it's a two-line wrapper around the same pixel accessor BitBlt
 * itself uses internally. */
extern COLORREF GetPixel(HDC hdc, INT x, INT y);

/* ---- bitmap file loading ---------------------------------------------------------
 * FILES.C's eventual job long-term; a real, narrow implementation
 * here - just enough to load the uncompressed 24-bpp BMP
 * skins/hp28c/REAL28C.BMP actually is - so this milestone's
 * compositing pipeline can be exercised end to end against the real
 * skin, not only synthetic test bitmaps. Compressed BMPs, non-BMP
 * formats, and the bPalette parameter's 256-color-remap behavior are
 * out of scope; this isn't meant to be FILES.C's eventual full
 * loader. */
extern HBITMAP LoadBitmapFile(LPCTSTR szFilename, BOOL bPalette);

#ifdef __cplusplus
}
#endif

#endif /* SHIM_GDI_H */
