#include "easy_http/EasyHttpOptionsBuilder.h"
#include "easy_http/EasyHttp.h"
#include <unordered_map>
#include <algorithm>
#include <utility>

#include <amxxmodule.h>
#include <undef_metamod.h>

using namespace ezhttp;

struct RequestData
{
    std::shared_ptr<RequestControl> request_control;
    int callback_id;
    std::optional<std::vector<cell>> user_data;

    cpr::Response response;

    RequestData() = delete;
    RequestData(std::shared_ptr<RequestControl> request_control, int callback_id, std::optional<std::vector<cell>> user_data) :
        request_control(std::move(request_control)),
        callback_id(callback_id),
        user_data(std::move(user_data))
    { }
};

using OptionsId = int32_t;
using RequestId = int32_t;

bool ValidateOptionsId(AMX* amx, OptionsId options_id);
bool ValidateRequestId(AMX* amx, RequestId request_id);
template <class TMethod> void SetKeyValueOption(AMX* amx, cell* params, TMethod method);
template <class TMethod> void SetStringOption(AMX* amx, cell* params, TMethod method);
std::string ConstructFtpUrl(const std::string& user, const std::string& password, const std::string& host, const std::string& remote_file);
RequestId SendRequest(AMX* amx, RequestMethod method, const RequestOptions& options, const std::string& url, const std::string& callback);
void InvokeResponseCallback(AMX* amx, RequestId request_id, const cpr::Response &response);


OptionsId g_CurrentOptions = 0;
RequestId g_CurrentRequest = 0;
std::unique_ptr<std::unordered_map<OptionsId, EasyHttpOptionsBuilder>> g_Options;
std::unique_ptr<std::unordered_map<RequestId, RequestData>> g_Requests;
std::unique_ptr<EasyHttpInterface> g_EasyHttp;

cell AMX_NATIVE_CALL ezhttp_create_options(AMX* amx, cell* params)
{
    OptionsId options_id = ++g_CurrentOptions;
    if (options_id == std::numeric_limits<int32_t>::max())
    {
        MF_LogError(amx, AMX_ERR_NATIVE, "Max options per map reached");
        return 0;
    }

    g_Options->emplace(options_id, EasyHttpOptionsBuilder{});

    return options_id;
}

cell AMX_NATIVE_CALL ezhttp_destroy_options(AMX* amx, cell* params)
{
    OptionsId options_id = params[1];

    if (!ValidateOptionsId(amx, options_id))
        return 0;

    g_Options->erase(options_id);

    return options_id;
}

cell AMX_NATIVE_CALL ezhttp_option_set_user_agent(AMX* amx, cell* params)
{
    SetStringOption(amx, params, &EasyHttpOptionsBuilder::SetUserAgent);
    return 0;
}

cell AMX_NATIVE_CALL ezhttp_option_add_url_parameter(AMX* amx, cell* params)
{
    SetKeyValueOption(amx, params, &EasyHttpOptionsBuilder::AddUrlParameter);
    return 0;
}

cell AMX_NATIVE_CALL ezhttp_option_add_form_payload(AMX* amx, cell* params)
{
    SetKeyValueOption(amx, params, &EasyHttpOptionsBuilder::AddFormPayload);
    return 0;
}

cell AMX_NATIVE_CALL ezhttp_option_set_body(AMX* amx, cell* params)
{
    SetStringOption(amx, params, &EasyHttpOptionsBuilder::SetBody);
    return 0;
}

cell AMX_NATIVE_CALL ezhttp_option_set_header(AMX* amx, cell* params)
{
    SetKeyValueOption(amx, params, &EasyHttpOptionsBuilder::SetHeader);
    return 0;
}

cell AMX_NATIVE_CALL ezhttp_option_set_cookie(AMX* amx, cell* params)
{
    SetKeyValueOption(amx, params, &EasyHttpOptionsBuilder::SetCookie);
    return 0;
}

