// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 CompileCraftWorks and Fitting Schlongs contributors

#pragma once

#include <cstdint>

namespace stsc {
struct GameVersion {
  std::uint16_t major{};
  std::uint16_t minor{};
  std::uint16_t patch{};
  std::uint16_t build{};

  constexpr bool operator==(const GameVersion &) const = default;
};

enum class RuntimeLayout {
  kUnsupported,
  kSE1597,
  kAE161170,
};

[[nodiscard]] constexpr RuntimeLayout
SelectRuntimeLayout(const GameVersion a_version) noexcept {
  if (a_version == GameVersion{1, 5, 97, 0}) {
    return RuntimeLayout::kSE1597;
  }
  if (a_version == GameVersion{1, 6, 1170, 0}) {
    return RuntimeLayout::kAE161170;
  }
  return RuntimeLayout::kUnsupported;
}

[[nodiscard]] constexpr bool
IsSupportedRuntime(const GameVersion a_version) noexcept {
  return SelectRuntimeLayout(a_version) != RuntimeLayout::kUnsupported;
}

[[nodiscard]] GameVersion CurrentGameVersion();
} // namespace stsc
