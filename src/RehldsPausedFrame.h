#pragma once

namespace ezhttp
{
    // Returns a diagnostic for the AMXX log. All queries run on the engine thread.
    const char *RegisterPausedFrameHook(void (*on_frame_end)(bool active_and_paused));
    void UnregisterPausedFrameHook();
    bool SupportsDispatchWhilePaused();
    bool IsServerPaused();
}
