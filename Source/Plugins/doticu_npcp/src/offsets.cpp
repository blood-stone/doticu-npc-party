/*
    Copyright © 2020 r-neal-kelly, aka doticu
*/

#include "offsets.h"

#include <Windows.h>

#include <cstdio>
#include <fstream>
#include <map>
#include <string>
#include <vector>

namespace {

    class VersionDbCompat
    {
    public:
        VersionDbCompat()
        {
            Clear();
        }

        bool GetExecutableVersion(int& major, int& minor, int& revision, int& build) const
        {
            TCHAR version_file[MAX_PATH];
            GetModuleFileName(nullptr, version_file, MAX_PATH);

            DWORD ver_handle = 0;
            DWORD ver_size = GetFileVersionInfoSize(version_file, &ver_handle);
            if (ver_size == 0) {
                return false;
            }

            std::vector<char> ver_data(ver_size);
            if (!GetFileVersionInfo(version_file, ver_handle, ver_size, ver_data.data())) {
                return false;
            }

            char* version_string = nullptr;
            UINT version_length = 0;
            if (VerQueryValueA(ver_data.data(),
                               "\\StringFileInfo\\040904B0\\ProductVersion",
                               reinterpret_cast<LPVOID*>(&version_string),
                               &version_length) &&
                version_length > 0 &&
                ParseVersionFromString(version_string, major, minor, revision, build)) {
                return true;
            }

            version_string = nullptr;
            version_length = 0;
            if (VerQueryValueA(ver_data.data(),
                               "\\StringFileInfo\\040904B0\\FileVersion",
                               reinterpret_cast<LPVOID*>(&version_string),
                               &version_length) &&
                version_length > 0 &&
                ParseVersionFromString(version_string, major, minor, revision, build)) {
                return true;
            }

            return false;
        }

        bool Load()
        {
            int major = 0;
            int minor = 0;
            int revision = 0;
            int build = 0;
            if (!GetExecutableVersion(major, minor, revision, build)) {
                return false;
            }

            return Load(major, minor, revision, build);
        }

        bool Load(int major, int minor, int revision, int build)
        {
            Clear();

            _ver[0] = major;
            _ver[1] = minor;
            _ver[2] = revision;
            _ver[3] = build;

            const std::vector<std::string> plugin_dirs = PluginDirectories();
            for (const std::string& plugin_dir : plugin_dirs) {
                if (LoadFile(plugin_dir + "\\versionlib-" + VersionSuffix() + ".bin")) {
                    return true;
                }

                if (LoadFile(plugin_dir + "\\version-" + VersionSuffix() + ".bin")) {
                    return true;
                }

                if (LoadFirstMatch(plugin_dir + "\\versionlib-" + VersionSuffix() + "*.bin")) {
                    return true;
                }

                if (LoadFirstMatch(plugin_dir + "\\version-" + VersionSuffix() + "*.bin")) {
                    return true;
                }
            }

            return false;
        }

        bool FindOffsetById(uint64_t id, uint64_t& result) const
        {
            auto itr = _data.find(id);
            if (itr == _data.end()) {
                return false;
            }

            result = itr->second;
            return true;
        }

    private:
        template <typename T>
        static T Read(std::ifstream& file)
        {
            T value{};
            file.read(reinterpret_cast<char*>(&value), sizeof(T));
            return value;
        }

        static bool ParseVersionFromString(const char* ptr,
                                           int& major,
                                           int& minor,
                                           int& revision,
                                           int& build)
        {
            return sscanf_s(ptr, "%d.%d.%d.%d", &major, &minor, &revision, &build) == 4 &&
                   ((major != 1 && major != 0) || minor != 0 || revision != 0 || build != 0);
        }

        static std::string ModuleDirectory()
        {
            char module_path[MAX_PATH]{};
            GetModuleFileNameA(nullptr, module_path, MAX_PATH);

            std::string path = module_path;
            const std::string::size_type slash = path.find_last_of("\\/");
            return slash == std::string::npos ? std::string(".") : path.substr(0, slash);
        }

        static std::vector<std::string> PluginDirectories()
        {
            std::vector<std::string> dirs;
            const std::string module_dir = ModuleDirectory();

            dirs.push_back(module_dir + "\\Data\\SKSE\\Plugins");
            dirs.push_back(module_dir + "\\SKSE\\Plugins");
            dirs.push_back(module_dir + "\\All in one (all game versions)-32444-11-1770897704\\SKSE\\Plugins");
            dirs.push_back("Data\\SKSE\\Plugins");
            dirs.push_back("SKSE\\Plugins");
            dirs.push_back("All in one (all game versions)-32444-11-1770897704\\SKSE\\Plugins");

            return dirs;
        }

