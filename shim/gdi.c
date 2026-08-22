/*
 *   gdi.c
 *
 *   This file is part of the Emu28 macOS/Linux port (see ../CLAUDE.md).
 *   Implementation of the GDI-drawing shim declared in gdi.h - see
 *   that file's header comment for the design (packed-COLORREF pixel
 *   format, the two supported bit depths, why row order is tracked
 *   per bitmap instead of always normalized).
 *
 *   New code, not derived from any Win32 SDK header. GPL-2.0, as a
 *   derivative work of Emu28 (Copyright (C) 2002 Christoph
 *   Giesselink), since it exists solely to run Emu28's source.
 */

#include "gdi.h"

#include <stdio.h>
#include <stdlib.h>

typedef enum {
	GDI_OBJ_BITMAP,
	GDI_OBJ_BRUSH
} GdiObjType;

typedef struct {
	GdiObjType type; /* always GDI_OBJ_BITMAP; lets SelectObject/
	                  * DeleteObject tell bitmaps and brushes apart
	                  * through the same HGDIOBJ. */
	INT   width, height; /* height is always the positive size */
	BOOL  topDown;        /* row order of the storage below - TRUE:
	                       * row 0 is the top of the image (what
	                       * CreateDIBSection produces for a negative
	                       * biHeight, and what every non-DIB bitmap
	                       * this shim creates uses); FALSE: row 0 is
	                       * the bottom (a positive-biHeight DIB
	                       * section, matching real Win32 - needed
	                       * because DISPLAY.C pokes a DIB section's
	                       * returned pointer directly and assumes
	                       * real GDI's row-order convention). */
	INT   bpp;             /* 8 or 32 */
	DWORD *pixels32;        /* bpp == 32 */
	BYTE  *pixels8;          /* bpp == 8, one byte per pixel, stride
	                         * == width exactly (no Win32 4-byte row
	                         * padding) - matches what DISPLAY.C
	                         * itself assumes of the one 8-bpp DIB
	                         * section it ever creates. */
	RGBQUAD palette[256];
	UINT    paletteCount;
} GdiBitmap;

typedef struct {
	GdiObjType type; /* always GDI_OBJ_BRUSH */
	COLORREF   color;
} GdiBrush;

typedef struct {
	GdiBitmap *bitmap; /* currently selected bitmap, or NULL */
	GdiBrush  *brush;   /* currently selected brush, or NULL */
} GdiDC;

/* ---- pixel access -------------------------------------------------------------- */

static COLORREF bitmap_get_pixel(CONST GdiBitmap *bmp, INT x, INT y)
{
	INT row = bmp->topDown ? y : (bmp->height - 1 - y);

	if (x < 0 || y < 0 || x >= bmp->width || y >= bmp->height)
		return 0;

	if (bmp->bpp == 8) {
		BYTE index = bmp->pixels8[(size_t)row * bmp->width + x];
		CONST RGBQUAD *c = &bmp->palette[index < bmp->paletteCount ? index : 0];
		return RGB(c->rgbRed, c->rgbGreen, c->rgbBlue);
	}
	return bmp->pixels32[(size_t)row * bmp->width + x];
}

static VOID bitmap_set_pixel(GdiBitmap *bmp, INT x, INT y, COLORREF color)
{
	INT row = bmp->topDown ? y : (bmp->height - 1 - y);

	if (x < 0 || y < 0 || x >= bmp->width || y >= bmp->height)
		return;

	if (bmp->bpp == 8) {
		/* Only ever written through DISPLAY.C's own WritePixel*()
		 * pokes directly into the returned buffer, never through
		 * BitBlt/PatBlt - but handle it correctly regardless. */
		bmp->pixels8[(size_t)row * bmp->width + x] = (BYTE)color;
		return;
	}
	bmp->pixels32[(size_t)row * bmp->width + x] = color;
}

/* ---- general ternary raster-operation evaluator --------------------------------
 * A Win32 ROP3 DWORD encodes an 8-entry Boolean truth table (one
 * output bit per combination of Pattern/Source/Destination) in its
 * bits 16-23, indexed as (P<<2)|(S<<1)|D - confirmed against SRCCOPY
 * (0xCC => Result=S), PATCOPY (0xF0 => Result=P), DSTINVERT (0x55 =>
 * Result=~D), and BLACKNESS (0x00 => Result=0), and against
 * DISPLAY.C's own two custom ROPs by independently deriving their
 * formulas from their RPN mnemonics (see CLAUDE.md's milestone 3
 * notes) and checking both routes agree. Expanding the truth table as
 * a sum of Boolean minterms and computing each minterm with ordinary
 * bitwise AND/OR/XOR/NOT works for *any* ROP3 code without needing to
 * know its name - so this one function covers every ROP this shim
 * will ever be asked to evaluate, not just the ones currently used. */
