package("directxtk")
    set_kind("library", {headeronly = true})
    set_homepage("https://github.com/microsoft/DirectXTK")
    set_description("DirectXTK headers only (SimpleMath). Skips fxc/xwbtool, which break under Wine.")

    set_urls("https://github.com/microsoft/DirectXTK/archive/$(version).zip",
             "https://github.com/microsoft/DirectXTK.git",
             {version = function (version)
                local versions = {
                    ["20.9.0"] = "sept2020",
                    ["21.4.0"] = "apr2021",
                    ["21.11.0"] = "nov2021",
                    ["24.2.0"] = "feb2024"
                }
                return versions[tostring(version)]
            end})
    add_versions("20.9.0", "9d5131243bf3e33db2e3a968720d860abdcbbe7cb037c2cb5dd06046d439ed09")
    add_versions("21.4.0", "481e769b1aabd08b46659bbec8363a2429f04d3bb9a1e857eb0ebd163304d1bf")
    add_versions("21.11.0", "d25e634b0e225ae572f82d0d27c97051b0069c6813d7be12453039a504dffeb8")
    add_versions("24.2.0", "edb643b2444ff24925339cfb1bc9f76c671d5404a5549d32ecaa0d61bbab28c9")

    on_install("windows", function (package)
        os.cp("Inc/*", package:installdir("include"))
    end)
