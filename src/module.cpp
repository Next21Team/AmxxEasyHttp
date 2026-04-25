#include <algorithm>
#include <utility>
#include <fstream>

#include <sdk/amxxmodule.h>

#include "EasyHttpModule.h"
#include "json/JsonMngr.h"
#include "json/JsonNatives.h"
#include "utils/ftp_utils.h"
#include "utils/string_utils.h"
#include "utils/amxx_utils.h"
#include "utils/TraceLog.h"

using namespace ezhttp;

bool ValidateOptionsId(AMX *amx, OptionsId options_id);
bool ValidateRequestId(AMX *amx, RequestId request_id);
bool ValidateQueueId(AMX *amx, QueueId queue_id);
bool ValidatePluginEndBehaviour(AMX *amx, PluginEndBehaviour plugin_end_behaviour);
bool ValidateFtpSecurity(AMX *amx, cell security_value);
bool ValidateDispatchOptions(AMX *amx, const OptionsData &options);
template <class TMethod>
void SetKeyValueOption(AMX *amx, cell *params, TMethod method);
template <class TMethod>
void SetStringOption(AMX *amx, cell *params, TMethod method);
using OptionsConfigurer = std::function<void(OptionsData &)>;
RequestId DispatchRequest(
    AMX *amx,
    RequestMethod method,
    OptionsId options_id,
    const std::string &url,
    const std::string &callback,
    std::unique_ptr<cell[]> data = nullptr,
    int data_len = 0,
    const OptionsConfigurer &configure = {});

std::unique_ptr<EasyHttpModule> g_EasyHttpModule;
std::unique_ptr<JSONMngr> g_JsonManager;
bool g_MapChangeResetDone = false;

namespace
{
    cvar_t cvar_ezhttp_trace = {"ezhttp_trace_log", "0", FCVAR_SERVER | FCVAR_SPONLY};

    void RefreshTraceLogSetting()
    {
        ezhttp::trace::SetEnabled(CVAR_GET_FLOAT("ezhttp_trace_log") != 0.0f);
    }
}

void CreateModules()
{
    ezhttp::trace::Initialize(MF_BuildPathname("addons/amxmodx/logs/ezhttp_trace.log"));
    RefreshTraceLogSetting();
    ezhttp::trace::Writef("module", "CreateModules begin");
    g_EasyHttpModule = std::make_unique<EasyHttpModule>(MF_BuildPathname("addons/amxmodx/data/amxx_easy_http_cacert.pem"));
    g_JsonManager = std::make_unique<JSONMngr>();
    g_MapChangeResetDone = false;
    ezhttp::trace::Writef("module", "CreateModules done easy_http=%p json=%p", g_EasyHttpModule.get(), g_JsonManager.get());
}

void DestroyModules()
{
    ezhttp::trace::Writef("module", "DestroyModules begin easy_http=%p json=%p", g_EasyHttpModule.get(), g_JsonManager.get());
    g_EasyHttpModule.reset();
    g_JsonManager.reset();
    ezhttp::trace::Writef("module", "DestroyModules done");
    ezhttp::trace::Shutdown();
}

// native EzHttpOptions:ezhttp_create_options(bool:auto_destroy = true);
cell AMX_NATIVE_CALL ezhttp_create_options(AMX *amx, cell *params)
{
    const bool auto_destroy = params[0] >= sizeof(cell)
                                  ? static_cast<bool>(params[1])
                                  : true;
    OptionsId options_id = g_EasyHttpModule->CreateOptions(auto_destroy);

    return (cell)options_id;
}

// native ezhttp_destroy_options(EzHttpOptions:options_id);
cell AMX_NATIVE_CALL ezhttp_destroy_options(AMX *amx, cell *params)
{
    auto options_id = (OptionsId)params[1];

    if (!ValidateOptionsId(amx, options_id))
        return 0;

    return g_EasyHttpModule->DeleteOptions(options_id);
}

// native ezhttp_option_set_user_agent(EzHttpOptions:options_id, const user_agent[]);
cell AMX_NATIVE_CALL ezhttp_option_set_user_agent(AMX *amx, cell *params)
{
    SetStringOption(amx, params, &ezhttp::EasyHttpOptionsBuilder::SetUserAgent);
    return 0;
}

// native ezhttp_option_add_url_parameter(EzHttpOptions:options_id, const key[], const value[]);
cell AMX_NATIVE_CALL ezhttp_option_add_url_parameter(AMX *amx, cell *params)
{
    SetKeyValueOption(amx, params, &ezhttp::EasyHttpOptionsBuilder::AddUrlParameter);
    return 0;
}

// native ezhttp_option_add_form_payload(EzHttpOptions:options_id, const key[], const value[]);
cell AMX_NATIVE_CALL ezhttp_option_add_form_payload(AMX *amx, cell *params)
{
    SetKeyValueOption(amx, params, &ezhttp::EasyHttpOptionsBuilder::AddFormPayload);
    return 0;
}

// native ezhttp_option_set_body(EzHttpOptions:options_id, const body[]);
cell AMX_NATIVE_CALL ezhttp_option_set_body(AMX *amx, cell *params)
{
    SetStringOption(amx, params, &ezhttp::EasyHttpOptionsBuilder::SetBody);
    return 0;
}