        bool LoadFirstMatch(const std::string& pattern)
        {
            WIN32_FIND_DATAA find_data{};
            HANDLE handle = FindFirstFileA(pattern.c_str(), &find_data);
            if (handle == INVALID_HANDLE_VALUE) {
                return false;
            }

            const std::string::size_type slash = pattern.find_last_of("\\/");
            const std::string directory = slash == std::string::npos ? std::string(".") : pattern.substr(0, slash);

            bool loaded = false;
            do {
                if ((find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
                    loaded = LoadFile(directory + "\\" + find_data.cFileName);
                    if (loaded) {
                        break;
                    }
                }
            } while (FindNextFileA(handle, &find_data));

            FindClose(handle);
            return loaded;
        }

        bool LoadFile(const std::string& path)
        {
            ResetData();

            std::ifstream file(path, std::ios::binary);
            if (!file.good()) {
                return false;
            }

            const int format = Read<int>(file);
            if (format != 1 && format != 2) {
                ResetData();
                return false;
            }

            for (int i = 0; i < 4; i += 1) {
                _ver[i] = Read<int>(file);
            }

            const int module_name_length = Read<int>(file);
            if (module_name_length < 0 || module_name_length >= 0x10000) {
                ResetData();
                return false;
            }

            _moduleName.assign(module_name_length, '\0');
            if (module_name_length > 0) {
                file.read(&_moduleName[0], module_name_length);
            }

            const int ptr_size = Read<int>(file);
            const int addr_count = Read<int>(file);

            uint64_t pvid = 0;
            uint64_t poffset = 0;
            for (int i = 0; i < addr_count; i += 1) {
                const unsigned char type = Read<unsigned char>(file);
                const unsigned char low = type & 0xF;
                const unsigned char high = type >> 4;

                uint64_t id = 0;
                switch (low) {
                case 0: id = Read<uint64_t>(file); break;
                case 1: id = pvid + 1; break;
                case 2: id = pvid + Read<unsigned char>(file); break;
                case 3: id = pvid - Read<unsigned char>(file); break;
                case 4: id = pvid + Read<unsigned short>(file); break;
                case 5: id = pvid - Read<unsigned short>(file); break;
                case 6: id = Read<unsigned short>(file); break;
                case 7: id = Read<unsigned int>(file); break;
                default:
                    ResetData();
                    return false;
                }

                uint64_t scaled_offset = ((high & 8) != 0) ? (poffset / static_cast<uint64_t>(ptr_size)) : poffset;
                uint64_t offset = 0;
                switch (high & 7) {
                case 0: offset = Read<uint64_t>(file); break;
                case 1: offset = scaled_offset + 1; break;
                case 2: offset = scaled_offset + Read<unsigned char>(file); break;
                case 3: offset = scaled_offset - Read<unsigned char>(file); break;
                case 4: offset = scaled_offset + Read<unsigned short>(file); break;
                case 5: offset = scaled_offset - Read<unsigned short>(file); break;
                case 6: offset = Read<unsigned short>(file); break;
                case 7: offset = Read<unsigned int>(file); break;
                default:
                    ResetData();
                    return false;
                }

                if ((high & 8) != 0) {
                    offset *= static_cast<uint64_t>(ptr_size);
                }

                _data[id] = offset;
                pvid = id;
                poffset = offset;
            }

            return true;
        }

        std::string VersionSuffix() const
        {
            char version_name[64]{};
            _snprintf_s(version_name,
                        sizeof(version_name),
                        _TRUNCATE,
                        "%d-%d-%d-%d",
                        _ver[0],
                        _ver[1],
                        _ver[2],
                        _ver[3]);
            return version_name;
        }

        void Clear()
        {
            ResetData();
            for (int& part : _ver) {
                part = 0;
            }
        }

        void ResetData()
        {
            _data.clear();
            _moduleName.clear();
        }

        std::map<uint64_t, uint64_t> _data;
        int _ver[4]{};
        std::string _moduleName;
    };

    VersionDbCompat g_version_db;
    bool g_has_database = false;
    bool g_offsets_initialized = false;
    int g_runtime_version[4]{};

    bool IsRuntimeVersion(int major, int minor, int revision, int build)
    {
        return g_runtime_version[0] == major &&
               g_runtime_version[1] == minor &&
               g_runtime_version[2] == revision &&
               g_runtime_version[3] == build;
    }

}

uintptr_t RelocationManager::s_baseAddr = 0;

bool RelocationManager::Initialize()
{
    if (s_baseAddr != 0) {
        return true;
    }

    HMODULE module = GetModuleHandleA("SkyrimSE.exe");
    if (!module) {
        module = GetModuleHandle(nullptr);
    }

    s_baseAddr = reinterpret_cast<uintptr_t>(module);
    if (s_baseAddr == 0) {
        return false;
    }

    g_version_db.GetExecutableVersion(g_runtime_version[0],
                                      g_runtime_version[1],
                                      g_runtime_version[2],
                                      g_runtime_version[3]);
    g_has_database = g_version_db.Load();
    return true;
}

bool RelocationManager::HasDatabase()
{
    return g_has_database;
}

uintptr_t RelocationManager::Resolve(uint64_t id, uintptr_t fallbackOffset)
{
    uint64_t resolved = 0;
    if (g_has_database && g_version_db.FindOffsetById(id, resolved)) {
        return static_cast<uintptr_t>(resolved);
    }

    return fallbackOffset;
}

namespace doticu_npcp { namespace Offsets {

    #define DOTICU_NPCP_DEFINE_OFFSET(NAMESPACE_, NAME_, FALLBACK_, ID_) \
        namespace NAMESPACE_ { uintptr_t NAME_ = FALLBACK_; }
    DOTICU_NPCP_OFFSETS(DOTICU_NPCP_DEFINE_OFFSET)
    #undef DOTICU_NPCP_DEFINE_OFFSET

    bool Initialize()
    {
        if (g_offsets_initialized) {
            return true;
        }

        if (!RelocationManager::Initialize()) {
            return false;
        }

        if (!RelocationManager::HasDatabase() && !IsRuntimeVersion(1, 5, 97, 0)) {
            return false;
        }

        #define DOTICU_NPCP_RESOLVE_OFFSET(NAMESPACE_, NAME_, FALLBACK_, ID_) \
            NAMESPACE_::NAME_ = RelocationManager::Resolve(ID_, FALLBACK_);
        DOTICU_NPCP_OFFSETS(DOTICU_NPCP_RESOLVE_OFFSET)
        #undef DOTICU_NPCP_RESOLVE_OFFSET

        g_offsets_initialized = true;
        return true;
    }

    bool Is_Initialized()
    {
        return g_offsets_initialized;
    }

}}
