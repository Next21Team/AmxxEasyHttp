#include "EasyHttpOptionsBuilder.h"

using namespace ezhttp;

void EasyHttpOptionsBuilder::SetUserAgent(const std::string& user_agent)
{
    options_.user_agent = cpr::UserAgent(user_agent);
}

void EasyHttpOptionsBuilder::AddUrlParameter(const std::string& key, const std::string& value)
{
    if (!options_.url_parameters)
        options_.url_parameters = cpr::Parameters{};

    options_.url_parameters->Add({key, value});
}

void EasyHttpOptionsBuilder::AddFormPayload(const std::string& key, const std::string& value)
{
    if (!options_.form_payload)
        options_.form_payload = cpr::Payload{};

    options_.form_payload->Add({key, value});
}

void EasyHttpOptionsBuilder::SetBody(const std::string& body_text)
{
    options_.body = cpr::Body(body_text);
}

void EasyHttpOptionsBuilder::SetHeader(const std::string& key, const std::string& value)
{
    if (!options_.header)
        options_.header = cpr::Header{};

    (*options_.header)[key] = value;
}

void EasyHttpOptionsBuilder::SetCookie(const std::string& key, const std::string& value)
{
    if (!options_.cookies)
        options_.cookies = cpr::Cookies{};

    (*options_.cookies)[key] = value;
}

void EasyHttpOptionsBuilder::SetTimeout(int32_t timeout_ms)
{
    options_.timeout = cpr::Timeout{timeout_ms};
}

void EasyHttpOptionsBuilder::SetConnectTimeout(int32_t timeout_ms)
{
    options_.connect_timeout = cpr::ConnectTimeout{timeout_ms};
}

void EasyHttpOptionsBuilder::SetProxy(const std::string &proxy_url)
{
    options_.proxy_url = proxy_url;
}

void EasyHttpOptionsBuilder::SetProxyAuth(const std::string &user, const std::string &password)
{
    options_.proxy_auth = {user, password};
}

void EasyHttpOptionsBuilder::SetAuth(const std::string &user, const std::string &password)
{
    options_.auth = {user, password};
}

void EasyHttpOptionsBuilder::SetUserData(const std::vector<cell>& user_data)
{
    options_.user_data = user_data;
}