// native bool:ezhttp_option_set_body_from_json(EzHttpOptions:options_id, EzJSON:json, bool:pretty = false);
cell AMX_NATIVE_CALL ezhttp_option_set_body_from_json(AMX *amx, cell *params)
{
    auto options_id = (OptionsId)params[1];
    auto json_handle = (JS_Handle)params[2];
    auto pretty = (bool)params[3];

    if (!ValidateOptionsId(amx, options_id))
        return 0;

    if (!g_JsonManager->IsValidHandle(json_handle))
    {
        MF_LogError(amx, AMX_ERR_NATIVE, "Invalid JSON handle! %d", json_handle);
        return 0;
    }

    char *json_str = g_JsonManager->SerialToString(json_handle, pretty);
    if (json_str == nullptr)
        return 0;

    g_EasyHttpModule->GetOptions(options_id).options_builder.SetBody(json_str);
    g_JsonManager->FreeString(json_str);

    return 1;
}

// native ezhttp_option_append_body(EzHttpOptions:options_id, const body[]);
cell AMX_NATIVE_CALL ezhttp_option_append_body(AMX *amx, cell *params)
{
    SetStringOption(amx, params, &ezhttp::EasyHttpOptionsBuilder::AppendBody);
    return 0;
}

// native ezhttp_option_set_header(EzHttpOptions:options_id, const key[], const value[]);
cell AMX_NATIVE_CALL ezhttp_option_set_header(AMX *amx, cell *params)
{
    SetKeyValueOption(amx, params, &ezhttp::EasyHttpOptionsBuilder::SetHeader);
    return 0;
}

// native ezhttp_option_set_cookie(EzHttpOptions:options_id, const key[], const value[]);
cell AMX_NATIVE_CALL ezhttp_option_set_cookie(AMX *amx, cell *params)
{
    SetKeyValueOption(amx, params, &ezhttp::EasyHttpOptionsBuilder::SetCookie);
    return 0;
}

// native ezhttp_option_set_timeout(EzHttpOptions:options_id, timeout_ms);
cell AMX_NATIVE_CALL ezhttp_option_set_timeout(AMX *amx, cell *params)
{
    auto options_id = (OptionsId)params[1];
    cell timeout_ms = params[2];

    if (!ValidateOptionsId(amx, options_id))
        return 0;

    g_EasyHttpModule->GetOptions(options_id).options_builder.SetTimeout(timeout_ms);
    return 0;
}

// native ezhttp_option_set_connect_timeout(EzHttpOptions:options_id, timeout_ms);
cell AMX_NATIVE_CALL ezhttp_option_set_connect_timeout(AMX *amx, cell *params)
{
    auto options_id = (OptionsId)params[1];
    cell timeout_ms = params[2];

    if (!ValidateOptionsId(amx, options_id))
        return 0;

    g_EasyHttpModule->GetOptions(options_id).options_builder.SetConnectTimeout(timeout_ms);
    return 0;
}

// native ezhttp_option_set_proxy(EzHttpOptions:options_id, const proxy_url[]);
cell AMX_NATIVE_CALL ezhttp_option_set_proxy(AMX *amx, cell *params)
{
    SetStringOption(amx, params, &ezhttp::EasyHttpOptionsBuilder::SetProxy);
    return 0;
}

// native ezhttp_option_set_proxy_auth(EzHttpOptions:options_id, const user[], const password[]);
cell AMX_NATIVE_CALL ezhttp_option_set_proxy_auth(AMX *amx, cell *params)
{
    SetKeyValueOption(amx, params, &ezhttp::EasyHttpOptionsBuilder::SetProxyAuth);
    return 0;
}

// native ezhttp_option_set_auth(EzHttpOptions:options_id, const user[], const password[]);
cell AMX_NATIVE_CALL ezhttp_option_set_auth(AMX *amx, cell *params)
{
    SetKeyValueOption(amx, params, &ezhttp::EasyHttpOptionsBuilder::SetAuth);
    return 0;
}

// native ezhttp_option_set_user_data(EzHttpOptions:options_id, const data[], len);
cell AMX_NATIVE_CALL ezhttp_option_set_user_data(AMX *amx, cell *params)
{
    auto options_id = (OptionsId)params[1];
    cell *data_addr = MF_GetAmxAddr(amx, params[2]);
    int data_len = params[3];

    if (!ValidateOptionsId(amx, options_id))
        return 0;

    std::vector<cell> user_data;
    user_data.resize(data_len);
    MF_CopyAmxMemory(user_data.data(), data_addr, data_len);

    g_EasyHttpModule->GetOptions(options_id).user_data = user_data;
    return 0;
}

// native ezhttp_option_set_plugin_end_behaviour(EzHttpOptions:options_id, EzHttpPluginEndBehaviour:plugin_end_behaviour);
cell AMX_NATIVE_CALL ezhttp_option_set_plugin_end_behaviour(AMX *amx, cell *params)
{
    auto options_id = (OptionsId)params[1];
    auto plugin_end_behaviour = (PluginEndBehaviour)params[2];

    if (!ValidateOptionsId(amx, options_id))
        return 0;

    if (!ValidatePluginEndBehaviour(amx, plugin_end_behaviour))
        return 0;

    g_EasyHttpModule->GetOptions(options_id).plugin_end_behaviour = plugin_end_behaviour;
    return 0;
}

// native ezhttp_option_set_queue(EzHttpOptions:options_id, EzHttpQueue:end_map_behaviour);
cell AMX_NATIVE_CALL ezhttp_option_set_queue(AMX *amx, cell *params)
{
    auto options_id = (OptionsId)params[1];
    auto queue_id = (QueueId)params[2];

    if (!ValidateOptionsId(amx, options_id))
        return 0;

    if (!ValidateQueueId(amx, queue_id))
        return 0;

    g_EasyHttpModule->GetOptions(options_id).queue_id = queue_id;
    return 0;
}

