# Build dependencies

## CommonLibSSE-NG

- Upstream: https://github.com/alandtse/CommonLibSSE-NG
- Branch: `ng`
- Version: `v6.7.0`
- Commit: `3d81614617910e7f34b33d8750881811b5e36445`
- License: GPL-3.0-or-later with the Modding Exception and GPL-3.0 Linking
  Exception described in `EXCEPTIONS.md`
- Expected path: `lib/commonlibsse-ng`

For a fresh GitHub checkout:

```powershell
git clone --branch v6.7.0 --depth 1 `
  https://github.com/alandtse/CommonLibSSE-NG.git `
  lib/commonlibsse-ng
```

The versioned source release includes this dependency's preferred source form
and its `COPYING` and `EXCEPTIONS.md` license terms so that they accompany the
corresponding Fitting Schlongs binary. Generated CommonLib build outputs are
intentionally excluded.

Transitive packages resolved by XMake are recorded in
`xmake-requires.lock`. Their license notices remain governed by their upstream
packages.
