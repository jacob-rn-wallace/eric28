# CLAUDE.md

Orientation doc for this repo, following the same convention as this
author's other calculator projects (`vger`, `soynut`). Read this before
touching anything here. See `DEVLOG.md` for the full story of how this
project got started - the Wine dead end, the `emu41gcc` question that
actually turned up the real precedent, and why this repo exists at all -
this file only covers current state and what's next.

## What this is

A native, SDL2-based port of Christoph Giesselink's **Emu28**
(`hp.giesselink.com/emu28.htm`) — the reference emulator for the HP-18C
and HP-28C, GPL-2.0. Upstream is Win32/MFC-only (Visual Studio 2022, no
macOS/Linux port, no cross-platform build system). This project's whole
purpose is making it build and run natively on macOS/Linux with plain
`gcc`/`clang` and SDL2 — no Wine, no Xcode/Cocoa.

## Why this exists

Born out of a research need for `vger` (a separate, sibling project — an
HP-41-inspired native calculator whose MENU/system-menu layer is
explicitly modeled on the HP-28C's real menu system; see `vger`'s
`CLAUDE.md`, "MENU UI design constraints"). Studying the real HP-28C ROM
hands-on needed a working emulator on macOS, and none existed. Public
because that gap isn't specific to one person — it affects anyone on the
HP calculator emulation/preservation side who isn't running Windows.

**This repo is independent of `vger`.** `vger`'s own `CLAUDE.md` is
explicit that no HP-28C ROM or emulator source may be vendored into that
project — it's a feasibility/behavior reference to study hands-on, never
a code dependency. This project is where that hands-on study tool
actually lives; nothing here gets pulled into `vger`.

## Prior art (architectural precedent, not vendored)

The same underlying technique — keep the original Win32-targeting C
engine untouched, add a from-scratch compatibility layer for a different
platform — has been done twice already for this exact emulator family,
by Regis Cosnier (`dgis` on GitHub):