// native EzHttpRequest:ezhttp_get(const url[], const on_complete[], EzHttpOptions:options_id = EzHttpOptions:0);
cell AMX_NATIVE_CALL ezhttp_get(AMX *amx, cell *params)
{
    enum
    {
        arg_count,
        arg_url,
        arg_callback,
        arg_option_id,
        arg_data,
        arg_data_len
    };

    int url_len;
    char *url = MF_GetAmxString(amx, params[arg_url], 0, &url_len);

    int callback_len;
    char *callback = MF_GetAmxString(amx, params[arg_callback], 1, &callback_len);

    int data_len = 0;
    cell *data = nullptr;
    if (params[arg_count] > 3)
    {
        data_len = params[arg_data_len];
        if (data_len > 0)
        {
            data = new cell[data_len];
            MF_CopyAmxMemory(data, MF_GetAmxAddr(amx, params[arg_data]), data_len);
        }
    }

    auto options_id = (OptionsId)params[arg_option_id];

    std::unique_ptr<cell[]> request_data(data);
    return (cell)DispatchRequest(amx, RequestMethod::HttpGet, options_id, std::string(url, url_len), std::string(callback, callback_len), std::move(request_data), data_len);
}

// native EzHttpRequest:ezhttp_post(const url[], const on_complete[], EzHttpOptions:options_id = EzHttpOptions:0);
cell AMX_NATIVE_CALL ezhttp_post(AMX *amx, cell *params)
{
    enum
    {
        arg_count,
        arg_url,
        arg_callback,
        arg_option_id,
        arg_data,
        arg_data_len
    };

    int url_len;
    char *url = MF_GetAmxString(amx, params[arg_url], 0, &url_len);

    int callback_len;
    char *callback = MF_GetAmxString(amx, params[arg_callback], 1, &callback_len);

    int data_len = 0;
    cell *data = nullptr;
    if (params[arg_count] > 3)
    {
        data_len = params[arg_data_len];
        if (data_len > 0)
        {
            data = new cell[data_len];
            MF_CopyAmxMemory(data, MF_GetAmxAddr(amx, params[arg_data]), data_len);
        }
    }

    auto options_id = (OptionsId)params[arg_option_id];

    std::unique_ptr<cell[]> request_data(data);
    return (cell)DispatchRequest(amx, RequestMethod::HttpPost, options_id, std::string(url, url_len), std::string(callback, callback_len), std::move(request_data), data_len);
}

// native EzHttpRequest:ezhttp_put(const url[], const on_complete[], EzHttpOptions:options_id = EzHttpOptions:0);
cell AMX_NATIVE_CALL ezhttp_put(AMX *amx, cell *params)
{
    enum
    {
        arg_count,
        arg_url,
        arg_callback,
        arg_option_id,
        arg_data,
        arg_data_len
    };

    int url_len;
    char *url = MF_GetAmxString(amx, params[arg_url], 0, &url_len);

    int callback_len;
    char *callback = MF_GetAmxString(amx, params[arg_callback], 1, &callback_len);

    int data_len = 0;
    cell *data = nullptr;
    if (params[arg_count] > 3)
    {
        data_len = params[arg_data_len];
        if (data_len > 0)
        {
            data = new cell[data_len];
            MF_CopyAmxMemory(data, MF_GetAmxAddr(amx, params[arg_data]), data_len);
        }
    }

    auto options_id = (OptionsId)params[arg_option_id];

    std::unique_ptr<cell[]> request_data(data);
    return (cell)DispatchRequest(amx, RequestMethod::HttpPut, options_id, std::string(url, url_len), std::string(callback, callback_len), std::move(request_data), data_len);
}

// native EzHttpRequest:ezhttp_patch(const url[], const on_complete[], EzHttpOptions:options_id = EzHttpOptions:0);
cell AMX_NATIVE_CALL ezhttp_patch(AMX *amx, cell *params)
{
    enum
    {
        arg_count,
        arg_url,
        arg_callback,
        arg_option_id,
        arg_data,
        arg_data_len
    };

    int url_len;
    char *url = MF_GetAmxString(amx, params[arg_url], 0, &url_len);

    int callback_len;
    char *callback = MF_GetAmxString(amx, params[arg_callback], 1, &callback_len);

    int data_len = 0;
    cell *data = nullptr;
    if (params[arg_count] > 3)
    {
        data_len = params[arg_data_len];
        if (data_len > 0)
        {
            data = new cell[data_len];
            MF_CopyAmxMemory(data, MF_GetAmxAddr(amx, params[arg_data]), data_len);
        }
    }

    auto options_id = (OptionsId)params[arg_option_id];

    std::unique_ptr<cell[]> request_data(data);
    return (cell)DispatchRequest(amx, RequestMethod::HttpPatch, options_id, std::string(url, url_len), std::string(callback, callback_len), std::move(request_data), data_len);
}

// native EzHttpRequest:ezhttp_delete(const url[], const on_complete[], EzHttpOptions:options_id = EzHttpOptions:0);
cell AMX_NATIVE_CALL ezhttp_delete(AMX *amx, cell *params)
{
    enum
    {
        arg_count,
        arg_url,
        arg_callback,
        arg_option_id,
        arg_data,
        arg_data_len
    };

    int url_len;
    char *url = MF_GetAmxString(amx, params[arg_url], 0, &url_len);

    int callback_len;
    char *callback = MF_GetAmxString(amx, params[arg_callback], 1, &callback_len);

    int data_len = 0;
    cell *data = nullptr;
    if (params[arg_count] > 3)
    {
        data_len = params[arg_data_len];
        if (data_len > 0)
        {
            data = new cell[data_len];
            MF_CopyAmxMemory(data, MF_GetAmxAddr(amx, params[arg_data]), data_len);
        }
    }

    auto options_id = (OptionsId)params[arg_option_id];

    std::unique_ptr<cell[]> request_data(data);
    return (cell)DispatchRequest(amx, RequestMethod::HttpDelete, options_id, std::string(url, url_len), std::string(callback, callback_len), std::move(request_data), data_len);
}

