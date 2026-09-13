# QuickPocket

QuickPocket is a QuickLoot IE add-on that opens a pickpocket-focused loot menu when you are sneaking and hovering a living NPC. It reuses QuickLoot IE's menu and actions, shows steal or plant chance in the info bar, and keeps vanilla pickpocket outcome handling.

## End User Dependencies

* [QuickLoot IE](https://www.nexusmods.com/skyrimspecialedition/mods/120075) 4.1.1+
* [Address Library for SKSE Plugins](https://www.nexusmods.com/skyrimspecialedition/mods/32444) (SE/AE)
* [VR Address Library for SKSEVR](https://www.nexusmods.com/skyrimspecialedition/mods/58101) (VR)
* [Microsoft Visual C++ Redistributable for Visual Studio 2022](https://learn.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist?view=msvc-170)
* [SKSE64](https://skse.silverlock.org/) matching your game version
* [SkyUI](https://www.nexusmods.com/skyrimspecialedition/mods/12604)
* [PapyrusUtil SE - Modders Scripting Utility Functions](https://www.nexusmods.com/skyrimspecialedition/mods/13048)

## Build Requirements

SKSE plugin built with [CommonLibSSE-NG](https://github.com/alandtse/CommonLibVR/tree/ng) and [xmake](https://xmake.io) 3.0+. Builds on Windows with MSVC, and on Linux with the same compiler through [msvc-wine](https://github.com/mstorsjo/msvc-wine).

**Windows:** [Visual Studio 2022 (C++ desktop workload)](https://visualstudio.microsoft.com/) and [xmake](https://xmake.io) 3.0+.

**Linux (Arch):**

```bash
sudo pacman -S --needed wine xmake python msitools samba python-simplejson
```

Debian / Ubuntu: `wine64`, `python3`, `msitools`, `winbind`, and [xmake 3.0+](https://xmake.io/#/guide/installation) if the distro package is too old.

The first Linux build installs MSVC and the Windows SDK into `~/msvc-bins` (override with `MSVC_BINS`). You can also run `./tools/setup-msvc-wine.sh` on its own.

ESP and Papyrus (optional for a DLL-only rebuild):

* [Spriggit](https://github.com/Mutagen-Modding/Spriggit)
* [Caprica Fork by KrisV-777](https://github.com/KrisV-777/Caprica/)

## Clone

```bash
git clone --recurse-submodules https://github.com/MissCorruption/QuickPocket.git
cd QuickPocket
```

`--recurse-submodules` is required. If you already cloned without it:

```bash
git submodule update --init --recursive
```

## Build

Linux:

```bash
./tools/build.sh
```

Windows:

```bat
tools\build.bat
```

## License

QuickPocket is licensed under the [GNU General Public License v3.0](LICENSE).

## Credits

* [AtomCrafty](https://github.com/AtomCrafty) for helping me figure out API stuff and adding to said API
* Nithog for testing QuickPocket with their reskins
