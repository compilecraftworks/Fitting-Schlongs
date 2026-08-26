# Fitting Schlongs

[![ko-fi](https://ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/M1P225QD23)

Fitting Schlongs is an ESP-less SKSE plugin that corrects genital visibility
for modular Skyrim outfits using slots 32 and 49. It supports the conventional
Schlongs of Skyrim (SOS) and The New Gentleman (TNG) keyword and genital-armor
structures.

## Requirements

- Skyrim Special Edition or Anniversary Edition
- SKSE64
- Address Library for SKSE Plugins
- Schlongs of Skyrim or The New Gentleman

Skyrim Fitting System is **not** a requirement. When
`SkyrimFittingSystem.dll` is loaded, Fitting Schlongs disables itself because
Skyrim Fitting System already contains this correction and both plugins must
not install the same skinning hooks.

## Behavior

- Slot 32 body armor normally conceals genital armor unless the item is
  classified as upper-only clothing.
- Recognized slot 32 upper-only clothing is corrected to revealing.
- Recognized slot 49 lower-body, underwear, swimwear, pants, and skirt items
  are corrected to concealing.
- A classifier result takes precedence over conflicting SOS/TNG keywords added
  by an ESP or KID rule.
- Ordinary slot 32 body armor keeps SOS/TNG's default covering behavior; the
  plugin does not add underwear keywords to otherwise unmodified body armor.
- Only armor identified as SOS/TNG genital armor is filtered. Unrelated slot 52
  equipment is not hidden.
- Runtime keyword changes exist only in memory and do not modify plugin files.

## Installation

Install the release archive with a mod manager. Its deployable layout is:

```text
SKSE/
  Plugins/
    FittingSchlongs.dll
LICENSE
NOTICE.md
THIRD_PARTY_NOTICES.md
```

## Building

The project uses XMake and CommonLibSSE-NG.

```powershell
xmake f -m releasedbg
xmake
```

The repository pins CommonLibSSE-NG as a submodule under
`lib/commonlibsse-ng`. Run `git submodule update --init lib/commonlibsse-ng`
after cloning. The source release archive includes the dependency source used
for the corresponding binary. Exact tool versions, commits, and integrity
information are recorded in `DEPENDENCIES.md`.
The reviewed API, ABI, runtime-layout, and build-tool results are recorded in
`COMPATIBILITY_AUDIT.md`.

Build output:

```text
build/windows/x64/releasedbg/FittingSchlongs.dll
```

## License and source availability

Fitting Schlongs is free software licensed under the
[GNU General Public License version 3](LICENSE). It is derived from the
SOS/TNG slot-correction work developed as part of Skyrim Fitting System. See
[NOTICE.md](NOTICE.md) for provenance and modification notices.

Binary releases are accompanied by the complete corresponding source for that
version. The source archive contains the preferred source form, build scripts,
locked dependency information, the CommonLibSSE-NG source used to build it,
and applicable license notices. You may use, modify, and redistribute the work
under GPLv3.

This project is not affiliated with Bethesda Game Studios, SKSE, Schlongs of
Skyrim, or The New Gentleman.
