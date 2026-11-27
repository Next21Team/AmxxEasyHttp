#pragma once

#include <utility>

namespace ezhttp
{
    enum class FrameSource { StartFrame, EngineFrameEnd, PausedEngineFrameEnd };

    class FramePump
    {
        bool saw_start_frame_ = false;

    public:
        bool ShouldDispatch(FrameSource source)
        {
            if (source == FrameSource::StartFrame)
            {
                saw_start_frame_ = true;
                return true;
            }

            // A callback may have paused the server inside this frame's StartFrame.
            const bool already_dispatched = std::exchange(saw_start_frame_, false);
            return source == FrameSource::PausedEngineFrameEnd && !already_dispatched;
        }
    };
}
