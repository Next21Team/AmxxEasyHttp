#pragma once
#include <cpr/cpr.h>

namespace ezhttp
{
    struct Response
    {
        long status_code{};
        std::string text{};
        cpr::Header header{};
        cpr::Url url{};
        double elapsed{};
        cpr::Cookies cookies{};
        cpr::Error error{};
        std::string raw_header{};
        std::string status_line{};
        std::string reason{};
        cpr::cpr_off_t uploaded_bytes{};
        cpr::cpr_off_t downloaded_bytes{};
        long redirect_count{};

        explicit Response() = default;

        explicit Response(const cpr::Response& cpr_response)
        {
            status_code = cpr_response.status_code;
            text = cpr_response.text;
            header = cpr_response.header;
            url = cpr_response.url;
            elapsed = cpr_response.elapsed;
            cookies = cpr_response.cookies;
            error = cpr_response.error;
            raw_header = cpr_response.raw_header;
            status_line = cpr_response.status_line;
            reason = cpr_response.reason;
            uploaded_bytes = cpr_response.uploaded_bytes;
            downloaded_bytes = cpr_response.downloaded_bytes;
            redirect_count = cpr_response.redirect_count;
        }
    };
}
