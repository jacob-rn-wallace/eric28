/*
 *   sound_stub.c
 *
 *   This file is part of the Emu28 macOS/Linux port (see ../CLAUDE.md).
 *
 *   Milestone 5: SOUND.C/SNDENUM.C themselves are never compiled -
 *   they're DirectSound/waveOut, a category this project deliberately
 *   doesn't port (sound isn't needed for the HP-28C menu/UI research
 *   this project exists to support - see CLAUDE.md's milestone
 *   notes). But OPCODES.C's beeper opcode handler still calls
 *   SoundOut() unconditionally, so *something* needs to define these
 *   six functions for a full link to succeed. Silent no-ops, not
 *   declared-only stubs like win32_types.h's out-of-scope categories,
 *   because these genuinely need bodies to link at all - revisit with
 *   a real SDL_AudioStream-backed implementation only if sound turns
 *   out to matter.
 *
 *   New code, not derived from any Win32 SDK header. GPL-2.0, as a
 *   derivative work of Emu28 (Copyright (C) 2002 Christoph
 *   Giesselink), since it exists solely to run Emu28's source.
 */

#include "win32_types.h"
#include "Emu28.h"

BOOL SoundAvailable(UINT uDeviceID)
{
	(void)uDeviceID;
	return FALSE;
}

BOOL SoundGetDeviceID(UINT *puDeviceID)
{
	(void)puDeviceID;
	return FALSE;
}

BOOL SoundOpen(UINT uDeviceID)
{
	(void)uDeviceID;
	return FALSE;
}

VOID SoundClose(VOID)
{
}

VOID SoundOut(CHIPSET *w, WORD wOut)
{
	(void)w;
	(void)wOut;
}

VOID SoundBeep(DWORD dwFrequency, DWORD dwDuration)
{
	(void)dwFrequency;
	(void)dwDuration;
}

VOID SetSoundDeviceList(HWND hWnd, UINT uDeviceID)
{
	(void)hWnd;
	(void)uDeviceID;
}
