-- MSVC via Wine (https://github.com/mstorsjo/msvc-wine).
-- SDK: --sdk= or $MSVC_BINS or ~/msvc-bins
-- Pair with the skse-linux package overlay (tools/xmake-repo/).
toolchain("msvc-wine")
    set_kind("standalone")
    set_homepage("https://github.com/mstorsjo/msvc-wine")
    set_description("MSVC via Wine for SKSE / CommonLibSSE-NG")
    set_runtimes("MT", "MTd", "MD", "MDd")

    on_check(function (toolchain)
        import("core.project.config")
        local sdk = toolchain:sdkdir() or config.get("sdk")
        if not sdk or sdk == "" then
            sdk = os.getenv("MSVC_BINS") or path.join(os.getenv("HOME") or "", "msvc-bins")
            if os.isdir(sdk) then
                config.set("sdk", sdk)
            end
        end
        return import("check", {rootdir = path.join(os.programdir(), "toolchains", "msvc")})(toolchain)
    end)

    on_load(function (toolchain)
        import("core.project.config")
        import("load", {rootdir = path.join(os.programdir(), "toolchains", "msvc")})(toolchain)
        local sdk = toolchain:sdkdir() or os.getenv("MSVC_BINS") or path.join(os.getenv("HOME") or "", "msvc-bins")
        local bindir = path.join(sdk, "bin", toolchain:is_arch("x86") and "x86" or "x64")
        local wrap = path.join(os.scriptdir(), "msvc-wine-cl")
        if os.isfile(wrap) then
            toolchain:set("toolset", "cc", "cl@" .. wrap)
            toolchain:set("toolset", "cxx", "cl@" .. wrap)
        end
        -- msvc-wine wrappers are `rc`/`link`, not `rc.exe`/`link.exe`.
        -- Also set config.mrc so platform.tool("mrc") does not come up empty.
        local rc = path.join(bindir, "rc")
        if os.isfile(rc) then
            toolchain:set("toolset", "mrc", "rc@" .. rc)
            if not config.get("mrc") then
                config.set("mrc", rc)
            end
        end
        if os.isfile(path.join(bindir, "link")) then
            toolchain:set("toolset", "ld", path.join(bindir, "link"))
            toolchain:set("toolset", "sh", path.join(bindir, "link"))
            toolchain:set("toolset", "ar", path.join(bindir, "link"))
        end
        -- xmake still emits -std:c++23 on Linux; MSVC ignores it unless rewritten.
        toolchain:add("cxxflags", "/std:c++latest")
        toolchain:add("cxflags", "/std:c++latest")
        -- /Zi needs mspdbsrv.exe; Wine deadlocks on that pipe. /Z7 embeds debug info.
        toolchain:add("cxflags", "/Z7")
        toolchain:add("cxxflags", "/Z7")
    end)
