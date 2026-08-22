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
  sdl_main.c             SDL2 platform layer (milestone 4, first
                         slice): opens a real on-screen window and
                         renders the composited skin+LCD through
                         gdi.h/DISPLAY.C's pipeline. Does not yet
                         parse a real .KML script, run the CPU-
                         emulation thread, or handle input - see the
                         file's own header comment and the milestone 4
                         notes below for the reasoning and what's
                         still ahead.
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
| Thread/event sync (`WaitForSingleObject`, `SetEvent`, `CreateEvent`, `CreateThread`, `INFINITE`) | `ENGINE.C`, `KEYMACRO.C` | Declared only |
| Clipboard (`OpenClipboard`, `GlobalAlloc`/`Lock`/`Unlock`/`Free`, `CF_TEXT`) | `STACK.C` | Declared only |
| Locale (`GetLocaleInfo` for the decimal-point character) | `STACK.C` | Real (via `localeconv()`) |
| winmm timer (`timeGetTime` vs. the periodic-callback family) | `ENGINE.C`, `TIMER.C` | `timeGetTime` real (aliases `GetTickCount`); `timeSetEvent`/`timeBeginPeriod`/etc. declared only |
| File I/O (`CreateFile`/`ReadFile`/`WriteFile`/`CloseHandle`/`SetFilePointer`) | `KEYMACRO.C`, `SYMBFILE.C` | Real (POSIX fd wrapped in `HANDLE`) |
| Winsock (`socket`/`sendto`/`gethostbyname`, `SOCKADDR_IN`) | `UDP.C` | Real (see below) |
| DDE (`DdeCreateDataHandle`, `XTYP_*`) | `DDESERV.C` | Declared only - may never be implemented (no macOS/Linux/SDL2 equivalent) |
| Dialog UI (`DialogBox`, `GetOpenFileName`, `OPENFILENAME`) | `KEYMACRO.C` | Declared only - may never be implemented (CLAUDE.md already rules out literal native dialogs) |
| Menu API + MRU's path helpers (`InsertMenu`, `GetFullPathName`) | `MRU.C` | Declared only - blocked on a menu-replacement design that doesn't exist yet |
| Cursor creation (`CreateCursor`) | `CURSOR.C` | Declared only (GDI, milestone 3) |

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
   work. All 21 of vendor's non-window/registry/sound files now
   compile clean (the 19 from milestone 2 plus `DISPLAY.C` and
   `KML.C`) - `EMU28.C`, `SETTINGS.C`, and the DirectSound/`waveOut`
   files are the only ones left, and per the table above they're
   pure window/registry/sound - milestones 4 and 5's job, not GDI's.
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
   read out of `REAL28C.KMI` by hand); the CPU-emulation worker thread
   and the real thread/event-sync primitives it needs (still the
   tagged-`HANDLE` design question flagged above); keyboard/mouse
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
5. Stub `SETTINGS.C` (registry) to a flat config file, and initially stub
   `SOUND.C`/`SNDENUM.C` entirely (sound isn't needed for the HP-28C
   menu/UI research this project exists to support) rather than porting
   DirectSound - revisit only if sound turns out to matter.
6. `STEGANO.C` (steganographic ROM-in-PNG loading, GDI-touching per the
   table above) - likely lowest priority; confirm it's not on the
   critical path for basic emulator bring-up before spending any time on
   it.

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
