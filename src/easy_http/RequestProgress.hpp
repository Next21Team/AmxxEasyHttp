#pragma once

namespace ezhttp
{
    struct RequestProgress
    {
        int32_t download_total{};
        int32_t download_now{};
        int32_t upload_total{};
        int32_t upload_now{};
    };
}