// native ezhttp_is_request_exists(EzHttpRequest:request_id);
cell AMX_NATIVE_CALL ezhttp_is_request_exists(AMX *amx, cell *params)
{
    auto request_id = (RequestId)params[1];

    return g_EasyHttpModule->IsRequestExists(request_id);
}

// native ezhttp_cancel_request(EzHttpRequest:request_id);
cell AMX_NATIVE_CALL ezhttp_cancel_request(AMX *amx, cell *params)
{
    auto request_id = (RequestId)params[1];

    if (!ValidateRequestId(amx, request_id))
        return 0;

    RequestData &request_data = g_EasyHttpModule->GetRequest(request_id);
    request_data.request_control->canceled.store(true);

    return 0;
}

// native ezhttp_request_progress(EzHttpRequest:request_id, progress[EzHttpProgress]);
cell AMX_NATIVE_CALL ezhttp_request_progress(AMX *amx, cell *params)
{
    auto request_id = (RequestId)params[1];

    if (!ValidateRequestId(amx, request_id))
        return 0;

    auto &request_control = g_EasyHttpModule->GetRequest(request_id).request_control;
    const auto progress = request_control->GetProgress();

    cell *p = MF_GetAmxAddr(amx, params[2]);
    p[0] = progress.download_now;
    p[1] = progress.download_total;
    p[2] = progress.upload_now;
    p[3] = progress.upload_total;

    return 0;
}

cell AMX_NATIVE_CALL ezhttp_get_http_code(AMX *amx, cell *params)
{
    auto request_id = (RequestId)params[1];

    if (!ValidateRequestId(amx, request_id))
        return 0;

    const Response &response = g_EasyHttpModule->GetRequest(request_id).response;

    return response.status_code;
}

cell AMX_NATIVE_CALL ezhttp_get_data(AMX *amx, cell *params)
{
    auto request_id = (RequestId)params[1];
    cell max_len = params[3];

    if (!ValidateRequestId(amx, request_id) || max_len == 0)
        return 0;

    const Response &response = g_EasyHttpModule->GetRequest(request_id).response;

    utils::SetAmxStringUTF8CharSafe(amx, params[2], response.text.c_str(), response.text.length(), max_len);

    return 0;
}

// native EzJSON:ezhttp_parse_json_response(EzHttpRequest:request_id, bool:with_comments = false);
cell AMX_NATIVE_CALL ezhttp_parse_json_response(AMX *amx, cell *params)
{
    auto request_id = (RequestId)params[1];
    bool with_comments = (bool)params[2];

    if (!ValidateRequestId(amx, request_id))
        return 0;

    const Response &response = g_EasyHttpModule->GetRequest(request_id).response;

    JS_Handle json_handle;
    bool result = g_JsonManager->Parse(response.text.c_str(), &json_handle, false, with_comments);

    return result ? json_handle : -1;
}

cell AMX_NATIVE_CALL ezhttp_get_url(AMX *amx, cell *params)
{
    auto request_id = (RequestId)params[1];
    cell max_len = params[3];

    if (!ValidateRequestId(amx, request_id) || max_len == 0)
        return 0;

    const Response &response = g_EasyHttpModule->GetRequest(request_id).response;

    utils::SetAmxStringUTF8CharSafe(amx, params[2], response.url.c_str(), response.url.str().length(), max_len);

    return 0;
}

cell AMX_NATIVE_CALL ezhttp_save_data_to_file(AMX *amx, cell *params)
{
    auto request_id = (RequestId)params[1];

    int file_path_len;
    char *file_path = MF_GetAmxString(amx, params[2], 0, &file_path_len);

    if (!ValidateRequestId(amx, request_id))
        return 0;

    const Response &response = g_EasyHttpModule->GetRequest(request_id).response;

    if (response.text.empty())
        return 0;

    std::ofstream file(MF_BuildPathname("%s", file_path), std::ofstream::out | std::ofstream::binary);
    if (!file.is_open())
        return 0;

    file.write(response.text.data(), response.text.length());
    file.close();

    return response.text.length();
}

cell AMX_NATIVE_CALL ezhttp_save_data_to_file2(AMX *amx, cell *params)
{
    auto request_id = (RequestId)params[1];
    FILE *file_handle = (FILE *)params[2];

    if (!ValidateRequestId(amx, request_id))
        return 0;

    const Response &response = g_EasyHttpModule->GetRequest(request_id).response;

    return std::fwrite(response.text.data(), sizeof(char), response.text.length(), file_handle);
}

cell AMX_NATIVE_CALL ezhttp_get_headers_count(AMX *amx, cell *params)
{
    auto request_id = (RequestId)params[1];

    if (!ValidateRequestId(amx, request_id))
        return 0;

    const Response &response = g_EasyHttpModule->GetRequest(request_id).response;

    return response.header.size();
}

cell AMX_NATIVE_CALL ezhttp_get_headers(AMX *amx, cell *params)
{
    auto request_id = (RequestId)params[1];
    int key_len;
    char *key = MF_GetAmxString(amx, params[2], 0, &key_len);
    cell value_max_len = params[4];

    if (!ValidateRequestId(amx, request_id))
        return 0;

    const Response &response = g_EasyHttpModule->GetRequest(request_id).response;

    const std::string header_key(key, key_len);
    if (response.header.count(header_key) == 1)
    {
        const std::string &header_value = response.header.at(header_key);

        utils::SetAmxStringUTF8CharSafe(amx, params[3], header_value.c_str(), header_value.length(), value_max_len);

        return 1;
    }

    return 0;
}

