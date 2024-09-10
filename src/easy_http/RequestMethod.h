#pragma once

namespace ezhttp
{
    enum class RequestMethod
    {
        HttpGet,
        HttpPost,
        HttpPut,
        HttpPatch,
        HttpDelete,
        FtpUpload,
        FtpDownload,
    };
}
