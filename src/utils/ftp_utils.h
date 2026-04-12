#pragma once
#include <filesystem>
#include <string>

namespace utils
{
    std::string ConstructFtpUrl(const std::string& user, const std::string& password, const std::string& host, const std::string& remote_file);
    std::string NormalizeFtpUrl(const std::string& url);
    bool HasWildcardPath(const std::string& url);
    std::string GetRemoteFileName(const std::string& url);
    std::filesystem::path BuildDownloadPath(const std::string& local_target, const std::string& remote_file_name, bool force_directory = false);
}
