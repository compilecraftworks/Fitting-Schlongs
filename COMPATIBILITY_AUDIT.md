# Dependency compatibility audit

## CommonLibSSE-NG 6.7.0

- Stable tag and commit: `v6.7.0`,
  `3d81614617910e7f34b33d8750881811b5e36445`.
- Reviewed the upstream changelog through 6.7.0. The 6.7.0 change adds an
  Address Library v5 compatibility flag; preceding 6.4-6.6 releases adjust
  newer AE runtime data layouts and menu layouts.
- Audited every CommonLib API and relocation used by this plugin. The
  signatures and SE/AE IDs used for worn-item visiting, model updates,
  keyword mutation, task dispatch, and trampoline calls remain compatible.
- Verified the relevant `ActorRuntimeData` and `AIProcess` layouts used by the
  plugin are unchanged for the configured SE/AE targets.
- This project explicitly enables SE and AE and disables VR. It therefore uses
  CommonLib's flat SE/AE layout and does not compile the cross-VR virtual
  dispatch path. No plugin-owned derived virtual override follows a
  runtime-exclusive base entry.
- The upstream XMake package versions and the exact xmake-repo commit remain
  locked in `xmake-requires.lock`; together they form the verified build
  closure and are not replaced independently merely because newer individual
  versions exist.

## Build tool

- Selected stable XMake 3.1.0 at commit
  `96ad28edb71dc4e9c8193924a491629c656e8e8c`.
- Verified the downloaded official Windows x64 bundle against the SHA-256 in
  `DEPENDENCIES.md` and confirmed that it reports version 3.1.0.
- Completed a clean dependency bootstrap with XMake 3.1.0. Its locked host
  package installed 7-Zip 19.00 successfully before resolving the project
  dependencies.
- Completed a clean SE/AE `releasedbg` compile and link with XMake 3.1.0,
  CommonLibSSE-NG 6.7.0, and Visual Studio 2026/MSVC 14.51.36231.
- Confirmed the generated compile definitions enable `ENABLE_SKYRIM_SE=1` and
  `ENABLE_SKYRIM_AE=1`, omit `ENABLE_SKYRIM_VR`, and use CommonLib 6.7.0.