cell AMX_NATIVE_CALL ezhttp_iterate_headers(AMX *amx, cell *params)
{
    auto request_id = (RequestId)params[1];
    cell iter = params[2];
    int key_len;
    char *key = MF_GetAmxString(amx, params[3], 0, &key_len);
    cell value_max_len = params[5];

    if (!ValidateRequestId(amx, request_id))
        return 0;

    const RequestData &request = g_EasyHttpModule->GetRequest(request_id);
    const cpr::Header &header = request.response.header;

    return 0;
}

cell AMX_NATIVE_CALL ezhttp_get_elapsed(AMX *amx, cell *params)
{
    auto request_id = (RequestId)params[1];

    if (!ValidateRequestId(amx, request_id))
        return 0;

    const Response &response = g_EasyHttpModule->GetRequest(request_id).response;

    return amx_ftoc(response.elapsed);
}

cell AMX_NATIVE_CALL ezhttp_get_cookies_count(AMX *amx, cell *params)
{
    auto request_id = (RequestId)params[1];

    if (!ValidateRequestId(amx, request_id))
        return 0;

    const Response &response = g_EasyHttpModule->GetRequest(request_id).response;

    size_t size = response.cookies.end() - response.cookies.begin();
    return (cell)size;
}

cell AMX_NATIVE_CALL ezhttp_get_cookies(AMX *amx, cell *params)
{
    auto request_id = (RequestId)params[1];
    int key_len;
    char *key = MF_GetAmxString(amx, params[2], 0, &key_len);
    cell value_max_len = params[4];

    if (!ValidateRequestId(amx, request_id))
        return 0;

    Response &response = g_EasyHttpModule->GetRequest(request_id).response;

    const std::string cookie_key(key, key_len);

    auto it = std::find_if(response.cookies.begin(), response.cookies.end(), [&cookie_key](const auto &item)
                           { return item.GetName() == cookie_key; });
    if (it != response.cookies.end())
    {
        const std::string &cookie_value = it->GetValue();

        utils::SetAmxStringUTF8CharSafe(amx, params[3], cookie_value.c_str(), cookie_value.length(), value_max_len);

        return 1;
    }

    return 0;
}

cell AMX_NATIVE_CALL ezhttp_iterate_cookies(AMX *amx, cell *params)
{
    return 0;
}

cell AMX_NATIVE_CALL ezhttp_get_error_code(AMX *amx, cell *params)
{
    auto request_id = (RequestId)params[1];

    if (!ValidateRequestId(amx, request_id))
        return 0;

    const Response &response = g_EasyHttpModule->GetRequest(request_id).response;

    return (cell)response.error.code;
}

cell AMX_NATIVE_CALL ezhttp_get_error_message(AMX *amx, cell *params)
{
    auto request_id = (RequestId)params[1];
    cell max_len = params[3];

    if (!ValidateRequestId(amx, request_id) || max_len == 0)
        return 0;

    const Response &response = g_EasyHttpModule->GetRequest(request_id).response;

    utils::SetAmxStringUTF8CharSafe(amx, params[2], response.error.message.c_str(), response.error.message.length(), max_len);

    return 0;
}

cell AMX_NATIVE_CALL ezhttp_get_redirect_count(AMX *amx, cell *params)
{
    auto request_id = (RequestId)params[1];

    if (!ValidateRequestId(amx, request_id))
        return 0;

    const Response &response = g_EasyHttpModule->GetRequest(request_id).response;

    return response.redirect_count;
}

cell AMX_NATIVE_CALL ezhttp_get_uploaded_bytes(AMX *amx, cell *params)
{
    auto request_id = (RequestId)params[1];

    if (!ValidateRequestId(amx, request_id))
        return 0;

    const Response &response = g_EasyHttpModule->GetRequest(request_id).response;

    return response.uploaded_bytes;
}

cell AMX_NATIVE_CALL ezhttp_get_downloaded_bytes(AMX *amx, cell *params)
{
    auto request_id = (RequestId)params[1];

    if (!ValidateRequestId(amx, request_id))
        return 0;

    const Response &response = g_EasyHttpModule->GetRequest(request_id).response;

    return response.downloaded_bytes;
}

cell AMX_NATIVE_CALL ezhttp_get_user_data(AMX *amx, cell *params)
{
    auto request_id = (RequestId)params[1];
    cell *data_addr = MF_GetAmxAddr(amx, params[2]);

    if (!ValidateRequestId(amx, request_id))
        return 0;

    const std::optional<std::vector<cell>> &user_data = g_EasyHttpModule->GetRequest(request_id).user_data;
    if (!user_data)
        return 0;

    MF_CopyAmxMemory(data_addr, user_data.value().data(), user_data.value().size());

    return 0;
}

