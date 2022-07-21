#include "hostfxr.h"
#include "coreclr_delegates.h"

#include <filesystem>
#include <cassert>
#include <array>

#if defined(__APPLE__)
#define OSX
#elif defined(_WIN32)
#define WINDOWS
#endif

#ifdef WINDOWS
#define EXPORTS_API __declspec(dllexport)
#else
#define EXPORTS_API
#endif

#ifdef WINDOWS
#include <Windows.h>
#else
#include <dlfcn.h>
#include <sys/sysctl.h>
#endif

#ifdef WINDOWS
#define STR(s) L##s
#else
#define STR(s) s
#endif

using std::filesystem::path;

namespace
{
    void* loadLibrary(char_t const* path)
    {
#ifdef WINDOWS
        HMODULE h = ::LoadLibraryW(path);
#else
        void* h = dlopen(path, RTLD_LAZY | RTLD_LOCAL);
#endif

        assert(h != nullptr);
        return (void*) h;
    }

    void* getExport(void* h, char const* name)
    {
#ifdef WINDOWS
        void* f = ::GetProcAddress(HMODULE(h), name);
#else
        void* f = dlsym(h, name);
#endif

        assert(f != nullptr);
        return f;
    }

    path getModulePath(void const* functionPtr)
    {
#ifdef WINDOWS
        std::array<wchar_t, MAX_PATH> path {};
        HMODULE hm = nullptr;

        auto success = GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(functionPtr), &hm);
        assert(success);

        success = GetModuleFileName(hm, path.data(), DWORD(path.size()));
        assert(success);

        return path.data();
#else
        Dl_info info;
        auto res = dladdr(functionPtr, &info);
        assert(res);

        return path(info.dli_fname);
#endif
    }
}

path getRepoRoot()
{
    return getModulePath((void const*)&getRepoRoot).parent_path().parent_path().parent_path().parent_path().parent_path();
}

path getHostfxr()
{
    auto p = getRepoRoot();
#if defined(WINDOWS)
    p /= "runtimes/dotnet-runtime-6.0.6-win-x64/host/fxr/6.0.6/hostfxr.dll";
#elif defined(__arm64__)
    p /= "runtimes/dotnet-runtime-6.0.6-osx-arm64/host/fxr/6.0.6/libhostfxr.dylib";
#else
    p /= "runtimes/dotnet-runtime-6.0.6-osx-x64/host/fxr/6.0.6/libhostfxr.dylib";
#endif
    return p;
}

load_assembly_and_get_function_pointer_fn startClr(path const& hostfxrPath, path const& runtimeConfig)
{
    // Load hostfxr and get desired exports
    void *lib = loadLibrary(hostfxrPath.c_str());

    auto init = hostfxr_initialize_for_runtime_config_fn(getExport(lib, "hostfxr_initialize_for_runtime_config"));
    auto getRuntimeDelegate = hostfxr_get_runtime_delegate_fn(getExport(lib, "hostfxr_get_runtime_delegate"));
    auto close = hostfxr_close_fn(getExport(lib, "hostfxr_close"));

    assert(init && getRuntimeDelegate && close);

    // Load .NET Core
    hostfxr_handle cxt = nullptr;

    int error = init(absolute(runtimeConfig).c_str(), nullptr, &cxt);
    assert(error == 0 && cxt != nullptr);

    // Get the load assembly function pointer
    void *loadFunc = nullptr;
    error = getRuntimeDelegate(cxt, hdt_load_assembly_and_get_function_pointer, &loadFunc);
    assert(error == 0 && loadFunc != nullptr);

    close(cxt);
    return load_assembly_and_get_function_pointer_fn(loadFunc);
}

// Entry point
extern "C" EXPORTS_API void Init_NativeHost()
{
#ifdef WINDOWS
    AllocConsole();
    (void) freopen("conout$", "w", stdout);
    (void) freopen("conout$", "w", stderr);
#endif

    auto runtimeConfig = getRepoRoot() / "runtimes/plugin.runtimeconfig.json";
    auto hostfxr = getHostfxr();
    auto assemblyPath = getRepoRoot() / "dotnet/bin/Debug/net6.0/DotnetOsxDebuggingRepro.dll";
    auto loadFunc = startClr(hostfxr, runtimeConfig);

    auto assembly =
#ifdef WINDOWS
        assemblyPath.wstring();
#else
        assemblyPath.string();
#endif

    void(*entryFunc)() = nullptr;

    auto error = loadFunc(
        assembly.c_str(),
        STR("DotnetOsxDebuggingRepro.EntryPoint, DotnetOsxDebuggingRepro"),
        STR("PluginEntry"),
        UNMANAGEDCALLERSONLY_METHOD,
        nullptr,
        (void**)&entryFunc);

    assert(error == 0);

    entryFunc();
}
