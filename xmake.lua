set_xmakever("3.0.0")

local PLUGIN      = "QuickPocket"
local AUTHOR      = "Miss Corruption"
local CONTACT     = ""
local DESCRIPTION = "QuickLootIE add-on for contextual pickpocket quick menu."
local VERSION     = "1.0.0"
local LICENSE     = "GPL-3.0"

set_config("skse_xbyak", true)
set_config("skyrim_se", true)
set_config("skyrim_ae", true)
set_config("skyrim_vr", true)

if is_host("linux") then
    includes("tools/toolchains/msvc-wine.lua")
    add_repositories("skse-linux tools/xmake-repo")
    set_toolchains("msvc-wine")
    add_cxxflags("cl::/std:c++latest", {force = true})
end

set_plat("windows")
set_arch("x64")

includes("lib/commonlibsse-ng")

set_project(PLUGIN)
set_version(VERSION)
set_license(LICENSE)
set_languages("c++23")
set_warnings("allextra")
set_encodings("utf-8")

add_rules("mode.debug", "mode.release", "mode.releasedbg")
add_rules("plugin.vsxmake.autoupdate")
set_defaultmode("releasedbg")
set_policy("build.ccache", true)
set_policy("check.auto_ignore_flags", false)

add_requires("nlohmann_json")

if is_host("linux") then
    target("commonlibsse-ng")
        add_cxxflags("cl::/std:c++latest", {force = true})
    target_end()
end

target(PLUGIN)
    add_rules("commonlibsse-ng.plugin", {
        name = PLUGIN,
        author = AUTHOR,
        contact = CONTACT,
        description = DESCRIPTION,
    })

    add_files("src/**.cpp")
    add_headerfiles("src/**.h")
    add_includedirs("src")
    add_packages("nlohmann_json", "spdlog")
    if os.isdir("include") then
        add_headerfiles("include/**.h")
        add_includedirs("include")
    end
    if os.isfile("src/pch.h") then
        set_pcxxheader("src/pch.h")
    elseif os.isfile("src/PCH.h") then
        set_pcxxheader("src/PCH.h")
    end

    add_defines("NOMINMAX", "WIN32_LEAN_AND_MEAN")

    -- Data-layout extras (SKSE/Plugins/*.ini, Scripts/, Interface/, …).
    if os.isdir("package") then
        add_installfiles("package/(**)")
    end
    add_installfiles("res/Source/Scripts/*.psc", { prefixdir = "Source/Scripts" })

    -- Plugin rule installs the DLL (and package/) after every rebuild.
    -- CLIB replaces this when XSE_TES5_MODS_PATH or XSE_TES5_GAME_PATH is set.
    set_installdir("$(projectdir)/build/install")

    after_install(function (target)
        -- Linux is case-sensitive; a leftover Interface/translations would ship
        -- beside SKSE's Interface/Translations and show up as two folders.
        local installdir = target:installdir()
        local staleTranslations = path.join(installdir, "Interface", "translations")
        if os.isdir(staleTranslations) then
            os.rm(staleTranslations)
        end
        local staleConfig = path.join(installdir, "QuickPocket")
        if os.isdir(staleConfig) then
            os.rm(staleConfig)
        end
    end)
target_end()