cell AMX_NATIVE_CALL ezhttp_ftp_upload(AMX *amx, cell *params)
{
    int len;
    std::string user(MF_GetAmxString(amx, params[1], 0, &len));
    std::string password(MF_GetAmxString(amx, params[2], 0, &len));
    std::string host = MF_GetAmxString(amx, params[3], 0, &len);
    std::string remote_file = MF_GetAmxString(amx, params[4], 0, &len);
    int local_file_len;
    char *local_file = MF_GetAmxString(amx, params[5], 1, &local_file_len);
    std::string callback = MF_GetAmxString(amx, params[6], 0, &len);
    cell security_value = params[7];
    auto options_id = (OptionsId)params[8];

    if (!ValidateFtpSecurity(amx, security_value))
        return 0;

    std::string url = utils::ConstructFtpUrl(user, password, host, remote_file);

    const std::string local_file_path = MF_BuildPathname("%s", local_file);
    const bool secure = security_value != 0;

    return (cell)DispatchRequest(
        amx,
        RequestMethod::FtpUpload,
        options_id,
        url,
        callback,
        nullptr,
        0,
        [local_file_path, secure](OptionsData &request_options)
        {
            request_options.options_builder.SetFilePath(local_file_path);
            request_options.options_builder.SetSecure(secure);
        });
}

cell AMX_NATIVE_CALL ezhttp_ftp_upload2(AMX *amx, cell *params)
{
    int url_str_len;
    char *url_str = MF_GetAmxString(amx, params[1], 0, &url_str_len);

    int local_file_len;
    char *local_file = MF_GetAmxString(amx, params[2], 1, &local_file_len);

    int callback_len;
    char *callback = MF_GetAmxString(amx, params[3], 2, &callback_len);

    cell security_value = params[4];
    auto options_id = (OptionsId)params[5];

    if (!ValidateFtpSecurity(amx, security_value))
        return 0;

    const std::string local_file_path = MF_BuildPathname("%s", local_file);
    const bool secure = security_value != 0;

    return (cell)DispatchRequest(
        amx,
        RequestMethod::FtpUpload,
        options_id,
        std::string(url_str, url_str_len),
        std::string(callback, callback_len),
        nullptr,
        0,
        [local_file_path, secure](OptionsData &request_options)
        {
            request_options.options_builder.SetFilePath(local_file_path);
            request_options.options_builder.SetSecure(secure);
        });
}

cell AMX_NATIVE_CALL ezhttp_ftp_download(AMX *amx, cell *params)
{
    int len;
    std::string user(MF_GetAmxString(amx, params[1], 0, &len));
    std::string password(MF_GetAmxString(amx, params[2], 0, &len));
    std::string host = MF_GetAmxString(amx, params[3], 0, &len);
    std::string remote_file = MF_GetAmxString(amx, params[4], 0, &len);
    int local_file_len;
    char *local_file = MF_GetAmxString(amx, params[5], 1, &local_file_len);
    std::string callback = MF_GetAmxString(amx, params[6], 0, &len);
    cell security_value = params[7];
    auto options_id = (OptionsId)params[8];

    if (!ValidateFtpSecurity(amx, security_value))
        return 0;

    std::string url = utils::ConstructFtpUrl(user, password, host, remote_file);

    const std::string local_file_path = MF_BuildPathname("%s", local_file);
    const bool secure = security_value != 0;

    return (cell)DispatchRequest(
        amx,
        RequestMethod::FtpDownload,
        options_id,
        url,
        callback,
        nullptr,
        0,
        [local_file_path, secure](OptionsData &request_options)
        {
            request_options.options_builder.SetFilePath(local_file_path);
            request_options.options_builder.SetSecure(secure);
        });
}

cell AMX_NATIVE_CALL ezhttp_ftp_download2(AMX *amx, cell *params)
{
    int url_str_len;
    char *url_str = MF_GetAmxString(amx, params[1], 0, &url_str_len);

    int local_file_len;
    char *local_file = MF_GetAmxString(amx, params[2], 1, &local_file_len);

    int callback_len;
    char *callback = MF_GetAmxString(amx, params[3], 2, &callback_len);

    cell security_value = params[4];
    auto options_id = (OptionsId)params[5];

    if (!ValidateFtpSecurity(amx, security_value))
        return 0;

    const std::string local_file_path = MF_BuildPathname("%s", local_file);
    const bool secure = security_value != 0;

    return (cell)DispatchRequest(
        amx,
        RequestMethod::FtpDownload,
        options_id,
        std::string(url_str, url_str_len),
        std::string(callback, callback_len),
        nullptr,
        0,
        [local_file_path, secure](OptionsData &request_options)
        {
            request_options.options_builder.SetFilePath(local_file_path);
            request_options.options_builder.SetSecure(secure);
        });
}

cell AMX_NATIVE_CALL ezhttp_create_queue(AMX *amx, cell *params)
{
    return (cell)g_EasyHttpModule->CreateQueue();
}

cell AMX_NATIVE_CALL ezhttp_steam_to_steam64(AMX *amx, cell *params)
{
    // doc https://developer.valvesoftware.com/wiki/SteamID

    int steam_len;
    char *steam_str = MF_GetAmxString(amx, params[1], 0, &steam_len);
    cell steam64_maxlen = params[3];

    std::string steam(steam_str, steam_len);

    if (steam.find("STEAM_") != 0)
        return 0;

    std::vector<std::string> tokens;
    utils::split(std::string_view(steam.c_str() + sizeof("STEAM_") - 1), ":", tokens);

    if (tokens.size() != 3)
        return 0;

    uint32_t account_id = std::atoi(tokens[2].c_str()); // 32 bit
    account_id = account_id << 1 | std::atoi(tokens[1].c_str());
    uint32_t account_instance = 1; // 20 bit, 1 for individual account
    uint8_t account_type = 1;      // 4 bit,  1 is individual account
    uint8_t universe = 1;          // 8 bit, using token[0] produces incorrect results, so use always 1

    uint64_t steam64 = (uint64_t)universe << 56 | (uint64_t)account_type << 52 | (uint64_t)account_instance << 32 | account_id;

    steam.assign(std::to_string(steam64));
    utils::SetAmxStringUTF8CharSafe(amx, params[2], steam.c_str(), steam.length(), steam64_maxlen);

    return 1;
}

