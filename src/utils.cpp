#include <string>
#include <sstream>

std::string ConstructFtpUrl(const std::string& user, const std::string& password, const std::string& host, const std::string& remote_file)
{
    std::ostringstream url;
    url << "ftp://" << user << ":" << password << "@" << host;

    if (remote_file.empty() || remote_file[0] != '/')
        url << "/";

    url << remote_file;

    return url.str();
}