cell AMX_NATIVE_CALL ezhttp_option_set_timeout(AMX* amx, cell* params)
{
    OptionsId options_id = params[1];
    cell timeout_ms = params[2];

    if (!ValidateOptionsId(amx, options_id))
        return 0;

    g_Options->at(options_id).SetTimeout(timeout_ms);
    return 0;
}

cell AMX_NATIVE_CALL ezhttp_option_set_connect_timeout(AMX* amx, cell* params)
{
    OptionsId options_id = params[1];
    cell timeout_ms = params[2];

    if (!ValidateOptionsId(amx, options_id))
        return 0;

    g_Options->at(options_id).SetConnectTimeout(timeout_ms);
    return 0;
}

cell AMX_NATIVE_CALL ezhttp_option_set_proxy(AMX* amx, cell* params)
{
    SetStringOption(amx, params, &EasyHttpOptionsBuilder::SetProxy);
    return 0;
}

cell AMX_NATIVE_CALL ezhttp_option_set_proxy_auth(AMX* amx, cell* params)
{
    SetKeyValueOption(amx, params, &EasyHttpOptionsBuilder::SetProxyAuth);
    return 0;
}

cell AMX_NATIVE_CALL ezhttp_option_set_auth(AMX* amx, cell* params)
{
    SetKeyValueOption(amx, params, &EasyHttpOptionsBuilder::SetAuth);
    return 0;
}

cell AMX_NATIVE_CALL ezhttp_option_set_user_data(AMX* amx, cell* params)
{
    OptionsId options_id = params[1];
    cell* data_addr = MF_GetAmxAddr(amx, params[2]);
    int data_len = params[3];

    if (!ValidateOptionsId(amx, options_id))
        return 0;

    std::vector<cell> user_data;
    user_data.resize(data_len);
    MF_CopyAmxMemory(user_data.data(), data_addr, data_len);

    g_Options->at(options_id).SetUserData(user_data);
    return 0;
}

cell AMX_NATIVE_CALL ezhttp_get(AMX* amx, cell* params)
{
    int url_len;
    char* url = MF_GetAmxString(amx, params[1], 0, &url_len);

    int callback_len;
    char* callback = MF_GetAmxString(amx, params[2], 1, &callback_len);

    OptionsId options_id = params[3];

    RequestOptions options;
    if (options_id != 0)
    {
        options = g_Options->at(options_id).GetOptions();
        g_Options->erase(options_id);
    }

    return SendRequest(amx, RequestMethod::HttpGet,  options, std::string(url, url_len), std::string(callback, callback_len));
}

cell AMX_NATIVE_CALL ezhttp_post(AMX* amx, cell* params)
{
    int url_len;
    char* url = MF_GetAmxString(amx, params[1], 0, &url_len);

    int callback_len;
    char* callback = MF_GetAmxString(amx, params[2], 1, &callback_len);

    OptionsId options_id = params[3];

    RequestOptions options;
    if (options_id != 0)
    {
        options = g_Options->at(options_id).GetOptions();
        g_Options->erase(options_id);
    }

    return SendRequest(amx, RequestMethod::HttpPost,  options, std::string(url, url_len), std::string(callback, callback_len));
}

cell AMX_NATIVE_CALL ezhttp_is_request_exists(AMX* amx, cell* params)
{
    RequestId request_id = params[1];

    return g_Requests->count(request_id) == 1;
}

cell AMX_NATIVE_CALL ezhttp_cancel_request(AMX* amx, cell* params)
{
    RequestId request_id = params[1];

    if (!ValidateRequestId(amx, request_id))
        return 0;

    RequestData& request_data = g_Requests->at(request_id);
    request_data.request_control->canceled.store(true);

    cpr::Response response;
    response.error.code = cpr::ErrorCode::REQUEST_CANCELLED;

    InvokeResponseCallback(amx, request_id, response);
    return 0;
}