static DWORD rop3(DWORD rop, DWORD P, DWORD S, DWORD D)
{
	BYTE  truth = (BYTE)(rop >> 16);
	DWORD result = 0;
	INT   i;

	for (i = 0; i < 8; ++i) {
		if (truth & (1 << i)) {
			INT p = (i >> 2) & 1, s = (i >> 1) & 1, d = i & 1;
			result |= (p ? P : ~P) & (s ? S : ~S) & (d ? D : ~D);
		}
	}
	return result;
}

/* ---- device contexts / bitmaps / brushes ------------------------------------------ */

HDC CreateCompatibleDC(HDC hdc)
{
	GdiDC *dc = (GdiDC *)calloc(1, sizeof(GdiDC));

	(void)hdc; /* nothing to be "compatible" with - one pixel format throughout */
	return (HDC)dc;
}

HBITMAP CreateCompatibleBitmap(HDC hdc, INT nWidth, INT nHeight)
{
	GdiBitmap *bmp;

	(void)hdc;
	if (nWidth <= 0 || nHeight <= 0)
		return NULL;

	bmp = (GdiBitmap *)calloc(1, sizeof(GdiBitmap));
	bmp->type = GDI_OBJ_BITMAP;
	bmp->width = nWidth;
	bmp->height = nHeight;
	bmp->topDown = TRUE;
	bmp->bpp = 32;
	bmp->pixels32 = (DWORD *)calloc((size_t)nWidth * nHeight, sizeof(DWORD));
	return (HBITMAP)bmp;
}

HBITMAP CreateDIBSection(HDC hdc, CONST BITMAPINFO *pbmi, DWORD usage,
                          LPVOID *ppvBits, HANDLE hSection, DWORD offset)
{
	GdiBitmap *bmp;
	CONST BITMAPINFOHEADER *bih = &pbmi->bmiHeader;
	INT width = bih->biWidth;
	INT height = (bih->biHeight < 0) ? -bih->biHeight : bih->biHeight;

	(void)hdc;
	(void)usage;
	(void)hSection;
	(void)offset;

	if (width <= 0 || height <= 0 || (bih->biBitCount != 8 && bih->biBitCount != 32))
		return NULL;

	bmp = (GdiBitmap *)calloc(1, sizeof(GdiBitmap));
	bmp->type = GDI_OBJ_BITMAP;
	bmp->width = width;
	bmp->height = height;
	bmp->topDown = (bih->biHeight < 0);
	bmp->bpp = bih->biBitCount;

	if (bmp->bpp == 8) {
		UINT i;

		bmp->pixels8 = (BYTE *)calloc((size_t)width * height, 1);
		bmp->paletteCount = bih->biClrUsed ? bih->biClrUsed : 256;
		if (bmp->paletteCount > 256)
			bmp->paletteCount = 256;
		for (i = 0; i < bmp->paletteCount; ++i)
			bmp->palette[i] = pbmi->bmiColors[i];
		*ppvBits = bmp->pixels8;
	} else {
		bmp->pixels32 = (DWORD *)calloc((size_t)width * height, sizeof(DWORD));
		*ppvBits = bmp->pixels32;
	}
	return (HBITMAP)bmp;
}

HGDIOBJ SelectObject(HDC hdc, HGDIOBJ h)
{
	GdiDC *dc = (GdiDC *)hdc;
	GdiObjType type = *(GdiObjType *)h;

	if (type == GDI_OBJ_BITMAP) {
		GdiBitmap *prev = dc->bitmap;
		dc->bitmap = (GdiBitmap *)h;
		return (HGDIOBJ)prev;
	} else {
		GdiBrush *prev = dc->brush;
		dc->brush = (GdiBrush *)h;
		return (HGDIOBJ)prev;
	}
}

BOOL DeleteObject(HGDIOBJ h)
{
	GdiObjType type;

	if (h == NULL)
		return FALSE;

	type = *(GdiObjType *)h;
	if (type == GDI_OBJ_BITMAP) {
		GdiBitmap *bmp = (GdiBitmap *)h;
		free(bmp->pixels32);
		free(bmp->pixels8);
		free(bmp);
	} else {
		free(h);
	}
	return TRUE;
}

BOOL DeleteDC(HDC hdc)
{
	free(hdc);
	return TRUE;
}

HBRUSH CreateSolidBrush(COLORREF color)
{
	GdiBrush *brush = (GdiBrush *)calloc(1, sizeof(GdiBrush));

	brush->type = GDI_OBJ_BRUSH;
	brush->color = color;
	return (HBRUSH)brush;
}

VOID GdiFlush(VOID)
{
	/* Real GdiFlush waits for the calling thread's batched GDI calls
	 * to finish; every call here is already synchronous. */
}

BOOL SelectPalette(HDC hdc, HPALETTE hPal, BOOL bForceBkgd)
{
	(void)hdc;
	(void)hPal;
	(void)bForceBkgd;
	return TRUE; /* no 256-color palette to realize against a truecolor target */
}

UINT RealizePalette(HDC hdc)
{
	(void)hdc;
	return 0;
}

