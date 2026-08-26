# Build dependencies

## CommonLibSSE-NG

- Upstream: https://github.com/alandtse/CommonLibSSE-NG
- Stable tag: `v6.7.0`
- Commit: `3d81614617910e7f34b33d8750881811b5e36445`
- License: GPL-3.0-or-later with the Modding Exception and GPL-3.0 Linking
  Exception described in `EXCEPTIONS.md`
- Expected path: `lib/commonlibsse-ng`

The repository records CommonLibSSE-NG as a Git submodule at that exact commit;
it does not track a floating branch. For a fresh GitHub checkout:

```powershell
git submodule update --init lib/commonlibsse-ng
git -C lib/commonlibsse-ng rev-parse HEAD
```

The reported commit must match the value above.

## XMake

- Upstream: https://github.com/xmake-io/xmake
- Stable version: `v3.1.0`
- Commit: `96ad28edb71dc4e9c8193924a491629c656e8e8c`
- Windows x64 bundle SHA-256:
  `41f497ed71f076a9ecf14100e77af5509656a5e41dfd4da8d068b1319a8ef895`

The project and CommonLibSSE-NG both require XMake 3.0.0 or newer. New release
build environments should use the stable version recorded above. CommonLibSSE-NG owns and
pins its XMake packages; Fitting Schlongs preserves those versions and the
exact xmake-repo commit recorded in `xmake-requires.lock` instead of overriding
tested transitive dependencies independently.

The versioned source release includes this dependency's preferred source form
and its `COPYING` and `EXCEPTIONS.md` license terms so that they accompany the
corresponding Fitting Schlongs binary. Generated CommonLib build outputs are
intentionally excluded.

Transitive packages resolved by XMake are recorded in
`xmake-requires.lock`. Their license notices remain governed by their upstream
packages.