- [`emu28android`](https://github.com/dgis/emu28android) — this exact
  emulator (Emu28), ported to Android/NDK via a ~170KB `win32-layer.c`/
  `.h` shim reimplementing the Win32 primitives the original code calls
  (critical sections, GDI-style bitmap drawing, timers, file I/O), plus a
  small (~12KB) Android-specific glue layer and a JNI bridge.
- [`emu48mac`](https://github.com/dgis/emu48mac) — the *sibling* emulator
  Emu48 (HP48 family, a related but distinct core from the same author),
  ported to native Cocoa/Objective-C in 2018 via a `MacPatch/` layer with
  the same shape: Win32-primitive shims plus KML-parsing/execution glue
  rewritten against the HP48 core's specific API surface.

Neither is vendored here. `emu28android`'s `win32-layer.c`/`.h` is the
closer architectural reference (same underlying Emu28 core we're
targeting) but is Android/NDK-specific in its rendering and JNI glue, so
it's a technique reference, not drop-in code. `emu48mac` proves the same
technique works natively on macOS at all, but its `MacPatch/` is written
against a different calculator core (Emu48's, not Emu28's) and targets
Cocoa, not SDL2.

## Architecture

```
vendor/emu28-upstream/   Pristine, unmodified Emu28 v1.39 source
                         (GPL-2.0, Copyright (C) 2002 Christoph
                         Giesselink). Never hand-edited. If upstream
                         needs a real fix, it goes in shim/ as an
                         override/wrapper, not a diff to these files -
                         keeps re-vendoring a future Emu28 release a
                         clean drop-in.
skins/hp28c/             Official HP-28C KML skin (faceplate bitmap +
                         key layout), also from hp.giesselink.com.
shim/                    New code only: Win32-type/API compatibility
                         layer + SDL2 platform layer. GPL-2.0 as a
                         derivative work.
  win32_types.h          Win32 types-and-primitives header (milestone
                         1). Fixed-width types, ANSI TCHAR/_T(), the
                         handful of opaque handle typedefs EMU28.H
                         needs to parse, and real gcc/clang
                         implementations of the CRT-ish primitives
                         (ZeroMemory, GetTickCount, critical sections,
                         GetLocaleInfo's one call pattern, ...).
                         GDI/window/registry/sound/clipboard/thread-
                         sync functions are declared here (so callers
                         type-check) but deliberately left
                         unimplemented - see the file's own section
                         comments for which category each belongs to.
  gdi.h, gdi.c           GDI-drawing shim (milestone 3): a software
                         rasterizer implementing the Win32 GDI subset
                         DISPLAY.C and KML.C use to composite the LCD/
                         annunciator display and the calculator skin -
                         memory device contexts backed by an owned
                         32-bpp (or, for DIB-section/monochrome
                         bitmaps, 8-bpp paletted or fixed-B&W) pixel
                         buffer, BitBlt/PatBlt/StretchBlt against them
                         (including a general ternary-ROP evaluator -
                         see gdi.c's rop3() - so the two custom ROP hex
                         literals DISPLAY.C/KML.C each define work
                         without this shim needing to know their
                         names), solid brushes and pens, and a narrow
                         BMP-file loader (real device contexts and
                         window-management calls are NOT here - see
                         win32_types.h's "window management" section,
                         they need milestone 4's actual SDL2 window).
                         KML.C's several window/shell/dialog APIs
                         entirely outside GDI (shell folder browsing,
                         combo boxes, window messaging, keyboard-layout
                         queries) are declared-only in win32_types.h
                         instead - see its own section comments.
                         Milestone 7 extended this with the real
                         DIB<->device-bitmap conversion FILES.C's own
                         BMP/GIF/PNG skin loaders need: CreateDIBitmap
                         (builds a GdiBitmap from an in-memory DIB's
                         raw bytes - 8-bit paletted or 24/32-bit
                         truecolor, same B-G-R(-pad) memory-order
                         handling LoadBitmapFile already did by hand),
                         GetDIBits (the reverse - reports a GdiBitmap's
                         format/palette in query mode, or copies its
                         pixels out bottom-up in fetch mode), and
                         CreatePalette (a real, distinct, non-NULL
                         HPALETTE - but doesn't need to store the color
                         table itself, since SelectPalette/
                         RealizePalette are already no-ops in a
                         truecolor-only renderer - see gdi.c's own
                         comment). Fixed a real bug this surfaced:
                         SelectPalette was typed to return BOOL, not
                         HPALETTE like real Win32 - harmless as a
                         no-op stub, but became a genuine "int to
                         pointer" compile error once FILES.C's
                         hOldPalette = SelectPalette(...) pattern
                         actually got compiled against it, so it now
                         tracks a real per-DC "currently selected
                         palette" field and returns the previous one,
                         same shape as SelectObject.

                         RESOLVED (milestone 8): the LoadBitmapFile
                         duplicate-symbol conflict flagged here no
                         longer applies - gdi.c's older, narrower
                         BMP-only placeholder was removed once the
                         whole-program link (milestone 8, below) needed
                         FILES.C's real LoadBitmapFile linked in
                         alongside everything else. Vendor code's own
                         EMU28.H already declares the prototype, so
                         nothing needed to change on the calling side.
  sdl_main.c             SDL2 platform layer (milestone 4, first
                         slice): opens a real on-screen window and
                         renders the composited skin+LCD through
                         gdi.h/DISPLAY.C's pipeline. Does not yet
                         parse a real .KML script, run the CPU-
                         emulation thread, or handle input - see the
                         file's own header comment and the milestone 4
                         notes below for the reasoning and what's
                         still ahead. Milestone 8 added the rest of
                         EMU28.C's own globals (settings flags, four
                         more CRITICAL_SECTIONs, dialog/thread/palette
                         handles, DDE's inert identifiers, a real
                         QueryPerformanceFrequency-populated lFreq) so
                         the whole program - not just DISPLAY.C's own
                         slice - can link; gave SetWindowTitle a real
                         SDL_SetWindowTitle-backed body now that the
                         window genuinely exists; and removed its own
                         DrawAnnunciator placeholder (KML.C's real one
                         is linked in now too) - see milestone 8's
                         notes below for why linking the real thing
                         alongside the placeholder became a duplicate-
                         symbol conflict rather than just dead code.
  win32_handle.h,        Tagged-dispatch HANDLE: file, event, and
  win32_handle.c         thread objects behind one opaque HANDLE type,
                         the same SelectObject-style trick gdi.c uses
                         for GDI objects. Needed because CloseHandle
                         and WaitForSingleObject are each called on
                         more than one kind of handle by vendor code
                         (KEYMACRO.C's CloseHandle on both a file and
                         event/thread handles; ENGINE.C's
                         WaitForSingleObject on both event and thread
                         handles) - real implementations throughout,
                         including real pthread-backed events and
                         threads, not declared-only stubs. File I/O
                         (CreateFile/ReadFile/WriteFile/
                         SetFilePointer/GetFileSize) moved here from
                         win32_types.h once CloseHandle needed to
                         dispatch across all three kinds together.
                         Milestone 7 added a fourth HANDLE kind
                         (file mapping) plus SetEndOfFile: real mmap-
                         backed CreateFileMapping/MapViewOfFile/
                         UnmapViewOfFile for FILES.C's whole-file BMP/
                         GIF/PNG loading. MapViewOfFile hands back a
                         bare LPVOID (matching real Win32), not a
                         tagged HANDLE, so UnmapViewOfFile can't read a
                         type tag back out of the pointer the way
                         CloseHandle does - a small side list of
                         {pointer, length} pairs recovers the mapping's
                         length instead (fine for this codebase: never
                         more than one or two mappings open at once).
  ini_file.h,            Real GetPrivateProfileString/
  ini_file.c             GetPrivateProfileInt/WritePrivateProfileString
                         (milestone 5) - SETTINGS.C's own default
                         build already uses these instead of the
                         registry, so this is a portable INI-file
                         reader/writer, not a registry-replacement
                         design.
  sound_stub.c           Silent no-op bodies (milestone 5) for
                         SoundOut/SoundOpen/.../SetSoundDeviceList -
                         needed so a full link succeeds (OPCODES.C's
                         beeper opcode calls SoundOut unconditionally),
                         since SOUND.C/SNDENUM.C (DirectSound/waveOut)
                         are still out of scope entirely.
  stegano_stub.c          Milestone 7: a single honest-failure stub for
                         SteganoDecodeHBm (always reports "no
                         steganography marker found") - FILES.C's real
                         MapRom() never touches it (only the niche
                         MapRomBmp() "ROM hidden inside a PNG" path
                         does, and nothing calls that path either), but
                         the symbol still needs *a* body to link, same
                         reasoning as sound_stub.c. STEGANO.C itself
                         stays untouched, per milestone 6.
  win32_ui_stub.c         Milestone 8: honest-failure/no-op bodies for
                         every remaining declared-only Win32 function
                         (menus, dialogs, common controls, resource
                         loading, clipboard, DDE, GDI font/text output,
                         window regions, cursor creation, power
                         status, the rest of winmm, shell folder
                         browsing, MessageBox, and a handful of
                         window-management calls) - needed once the
                         whole-program link (milestone 8, see the
                         milestone notes below) required every symbol
                         any linked vendor file references to resolve,
                         not just the ones sdl_main.c's own code path
                         happens to call.
  compat/                Fake Win32 SDK / MSVC CRT headers
                         (windows.h, tchar.h, winsock2.h, shellapi.h,
                         commctrl.h, shlobj.h, crtdbg.h, malloc.h,
                         direct.h, conio.h). Each just forwards to
                         win32_types.h (or, for malloc.h, to the real
                         <stdlib.h>). This is how vendor/'s PCH.H gets
                         satisfied without editing it: PCH.H's
                         `#include <windows.h>` etc. are angle-bracket,
                         so they resolve via the compiler's -I search
                         path to these stubs instead of a real Windows
                         SDK - quote-form includes (`#include "pch.h"`
                         itself) still resolve to vendor/'s own files
                         first, which is why PCH.H can't be replaced
                         this way, only what it includes.
COPYING.TXT              GPL-2.0 license text (from
                         hp.giesselink.com/COPYING.TXT).
```

### Why vendor/ stays pristine

Matches the discipline `vger`'s own `DEVIATIONS.md` established for its
(much smaller) soynut driver port: never hand-edit vendored third-party
source in place. Here it matters more, not less - unlike that soynut
case (same author porting their own code), Emu28 is a genuine third-party
GPL-2.0 dependency, and keeping `vendor/emu28-upstream/` byte-identical
to what Giesselink actually published means a future Emu28 point release
can be dropped in as a clean replacement, and any diff against upstream
is trivially auditable.

## Win32 API surface (as of Emu28 v1.39, mapped 2026-08-22)

The full source is 53 files. Only four categories of Win32-specific API
actually appear anywhere in it (checked directly against the source, not
assumed from file names):

| Category | Files that touch it |
|---|---|
| GDI drawing (`BitBlt`, `CreateCompatibleDC`, `SelectObject`, `GetDC`, `CreateDIBSection`, `StretchBlt`) | `DEBUGGER.C`, `DISPLAY.C`, `EMU28.C`, `FILES.C`, `KML.C`, `STEGANO.C` |
| Window/message-pump (`WinMain`, `CreateWindow`, `WndProc`, `RegisterClass`, `DispatchMessage`) | `EMU28.C`, `SOUND.C` |
| Registry (`RegOpenKey`, `RegSetValue`, `RegQueryValue`) | `SETTINGS.C` |
| DirectSound/`waveOut` | `SNDENUM.C`, `SOUND.C` |

Everything else — the actual CPU/RPL engine (`OPCODES.C`, `MOPS.C`,
`ENGINE.C`, `FETCH.C`, `RPL.C`, `STACK.C`), the disassembler/debugger
*logic* (as opposed to its dialog UI), `KEYBOARD.C`, `KEYMACRO.C`,
`MRU.C`, `LODEPNG.C` (already a portable third-party PNG library),
`SYMBFILE.C`, `TIMER.C`, `CURSOR.C`, `DDESERV.C`, `UDP.C` — never calls
into any of the four categories above. It still assumes Win32 *types*
throughout (`DWORD`, `BOOL`, `LPCTSTR`, `_T()`, `HANDLE`,
`CRITICAL_SECTION`), so `shim/` needs a lightweight types-and-primitives
compatibility header regardless (same role as `emu28android`'s
`win32-layer.h`), but the real platform-specific *behavior* work is
narrowly scoped to those 8 distinct files.

That table was compiled by reading the source; actually compiling all
19 non-GDI/window/registry/sound files against `shim/win32_types.h`
turned up several more genuinely OS-specific categories those
"portable" files call into, that the grep-based survey above missed
because they're not GDI/window/registry/sound:

| Category | Representative files | Real implementation or declared-only? |
|---|---|---|
| Power status (`GetSystemPowerStatus`, low-battery auto-shutdown) | `MOPS.C` | Declared only |
| Thread/event sync (`WaitForSingleObject`, `SetEvent`, `CreateEvent`, `CreateThread`, `INFINITE`) | `ENGINE.C`, `KEYMACRO.C` | Real (milestone 4 - pthread-backed tagged `HANDLE`, see `win32_handle.h`/`.c`) |
| Clipboard (`OpenClipboard`, `GlobalAlloc`/`Lock`/`Unlock`/`Free`, `CF_TEXT`) | `STACK.C` | Declared only |
| Locale (`GetLocaleInfo` for the decimal-point character) | `STACK.C` | Real (via `localeconv()`) |
| winmm timer (`timeGetTime` vs. the periodic-callback family) | `ENGINE.C`, `TIMER.C` | `timeGetTime` real (aliases `GetTickCount`); `timeSetEvent`/`timeBeginPeriod`/etc. declared only |
| File I/O (`CreateFile`/`ReadFile`/`WriteFile`/`CloseHandle`/`SetFilePointer`) | `KEYMACRO.C`, `SYMBFILE.C`, `FILES.C` | Real (POSIX fd wrapped in `HANDLE`) |
| File mapping (`CreateFileMapping`/`MapViewOfFile`/`UnmapViewOfFile`, `SetEndOfFile`) | `FILES.C` | Real (milestone 7 - real `mmap`, a fourth `HANDLE` kind) |
| CRT path splitting (`_tsplitpath`/`_tmakepath`) | `FILES.C` | Real (milestone 7) |
| Window position persistence (`WINDOWPLACEMENT`, `GetWindowPlacement`/`SetWindowPlacement`) | `FILES.C` | Real (milestone 7 - via SDL2, unlike the rest of "window management" below) |
| GDI DIB<->device-bitmap conversion (`CreateDIBitmap`, `GetDIBits`, `CreatePalette`) | `FILES.C` | Real (milestone 7, GDI shim) |
| Winsock (`socket`/`sendto`/`gethostbyname`, `SOCKADDR_IN`) | `UDP.C` | Real (see below) |
| DDE (`DdeCreateDataHandle`, `XTYP_*`) | `DDESERV.C` | Declared only - may never be implemented (no macOS/Linux/SDL2 equivalent) |
| Dialog UI (`DialogBox`, `GetOpenFileName`, `OPENFILENAME`) | `KEYMACRO.C` | Declared only - may never be implemented (CLAUDE.md already rules out literal native dialogs) |
| Menu API + MRU's path helpers (`InsertMenu`, `GetFullPathName`) | `MRU.C` | Declared only - blocked on a menu-replacement design that doesn't exist yet |
| Cursor creation (`CreateCursor`) | `CURSOR.C` | Declared only (GDI, milestone 3) |
| Window icon loading (`LoadImage`, `WM_SETICON`) | `FILES.C` | Declared only (milestone 7 - purely cosmetic, `.ico` unsupported) |
| Debugger window UI (dialogs, common controls, resource loading, fonts, virtual-key codes) | `DEBUGGER.C` | Declared only (milestone 7 - real breakpoint-list logic only, the dialog/toolbar/tooltip UI around it never renders) |
| Window regions (`ExtCreateRegion`, `RGNDATA`) | `FILES.C` | Declared only (milestone 7 - same bucket as `SetWindowRgn` below) |
| Steganographic ROM-in-bitmap decoding (`SteganoDecodeHBm`) | `FILES.C` | Stubbed (milestone 7 - single honest-failure body, `STEGANO.C` itself untouched per milestone 6) |

"Declared only" means: the function/type is present so the file
type-checks, but calling it will fail to *link* until a later
milestone gives it a real body. That's a deliberate boundary, not an
oversight - each belongs to a specific later milestone (see the
per-item notes below and the milestone list), and folding a rushed
implementation into this types-and-primitives header would hide that
decision rather than make it. Two are called out explicitly because
the reasoning isn't obvious:
- Thread/event sync: `ENGINE.C` calls `WaitForSingleObject` on both
  event handles (`hEventDebug`, `hEventShutdn`) *and* a thread handle
  (`hThread`) - a real Win32 `HANDLE` is a polymorphic kernel object,
  so replicating this in POSIX needs a tagged handle representation
  (thread vs. event), not just a pthread mutex+cond pair. That's
  design work for milestone 4 (SDL2 event loop / threading).
- DDE and dialog UI: unlike everything else in this table, these two
  may simply never get real implementations in this port at all - DDE
  has no macOS/Linux/SDL2 equivalent, and CLAUDE.md already commits to
  not reproducing Emu28's dialog-based UI literally. They're declared
  purely so `KEYMACRO.C`/`DDESERV.C` type-check as a unit; nothing
  currently plans to link them.

File I/O and Winsock got real implementations because - unlike the
above - they're genuinely portable primitives once you look past the
Win32 names, in the same sense `ZeroMemory`/`lstrcpy` are: `HANDLE`
from `CreateFile` maps 1:1 onto a POSIX fd, and Winsock's API is
deliberately BSD-socket-shaped. Winsock's `SOCKADDR_IN` needed real
care, though, not just a rename - see milestone 2's notes below for
the two correctness bugs (not compile errors) that trap.

`EMU28.C` is the outlier worth noting: despite being the "main" file, it
has no `WinMain`/window-creation code of its own in this source package
— building the real Windows binary needs `EMU28.RC` (a Win32 resource
script: dialogs, menus, icon) compiled in via the `e28vs2022.zip`
project, which is *not* vendored here (irrelevant to the port - SDL2
handles window creation and the event loop directly, and this project
won't reproduce Emu28's Windows-native dialog-based settings/debugger UI
as literal dialogs).

## Not yet decided / next milestones

1. ~~Write `shim/`'s Win32-types-and-primitives header~~ - done:
   `shim/win32_types.h` + the `compat/` redirect-header trick (see
   Architecture above for how the latter satisfies PCH.H's
   angle-bracket includes without touching vendor/).
2. ~~Get `vendor/emu28-upstream/`'s non-platform-specific files (the
   CPU/RPL engine list above) compiling standalone against that
   header~~ - done, and extended to all 19 files from the "everything
   else" list too (KEYBOARD.C, KEYMACRO.C, MRU.C, SYMBFILE.C, TIMER.C,
   CURSOR.C, DDESERV.C, UDP.C, DISASM.C, DISMEM.C, DISPNUM.C, DISRPL.C,
   LODEPNG.C, plus the original 6-file CPU/RPL engine set). All compile
   clean with plain `clang`/`gcc` (`-x c`, since clang treats
   upper-case `.C` as C++ by default - remember this flag for the
   eventual build system), no duplicate external symbols across the 19
   object files, remaining warnings are pre-existing benign
   vendor-source noise (unused opcode-handler parameters, missing
   braces in FETCH.C/DISPNUM.C's dispatch/lookup tables).

   Getting the last 13 files compiling required a much bigger
   `win32_types.h` than the first pass needed, and turned up genuine
   *correctness* traps, not just missing declarations - worth
   remembering if this ever gets redone from scratch:
   - `PCH.H`'s own `CLL()` macro (`#if _MSC_VER <= 1200`) misfires
     under any non-MSVC compiler, since an undefined `_MSC_VER`
     evaluates as `0` and `0 <= 1200` is true - it silently picks
     PCH.H's *oldest* fallback branch (an MSVC6-era `i64` integer
     suffix gcc/clang reject outright) instead of skipping it.
     `win32_types.h` now defines `_MSC_VER` to a modern value up
     front, which makes all of PCH.H's own version guards behave as
     they would under current MSVC.
   - UDP.C's `SOCKADDR_IN` can't just be `struct sockaddr_in` from
     `<netinet/in.h>`: Winsock's `struct in_addr` is a union whose
     first member is a 4-byte sub-struct (so `{255,255,255,255}`
     brace-initializes it, and `.s_addr` reads it back as one DWORD) -
     POSIX's is a bare scalar field, which silently accepts the same
     initializer as "excess elements" and only keeps the *first* `255`
     as the whole address (0.0.0.255, not 255.255.255.255). Fixed by
     giving `SOCKADDR_IN`/`IN_ADDR` their own Winsock-shaped
     definitions and writing `sendto()` as a real conversion wrapper
     into a native `struct sockaddr_in` (needed regardless, since
     macOS's version of that struct - unlike Linux's - carries a
     leading `sin_len` byte a raw pointer-cast would get wrong).
   - Win32's own `INADDR_NONE` is typed `unsigned long`, which is
     64-bit on LP64 macOS/Linux; compared against the (32-bit)
     `in_addr_t` address field, every `== INADDR_NONE` check was
     silently always-false. Defined here as `(in_addr_t)0xffffffff`
     instead. Neither of these two socket bugs was a compiler error or
     even a build-breaking warning - both would have shipped a
     UDP.C that compiled clean and just never worked, which is the
     real argument for the runtime smoke test (loopback send/receive
     through the shim's actual `sendto()` path, not just
     `-fsyntax-only`) that caught them.
3. GDI-drawing shim: route `KML.C`/`DISPLAY.C`'s bitmap composition into
   an SDL2 texture instead of a Windows DC. This is the real rendering
   work and the least precedented piece (Android's version draws through
   a Canvas, not applicable directly). **`DISPLAY.C` done** -
   `shim/gdi.h`/`gdi.c` implement CreateCompatibleDC/CreateDIBSection-
   style memory DCs, BitBlt/PatBlt (with a *general* ternary-ROP
   evaluator, not a lookup table - see gdi.c's `rop3()`), solid
   brushes, and a narrow BMP loader; `DISPLAY.C` compiles clean
   against it. Verified two ways, not just compiled: (1) 216
   P/S/D combinations checked against independently hand-derived
   formulas for DISPLAY.C's own two custom ROPs (`ROP_PDSPxax`,
   `ROP_PSDPxax` - both decoded from their RPN mnemonics, e.g.
   `PDSPxax` → `P ^ (D & (S ^ P))`, cross-checked byte-for-byte
   against the standard ROP3 truth-table encoding before trusting
   either), and (2) an actual end-to-end run: `LoadBitmapFile` loading
   the real `skins/hp28c/REAL28C.BMP`, `BitBlt`'d through the shim,
   dumped to PNG via the already-portable `LODEPNG.C` and visually
   confirmed pixel-perfect, plus a synthetic mask+background+ink
   composite reproducing `UpdateMainDisplay`'s exact two-`BitBlt`
   algorithm end to end (also visually confirmed, plus a numeric
   per-pixel spot-check). That end-to-end pass caught a real, latent
   bug the compile-only milestone-2 checks couldn't have: `win32_types.h`
   had `LONG`/`ULONG` typedef'd to native `long`/`unsigned long`,
   which is 64-bit on LP64 macOS/Linux; real Win32 `LONG` is *always*
   32-bit (Windows' LLP64 model). `BITMAPINFOHEADER` silently came out
   64 bytes instead of the required 40, corrupting every field offset
   - `sizeof()` mismatches like this produce no compiler warning at
   all. Fixed (now `int32_t`/`uint32_t`); worth remembering if another
   binary-format struct gets added later.

   **`KML.C` done too** - turned out smaller than expected once
   attempted directly. Extended `shim/gdi.h`/`gdi.c` with `StretchBlt`
   (nearest-neighbor), `GetObject`/`GetCurrentObject`, `SetBkColor`,
   and solid pens with `MoveToEx`/`LineTo`, plus `CreateBitmap`'s one
   real use here: building a monochrome (1-bpp) transparency mask for
   annunciator drawing. That last one needed genuine new logic, not
   just plumbing - `BitBlt`ing a color source into a monochrome
   destination applies real GDI's color-reduction rule (a source pixel
   becomes destination-white exactly when it equals the *source* DC's
   `SetBkColor`, black otherwise; see gdi.c's `reduce_for_mono_dest()`)
   - verified by reproducing `DrawAnnunciator()`'s exact algorithm
   (backdrop-with-glyph → mono mask → re-composite with
   `ROP_PSDPxax` and an ink color) end to end, numerically
   spot-checked and dumped to PNG. `StretchBlt` and `LineTo` got the
   same treatment (checkerboard-scaling and rectangle-border tests).
   Everything else `KML.C` needed - shell folder browsing
   (`SHBrowseForFolder`/`SHGetMalloc`/COM-shaped `IMalloc`), combo-box
   messages, `DialogBoxParam`, window messaging (`SendMessage`,
   `GetClassLongPtr`, ...), keyboard-layout queries - went into
   `win32_types.h` as declared-only, same treatment and same reasoning
   as the dialog/DDE/menu groups from milestone 2's extension: real
   OS-specific behavior with no macOS/Linux/SDL2 equivalent (shell
   browsing, DDE) or genuinely dependent on milestone 4's not-yet-built
   window (messaging, dialogs). `FindFirstFile`/`FindNextFile`/
   `FindClose`/`GetFileSize`/`SetCurrentDirectory`/`MulDiv` turned out
   to be real portable-primitive territory (POSIX `opendir`/`readdir`/
   `fnmatch`, `fstat`, `chdir`, plain arithmetic) and got real
   implementations instead, same bucket as milestone 2's file-I/O
   work. All 21 of vendor's non-window/registry/sound/debugger files
   now compile clean (the 19 from milestone 2 plus `DISPLAY.C` and
   `KML.C`) - `EMU28.C`, `SETTINGS.C`, `DEBUGGER.C`, and the
   DirectSound/`waveOut` files are the only ones left, and per the
   table above they're pure window/registry/sound/debugger-dialog-UI -
   milestones 4, 5, and 7's job, not GDI's. (Correction, milestone 7:
   this note originally undercounted by one - `DEBUGGER.C` is also in
   the GDI-touching category per the table above and was never
   actually attempted this milestone, despite this paragraph's original
   wording arguably implying otherwise; it stayed genuinely unbuilt
   until milestone 7 tackled it directly - see that section below.)
4. SDL2 event loop replacing `EMU28.C`'s window/message-pump section
   (small surface, per the table above) - this is also where the
   thread/event-sync `HANDLE` design decision flagged above belongs
   (`WaitForSingleObject` et al., currently declared-not-implemented in
   `win32_types.h`).

   **First slice done**: `shim/sdl_main.c` opens a real, on-screen
   SDL2 window and renders the fully-composited HP-28C skin + LCD
   through the complete milestones-1-3 pipeline - `CreateMainBitmap`
   loading the real `skins/hp28c/REAL28C.BMP`, `CreateLcdBitmap`/
   `UpdateMainDisplay` compositing the (currently all-off, since
   `Chipset` is zero-initialized with no CPU thread running yet) LCD
   state, uploaded into an `SDL_Texture` and presented every frame.
   Verified with an actual screenshot of the live window (via
   `SDL_RenderReadPixels`, called *before* `SDL_RenderPresent` -
   reading after present raced the buffer swap and silently
   screenshotted blank frames the first few tries, worth remembering),
   pixel-identical to a direct `hWindowDC` dump that bypasses SDL
   entirely - confirming the SDL plumbing, not just the GDI shim
   underneath it, is correct.

   Deliberately out of scope for this slice (see the file's own header
   comment for the reasoning on each): parsing a real `.KML` script via
   `KML.C`'s `InitKML()` (its code path also touches FILES.C's
   ROM-loading/patch-checking functions for any skin with a `Rom` line,
   which `REAL28CL.KML` has - a separate follow-up, not this slice's;
   this file calls `CreateMainBitmap`/`CreateLcdBitmap` directly
   instead, with the real skin's `Lcd Zoom 2 / Offset 474 177` values
   read out of `REAL28C.KMI` by hand); actually starting the
   CPU-emulation worker thread and wiring it into this window (the
   thread/event-sync primitives it needs are done now - see below -
   but nothing calls `ENGINE.C`'s `WorkerThread` yet, and doing that
   usefully needs a ROM, which needs FILES.C too); keyboard/mouse
   input; window resizing/menus/dialogs (the stub functions in
   `sdl_main.c` are the minimum needed to satisfy `DISPLAY.C`'s *other*
   functions, which still get compiled into its object file and still
   need their symbols to resolve even though this slice never calls
   them - most are honest no-ops for this architecture, e.g.
   `InvalidateRect`: the whole frame gets redrawn every loop iteration
   already, so dirty-rect invalidation has nothing to do here).

   **A real, genuine bug this slice's runtime testing caught that no
   amount of `-fsyntax-only` checking could have**: `gdi.c`'s
   `SelectObject` dereferenced its `HGDIOBJ` argument's type tag
   without a NULL check. `DestroyLcdBitmap()` relies on a standard GDI
   idiom - stash whatever was selected before, select the new object,
   later select the stashed handle back in and delete whatever comes
   back - and on the very first frame (before any contrast change),
   that stashed handle genuinely is `NULL`, because this shim's fresh
   DCs start with nothing selected (real GDI always has stock objects
   pre-selected, so this exact case can't arise there). Every earlier
   test exercised `BitBlt`/`PatBlt`/compositing but never a full
   create-then-destroy cycle, so it went uncaught through the whole of
   milestones 2 and 3 - only surfaced once this slice actually ran the
   real init/cleanup path end to end, immediately crashing on launch.
   Fixed by special-casing `SelectObject(hdc, NULL)` as "deselect the
   current brush," the one real interpretation the vendor code
   actually relies on (real Win32 leaves the call undefined otherwise
   anyway). Reinforces the same lesson as milestone 3's `LONG`/`ULONG`
   and Winsock bugs: compiling clean proves nothing about runtime
   correctness for code paths a syntax check can't exercise - only
   actually running it does.

   **Thread/event-sync primitives done**: `shim/win32_handle.h`/`.c`
   give `CreateEvent`/`SetEvent`/`ResetEvent`/`CreateThread`/
   `WaitForSingleObject`/`CloseHandle` real implementations - resolving
   the tagged-`HANDLE` design question flagged since milestone 2's
   extension (`ENGINE.C` calls `WaitForSingleObject` on both event and
   thread handles; `KEYMACRO.C` calls `CloseHandle` on file, event,
   *and* thread handles, all in one function). Same shape as gdi.c's
   `SelectObject`/`DeleteObject` trick: every `HANDLE` this file hands
   out points at a small struct whose first field says what kind of
   kernel object it actually is, and every function reads that tag
   before deciding what to do. Events and threads both reduce to "wait
   until this mutex-guarded flag becomes true" (a thread's flag is set
   by a pthread trampoline once its start routine returns) - which is
   exactly the polymorphism real Win32's `WaitForSingleObject` relies
   on, and exactly why `ENGINE.C` is allowed to call it on either kind
   without knowing which it has. `CreateFile`/`ReadFile`/`WriteFile`/
   `SetFilePointer`/`GetFileSize` moved here from `win32_types.h` too,
   since `CloseHandle` needed one dispatch point across all three
   HANDLE flavors, not three independent implementations sharing a
   type name.

   Verified with real concurrent behavior, not just API surface: a
   background thread that sleeps 120ms then calls `SetEvent`, checked
   against wall-clock time to confirm the main thread actually blocked
   until signaled rather than returning early (a bug here would most
   likely look like premature/spurious wake-ups, which a
   single-threaded test can't expose at all); manual- vs. auto-reset
   consume-on-wait semantics; `WaitForSingleObject` on a thread handle
   actually waiting for the worker to finish (checked via a flag the
   worker sets right before returning); a timeout that genuinely
   expires (`WAIT_TIMEOUT`); and a file handle handed to
   `WaitForSingleObject` failing cleanly instead of misinterpreting
   the union and crashing.

   Not yet done: actually calling `CreateThread` on `ENGINE.C`'s
   `WorkerThread` from `sdl_main.c` - see the milestone 4 note above on
   why that's blocked on FILES.C/ROM loading to be useful, not on
   anything in this primitive layer.
5. ~~Stub `SETTINGS.C` (registry) to a flat config file~~ - done, and
   turned out simpler than the milestone name suggests: `SETTINGS.C`'s
   own `#if !defined REGISTRY` split (`REGISTRY` is commented out, so
   this is the branch that actually compiles) already avoids the
   registry in favor of Win32's INI-file API
   (`GetPrivateProfileString`/`GetPrivateProfileInt`/
   `WritePrivateProfileString`, against a plain `Emu28.ini`) - no
   registry-emulation design needed, just real portable bodies for
   those three functions. `shim/ini_file.h`/`.c` parses the whole file
   into sections/key=value pairs on every read, applies the read or
   write, and (for writes) rewrites the whole file - appropriate for a
   settings file touched a few dozen times at startup/exit, not a hot
   path. `SETTINGS.C` compiles completely clean against it (zero
   warnings). Verified with a real file-persistence test, not just API
   surface: string/int values written then read back through a *fresh*
   parse of the file (proving persistence, not an in-memory echo),
   overwriting an existing key updates rather than duplicates it,
   deleting one key leaves its section siblings intact, and
   section/key matching is case-insensitive (matching real Win32).

   ~~initially stub `SOUND.C`/`SNDENUM.C` entirely~~ - done:
   `shim/sound_stub.c` gives `SoundOut`/`SoundOpen`/`SoundClose`/
   `SoundAvailable`/`SoundGetDeviceID`/`SoundBeep`/
   `SetSoundDeviceList` silent no-op bodies (not declared-only
   externs like win32_types.h's out-of-scope categories - OPCODES.C's
   beeper opcode handler calls `SoundOut` unconditionally, so *some*
   definition has to exist for a full link to succeed). `SOUND.C`/
   `SNDENUM.C` themselves are still never compiled - this project
   still isn't porting DirectSound/waveOut, sound still isn't needed
   for the HP-28C menu/UI research this project exists to support -
   revisit only if that changes.
6. ~~`STEGANO.C` (steganographic ROM-in-PNG loading, GDI-touching per
   the table above) - likely lowest priority; confirm it's not on the
   critical path for basic emulator bring-up before spending any time
   on it.~~ - confirmed, not on the critical path: `FILES.C` is
   `STEGANO.H`'s only includer among vendor `.C` files besides
   `STEGANO.C` itself, nothing else references it, and `FILES.C` is
   itself still a future milestone (needed for ordinary direct-ROM-
   file loading; Stegano would only matter for the niche "ROM hidden
   inside a PNG via steganography" loading path within that). No code
   changes needed - correctly stays untouched until (if ever) that
   niche path is actually wanted. (Also worth noting for whenever
   `FILES.C` is tackled: `STEGANO.H`'s own header comment says "This
   file is part of Emu42" - a leftover from the sibling emulator this
   code was seemingly shared with upstream, not an error to fix here;
   `vendor/` stays byte-identical to what Giesselink published.)

7. `FILES.C` (ROM loading, document save/load, BMP/GIF/PNG skin-image
   decoding, window-position persistence) - done. This is the file
   milestone 4's own notes already flagged as blocking real progress:
   `KML.C`'s real `InitKML()` needs it for any skin with a `Rom` line,
   and starting `ENGINE.C`'s `WorkerThread` needs a ROM loaded at all.
   Turned out to need real new Win32 surface across every category this
   project already has a place for, not a new one - genuinely portable
   pieces got real implementations, Windows-specific ones with no
   SDL2 equivalent stayed declared-only, same judgment call as every
   earlier milestone:

   - CRT path splitting (`_tsplitpath`/`_tmakepath`, `_MAX_PATH` and
     friends) - real, in `win32_types.h`: genuinely portable once you
     set aside the drive-letter concept POSIX doesn't have (`drive` is
     always written empty).
   - `WINDOWPLACEMENT`/`GetWindowPlacement`/`SetWindowPlacement`
     (saving/restoring the window's on-screen position across
     sessions) - real, in `sdl_main.c` via
     `SDL_GetWindowPosition`/`SDL_SetWindowPosition`, alongside
     `GetClientRect`. Unlike the rest of `win32_types.h`'s "window
     management" section (still declared-only - real resizing/menus
     still don't exist), this one only ever touches `.length` and
     `.rcNormalPosition`, and milestone 4 already has a live window by
     the time `FILES.C` needs it.
   - File mapping (`CreateFileMapping`/`MapViewOfFile`/
     `UnmapViewOfFile`, `SetEndOfFile`) - real, in `win32_handle.h`/
     `.c` as a fourth `HANDLE` kind, backed by real `mmap`. See that
     file's Architecture entry above for the "how does UnmapViewOfFile
     recover a bare pointer's length" design note.
   - GDI DIB<->device-bitmap conversion (`CreateDIBitmap`, `GetDIBits`,
     `CreatePalette`, plus a real bug-fix to `SelectPalette`'s return
     type) - real, in `gdi.h`/`gdi.c`. See that file's Architecture
     entry above, including the known `LoadBitmapFile` duplicate-symbol
     follow-up this surfaced.
   - Window icon loading (`LoadImage`/`WM_SETICON`/...) - declared
     only: purely cosmetic (the real UI is the composited skin bitmap,
     not window chrome), and `.ico` is a format this shim has no
     reader for.
   - Window regions (`ExtCreateRegion`, `RGNDATA` and friends, used by
     `CreateRgnFromBitmap` to build a shaped/non-rectangular window
     from the skin's transparent color) - declared only, same bucket as
     the already-declared-only `SetWindowRgn`: no SDL2 equivalent
     (SDL2 windows are always rectangular), and this project's window
     is always the full skin rectangle regardless, with transparency
     already handled by the GDI compositing pipeline itself.
   - `SteganoDecodeHBm` - a single honest-failure stub
     (`shim/stegano_stub.c`), not a real port of `STEGANO.C` - see that
     file's Architecture entry above.

   Verified past just compiling, same bar as every earlier milestone:
   all 23 non-window/registry/sound/debugger vendor files (the 22 from
   milestones 2-5 plus `FILES.C` now) compile clean, and three
   dedicated real-behavior test harnesses (scratchpad, not committed,
   same as every earlier milestone's) exercised the new code for real:
   (1) a `_tsplitpath`/`_tmakepath` round-trip test across several path
   shapes, plus a real-`mmap` `CreateFileMapping`/`MapViewOfFile`/
   `UnmapViewOfFile` test (whole-file and partial-length mappings
   against a real file, cross-checked against an independent plain
   `ReadFile`, plus `SetEndOfFile` truncation verified by re-opening
   the file afterward) - both passed outright; (2) a `CreateDIBitmap`/
   `GetDIBits` round-trip test (8-bit paletted and 24-bit truecolor
   synthetic DIBs, checking pixel values, palette entries, and
   `SelectPalette`'s previous-handle return survive the round trip
   exactly) - caught a real test-harness bug (a bare `BITMAPINFO`'s
   single-entry `bmiColors[1]` is too small for `GetDIBits` to write a
   3-entry palette into - a stack buffer overflow that crashed the test
   until the harness allocated a properly-sized buffer, same pattern
   real Win32 callers always use); (3) a genuine link-and-run test of
   `FILES.C`'s own compiled `MapRom()`/`UnpackRom()` against the real
   `28C_1CC.ROM` the user provided (see `README.md`/`.gitignore` for
   why it's never committed) - required ~25 link-satisfying stubs for
   symbols `FILES.C.o` references from other not-yet-built vendor files
   (`KML.C`'s real `InitKML`, `ENGINE.C`'s `CpuReset`, `RPL.C`,
   `FETCH.C`, `DEBUGGER.C`'s breakpoint-list functions - `DEBUGGER.C`
   itself doesn't even compile against the shim yet, being dialog-UI
   heavy) that `MapRom()` itself never calls but the linker still needs
   resolved, scoped to the test file only, not shim code. This test
   caught a real bug in the TEST's own first-draft expectations, not in
   `MapRom`/`UnpackRom`: `UnpackRom`'s unpack loop walks backward across
   the *entire* buffer including the header region `MapRom` had already
   saved verbatim, so a packed ROM's first 4 bytes end up holding
   *unpacked* nibbles by the time `MapRom` returns, not a raw copy of
   the file's first 4 bytes - the test's original blanket "first 4
   bytes match" assertion was simply wrong for the packed case (true
   only for an already-unpacked ROM), caught by actually running it
   against the real ROM rather than assuming the algorithm's shape.
   Once corrected, confirmed against the real `28C_1CC.ROM`: it's
   nibble-packed (`dwRomSize` doubles from the 131072-byte file to
   262144), and both the very first packed byte and a byte further into
   the ROM unpack to the exact nibble values hand-computed from the
   file's own raw bytes.

   **`DEBUGGER.C` done too**, same session - `FILES.C`'s breakpoint-
   list calls (`LoadBreakpointList`/`SaveBreakpointList`/
   `CreateBackupBreakpointList`/`RestoreBackupBreakpointList`/
   `DisableDebugger`/`OnToolDebug`) need it to compile as a unit, and
   milestone 3's own completion note had incorrectly implied it was
   already handled (see the correction just above). Emu28's
   disassembler/memory/stack/breakpoint debugger window - 3883 lines,
   almost entirely native Win32 dialog/common-control UI (owner-drawn
   list boxes, a toolbar built from an `RT_TOOLBAR` resource, a system-
   menu extension, tooltips, several modeless dialogs) with a small
   real core (the breakpoint list itself). ~160 distinct declared-only
   additions to `win32_types.h`'s new "debugger window UI" section and
   `gdi.h`'s new font/text-output section (window/menu/dialog
   functions, listbox/combobox/toolbar messages and notification
   structs, virtual-key codes, resource loading, fonts) - same
   treatment as every other native-dialog-shaped surface this project
   has already ruled out reproducing, just far more of it in one file.
   `CheckMenuItem`/`EnableMenuItem` extended the existing menu-API
   section; `FlushFileBuffers` (real, `fsync`-backed) joined
   `win32_handle.c`'s other real file-I/O functions. Verified to the
   same bar as milestones 2's original 19-file pass (compile-clean, no
   regressions to any of milestone 7's other three real-behavior tests
   or the milestone-4 SDL link) rather than milestone 3/4's deeper
   real-link-and-run treatment - unlike `FILES.C`'s `MapRom()` or the
   GDI DIB functions, `DEBUGGER.C`'s only non-dialog real logic (the
   breakpoint list) is a thin, low-risk wrapper around already-
   thoroughly-tested `ReadFile`/`WriteFile`, and a real link-and-run
   test of it would need on the order of 100+ additional throwaway
   stubs for `DEBUGGER.C`'s other dependencies (the disassembler,
   RPL-object viewer, settings I/O, `ENGINE.C` globals) for very little
   additional confidence - not worth it here, unlike `MapRom()` where
   the real ROM-unpack algorithm itself was the thing being verified.
   All 24 non-window/registry/sound vendor files (every file left
   except `EMU28.C` and the DirectSound/`waveOut` pair) now compile
   clean.

