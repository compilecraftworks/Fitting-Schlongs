# Build dependencies

## CommonLibSSE-NG

- Upstream: https://github.com/CharmedBaryon/CommonLibSSE-NG
- Version: `v3.7.0`
- Commit: `c4ab853d095e81e3390b282d7ba01ab2f24ebf25`
- License: MIT
- Expected path: `lib/commonlibsse-ng`

For a fresh GitHub checkout:

```powershell
git clone --branch v3.7.0 --depth 1 `
  https://github.com/CharmedBaryon/CommonLibSSE-NG.git `
  lib/commonlibsse-ng
```

The versioned source release includes this dependency's preferred source form
and MIT license so that it accompanies the corresponding Fitting Schlongs
binary. Generated CommonLib build outputs are intentionally excluded.

Transitive packages resolved by XMake are recorded in
`xmake-requires.lock`. Their license notices remain governed by their upstream
packages.