cell AMX_NATIVE_CALL ezhttp_request_progress(AMX* amx, cell* params)
{
    RequestId request_id = params[1];

    if (!ValidateRequestId(amx, request_id))
        return 0;

    auto progress = g_Requests->at(request_id).request_control->progress.load();

    cell* p = MF_GetAmxAddr(amx, params[2]);
    p[0] = progress.download_now;
    p[1] = progress.download_total;
    p[2] = progress.upload_now;
    p[3] = progress.upload_total;

    return 0;
}

cell AMX_NATIVE_CALL ezhttp_get_http_code(AMX* amx, cell* params)
{
    RequestId request_id = params[1];

    if (!ValidateRequestId(amx, request_id))
        return 0;

    const cpr::Response& response = g_Requests->at(request_id).response;

    return response.status_code;
}

cell AMX_NATIVE_CALL ezhttp_get_data(AMX* amx, cell* params)
{
    RequestId request_id = params[1];
    cell max_len = params[3];

    if (!ValidateRequestId(amx, request_id) || max_len == 0)
        return 0;

    const cpr::Response& response = g_Requests->at(request_id).response;

    MF_SetAmxString(amx, params[2], response.text.c_str(), max_len);

    return 0;
}

cell AMX_NATIVE_CALL ezhttp_get_url(AMX* amx, cell* params)
{
    RequestId request_id = params[1];
    cell max_len = params[3];

    if (!ValidateRequestId(amx, request_id) || max_len == 0)
        return 0;

    const cpr::Response& response = g_Requests->at(request_id).response;

    MF_SetAmxString(amx, params[2], response.url.c_str(), max_len);

    return 0;
}

cell AMX_NATIVE_CALL ezhttp_save_data_to_file(AMX* amx, cell* params)
{
    RequestId request_id = params[1];

    int file_path_len;
    char* file_path = MF_GetAmxString(amx, params[2], 0, &file_path_len);
    cell max_len = params[3];

    if (!ValidateRequestId(amx, request_id))
        return 0;

    const cpr::Response& response = g_Requests->at(request_id).response;

    if (response.text.empty())
        return 0;

    std::ofstream file(MF_BuildPathname("%s", file_path), std::ofstream::out | std::ofstream::binary);
    if (!file.is_open())
        return 0;

    file.write(response.text.data(), response.text.length());
    file.close();

    return response.text.length();
}

cell AMX_NATIVE_CALL ezhttp_save_data_to_file2(AMX* amx, cell* params)
{
    RequestId request_id = params[1];
    FILE* file_handle = (FILE*)params[2];

    if (!ValidateRequestId(amx, request_id))
        return 0;

    const cpr::Response& response = g_Requests->at(request_id).response;

    return std::fwrite(response.text.data(), sizeof(char), response.text.length(), file_handle);
}

cell AMX_NATIVE_CALL ezhttp_get_headers_count(AMX* amx, cell* params)
{
    RequestId request_id = params[1];

    if (!ValidateRequestId(amx, request_id))
        return 0;

    const cpr::Response& response = g_Requests->at(request_id).response;

    return response.header.size();
}

cell AMX_NATIVE_CALL ezhttp_get_headers(AMX* amx, cell* params)
{
    RequestId request_id = params[1];
    int key_len;
    char* key = MF_GetAmxString(amx, params[2], 0, &key_len);
    cell value_max_len = params[4];

    if (!ValidateRequestId(amx, request_id))
        return 0;

    const cpr::Response& response = g_Requests->at(request_id).response;

    const std::string header_key(key, key_len);
    if (response.header.count(header_key) == 1)
    {
        const std::string& header_value = response.header.at(header_key);

        MF_SetAmxString(amx, params[3], header_value.c_str(), value_max_len);

        return 1;
    }

    return 0;
}