RequestId DispatchRequest(
    AMX *amx,
    RequestMethod method,
    OptionsId options_id,
    const std::string &url,
    const std::string &callback,
    std::unique_ptr<cell[]> data,
    const int data_len,
    const OptionsConfigurer &configure)
{
    if (options_id != OptionsId::Null && !ValidateOptionsId(amx, options_id))
        return RequestId::Null;

    OptionsData request_options = options_id == OptionsId::Null
                                      ? OptionsData{}
                                      : g_EasyHttpModule->CreateOptionsSnapshot(options_id);

    if (configure)
        configure(request_options);

    if (!ValidateDispatchOptions(amx, request_options))
        return RequestId::Null;

    int callback_id = -1;
    if (!callback.empty())
    {
        if (!data)
        {
            callback_id = MF_RegisterSPForwardByName(amx, callback.c_str(), FP_CELL, FP_DONE);
        }
        else
        {
            callback_id = MF_RegisterSPForwardByName(amx, callback.c_str(), FP_CELL, FP_ARRAY, FP_DONE);
        }

        if (callback_id == -1)
        {
            MF_LogError(amx, AMX_ERR_NATIVE, "Callback function \"%s\" is not exists", callback.c_str());
            return RequestId::Null;
        }
    }

    RequestId request_id = g_EasyHttpModule->SendRequest(
        method,
        url,
        std::move(request_options),
        callback_id,
        std::move(data),
        data_len,
        options_id);

    if (request_id == RequestId::Null)
    {
        if (callback_id != -1)
            MF_UnregisterSPForward(callback_id);

        MF_LogError(amx, AMX_ERR_NATIVE, "Failed to dispatch request due to invalid internal state");
    }

    return request_id;
}

bool ValidateOptionsId(AMX *amx, OptionsId options_id)
{
    if (!g_EasyHttpModule->IsOptionsExists(options_id))
    {
        MF_LogError(amx, AMX_ERR_NATIVE, "Options id %d not exists", options_id);
        return false;
    }

    return true;
}

bool ValidateRequestId(AMX *amx, RequestId request_id)
{
    if (!g_EasyHttpModule->IsRequestExists(request_id))
    {
        MF_LogError(amx, AMX_ERR_NATIVE, "Request id %d not exists", request_id);
        return false;
    }

    return true;
}

bool ValidateQueueId(AMX *amx, QueueId queue_id)
{
    if (!g_EasyHttpModule->IsQueueExists(queue_id))
    {
        MF_LogError(amx, AMX_ERR_NATIVE, "Queue id %d not exists", queue_id);
        return false;
    }

    return true;
}

bool ValidatePluginEndBehaviour(AMX *amx, PluginEndBehaviour plugin_end_behaviour)
{
    switch (plugin_end_behaviour)
    {
    case PluginEndBehaviour::CancelRequests:
    case PluginEndBehaviour::ForgetRequests:
        return true;
    }

    MF_LogError(amx, AMX_ERR_NATIVE, "Invalid plugin end behaviour %d", static_cast<int>(plugin_end_behaviour));
    return false;
}

bool ValidateFtpSecurity(AMX *amx, cell security_value)
{
    if (security_value == 0 || security_value == 1)
        return true;

    MF_LogError(
        amx,
        AMX_ERR_NATIVE,
        "Invalid FTP security value %d. Expected EZH_UNSECURE (0) or EZH_SECURE_EXPLICIT (1)",
        security_value);
    return false;
}

bool ValidateDispatchOptions(AMX *amx, const OptionsData &options)
{
    if (!ValidateQueueId(amx, options.queue_id))
        return false;

    if (!ValidatePluginEndBehaviour(amx, options.plugin_end_behaviour))
        return false;

    return true;
}

template <class TMethod>
void SetKeyValueOption(AMX *amx, cell *params, TMethod method)
{
    auto options_id = (OptionsId)params[1];
    int key_len;
    char *key = MF_GetAmxString(amx, params[2], 0, &key_len);
    int value_len;
    char *value = MF_GetAmxString(amx, params[3], 1, &value_len);

    if (!ValidateOptionsId(amx, options_id))
        return;

    (g_EasyHttpModule->GetOptions(options_id).options_builder.*method)(std::string(key, key_len), std::string(value, value_len));
}

template <class TMethod>
void SetStringOption(AMX *amx, cell *params, TMethod method)
{
    auto options_id = (OptionsId)params[1];
    int value_len;
    char *value = MF_GetAmxString(amx, params[2], 0, &value_len);

    if (!ValidateOptionsId(amx, options_id))
        return;

    (g_EasyHttpModule->GetOptions(options_id).options_builder.*method)(std::string(value, value_len));
}

