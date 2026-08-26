#include "RuntimeSupport.h"

#include <cassert>

int main() {
  using stsc::GameVersion;
  using stsc::RuntimeLayout;

  static_assert(stsc::SelectRuntimeLayout({1, 5, 97, 0}) ==
                RuntimeLayout::kSE1597);
  static_assert(stsc::SelectRuntimeLayout({1, 6, 1170, 0}) ==
                RuntimeLayout::kAE161170);

  assert(!stsc::IsSupportedRuntime(GameVersion{1, 5, 96, 0}));
  assert(stsc::IsSupportedRuntime(GameVersion{1, 5, 97, 0}));
  assert(!stsc::IsSupportedRuntime(GameVersion{1, 5, 98, 0}));
  assert(!stsc::IsSupportedRuntime(GameVersion{1, 6, 1169, 0}));
  assert(stsc::IsSupportedRuntime(GameVersion{1, 6, 1170, 0}));
  assert(!stsc::IsSupportedRuntime(GameVersion{1, 6, 1171, 0}));
  assert(!stsc::IsSupportedRuntime(GameVersion{1, 7, 0, 0}));
}