cell AMX_NATIVE_CALL ezhttp_iterate_headers(AMX* amx, cell* params)
{
    RequestId request_id = params[1];
    cell iter = params[2];
    int key_len;
    char* key = MF_GetAmxString(amx, params[3], 0, &key_len);
    cell value_max_len = params[5];

    if (!ValidateRequestId(amx, request_id))
        return 0;

    const RequestData& request = g_Requests->at(request_id);
    const cpr::Header& header = request.response.header;


    return 0;
}

cell AMX_NATIVE_CALL ezhttp_get_elapsed(AMX* amx, cell* params)
{
    RequestId request_id = params[1];

    if (!ValidateRequestId(amx, request_id))
        return 0;

    const cpr::Response& response = g_Requests->at(request_id).response;

    return amx_ftoc(response.elapsed);
}

cell AMX_NATIVE_CALL ezhttp_get_cookies_count(AMX* amx, cell* params)
{
    RequestId request_id = params[1];

    if (!ValidateRequestId(amx, request_id))
        return 0;

    const cpr::Response& response = g_Requests->at(request_id).response;

    return response.cookies.size();
}

cell AMX_NATIVE_CALL ezhttp_get_cookies(AMX* amx, cell* params)
{
    RequestId request_id = params[1];
    int key_len;
    char* key = MF_GetAmxString(amx, params[2], 0, &key_len);
    cell value_max_len = params[4];

    if (!ValidateRequestId(amx, request_id))
        return 0;

    cpr::Response& response = g_Requests->at(request_id).response;

    const std::string cookie_key(key, key_len);
    if (response.cookies.contains(cookie_key))
    {
        const std::string& cookie_value = response.cookies[cookie_key];

        MF_SetAmxString(amx, params[3], cookie_value.c_str(), value_max_len);

        return 1;
    }

    return 0;
}

cell AMX_NATIVE_CALL ezhttp_iterate_cookies(AMX* amx, cell* params)
{
    return 0;
}

cell AMX_NATIVE_CALL ezhttp_get_error_code(AMX* amx, cell* params)
{
    RequestId request_id = params[1];

    if (!ValidateRequestId(amx, request_id))
        return 0;

    const cpr::Response& response = g_Requests->at(request_id).response;

    return (cell)response.error.code;
}

cell AMX_NATIVE_CALL ezhttp_get_error_message(AMX* amx, cell* params)
{
    RequestId request_id = params[1];
    cell max_len = params[3];

    if (!ValidateRequestId(amx, request_id) || max_len == 0)
        return 0;

    const cpr::Response& response = g_Requests->at(request_id).response;

    MF_SetAmxString(amx, params[2], response.error.message.c_str(), max_len);

    return 0;
}

cell AMX_NATIVE_CALL ezhttp_get_redirect_count(AMX* amx, cell* params)
{
    RequestId request_id = params[1];

    if (!ValidateRequestId(amx, request_id))
        return 0;

    const cpr::Response& response = g_Requests->at(request_id).response;

    return response.redirect_count;
}

cell AMX_NATIVE_CALL ezhttp_get_uploaded_bytes(AMX* amx, cell* params)
{
    RequestId request_id = params[1];

    if (!ValidateRequestId(amx, request_id))
        return 0;

    const cpr::Response& response = g_Requests->at(request_id).response;

    return response.uploaded_bytes;
}

cell AMX_NATIVE_CALL ezhttp_get_downloaded_bytes(AMX* amx, cell* params)
{
    RequestId request_id = params[1];

    if (!ValidateRequestId(amx, request_id))
        return 0;

    const cpr::Response& response = g_Requests->at(request_id).response;

    return response.downloaded_bytes;
}

cell AMX_NATIVE_CALL ezhttp_get_user_data(AMX* amx, cell* params)
{
    RequestId request_id = params[1];
    cell* data_addr = MF_GetAmxAddr(amx, params[2]);

    if (!ValidateRequestId(amx, request_id))
        return 0;

    const std::optional<std::vector<cell>>& user_data = g_Requests->at(request_id).user_data;
    if (!user_data)
        return 0;

    MF_CopyAmxMemory(data_addr, user_data.value().data(), user_data.value().size());

    return 0;
}

