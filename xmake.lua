set_xmakever("2.9.5")

-- This needs to be included before the project setup
if os.isdir("extern/CommonLibSSE-NG") then
    includes("extern/CommonLibSSE-NG")
end

PROJECT_NAME = "QuickPocket"

set_project(PROJECT_NAME)
set_version("0.1.0")
set_languages("cxx23")
set_license("GPL-3.0")
set_warnings("allextra")

add_rules("mode.debug", "mode.release", "mode.releasedbg")
add_rules("plugin.vsxmake.autoupdate")

set_policy("package.requires_lock", false)
set_policy("check.auto_ignore_flags", false)

if is_mode("debug") then
    add_defines("DEBUG")
    set_optimize("none")
elseif is_mode("release", "releasedbg") then
    add_defines("NDEBUG")
    set_optimize("fastest")
    set_symbols("debug")
end

set_config("skse_xbyak", true)
set_config("skyrim_se", true)
set_config("skyrim_ae", true)
set_config("skyrim_vr", true)

if not os.isdir("extern/CommonLibSSE-NG") then
    add_requires("commonlibsse-ng")
end

target(PROJECT_NAME)
    set_kind("shared")
    if os.isdir("extern/CommonLibSSE-NG") then
        add_deps("commonlibsse-ng")
        add_rules("commonlibsse-ng.plugin", {
            name = PROJECT_NAME,
            author = "Miss Corruption",
            description = "QuickLootIE add-on for contextual pickpocket quick menu."
        })
    else
        add_packages("commonlibsse-ng")
    end

    set_pcxxheader("src/PCH.h")
    add_files("src/**.cpp")
    add_headerfiles("src/**.h", "include/**.h")
    add_includedirs("src", "include")

    add_cxxflags(
        "cl::/cgthreads4",
        "cl::/diagnostics:caret",
        "cl::/FS",
        "cl::/DNOMINMAX",
        "cl::/external:W0",
        "cl::/fp:contract",
        "cl::/fp:except-",
        "cl::/guard:cf-",
        "cl::/Zc:enumTypes",
        "cl::/Zc:preprocessor",
        "cl::/Zc:templateScope",
        "cl::/utf-8"
    )
    add_cxxflags(
        "cl::/wd4068",
        "cl::/wd4200",
        "cl::/wd4201"
    )

    if is_mode("release", "releasedbg") then
        add_cxxflags("cl::/Zc:inline", "cl::/JMC-", "cl::/Ob3")
    end
target_end()
