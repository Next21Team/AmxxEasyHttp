#pragma once

// The callback runs on the engine thread while an active game is paused.
namespace ezhttp
{
    bool RegisterPausedFrameHook(void (*on_paused_frame)());
    void UnregisterPausedFrameHook();
}