cell AMX_NATIVE_CALL ezhttp_ftp_upload(AMX* amx, cell* params)
{
    int len;
    std::string user(MF_GetAmxString(amx, params[1], 0, &len));
    std::string password (MF_GetAmxString(amx, params[2], 0, &len));
    std::string host = MF_GetAmxString(amx, params[3], 0, &len);
    std::string remote_file = MF_GetAmxString(amx, params[4], 0, &len);
    int local_file_len;
    char* local_file = MF_GetAmxString(amx, params[5], 1, &local_file_len);
    std::string callback = MF_GetAmxString(amx, params[6], 0, &len);
    bool secure = params[7];

    std::string url = ConstructFtpUrl(user, password, host, remote_file);

    RequestOptions options;
    options.file_path.emplace(local_file, local_file_len);
    options.require_secure = secure;

    SendRequest(amx, RequestMethod::FtpUpload, options, url, callback);

    return 0;
}

cell AMX_NATIVE_CALL ezhttp_ftp_upload2(AMX* amx, cell* params)
{
    int url_str_len;
    char* url_str = MF_GetAmxString(amx, params[1], 0, &url_str_len);

    int local_file_len;
    char* local_file = MF_GetAmxString(amx, params[2], 1, &local_file_len);

    int callback_len;
    char* callback = MF_GetAmxString(amx, params[3], 2, &callback_len);

    bool secure = params[4];

    RequestOptions options;
    options.file_path.emplace(local_file, local_file_len);
    options.require_secure = secure;

    SendRequest(amx, RequestMethod::FtpUpload, options, std::string(url_str, url_str_len), std::string(callback, callback_len));

    return 0;
}

cell AMX_NATIVE_CALL ezhttp_ftp_download(AMX* amx, cell* params)
{
    int len;
    std::string user(MF_GetAmxString(amx, params[1], 0, &len));
    std::string password (MF_GetAmxString(amx, params[2], 0, &len));
    std::string host = MF_GetAmxString(amx, params[3], 0, &len);
    std::string remote_file = MF_GetAmxString(amx, params[4], 0, &len);
    int local_file_len;
    char* local_file = MF_GetAmxString(amx, params[5], 1, &local_file_len);
    std::string callback = MF_GetAmxString(amx, params[6], 0, &len);
    bool secure = params[7];

    std::string url = ConstructFtpUrl(user, password, host, remote_file);

    RequestOptions options;
    options.file_path.emplace(local_file, local_file_len);
    options.require_secure = secure;

    SendRequest(amx, RequestMethod::FtpDownload, options, url, callback);

    return 0;
}

cell AMX_NATIVE_CALL ezhttp_ftp_download2(AMX* amx, cell* params)
{
    int url_str_len;
    char* url_str = MF_GetAmxString(amx, params[1], 0, &url_str_len);

    int local_file_len;
    char* local_file = MF_GetAmxString(amx, params[2], 1, &local_file_len);

    int callback_len;
    char* callback = MF_GetAmxString(amx, params[3], 2, &callback_len);

    bool secure = params[4];

    RequestOptions options;
    options.file_path.emplace(local_file, local_file_len);
    options.require_secure = secure;

    SendRequest(amx, RequestMethod::FtpDownload, options, std::string(url_str, url_str_len), std::string(callback, callback_len));

    return 0;
}

std::string ConstructFtpUrl(const std::string& user, const std::string& password, const std::string& host, const std::string& remote_file)
{
    std::ostringstream url;
    url << "ftp://" << user << ":" << password << "@" << host;

    if (remote_file.empty() || remote_file[0] != '/')
        url << "/";

    url << remote_file;

    return url.str();
}

