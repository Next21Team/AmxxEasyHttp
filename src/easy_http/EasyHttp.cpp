#include "EasyHttp.h"

using namespace ezhttp;
using namespace concurrencpp;
using namespace std::chrono_literals;

EasyHttp::EasyHttp()
{
    request_executor_ = runtime_.make_executor<thread_pool_executor>("easy_http_request", 10, 30s);
    update_executor_ = runtime_.make_manual_executor();
}

std::shared_ptr<RequestControl> EasyHttp::SendRequest(RequestMethod method, const cpr::Url &url, const RequestOptions &options, std::shared_ptr<RequestCallback> on_complete)
{
    auto request_control = std::make_shared<RequestControl>();

    update_executor_->submit([this, method, request_control, url, options, on_complete]() {
        return SendRequestCoroutine(request_control, method, url, on_complete, options);
    });

    return request_control;
}

void EasyHttp::RunFrame()
{
    update_executor_->loop(10);
}

result<void> EasyHttp::SendRequestCoroutine(std::shared_ptr<RequestControl> request_control, RequestMethod method, cpr::Url url, std::shared_ptr<RequestCallback> on_complete,
                                            RequestOptions options)
{
    cpr::Response response = co_await request_executor_->submit([request_control, method, url, options]() {
        return SendRequestCpr(request_control, method, url, options);
    });

    if (request_control->canceled)
        co_return;

    co_await resume_on(update_executor_);

    (*on_complete)(response);
}

cpr::Response EasyHttp::SendRequestCpr(const std::shared_ptr<RequestControl>& request_control, RequestMethod method, cpr::Url url, RequestOptions options)
{
    cpr::Session session;

    session.SetUrl(url);

    session.SetProgressCallback(cpr::ProgressCallback(
        [request_control](cpr::cpr_off_t downloadTotal, cpr::cpr_off_t downloadNow, cpr::cpr_off_t uploadTotal, cpr::cpr_off_t uploadNow, intptr_t userdata) {
            request_control->progress = RequestControl::Progress{ (int32_t)(downloadTotal), (int32_t)(downloadNow), (int32_t)(uploadTotal), (int32_t)(uploadNow) };

            return !request_control->canceled;
        }));

    if (options.user_agent)
        session.SetUserAgent(*options.user_agent);

    if (options.url_parameters)
        session.SetParameters(*options.url_parameters);

    if (options.form_payload)
        session.SetPayload(*options.form_payload);

    if (options.body)
        session.SetBody(*options.body);

    if (options.header)
        session.SetHeader(*options.header);

    if (options.cookies)
        session.SetCookies(*options.cookies);

    if (options.timeout)
        session.SetTimeout(*options.timeout);

    if (options.proxy_url)
    {
        std::string protocol = url.str().substr(0, url.str().find(':'));
        session.SetProxies({{protocol, *options.proxy_url}});
    }

    if (options.proxy_auth)
    {
        std::string user = options.proxy_auth->first;
        std::string password = options.proxy_auth->second;

        std::string protocol = url.str().substr(0, url.str().find(':'));
        session.SetProxyAuth({{protocol, cpr::EncodedAuthentication{user, password}}});
    }

    if (options.auth)
        session.SetAuth(*options.auth);

    if (method == RequestMethod::Get)
        return session.Get();

    return session.Post();
}