/* ---- blitting ------------------------------------------------------------------- */

BOOL BitBlt(HDC hdcDest, INT xDest, INT yDest, INT w, INT h,
            HDC hdcSrc, INT xSrc, INT ySrc, DWORD rop)
{
	GdiDC *dst = (GdiDC *)hdcDest;
	GdiDC *src = (GdiDC *)hdcSrc;
	COLORREF patColor = dst->brush ? dst->brush->color : 0;
	INT x, y;

	if (dst->bitmap == NULL)
		return FALSE;

	for (y = 0; y < h; ++y) {
		for (x = 0; x < w; ++x) {
			COLORREF s = (src && src->bitmap) ? bitmap_get_pixel(src->bitmap, xSrc + x, ySrc + y) : 0;
			COLORREF d = bitmap_get_pixel(dst->bitmap, xDest + x, yDest + y);
			COLORREF result = rop3(rop, patColor, s, d);

			bitmap_set_pixel(dst->bitmap, xDest + x, yDest + y, result);
		}
	}
	return TRUE;
}

COLORREF GetPixel(HDC hdc, INT x, INT y)
{
	GdiDC *dc = (GdiDC *)hdc;

	if (dc->bitmap == NULL)
		return (COLORREF)-1;
	return bitmap_get_pixel(dc->bitmap, x, y);
}

BOOL PatBlt(HDC hdc, INT x0, INT y0, INT w, INT h, DWORD rop)
{
	GdiDC *dc = (GdiDC *)hdc;
	COLORREF patColor = dc->brush ? dc->brush->color : 0;
	INT x, y;

	if (dc->bitmap == NULL)
		return FALSE;

	for (y = 0; y < h; ++y) {
		for (x = 0; x < w; ++x) {
			COLORREF d = bitmap_get_pixel(dc->bitmap, x0 + x, y0 + y);
			COLORREF result = rop3(rop, patColor, 0, d);

			bitmap_set_pixel(dc->bitmap, x0 + x, y0 + y, result);
		}
	}
	return TRUE;
}

/* ---- bitmap file loading --------------------------------------------------------
 * Reads an uncompressed 24-bpp Windows BMP - the exact format
 * skins/hp28c/REAL28C.BMP is - into a top-down 32-bpp GdiBitmap. See
 * gdi.h's header comment for why this is narrower than a real
 * loader. */
HBITMAP LoadBitmapFile(LPCTSTR szFilename, BOOL bPalette)
{
	FILE *f;
	BITMAPFILEHEADER bfh;
	BITMAPINFOHEADER bih;
	GdiBitmap *bmp;
	INT width, height;
	BOOL sourceTopDown;
	INT row, rowBytes, pad;
	BYTE *rowBuf;

	(void)bPalette;

	f = fopen(szFilename, "rb");
	if (f == NULL)
		return NULL;

	if (fread(&bfh, sizeof(bfh), 1, f) != 1 || bfh.bfType != 0x4D42 ||
	    fread(&bih, sizeof(bih), 1, f) != 1 || bih.biBitCount != 24 || bih.biCompression != BI_RGB) {
		fclose(f);
		return NULL;
	}

	sourceTopDown = (bih.biHeight < 0);
	width = bih.biWidth;
	height = sourceTopDown ? -bih.biHeight : bih.biHeight;
	if (width <= 0 || height <= 0) {
		fclose(f);
		return NULL;
	}

	bmp = (GdiBitmap *)calloc(1, sizeof(GdiBitmap));
	bmp->type = GDI_OBJ_BITMAP;
	bmp->width = width;
	bmp->height = height;
	bmp->topDown = TRUE; /* normalized on the way in, regardless of on-disk order */
	bmp->bpp = 32;
	bmp->pixels32 = (DWORD *)calloc((size_t)width * height, sizeof(DWORD));

	rowBytes = width * 3;
	pad = (4 - (rowBytes % 4)) % 4; /* BMP rows are padded to a 4-byte boundary */
	rowBuf = (BYTE *)malloc((size_t)rowBytes);

	fseek(f, (long)bfh.bfOffBits, SEEK_SET);
	for (row = 0; row < height; ++row) {
		INT destRow = sourceTopDown ? row : (height - 1 - row);
		INT x;

		if (fread(rowBuf, 1, (size_t)rowBytes, f) != (size_t)rowBytes) {
			free(rowBuf);
			free(bmp->pixels32);
			free(bmp);
			fclose(f);
			return NULL;
		}
		fseek(f, pad, SEEK_CUR);

		for (x = 0; x < width; ++x) {
			BYTE b = rowBuf[x * 3 + 0], g = rowBuf[x * 3 + 1], r = rowBuf[x * 3 + 2];
			bmp->pixels32[(size_t)destRow * width + x] = RGB(r, g, b);
		}
	}

	free(rowBuf);
	fclose(f);
	return (HBITMAP)bmp;
}
