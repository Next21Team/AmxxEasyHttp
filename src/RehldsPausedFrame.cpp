#include "RehldsPausedFrame.h"

#include <archtypes.h>
#include <maintypes.h>
#include <custom.h>
#include <rehlds_api.h>

namespace
{
    IRehldsHookRegistry_SV_Frame *frame_registry = nullptr;
    IRehldsServerData *server_data = nullptr;
    void (*paused_frame_callback)() = nullptr;
    unsigned int dispatch_count = 0;

    IRehldsApi *FindRehldsApi()
    {
#ifdef _WIN32
        for (const char *name : {"swds.dll", "sw.dll", "hw.dll"})
        {
            HMODULE module = GetModuleHandleA(name);
            if (!module)
                continue;

            auto factory = reinterpret_cast<CreateInterfaceFn>(GetProcAddress(module, CREATEINTERFACE_PROCNAME));
            if (factory)
            {
                if (auto *api = factory(VREHLDS_HLDS_API_VERSION, nullptr))
                    return reinterpret_cast<IRehldsApi *>(api);
            }
        }
        return nullptr;
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
        const auto dispatches_before_frame = dispatch_count;
        chain->callNext();
        // A StartFrame callback may have paused the game after dispatch already began.
        if (dispatch_count == dispatches_before_frame && server_data && server_data->IsActive() && server_data->IsPaused())
            paused_frame_callback();
    }
}

void ezhttp::MarkFrameDispatched()
{
    ++dispatch_count;
}

bool ezhttp::RegisterPausedFrameHook(void (*on_paused_frame)())
{
    if (frame_registry)
        return true;

    IRehldsApi *api = FindRehldsApi();
    // IsActive() and IsPaused() were added in API 3.10.
    if (!api || api->GetMajorVersion() != 3 || api->GetMinorVersion() < 10)
        return false;

    server_data = api->GetServerData();
    frame_registry = api->GetHookchains()->SV_Frame();
    paused_frame_callback = on_paused_frame;
    frame_registry->registerHook(OnServerFrame);
    return true;
}

void ezhttp::UnregisterPausedFrameHook()
{
    if (frame_registry)
        frame_registry->unregisterHook(OnServerFrame);

    frame_registry = nullptr;
    server_data = nullptr;
    paused_frame_callback = nullptr;
}
