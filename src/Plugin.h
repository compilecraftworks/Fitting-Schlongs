// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 CompileCraftWorks and Fitting Schlongs contributors

#pragma once

#ifndef STSC_VERSION_MAJOR
#error "STSC_VERSION_MAJOR must be provided by the build system"
#endif

#ifndef STSC_VERSION_MINOR
#error "STSC_VERSION_MINOR must be provided by the build system"
#endif

#ifndef STSC_VERSION_PATCH
#error "STSC_VERSION_PATCH must be provided by the build system"
#endif

#ifndef STSC_VERSION_STRING
#error "STSC_VERSION_STRING must be provided by the build system"
#endif

namespace stsc::plugin {
inline constexpr auto NAME = "Fitting Schlongs"sv;
inline constexpr REL::Version VERSION{STSC_VERSION_MAJOR, STSC_VERSION_MINOR,
                                      STSC_VERSION_PATCH, 0};
inline constexpr std::string_view VERSION_STRING{STSC_VERSION_STRING};
inline constexpr auto AUTHOR = "PenguinToast"sv;
} // namespace stsc::plugin
