-- SPDX-License-Identifier: GPL-3.0-only
-- Copyright (C) 2026 CompileCraftWorks and Fitting Schlongs contributors

set_xmakever("3.0.0")

set_config("skse_xbyak", true)
set_config("skyrim_vr", false)

includes("lib/commonlibsse-ng")

local build_version = os.getenv("FITTING_SCHLONGS_BUILD_VERSION") or "1.0.3"
local build_version_string =
    os.getenv("FITTING_SCHLONGS_BUILD_VERSION_STRING") or build_version
local major, minor, patch = build_version:match("^(%d+)%.(%d+)%.(%d+)$")
if not major then
    error("FITTING_SCHLONGS_BUILD_VERSION must be in major.minor.patch format, got " .. build_version)
end

set_project("FittingSchlongs")
set_version(build_version)
set_license("GPL-3.0")

set_languages("c++23")
set_warnings("allextra")

set_policy("package.requires_lock", true)

add_rules("mode.release", "mode.debug", "mode.releasedbg")
add_rules("plugin.vsxmake.autoupdate")

target("FittingSchlongs")
    set_version(build_version)

    add_deps("commonlibsse-ng")

    add_rules("commonlibsse-ng.plugin", {
        name = "Fitting Schlongs",
        author = "PenguinToast",
        description = "ESP-less Schlongs of Skyrim and The New Gentleman slot compatibility correction plugin"
    })

    add_defines("STSC_VERSION_MAJOR=" .. major)
    add_defines("STSC_VERSION_MINOR=" .. minor)
    add_defines("STSC_VERSION_PATCH=" .. patch)
    add_defines('STSC_VERSION_STRING="' .. build_version_string .. '"')

    add_files("src/**.cpp")
    add_headerfiles("src/**.h")
    add_includedirs("src")
    set_pcxxheader("src/pch.h")
