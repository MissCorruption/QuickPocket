package("spdlog")
    set_homepage("https://github.com/gabime/spdlog")
    set_description("Compiled spdlog via xmake (CMake pthread probes hang under Wine).")
    set_license("MIT")

    add_urls("https://github.com/gabime/spdlog/archive/refs/tags/$(version).zip",
             "https://github.com/gabime/spdlog.git")
    add_versions("v1.16.0", "3d25808d2fc4db86621a46855800c99ab5734999b61c4cbf9470edf631555397")

    add_configs("header_only",     {description = "Use header only version.", default = true, type = "boolean"})
    add_configs("std_format",      {description = "Use std::format instead of fmt library.", default = false, type = "boolean"})
    add_configs("wchar",           {description = "Support wchar api.", default = false, type = "boolean"})
    add_configs("wchar_filenames", {description = "Support wchar filenames.", default = false, type = "boolean"})
    add_configs("wchar_console",   {description = "Support wchar output to console.", default = false, type = "boolean"})
    add_configs("tls",             {description = "Allow spdlog to using thread local storage.", default = true, type = "boolean"})
    add_configs("thread_id",       {description = "Allow spdlog to querying the thread id.", default = true, type = "boolean"})
    add_configs("noexcept",        {description = "Compile with -fno-exceptions.", default = false, type = "boolean"})

    on_load(function (package)
        if package:config("header_only") then
            package:set("kind", "library", {headeronly = true})
        else
            -- Matches Windows CI: compiled lib so public headers do not pull windows.h
            -- into CommonLibSSE-NG's PCH (REX::W32::CP_UTF8 vs CP_UTF8 macro).
            package:add("defines", "SPDLOG_COMPILED_LIB")
        end
        if package:config("std_format") then
            package:add("defines", "SPDLOG_USE_STD_FORMAT")
        end
        if package:config("wchar") then
            package:add("defines", "SPDLOG_WCHAR_TO_UTF8_SUPPORT")
        end
        if package:config("wchar_filenames") then
            package:add("defines", "SPDLOG_WCHAR_FILENAMES")
        end
        if package:config("wchar_console") then
            package:add("defines", "SPDLOG_UTF8_TO_WCHAR_CONSOLE")
        end
        if package:config("noexcept") then
            package:add("defines", "SPDLOG_NO_EXCEPTIONS")
        end
        if not package:config("tls") then
            package:add("defines", "SPDLOG_NO_TLS")
        end
        if not package:config("thread_id") then
            package:add("defines", "SPDLOG_NO_THREAD_ID")
        end
    end)

    on_install(function (package)
        if package:config("header_only") then
            os.cp("include", package:installdir())
            return
        end

        local defines = {"SPDLOG_COMPILED_LIB"}
        if package:config("std_format") then
            table.insert(defines, "SPDLOG_USE_STD_FORMAT")
        end
        if package:config("wchar") then
            table.insert(defines, "SPDLOG_WCHAR_TO_UTF8_SUPPORT")
        end
        if package:config("wchar_filenames") then
            table.insert(defines, "SPDLOG_WCHAR_FILENAMES")
        end
        if package:config("wchar_console") then
            table.insert(defines, "SPDLOG_UTF8_TO_WCHAR_CONSOLE")
        end
        if package:config("noexcept") then
            table.insert(defines, "SPDLOG_NO_EXCEPTIONS")
        end
        if not package:config("tls") then
            table.insert(defines, "SPDLOG_NO_TLS")
        end
        if not package:config("thread_id") then
            table.insert(defines, "SPDLOG_NO_THREAD_ID")
        end

        local files = {
            "src/spdlog.cpp",
            "src/stdout_sinks.cpp",
            "src/color_sinks.cpp",
            "src/file_sinks.cpp",
            "src/async.cpp",
            "src/cfg.cpp",
        }
        if not package:config("std_format") then
            table.insert(files, "src/bundled_fmtlib_format.cpp")
        end

        local quoted_files = {}
        for _, f in ipairs(files) do
            table.insert(quoted_files, '"' .. f .. '"')
        end
        local quoted_defines = {}
        for _, d in ipairs(defines) do
            table.insert(quoted_defines, '"' .. d .. '"')
        end

        io.writefile("xmake.lua", table.concat({
            'add_rules("mode.debug", "mode.release")',
            'set_languages("c++20")',
            'target("spdlog")',
            '    set_kind("static")',
            '    add_files(' .. table.concat(quoted_files, ", ") .. ')',
            '    add_includedirs("include", {public = true})',
            '    add_headerfiles("include/(**.h)")',
            '    add_defines(' .. table.concat(quoted_defines, ", ") .. ', {public = true})',
            '    add_cxflags("cl::/utf-8", "cl::/std:c++20", {force = true})',
            'target_end()',
            "",
        }, "\n"))
        import("package.tools.xmake").install(package)
    end)
