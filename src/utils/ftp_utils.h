#pragma once
#include <string>

namespace utils
{
    std::string ConstructFtpUrl(const std::string& user, const std::string& password, const std::string& host, const std::string& remote_file);
}