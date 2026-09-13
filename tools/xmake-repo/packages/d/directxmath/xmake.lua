package("directxmath")
    set_kind("library", {headeronly = true})
    set_homepage("https://github.com/microsoft/DirectXMath")
    set_description("DirectXMath headers; install by copy so CMake try_compile is not required under Wine.")
    set_license("MIT")

    local tag = {
        ["2022.12"] = "dec2022",
        ["2024.02"] = "feb2024"
    }
    add_urls("https://github.com/microsoft/DirectXMath/archive/refs/tags/$(version).zip", {version = function (version) return tag[tostring(version)] end})
    add_urls("https://github.com/microsoft/DirectXMath.git")
    add_versions("2022.12", "2ed0ae7d7fe5d11ad11f6d3e3d9b31ce686024a551cf82ade723de86aa7b4b57e1")
    add_versions("2024.02", "214d71420107249dfb4bbc37a573f288b0951cc9ffe323dbf662101f3df4d766")

    add_includedirs("include/directxmath")

    on_install("windows", function (package)
        os.cp("Inc/*", package:installdir("include", "directxmath"))
    end)
