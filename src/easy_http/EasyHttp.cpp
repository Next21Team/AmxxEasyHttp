#include "EasyHttp.h"

#include <utility>

using namespace ezhttp;
using namespace std::chrono_literals;

EasyHttp::EasyHttp() :
    request_scheduler_(10)
{ }

std::shared_ptr<RequestControl> EasyHttp::SendRequest(RequestMethod method, const cpr::Url &url, const RequestOptions &options, const RequestCallback& on_complete)
{
    auto request_control = std::make_shared<RequestControl>();

    async::spawn(request_scheduler_, [request_control, method, url, options]()
    {
        return SendRequestCpr(request_control, method, url, options);
    })
    .then(update_scheduler_, [request_control, on_complete](cpr::Response response)
    {
        if (!request_control->canceled)
            on_complete(std::move(response));
    });

    return request_control;
}

void EasyHttp::RunFrame()
{
    for (int i = 0; i < 10; i++)
    {
        if (!update_scheduler_.try_run_one_task())
            break;
    }
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
