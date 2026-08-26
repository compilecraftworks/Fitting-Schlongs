// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 CompileCraftWorks and Fitting Schlongs contributors

#include "RuntimeSupport.h"

#include "PCH.h"

namespace stsc {
GameVersion CurrentGameVersion() {
  const auto version = REL::Module::get().version();
  return {version.major(), version.minor(), version.patch(), version.build()};
}
} // namespace stsc