8. **Whole-program link** - not a planned milestone going in, but a
   natural question once every non-window/registry/sound/debugger
   vendor file compiled clean (milestone 7): does the *entire*
   emulator - every one of those files, plus `DISPLAY.C`/`KML.C`/
   `SETTINGS.C` from earlier milestones, plus the shim - actually
   *link* into one real program? Answer: yes, as of this session.
   `clang`ing every vendor `.C` file except `EMU28.C` (superseded by
   `sdl_main.c` entirely - see its own note below) and the
   DirectSound/`waveOut`/`STEGANO.C` trio (still deliberately out of
   scope) together with every `shim/*.c` file produces a real,
   running Mach-O executable - confirmed by actually launching it
   (stays alive, prints its startup message, no crash) rather than
   just checking the linker's exit code.

   Two files nobody had tried compiling before turned out to already
   work with zero changes: `REDEYE.C` (infrared "red-eye" printer
   emulation - `MOPS.C`'s beeper-adjacent `IrPrinter` opcode handler
   needs it to link) and `PNGCRC.C` (a faster CRC32 table Emu28's own
   `LODEPNG.H` expects to be linked in externally when compiled with
   `-DLODEPNG_NO_COMPILE_CRC` - without that flag, `LODEPNG.C`'s own
   built-in `lodepng_crc32` collides with `PNGCRC.C`'s at link time;
   this is genuine upstream Emu28 build knowledge, not a shim
   workaround, worth remembering alongside `-x c` for whenever a real
   build system exists). `STEGANO.C` remains untouched per milestone 6
   - genuinely out of scope, not just untested.

   Getting an actual, complete link (not just "every file compiles
   standalone," which milestones 2-7 had already reached) needed two
   more kinds of work:

   - **Every remaining declared-only Win32 function needed a real
     body** - not to make it *work*, but because unlike compiling a
     single file, linking an executable requires every symbol any
     linked object file references to resolve, even from functions
     that are never actually called at runtime (an unreachable
     function's undefined symbol still blocks the link - dead-code
     elimination doesn't enter into it without `--gc-sections`, which
     this ad hoc build doesn't use). `shim/win32_ui_stub.c` (new)
     collects every one of these: menus, dialogs, common controls
     (`CreateToolbarEx`/tooltips), resource loading, clipboard, DDE,
     GDI font/text output, window regions, cursor creation, power
     status, the remaining winmm periodic-timer functions, shell
     folder browsing, `MessageBox`, and the handful of window-
     management calls `DEBUGGER.C`/`KML.C`/`MRU.C` reach for that
     `sdl_main.c`'s own stubs didn't already cover
     (`GetCurrentDirectory`/`GetFullPathName`/`GetKeyboardLayoutName`
     among them). Every body is an honest "this operation is
     unavailable" - same idiom as `sound_stub.c`'s `SoundOut` and
     `stegano_stub.c`'s `SteganoDecodeHBm` - not a step toward actually
     implementing any of it; CLAUDE.md's stance on not reproducing
     Emu28's dialog-based UI hasn't changed, this just makes that
     stance link-clean instead of merely compile-clean.
   - **`sdl_main.c` needed the rest of `EMU28.C`'s globals** - not just
     the handful `DISPLAY.C` alone touches (`hWnd`/`hWindowDC`/
     `Chipset`/...), but everything `SETTINGS.C`/`ENGINE.C`/`TIMER.C`/
     `KEYBOARD.C`/`MOPS.C`/`DDESERV.C`/`DEBUGGER.C` reference too
     (settings-flag `BOOL`s, four more `CRITICAL_SECTION`s -
     `csKeyLock`/`csTLock`/`csSlowLock`/`csDbgLock`, real dialog/
     thread/palette handles that just start empty since nothing
     creates a real dialog or CPU thread yet, DDE's inert
     `szAppName`/`szTopic`/`uCF_HpObj`, `lFreq` - now genuinely
     populated via the already-real `QueryPerformanceFrequency`).
     `SetWindowTitle` got a real body too (`SDL_SetWindowTitle` -
     milestone 4's window genuinely exists now, so this one didn't
     need to stay a stub); `ForceForegroundWindow`/
     `CopyItemsToClipboard` are honest no-ops, same reasoning as
     `win32_ui_stub.c`'s functions. Two of `sdl_main.c`'s *own*
     milestone-4 placeholder bodies had to come back out once this
     linked the real thing alongside them for the first time
     (duplicate-symbol conflicts, same shape as the `LoadBitmapFile`
     one already resolved above): its `DrawAnnunciator` stub (real one
     now linked in from `KML.C`) and nothing else needed removing -
     `LoadBitmapFile`'s removal from `gdi.c` (this same milestone, see
     that entry above) was the only other one.

   Explicitly **not** claimed by this milestone: the resulting
   executable doesn't behave any differently than milestone 4's
   original slice at runtime - `sdl_main.c`'s own code still never
   calls `InitKML`, `MapRom`, or `CreateThread` on `WorkerThread`, so
   no ROM loads, no real skin gets parsed, and the CPU never runs; all
   of `KML.C`/`FILES.C`/`ENGINE.C`'s real logic is now linked in and
   *reachable*, but nothing in `sdl_main.c` calls it yet - that's real
   milestone-4-completion work, still ahead, not something this link
   milestone did in passing. What this milestone actually proves is
   narrower but still significant: the whole shim's Win32 surface is
   now large enough, and consistent enough across every vendor file
   that touches it, for the *entire* upstream codebase to link into one
   coherent program - the strongest cross-file consistency check this
   project has run yet, well beyond what compiling each file standalone
   could catch (a per-file `-fsyntax-only` check can't see a type
   mismatch between two files' independent uses of the same shim
   function, the way a real link's "conflicting types"/duplicate-symbol
   errors can and did, twice, this session).

No build system exists yet. Given the file-portability split above, a
CMake setup mirroring `vger`'s own (`core`-style static library for the
untouched-engine files, linked into an SDL2 executable) is the likely
shape, but this hasn't been set up. The ad hoc `clang -x c -I shim/compat
-I shim -I vendor/emu28-upstream` invocation used to verify milestone 2
is not that build system - it was just enough to prove the six files
compile; CMake still needs setting up for real.

## ROM images

Never committed here - see `.gitignore` and `README.md`. Emu28's own
author has no license to redistribute HP-28C ROM images; this project
follows the same policy `vger` and `soynut` already follow for their own
ROM dependencies.

## License

GPL-2.0, inherited from upstream Emu28 - see `COPYING.TXT`. Any new code
under `shim/` is GPL-2.0 too, as a derivative work building directly on
Emu28's engine.
