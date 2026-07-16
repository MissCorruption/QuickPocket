# QuickPocket

QuickPocket is a QuickLoot IE add-on that opens a pickpocket-focused loot menu when you are sneaking and hovering a living NPC. It reuses QuickLoot IE's menu and actions, shows steal or plant chance in the info bar, and keeps vanilla pickpocket outcome handling.

If QuickLoot IE is missing or its API cannot be resolved, QuickPocket disables itself at runtime and logs the reason.

## End User Dependencies

* [QuickLoot IE](https://www.nexusmods.com/skyrimspecialedition/mods/120075) (required)
* [Address Library for SKSE Plugins](https://www.nexusmods.com/skyrimspecialedition/mods/32444)
* [Microsoft Visual C++ Redistributable for Visual Studio 2022](https://learn.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist?view=msvc-170)
* [SKSE64](https://skse.silverlock.org/)
* [SkyUI](https://www.nexusmods.com/skyrimspecialedition/mods/12604)
* [PapyrusUtil SE - Modders Scripting Utility Functions](https://www.nexusmods.com/skyrimspecialedition/mods/13048)

## Build Requirements

### Tools

* [Spriggit](https://github.com/Mutagen-Modding/Spriggit)
* [Caprica Fork by KrisV-777](https://github.com/KrisV-777/Caprica/)
* [PowerShell](https://github.com/PowerShell/PowerShell/releases/latest)
* [Visual Studio Community 2022](https://visualstudio.microsoft.com/) or any other C++23 compiler
	* Desktop development with C++
* One of the following:
	* [XMake](https://xmake.io)
	* [CMake](https://cmake.org/download/)
		* Add this to your `PATH`
		* If you're using CMake:
			* [Vcpkg](https://github.com/microsoft/vcpkg)
			* Add the environment variable `VCPKG_ROOT` with the value as the path to the folder containing vcpkg

## Building Instructions

### Using XMake

```bat
# Cloning the repo with the --recursive flag to init the submodules
git clone https://github.com/MissCorruption/QuickPocket --recursive
cd QuickPocket

# Building the xmake project
xmake build

# Building the ESP file
Path/To/Spriggit.CLI.exe deserialize --InputPath res\esp --OutputPath res\esp\QuickPocket.esp
```

### Using CMake

```bat
# Cloning the repo with the --recursive flag to init the submodules
git clone https://github.com/MissCorruption/QuickPocket --recursive
cd QuickPocket

# Building the CMake project
cmake -S . -B build
cmake --build build --config Release

# Building the ESP file
Path/To/Spriggit.CLI.exe deserialize --InputPath res\esp --OutputPath res\esp\QuickPocket.esp
```

## License

QuickPocket is licensed under the [GNU General Public License v3.0](LICENSE).

The CommonLibSSE-NG submodule remains under its own MIT license.

## Credits

* [AtomCrafty](https://github.com/AtomCrafty) for helping me figure out API stuff and adding to said API
* Nithog for testing QuickPocket with their reskins
