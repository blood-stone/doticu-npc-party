# Building The DLL

The SKSE plugin project is no longer tied to the original author's private `_plugin\skse64` checkout or a fixed Skyrim `1.5.97` runtime.

## What you still need

- A Windows C++ toolchain with the `v143` MSVC x64 tools and a Windows 10 SDK.
- A matching SKSE source tree for the runtime you want to target.
- The Address Library files installed at runtime under `Data\SKSE\Plugins`.

## Expected SKSE layout

By default the project now points at the `2.2.3` tree currently in this repo:

- `D:\repository\2026\doticu-npc-party\skse64_2_02_03\skse64_2_02_03\src\skse64`
- `D:\repository\2026\doticu-npc-party\skse64_2_02_03\skse64_2_02_03\src\common`

If your SKSE source tree lives somewhere else, set the MSBuild properties `Skse64Root` and `SkseCommonRoot` before building.

## Address Library behavior

- On Skyrim `1.6.x`, the plugin now looks for `versionlib-<game version>.bin`.
- On Skyrim `1.5.x`, it still accepts the older `version-<game version>.bin`.
- If no database file is found, the code only falls back to baked-in offsets for Skyrim `1.5.97.0`.

The bundled Address Library archive in this repo already includes `versionlib-1-6-640-0.bin`, which is the file needed for Skyrim `1.6.640`.

## Notes

- Before building `doticu_npcp`, build these SKSE libraries once in `Release x64`:
  - `src\common\common\common_vc14.vcxproj`
  - `src\skse64\skse64_common\skse64_common.vcxproj`
  - `src\skse64\skse64\skse64.vcxproj` as `Release_Lib_VC142|x64`
- The plugin project links directly against the built library outputs from those projects.
- The post-build copy step now skips itself if no local `SKSE\Plugins` folder exists.
- The plugin source has been updated so its own hard-coded relocation points now resolve through Address Library IDs instead of staying fixed to `1.5.97`.

## Build From A Shell

Visual Studio's GUI can cache stale solution settings on older SKSE projects. If the IDE starts insisting on the wrong SDK, toolset, or `Win32`, use the Visual Studio developer shell instead.

From `Developer PowerShell for Visual Studio`, run:

```powershell
& 'C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe' `
  'D:\repository\2026\doticu-npc-party\Source\Plugins\doticu_npcp\doticu_npcp.sln' `
  /t:doticu_npcp /p:Configuration=Release /p:Platform=x64
```

That bypasses most of the GUI state issues and forces the expected `Release|x64` build.

## Papyrus / MCM Scripts

The MCM root script lives at `Source\Scripts\doticu_npcp_mcm.psc`. Rebuilding it is useful when the MCM opens but does not populate correctly.

The local Papyrus compiler can live under `tools\Papyrus`, but it is intentionally ignored by git because it is a machine-local tool drop.

To compile the MCM script from a shell, you need:

- `tools\Papyrus\PapyrusCompiler.exe`
- `tools\Papyrus\TESV_Papyrus_Flags.flg`
- Skyrim/vanilla script sources
- SKSE script sources
- SkyUI script headers, especially `SKI_ConfigBase.psc`

The compiler alone is not enough for `doticu_npcp_mcm.psc`, because that script extends `SKI_ConfigBase`.

Recommended local source layout:

- `Source\Scripts`
- `skse64_2_02_03\skse64_2_02_03\Data\Scripts\Source`
- a local SkyUI headers/source folder containing `SKI_ConfigBase.psc`

Example shell command once those sources exist:

```powershell
& 'D:\repository\2026\doticu-npc-party\tools\Papyrus\PapyrusCompiler.exe' `
  'D:\repository\2026\doticu-npc-party\Source\Scripts\doticu_npcp_mcm.psc' `
  -f='D:\repository\2026\doticu-npc-party\tools\Papyrus\TESV_Papyrus_Flags.flg' `
  -i='D:\repository\2026\doticu-npc-party\Source\Scripts;D:\repository\2026\doticu-npc-party\skse64_2_02_03\skse64_2_02_03\Data\Scripts\Source;D:\path\to\SkyUI\Scripts\Source' `
  -o='D:\repository\2026\doticu-npc-party\scripts'
```

If the MCM still misbehaves after rebuilding the script, check:

- `Documents\My Games\Skyrim Special Edition\SKSE\doticu_npcp.log`
- `Documents\My Games\Skyrim Special Edition\SKSE\skse64.log`

The DLL now logs `MCM building page: ...` when native page generation runs, which helps separate a Papyrus-side problem from a DLL-side one.
