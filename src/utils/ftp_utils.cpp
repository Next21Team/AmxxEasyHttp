#include "ftp_utils.h"

#include <cctype>
#include <string>
#include <sstream>

#include <curl/urlapi.h>

namespace utils
{
    namespace
    {
        bool StartsWithCaseInsensitive(const std::string& value, const std::string& prefix)
        {
            if (value.size() < prefix.size())
                return false;

            for (size_t i = 0; i < prefix.size(); ++i)
            {
                if (std::tolower(static_cast<unsigned char>(value[i])) != std::tolower(static_cast<unsigned char>(prefix[i])))
                    return false;
            }

            return true;
        }

        std::string DecodeUserInfo(const std::string& value)
        {
            std::string decoded;
            decoded.reserve(value.size());

            for (size_t i = 0; i < value.size(); ++i)
            {
                if (value[i] == '%' &&
                    i + 2 < value.size() &&
                    std::isxdigit(static_cast<unsigned char>(value[i + 1])) &&
                    std::isxdigit(static_cast<unsigned char>(value[i + 2])))
                {
                    decoded.push_back(static_cast<char>(std::stoi(value.substr(i + 1, 2), nullptr, 16)));
                    i += 2;
                    continue;
                }

                decoded.push_back(value[i]);
            }

            return decoded;
        }

        std::string EncodeUserInfo(const std::string& value)
        {
            std::ostringstream encoded;

            for (unsigned char ch : value)
            {
                if (std::isalnum(ch) || ch == '-' || ch == '.' || ch == '_' || ch == '~')
                {
                    encoded << static_cast<char>(ch);
                }
                else
                {
                    encoded << '%' << std::uppercase << std::hex;
                    encoded.width(2);
                    encoded.fill('0');
                    encoded << static_cast<int>(ch);
                    encoded << std::nouppercase << std::dec;
                }
            }

            return encoded.str();
        }

        std::string GetUrlPath(const std::string& url)
        {
            CURLU* curl_url_handle = curl_url();
            if (curl_url_handle == nullptr)
                return {};

            std::string path;

            if (curl_url_set(curl_url_handle, CURLUPART_URL, url.c_str(), 0) == CURLUE_OK)
            {
                char* raw_path = nullptr;
                if (curl_url_get(curl_url_handle, CURLUPART_PATH, &raw_path, 0) == CURLUE_OK && raw_path != nullptr)
                {
                    path = raw_path;
                    curl_free(raw_path);
                }
            }

            curl_url_cleanup(curl_url_handle);
            return path;
        }

        bool IsDirectoryTarget(const std::string& local_target)
        {
            if (local_target.empty())
                return false;

            const char last_char = local_target.back();
            if (last_char == '/' || last_char == '\\')
                return true;

            std::error_code ec;
            const std::filesystem::path target_path(local_target);
            return std::filesystem::exists(target_path, ec) && std::filesystem::is_directory(target_path, ec);
        }
    }

    std::string ConstructFtpUrl(const std::string& user, const std::string& password, const std::string& host, const std::string& remote_file)
    {
        std::ostringstream url;
        url << "ftp://";

        if (!user.empty() || !password.empty())
            url << EncodeUserInfo(user) << ":" << EncodeUserInfo(password) << "@";

        if (remote_file.empty() || remote_file[0] != '/')
            url << host << "/";
        else
            url << host;

        url << remote_file;

        return url.str();
    }

    std::string NormalizeFtpUrl(const std::string& url)
    {
        if (!StartsWithCaseInsensitive(url, "ftp://") && !StartsWithCaseInsensitive(url, "ftps://"))
            return url;

        const size_t scheme_sep_pos = url.find("://");
        if (scheme_sep_pos == std::string::npos)
            return url;

        const size_t authority_begin_pos = scheme_sep_pos + 3;
        const size_t path_begin_pos = url.find_first_of("/?#", authority_begin_pos);
        const std::string authority = url.substr(authority_begin_pos, path_begin_pos - authority_begin_pos);
        const size_t credentials_end_pos = authority.rfind('@');

        if (credentials_end_pos == std::string::npos)
            return url;

        const std::string user_info = authority.substr(0, credentials_end_pos);
        const std::string host_info = authority.substr(credentials_end_pos + 1);
        if (host_info.empty())
            return url;

        const size_t password_sep_pos = user_info.find(':');
        const std::string user = DecodeUserInfo(password_sep_pos == std::string::npos ? user_info : user_info.substr(0, password_sep_pos));
        const std::string password = password_sep_pos == std::string::npos ? std::string{} : DecodeUserInfo(user_info.substr(password_sep_pos + 1));

        std::ostringstream normalized_url;
        normalized_url << url.substr(0, authority_begin_pos);
        normalized_url << EncodeUserInfo(user);
        if (password_sep_pos != std::string::npos)
            normalized_url << ":" << EncodeUserInfo(password);
        normalized_url << "@" << host_info;

        if (path_begin_pos != std::string::npos)
            normalized_url << url.substr(path_begin_pos);

        return normalized_url.str();
    }

    bool HasWildcardPath(const std::string& url)
    {
        return GetUrlPath(url).find_first_of("*?[") != std::string::npos;
    }

    std::string GetRemoteFileName(const std::string& url)
    {
        return std::filesystem::path(GetUrlPath(url)).filename().string();
    }

    std::filesystem::path BuildDownloadPath(const std::string& local_target, const std::string& remote_file_name, bool force_directory)
    {
        std::filesystem::path target_path(local_target);

        if (force_directory || IsDirectoryTarget(local_target))
            return remote_file_name.empty() ? target_path : target_path / remote_file_name;

        return target_path;
    }
}