RequestId SendRequest(AMX* amx, RequestMethod method, const RequestOptions& options, const std::string& url, const std::string& callback)
{
    int callback_id = -1;
    if (!callback.empty())
    {
        callback_id = MF_RegisterSPForwardByName(amx, callback.c_str(), FP_CELL, FP_DONE);
        if (callback_id == -1)
        {
            MF_LogError(amx, AMX_ERR_NATIVE, "Callback function \"%s\" is not exists", callback.c_str());
            return 0;
        }
    }

    RequestId request_id = ++g_CurrentRequest;
    if (request_id == std::numeric_limits<int32_t>::max())
    {
        MF_LogError(amx, AMX_ERR_NATIVE, "Max requests per map reached");
        return 0;
    }

    auto on_complete = [amx, request_id](const cpr::Response &response) {
        InvokeResponseCallback(amx, request_id, response);
    };

    g_Requests->emplace(request_id, RequestData {g_EasyHttp->SendRequest(method, cpr::Url(url), options, on_complete), callback_id, options.user_data});

    return request_id;
}

bool ValidateOptionsId(AMX* amx, OptionsId options_id)
{
    if (g_Options->count(options_id) == 0)
    {
        MF_LogError(amx, AMX_ERR_NATIVE, "Options id %d not exists", options_id);
        return false;
    }

    return true;
}

bool ValidateRequestId(AMX* amx, RequestId request_id)
{
    if (g_Requests->count(request_id) == 0)
    {
        MF_LogError(amx, AMX_ERR_NATIVE, "Request id %d not exists", request_id);
        return false;
    }

    return true;
}

void InvokeResponseCallback(AMX* amx, RequestId request_id, const cpr::Response &response)
{
    RequestData& request = g_Requests->at(request_id);

    int callback_id = request.callback_id;
    if (callback_id == -1)
        return;

    request.response = response;

    MF_ExecuteForward(callback_id, request_id);
    MF_UnregisterSPForward(callback_id);

    g_Requests->erase(request_id);
}

template <class TMethod>
void SetKeyValueOption(AMX* amx, cell* params, TMethod method)
{
    OptionsId options_id = params[1];
    int key_len;
    char* key = MF_GetAmxString(amx, params[2], 0, &key_len);
    int value_len;
    char* value = MF_GetAmxString(amx, params[3], 1, &value_len);

    if (!ValidateOptionsId(amx, options_id))
        return;

    (g_Options->at(options_id).*method)(std::string(key, key_len), std::string(value, value_len));
}

template <class TMethod>
void SetStringOption(AMX* amx, cell* params, TMethod method)
{
    OptionsId options_id = params[1];
    int value_len;
    char* value = MF_GetAmxString(amx, params[2], 0, &value_len);

    if (!ValidateOptionsId(amx, options_id))
        return;

    (g_Options->at(options_id).*method)(std::string(value, value_len));
}

