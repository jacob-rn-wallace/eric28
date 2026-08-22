# DEVLOG

Chronological record of how this project got started and why it's shaped
the way it is. `CLAUDE.md` is the authoritative current-state/architecture
doc; this file is the "why we ended up here" history behind it, following
the same DEVLOG convention as `vger`/`soynut`.

## 2026-08-22: Origin and the road to this repo

Started as a `vger` session tangent: the user wanted Emu28 (the HP-28C
emulator) running natively on macOS with a one-click/double-click
launch, similar to `soynut`'s `sim/`. What followed was a real
investigation, not a quick download-and-go, and it's worth recording the
dead ends so a future session doesn't repeat them.

### First finding: Emu28 is Win32/MFC-only

`hp.giesselink.com/emu28.htm` confirms: GPL-2.0, source compiles with
Visual Studio 2022, "runs on all Win32 platforms." No macOS/Linux port,
no cross-platform build system, no SDL/portable UI layer mentioned
anywhere on the page.

**MAME was checked and ruled out**: its `src/mame/hp/hp48.cpp` driver
(confirmed by reading the actual `COMP()` macro list in the file) covers
HP48SX/S/GX/G/G+, HP49G, HP38G, HP39G — but nothing earlier. No HP-28C or
HP-28S driver exists in MAME at all.

### Wine attempt (abandoned for now)

Proposed running the official Windows binary under Wine, wrapped in a
minimal native `.app` for double-click launch (no Whisky dependency,
fully scriptable). Hit real, current problems, not flaky one-offs:

- **Every Homebrew Wine cask is Gatekeeper-deprecated.** `wine-stable`,
  `wine@staging`, `wine@devel` are all flagged "does not pass the macOS
  Gatekeeper check" and scheduled for removal 2026-09-01 — about a week
  out at the time of writing. Same failure class as the ARM toolchain
  SIGKILL issue documented in `vger`'s own `CLAUDE.md` ("Toolchain
  note"): ad-hoc-resigned Homebrew bottles get rejected by modern
  macOS code-signing enforcement.
- **The 146MB `gstreamer-runtime` dependency wouldn't fully download** in
  the sandboxed Claude Code session used for this work — truncated
  partway on both the default HTTP/2 attempt and a manual HTTP/1.1
  workaround (once at ~5.5%). Looked like an environment-level
  restriction on large/long-lived downloads in that specific sandbox,
  not a server or Homebrew problem (the file itself served fine via a
  plain `curl -I`).
- **Whisky** (the GUI Wine wrapper, the fallback option) had already been
  disabled in Homebrew back in April 2026 for being unmaintained
  upstream (last real release was v2.3.5, April 2025). Direct-from-
  GitHub-releases install (a 5MB zip) was identified as still possible
  but never actually tried before the user redirected the conversation.

**Bottom line if picking Wine back up later:** try it from a real
terminal outside any sandboxed agent session (the download-truncation
problem looks environment-specific), and go in expecting the Gatekeeper
rejection is a live risk regardless — it may need to be confirmed dead
before investing further, same as the ARM toolchain issue was.

### The emu41gcc question - the actual turning point

The user asked directly: soynut's HP-41 side has `emu41gcc` (a GCC port
of J-F Garnier's original *DOS* HP-41 emulator - portable because DOS
was already a simple target) - is there an equivalent for HP-28C?

Answer, after checking: **no**, and the reason matters architecturally.
Emu28 was written against Win32/MFC from day one; there's no portable
DOS-era ancestor to GCC-port the way `emu41gcc` had one. This ruled out
a "just recompile it" shortcut entirely.

That question is what led to actually searching GitHub for prior
portability work on this exact emulator family, which turned up the two
pieces of real precedent this project is built on:

- **[`dgis/emu28android`](https://github.com/dgis/emu28android)** - this
  *exact* emulator (Emu28), ported to Android/NDK. Confirmed by reading
  its actual source tree: a ~170KB `win32-layer.c`/`.h` (reimplements
  Win32 primitives - critical sections, GDI-style bitmap drawing,
  timers, file I/O) plus a small (~12KB) Android-specific glue layer
  (`android-emu.c`, `android-layer.c`) and a 48KB JNI bridge
  (`emu-jni.c`). The untouched original Emu28 `core/` sits underneath.
- **[`dgis/emu48mac`](https://github.com/dgis/emu48mac)** - the same
  author's native **Cocoa** port, but of the *sibling* emulator Emu48
  (HP48 family - a related but distinct core from the same original
  author, Giesselink), from 2018 ("Emu48 running on High Sierra and
  Mojave", last pushed 2018-11-02, GPL-2.0). Real Xcode project,
  Objective-C source, a `MacPatch/` directory (~130KB of `.m`/`.h`)
  doing the same shim job as `win32-layer.c` but for Cocoa instead of
  Android, plus real KML-parsing/execution glue written directly
  against Emu48's specific `Core/` API surface.

This is what proved a native macOS port of this emulator family is real
and has actually been done - but `emu48mac` targets a *different*
calculator core (Emu48/HP48, not Emu28/HP18C-28C) and a Cocoa UI, so it's
architectural precedent, not a drop-in. Also considered and ruled out:
**x48** (Eric Smith - same author as `Nonpareil`, which `vger`'s
`CLAUDE.md` already treats as reference material) is a genuinely
portable, X11-native, GCC-buildable Saturn-CPU emulator - but it targets
HP48SX/GX ROMs, not HP-28C, and running it would surface the HP48-style
RPL menu system that `vger`'s own `CLAUDE.md` ("MENU UI design
constraints") explicitly rejected as a model in favor of the HP-28C
itself. Not useful for the actual research goal even though it's
technically the easiest thing to build.

### Decision

Given no existing SDL2 port of anything in this emulator family exists
(the precedent is Android/NDK for this exact core, and Cocoa for the
sibling core), the user chose to start that project directly: reuse
Emu28's own untouched engine, write a *new* Win32-compatibility shim
targeting SDL2 instead of Cocoa or Android NDK. Same toolchain `vger`'s
own harness and `soynut`'s `sim/` already use (plain `gcc`/`clang` +
SDL2) - no Wine, no Xcode, no Gatekeeper fights.

Repo name/location (`/Users/jake/eric28`, GitHub `jacob-rn-wallace/eric28`,
public) and the name itself were the user's own call: named for Eric, the
*previous owner* of the sentient HP-28C in the SCP Foundation's
[SCP-168](https://scp-wiki.wikidot.com/scp-168) (not the calculator
itself - corrected mid-session after an initial wrong guess at the
reference). Public because the underlying gap (no native Emu28 on
macOS/Linux) isn't specific to one person.

### What actually landed this session

1. Vendored Emu28 v1.39 source unmodified into `vendor/emu28-upstream/`
   (from `hp.giesselink.com/Emu28/E28SP139.ZIP`, 308KB) and the official
   HP-28C KML skin into `skins/hp28c/` (from
   `hp.giesselink.com/Emu28/Kmlpc/SKN28C.ZIP`, 374KB).
2. Fetched `COPYING.TXT` (GPL-2.0 text) from
   `hp.giesselink.com/COPYING.TXT` as this project's `LICENSE`.
3. **Mapped the real Win32 API surface directly against the source**
   (not guessed from filenames) - see `CLAUDE.md`'s table. Only 8 of the
   53 vendored files touch GDI drawing, window/message-pump, the
   registry, or DirectSound/`waveOut` at all; the CPU/RPL engine and
   most supporting logic never does.
4. Confirmed `EMU28.C` has no `WinMain`/window-creation code of its own
   in this source package - the real Windows binary needs `EMU28.RC`
   (a Win32 resource script) built in via the separate `e28vs2022.zip`
   project (not vendored - irrelevant to an SDL2 port, which handles
   window creation and the event loop itself).
5. Wrote `README.md` and `CLAUDE.md`, created the public GitHub repo,
   pushed the initial commit (`b3c7d57`).

**Nothing in `shim/` exists yet.** No build system, no SDL2 integration,
no compiled output. See `CLAUDE.md`'s "Not yet decided / next milestones"
for the actual next steps, in order: (1) a Win32-types-and-primitives
shim header, (2) get the portable-core files compiling standalone against
it with everything else stubbed, (3) the GDI-to-SDL2 rendering shim (the
hardest, least-precedented piece - Android's version draws through a
Canvas, not directly transferable), (4) SDL2 event loop replacing
`EMU28.C`'s small windowing section, (5) stub `SETTINGS.C`/`SOUND.C`
rather than porting the registry/DirectSound, (6) `STEGANO.C` last,
lowest priority.

### One tooling gotcha worth remembering

macOS's default `grep` in the Claude Code shell used for this work is
aliased to a `ugrep`-based wrapper (`--ignore-files --hidden -I`) that
silently treats the vendored source files as binary and skips them -
because they're ISO-8859-1-encoded (e.g. "Gießelink" in the copyright
headers) rather than UTF-8, and `-I` skip-binary detection false-positives
on that. This produces **silent false "zero matches"**, not an error - a
`grep -c` returns nothing and exit code 1, indistinguishable at a glance
from a genuine zero-match result. Any future search across
`vendor/emu28-upstream/` should use `/usr/bin/grep` directly (or
`iconv -f ISO-8859-1 -t UTF-8` first) to get real results.
