#pragma once

namespace ezhttp
{
    struct RequestControl
    {
        std::mutex control_mutex;

        struct Progress
        {
            int32_t download_total;
            int32_t download_now;
            int32_t upload_total;
            int32_t upload_now;
        };

        bool completed{};
        bool forgotten{};
        bool canceled{};
        Progress progress{};
    };
}