AMX_NATIVE_INFO g_Natives[] =
    {
        // options
        {"ezhttp_create_options", ezhttp_create_options},
        {"ezhttp_destroy_options", ezhttp_destroy_options},
        {"ezhttp_option_set_user_agent", ezhttp_option_set_user_agent},
        {"ezhttp_option_add_url_parameter", ezhttp_option_add_url_parameter},
        {"ezhttp_option_add_form_payload", ezhttp_option_add_form_payload},
        {"ezhttp_option_set_body", ezhttp_option_set_body},
        {"ezhttp_option_set_body_from_json", ezhttp_option_set_body_from_json},
        {"ezhttp_option_append_body", ezhttp_option_append_body},
        {"ezhttp_option_set_header", ezhttp_option_set_header},
        {"ezhttp_option_set_cookie", ezhttp_option_set_cookie},
        {"ezhttp_option_set_timeout", ezhttp_option_set_timeout},
        {"ezhttp_option_set_connect_timeout", ezhttp_option_set_connect_timeout},
        {"ezhttp_option_set_proxy", ezhttp_option_set_proxy},
        {"ezhttp_option_set_proxy_auth", ezhttp_option_set_proxy_auth},
        {"ezhttp_option_set_auth", ezhttp_option_set_auth},
        {"ezhttp_option_set_user_data", ezhttp_option_set_user_data},
        {"ezhttp_option_set_plugin_end_behaviour", ezhttp_option_set_plugin_end_behaviour},
        {"ezhttp_option_set_queue", ezhttp_option_set_queue},

        // requests
        {"ezhttp_get", ezhttp_get},
        {"ezhttp_post", ezhttp_post},
        {"ezhttp_put", ezhttp_put},
        {"ezhttp_patch", ezhttp_patch},
        {"ezhttp_delete", ezhttp_delete},
        {"ezhttp_is_request_exists", ezhttp_is_request_exists},
        {"ezhttp_cancel_request", ezhttp_cancel_request},
        {"ezhttp_request_progress", ezhttp_request_progress},

        // response
        {"ezhttp_get_http_code", ezhttp_get_http_code},
        {"ezhttp_get_data", ezhttp_get_data},
        {"ezhttp_parse_json_response", ezhttp_parse_json_response},
        {"ezhttp_get_url", ezhttp_get_url},
        {"ezhttp_save_data_to_file", ezhttp_save_data_to_file},
        {"ezhttp_save_data_to_file2", ezhttp_save_data_to_file2},
        {"ezhttp_get_headers_count", ezhttp_get_headers_count},
        {"ezhttp_get_headers", ezhttp_get_headers},
        {"ezhttp_iterate_headers", ezhttp_iterate_headers},
        {"ezhttp_get_elapsed", ezhttp_get_elapsed},
        {"ezhttp_get_cookies_count", ezhttp_get_cookies_count},
        {"ezhttp_get_cookies", ezhttp_get_cookies},
        {"ezhttp_iterate_cookies", ezhttp_iterate_cookies},
        {"ezhttp_get_error_code", ezhttp_get_error_code},
        {"ezhttp_get_error_message", ezhttp_get_error_message},
        {"ezhttp_get_redirect_count", ezhttp_get_redirect_count},
        {"ezhttp_get_uploaded_bytes", ezhttp_get_uploaded_bytes},
        {"ezhttp_get_downloaded_bytes", ezhttp_get_downloaded_bytes},
        {"ezhttp_get_user_data", ezhttp_get_user_data},

        // ftp
        {"ezhttp_ftp_upload", ezhttp_ftp_upload},
        {"ezhttp_ftp_upload2", ezhttp_ftp_upload2},
        {"ezhttp_ftp_download", ezhttp_ftp_download},
        {"ezhttp_ftp_download2", ezhttp_ftp_download2},

        // queue
        {"ezhttp_create_queue", ezhttp_create_queue},

        // special
        {"_ezhttp_steam_to_steam64", ezhttp_steam_to_steam64},
        {nullptr, nullptr},
};

cvar_t cvar_ezhttp_version = {"ezhttp_version", MODULE_VERSION, FCVAR_SERVER | FCVAR_SPONLY};

void OnAmxxAttach()
{
    MF_AddNatives(g_Natives);
    MF_AddNatives(g_JsonNatives);

    CVAR_REGISTER(&cvar_ezhttp_version);
    CVAR_REGISTER(&cvar_ezhttp_trace);

    CreateModules();
}

void OnAmxxDetach()
{
    DestroyModules();
}

void OnPluginsUnloading()
{
    RefreshTraceLogSetting();
    ezhttp::trace::Writef("module", "OnPluginsUnloading enter easy_http=%p json=%p mapchange_reset_done=%d", g_EasyHttpModule.get(), g_JsonManager.get(), g_MapChangeResetDone);

    if (g_EasyHttpModule && !g_MapChangeResetDone)
        g_EasyHttpModule->ServerDeactivate();

    if (g_JsonManager)
        g_JsonManager->FreeAllHandles();

    ezhttp::trace::Writef("module", "OnPluginsUnloading exit");
}

void ServerActivate(edict_t * /*pEdictList*/, int /*edictCount*/, int /*clientMax*/)
{
    g_MapChangeResetDone = false;
    RefreshTraceLogSetting();
    ezhttp::trace::Writef("module", "Metamod ServerActivate mapchange_reset_done=%d", g_MapChangeResetDone);
    SET_META_RESULT(MRES_IGNORED);
}

void StartFrame()
{
    RefreshTraceLogSetting();

    if (g_EasyHttpModule)
        g_EasyHttpModule->RunFrame();

    SET_META_RESULT(MRES_IGNORED);
}

void ServerDeactivate()
{
    RefreshTraceLogSetting();
    ezhttp::trace::Writef("module", "Metamod ServerDeactivate enter easy_http=%p json=%p", g_EasyHttpModule.get(), g_JsonManager.get());

    if (g_EasyHttpModule)
        g_EasyHttpModule->ServerDeactivate();

    g_MapChangeResetDone = true;

    if (g_JsonManager)
        g_JsonManager->FreeAllHandles();

    ezhttp::trace::Writef("module", "Metamod ServerDeactivate exit mapchange_reset_done=%d", g_MapChangeResetDone);

    SET_META_RESULT(MRES_IGNORED);
}

void GameShutdown()
{
    RefreshTraceLogSetting();
    ezhttp::trace::Writef("module", "GameShutdown");
    DestroyModules();
}
