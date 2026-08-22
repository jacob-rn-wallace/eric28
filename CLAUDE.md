# CLAUDE.md

Orientation doc for this repo, following the same convention as this
author's other calculator projects (`vger`, `soynut`). Read this before
touching anything here.

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
shim/                    (not yet created) New code only: Win32-type/
                         API compatibility layer + SDL2 platform layer.
                         GPL-2.0 as a derivative work.
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

`EMU28.C` is the outlier worth noting: despite being the "main" file, it
has no `WinMain`/window-creation code of its own in this source package
— building the real Windows binary needs `EMU28.RC` (a Win32 resource
script: dialogs, menus, icon) compiled in via the `e28vs2022.zip`
project, which is *not* vendored here (irrelevant to the port - SDL2
handles window creation and the event loop directly, and this project
won't reproduce Emu28's Windows-native dialog-based settings/debugger UI
as literal dialogs).

## Not yet decided / next milestones

1. Write `shim/`'s Win32-types-and-primitives header (the equivalent of
   `emu28android`'s `win32-layer.h`, but new code, not ported - that file
   is Android/NDK-specific in its actual implementations even though the
   API surface it covers is the right reference).
2. Get `vendor/emu28-upstream/`'s non-platform-specific files (the CPU/
   RPL engine list above) compiling standalone against that header with
   plain `gcc`/`clang`, everything else stubbed out - proves the core
   engine builds portably before any rendering work starts.
3. GDI-drawing shim: route `KML.C`/`DISPLAY.C`'s bitmap composition into
   an SDL2 texture instead of a Windows DC. This is the real rendering
   work and the least precedented piece (Android's version draws through
   a Canvas, not applicable directly).
4. SDL2 event loop replacing `EMU28.C`'s window/message-pump section
   (small surface, per the table above).
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
shape, but this hasn't been set up.

## ROM images

Never committed here - see `.gitignore` and `README.md`. Emu28's own
author has no license to redistribute HP-28C ROM images; this project
follows the same policy `vger` and `soynut` already follow for their own
ROM dependencies.

## License

GPL-2.0, inherited from upstream Emu28 - see `COPYING.TXT`. Any new code
under `shim/` is GPL-2.0 too, as a derivative work building directly on
Emu28's engine.