AMX_NATIVE_INFO g_Natives[] =
{
    { "ezhttp_create_options",              ezhttp_create_options },
    { "ezhttp_destroy_options",             ezhttp_destroy_options },
    { "ezhttp_option_set_user_agent",       ezhttp_option_set_user_agent },
    { "ezhttp_option_add_url_parameter",    ezhttp_option_add_url_parameter },
    { "ezhttp_option_add_form_payload",     ezhttp_option_add_form_payload },
    { "ezhttp_option_set_body",             ezhttp_option_set_body },
    { "ezhttp_option_set_header",           ezhttp_option_set_header },
    { "ezhttp_option_set_cookie",           ezhttp_option_set_cookie },
    { "ezhttp_option_set_timeout",          ezhttp_option_set_timeout },
    { "ezhttp_option_set_connect_timeout",  ezhttp_option_set_connect_timeout },
    { "ezhttp_option_set_proxy",            ezhttp_option_set_proxy },
    { "ezhttp_option_set_proxy_auth",       ezhttp_option_set_proxy_auth },
    { "ezhttp_option_set_auth",             ezhttp_option_set_auth },
    { "ezhttp_option_set_user_data",        ezhttp_option_set_user_data },

    { "ezhttp_get",                         ezhttp_get },
    { "ezhttp_post",                        ezhttp_post },
    { "ezhttp_is_request_exists",           ezhttp_is_request_exists },
    { "ezhttp_cancel_request",              ezhttp_cancel_request },
    { "ezhttp_request_progress",            ezhttp_request_progress },

    { "ezhttp_get_http_code",               ezhttp_get_http_code },
    { "ezhttp_get_data",                    ezhttp_get_data },
    { "ezhttp_get_url",                     ezhttp_get_url },
    { "ezhttp_save_data_to_file",           ezhttp_save_data_to_file },
    { "ezhttp_save_data_to_file2",          ezhttp_save_data_to_file2 },
    { "ezhttp_get_headers_count",           ezhttp_get_headers_count },
    { "ezhttp_get_headers",                 ezhttp_get_headers },
    { "ezhttp_iterate_headers",             ezhttp_iterate_headers },
    { "ezhttp_get_elapsed",                 ezhttp_get_elapsed },
    { "ezhttp_get_cookies_count",           ezhttp_get_cookies_count },
    { "ezhttp_get_cookies",                 ezhttp_get_cookies },
    { "ezhttp_iterate_cookies",             ezhttp_iterate_cookies },
    { "ezhttp_get_error_code",              ezhttp_get_error_code },
    { "ezhttp_get_error_message",           ezhttp_get_error_message },
    { "ezhttp_get_redirect_count",          ezhttp_get_redirect_count },
    { "ezhttp_get_uploaded_bytes",          ezhttp_get_uploaded_bytes },
    { "ezhttp_get_downloaded_bytes",        ezhttp_get_downloaded_bytes },
    { "ezhttp_get_user_data",               ezhttp_get_user_data },

    { "ezhttp_ftp_upload",                  ezhttp_ftp_upload },
    { "ezhttp_ftp_upload2",                 ezhttp_ftp_upload2 },
    { "ezhttp_ftp_download",                ezhttp_ftp_download },
    { "ezhttp_ftp_download2",               ezhttp_ftp_download2 },

    { nullptr,                              nullptr },
};

void ReInitialize()
{
    std::string ca_cert_path(GET_GAME_INFO(PLID, GINFO_GAMEDIR));
    ca_cert_path.append("/addons/amxmodx/data/amxx_easy_http_cacert.pem");

    if (g_Requests)
    {
        for (auto &request_kv: *g_Requests)
            request_kv.second.request_control->canceled.store(true);
    }

    g_CurrentOptions = 0;
    g_CurrentRequest = 0;
    g_Options = std::make_unique<std::unordered_map<OptionsId, EasyHttpOptionsBuilder>>();
    g_Requests = std::make_unique<std::unordered_map<RequestId, RequestData>>();
    g_EasyHttp = std::make_unique<EasyHttp>(ca_cert_path);
}

void UnInitialize()
{
    for (auto& request_kv : *g_Requests)
        request_kv.second.request_control->canceled.store(true);

    g_Options.reset();
    g_Requests.reset();
    g_EasyHttp.reset();
}

void OnAmxxAttach()
{
    MF_AddNatives(g_Natives);

    ReInitialize();
}

void OnAmxxDetach()
{
    UnInitialize();
}

void StartFrame()
{
    if (g_EasyHttp)
        g_EasyHttp->RunFrame();

    SET_META_RESULT(MRES_IGNORED);
}

void ServerActivate(edict_t *pEdictList, int edictCount, int clientMax)
{
    ReInitialize();

    SET_META_RESULT(MRES_IGNORED);
}

void ServerDeactivate()
{
    UnInitialize();

    SET_META_RESULT(MRES_IGNORED);
}