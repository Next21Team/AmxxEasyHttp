#include "RehldsPausedFrame.h"

#include <archtypes.h>
#include <maintypes.h>
#include <custom.h>
#include <rehlds_api.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace
{
    // IsActive() and IsPaused() were introduced in API 3.10.
    constexpr int kMinPausedFrameMinorVersion = 10;
    IRehldsHookRegistry_SV_Frame *frame_registry = nullptr;
    IRehldsServerData *server_data = nullptr;
    void (*frame_end_callback)(bool) = nullptr;

    IRehldsApi *FindRehldsApi()
    {
#ifdef _WIN32
        HMODULE module = GetModuleHandleA("swds.dll");
        if (!module)
            return nullptr;

        auto factory = reinterpret_cast<CreateInterfaceFn>(GetProcAddress(module, CREATEINTERFACE_PROCNAME));
        return factory ? reinterpret_cast<IRehldsApi *>(factory(VREHLDS_HLDS_API_VERSION, nullptr)) : nullptr;
#else
        void *module = dlopen("engine_i486.so", RTLD_NOW | RTLD_NOLOAD);
        if (!module)
            return nullptr;

        auto factory = reinterpret_cast<CreateInterfaceFn>(dlsym(module, CREATEINTERFACE_PROCNAME));
        auto *api = factory ? factory(VREHLDS_HLDS_API_VERSION, nullptr) : nullptr;
        dlclose(module);
        return reinterpret_cast<IRehldsApi *>(api);
#endif
    }

    void EXT_FUNC OnServerFrame(IRehldsHook_SV_Frame *chain)
    {
        chain->callNext();
        if (frame_end_callback)
            frame_end_callback(ezhttp::IsServerPaused());
    }
}

bool ezhttp::SupportsDispatchWhilePaused()
{
    return frame_registry != nullptr;
}

bool ezhttp::IsServerPaused()
{
    return server_data && server_data->IsActive() && server_data->IsPaused();
}

const char *ezhttp::RegisterPausedFrameHook(void (*on_frame_end)(bool))
{
    IRehldsApi *api = FindRehldsApi();
    if (!api)
        return "unavailable (ReHLDS API not found)";
    if (api->GetMajorVersion() != REHLDS_API_VERSION_MAJOR || api->GetMinorVersion() < kMinPausedFrameMinorVersion)
        return "unavailable (requires ReHLDS API 3.x, minimum 3.10)";

    server_data = api->GetServerData();
    frame_registry = api->GetHookchains()->SV_Frame();
    frame_end_callback = on_frame_end;
    frame_registry->registerHook(OnServerFrame);
    return "available (ReHLDS SV_Frame hook registered)";
}

void ezhttp::UnregisterPausedFrameHook()
{
    if (frame_registry)
        frame_registry->unregisterHook(OnServerFrame);

    frame_registry = nullptr;
    server_data = nullptr;
    frame_end_callback = nullptr;
}
