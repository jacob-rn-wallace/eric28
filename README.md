# eric28

A native, SDL2-based port of [Christoph Giesselink's Emu28](https://hp.giesselink.com/emu28.htm)
— the HP-18C/HP-28C emulator — replacing its Win32/MFC dependency with a
portable Win32-compatibility shim and an SDL2 front end, so it builds and
runs natively on macOS/Linux with plain `gcc`/`clang`. No Wine required.

Named for Eric, the previous owner of the sentient HP-28C in the SCP
Foundation's [SCP-168](https://scp-wiki.wikidot.com/scp-168).

## Why

Emu28's real HP-28C ROM emulation is useful hands-on reference material for
understanding how the HP-28C's menu system and softkey UI actually behaved
— see [vger](https://github.com/jacob-rn-wallace/vger)'s `CLAUDE.md`
("MENU UI design constraints"), which cites the HP-28C as its primary
design reference for an HP-41-inspired calculator's menu layer. Emu28
itself is Win32/MFC-only with no macOS/Linux port; this project exists to
close that gap, and is public because the same gap presumably affects
anyone else on the HP calculator emulation/preservation side who isn't on
Windows.

## Status

Early scaffolding. The upstream Emu28 v1.39 source has been vendored
unmodified (see "Architecture" below) and its Win32-API surface mapped;
no shim or SDL2 front end has been written yet. See `CLAUDE.md` for the
concrete architecture and the actual file-by-file Win32 dependency
breakdown that scopes the remaining work.

## Architecture

- `vendor/emu28-upstream/` — pristine, unmodified Emu28 v1.39 source
  (GPL-2.0, Copyright (C) 2002 Christoph Giesselink), the actual
  CPU/RPL/KML/debugger engine. Never hand-edited directly.
- `skins/hp28c/` — the official HP-28C KML skin (faceplate bitmap + key
  layout), also from hp.giesselink.com.
- `shim/` (not yet created) — new code: a Win32-type/API compatibility
  layer plus an SDL2 platform layer, following the same technique proven
  twice already for this exact emulator family by Regis Cosnier (`dgis`):
  [`emu28android`](https://github.com/dgis/emu28android) (NDK) and
  [`emu48mac`](https://github.com/dgis/emu48mac) (Cocoa, for the sibling
  HP48 emulator). Neither is vendored here — different target platform
  and, for emu48mac, a different calculator core — but both are direct
  architectural precedent.

## ROM images

Like all emulators, Emu28 needs a ROM image to run, and the author has no
license to distribute one. **No ROM is included or will ever be
committed here** — supply your own dump and point the emulator at it at
runtime. See `hp.giesselink.com/Emu28/ROMDMP.TXT` for the author's notes
on ROM uploading if you own real hardware.

## License

GPL-2.0, inherited from upstream Emu28 (see `COPYING.TXT`). Any new code
in `shim/` is licensed the same way, as a derivative work.
