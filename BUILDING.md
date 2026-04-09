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

- This repo still does not include the external SKSE source tree, so the project cannot compile until that dependency is added locally.
- The plugin source has been updated so its own hard-coded relocation points now resolve through Address Library IDs instead of staying fixed to `1.5.97`.
