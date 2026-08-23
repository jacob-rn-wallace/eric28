/*
 *   stegano_stub.c
 *
 *   This file is part of the Emu28 macOS/Linux port (see ../CLAUDE.md).
 *
 *   Milestone 6 already confirmed STEGANO.C itself isn't on the
 *   critical path: FILES.C's ordinary MapRom() (loading a plain .ROM
 *   file - what this project actually needs) never touches it. Only
 *   MapRomBmp() - the niche "ROM image hidden inside a PNG via
 *   steganography" loading path - calls SteganoDecodeHBm() at all, and
 *   nothing in this codebase currently calls MapRomBmp() either. But
 *   FILES.C references SteganoDecodeHBm() unconditionally as an
 *   external symbol, so - same reasoning as sound_stub.c's SoundOut -
 *   *something* needs to define it for a full link to succeed. A
 *   single honest failure stub, not a real port of STEGANO.C: it always
 *   returns STG_ERR_STGMARKER ("no steganography marker found"), the
 *   same outcome a real implementation would report for any ordinary
 *   (non-steganographic) bitmap, which is all this project's own skins
 *   ever are.
 *
 *   New code, not derived from any Win32 SDK header. GPL-2.0, as a
 *   derivative work of Emu28 (Copyright (C) 2002 Christoph
 *   Giesselink), since it exists solely to run Emu28's source.
 */

#include "win32_types.h"
#include "gdi.h"
#include "stegano.h"

enum STG_ERRCODE SteganoDecodeHBm(LPBYTE *ppbyData, DWORD *pdwDataSize, INT nOutBitPerByte, HBITMAP hBmp)
{
	(void)nOutBitPerByte;
	(void)hBmp;
	*ppbyData = NULL;
	*pdwDataSize = 0;
	return STG_ERR_STGMARKER;
}
