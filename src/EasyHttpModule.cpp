#include "EasyHttpModule.h"
#include "easy_http/EasyHttp.h"
#include <utility>

using namespace ezhttp;

EasyHttpModule::EasyHttpModule(std::string ca_cert_path) :
    ca_cert_path_(std::move(ca_cert_path))
{
    // as this is a first insertion in queue then these EasyHttps will have QueueId == 1 and therefore QueueId == QueueId::Main
    CreateQueue();
}

EasyHttpModule::~EasyHttpModule()
{
    ResetMainAndRemoveUsersQueues();

    while (!forgotten_easy_http_.empty())
        RunCleanupFrameForForgottenEasyHttp();
}

void EasyHttpModule::RunFrame()
{
    RunFrameEasyHttp();
    RunCleanupFrameForForgottenEasyHttp();
}

void EasyHttpModule::ServerDeactivate()
{
    ResetMainAndRemoveUsersQueues();
}

RequestId EasyHttpModule::SendRequest(RequestMethod method, const std::string& url, OptionsId options_id, const ModuleRequestCallback& callback)
{
    if (options_id == OptionsId::Null)
        options_id = CreateOptions();
    OptionsData& options = GetOptions(options_id);

    RequestId request_id = requests_.Add(RequestData{});
    RequestData& request = GetRequest(request_id);

    QueueId queue_id = options.queue_id;
    if (!IsQueueExists(queue_id)) // not so good, error suppression is going on, need to fix this in future
        queue_id = QueueId::Main;

    auto& easy_http = GetEasyHttp(queue_id, options.plugin_end_behaviour);

    ResponseCallback cb_proxy = [this, request_id, callback](const cpr::Response& response) {
        GetRequest(request_id).response = response;
        callback(request_id);
    };
    request.request_control = easy_http->SendRequest(method, url, options.options_builder.BuildOptions(), cb_proxy);
    request.options_id = options_id;

    return request_id;
}

bool EasyHttpModule::DeleteRequest(RequestId handle, bool delete_related_options)
{
    if (delete_related_options)
        DeleteOptions(GetRequest(handle).options_id);

    return requests_.Remove(handle);
}

OptionsId EasyHttpModule::CreateOptions()
{
    return options_.Add(OptionsData{});
}

bool EasyHttpModule::DeleteOptions(OptionsId handle)
{
    return options_.Remove(handle);
}


void EasyHttpModule::ResetMainAndRemoveUsersQueues()
{
    for (auto it = easy_http_pack_.begin(); it != easy_http_pack_.end(); )
    {
        auto& terminating_ez = it->second.terminating_easy_http;
        auto& forgettable_ez = it->second.forgettable_easy_http;

        if (terminating_ez)
        {
            terminating_ez->CancelAllRequests();

            if (it->first == QueueId::Main)
                terminating_ez = std::make_unique<ezhttp::EasyHttp>(ca_cert_path_, kMainQueueThreads);
            else
                terminating_ez = nullptr;
        }

        if (forgettable_ez)
        {
            forgettable_ez->ForgetAllRequests();
            forgotten_easy_http_.emplace_back(std::move(forgettable_ez));

            if (it->first == QueueId::Main)
                forgettable_ez = std::make_unique<ezhttp::EasyHttp>(ca_cert_path_, kMainQueueThreads);
            else
                forgettable_ez = nullptr;
        }

        if (it->first != QueueId::Main)
            it = easy_http_pack_.Remove(it);
        else
            ++it;
    }
}

void EasyHttpModule::RunFrameEasyHttp()
{
    for (auto& pack_kv : easy_http_pack_)
    {
        auto& terminating_ez = pack_kv.second.terminating_easy_http;
        auto& forgettable_ez = pack_kv.second.forgettable_easy_http;

        if (terminating_ez)
            terminating_ez->RunFrame();

        if (forgettable_ez)
            forgettable_ez->RunFrame();
    }
}

void EasyHttpModule::RunCleanupFrameForForgottenEasyHttp()
{
    for (auto it = forgotten_easy_http_.begin(); it != forgotten_easy_http_.end(); )
    {
        it->get()->RunFrame();

        if (it->get()->GetActiveRequestCount() == 0)
            it = forgotten_easy_http_.erase(it);
        else
            it++;
    }
}

std::unique_ptr<ezhttp::EasyHttpInterface>& EasyHttpModule::GetEasyHttp(QueueId queue_id, PluginEndBehaviour end_map_behaviour)
{
    EasyHttpPack& easy_http_pack = easy_http_pack_.at(queue_id);
    int easy_http_threads = queue_id == QueueId::Main ? kMainQueueThreads : 1;

    switch (end_map_behaviour)
    {
        default:
        case PluginEndBehaviour::CancelRequests:
            if (!easy_http_pack.terminating_easy_http)
                easy_http_pack.terminating_easy_http = std::make_unique<EasyHttp>(ca_cert_path_, easy_http_threads);
            return easy_http_pack.terminating_easy_http;

        case PluginEndBehaviour::ForgetRequests:
            if (!easy_http_pack.forgettable_easy_http)
                easy_http_pack.forgettable_easy_http = std::make_unique<EasyHttp>(ca_cert_path_, easy_http_threads);
            return easy_http_pack.forgettable_easy_http;
    }
}

QueueId EasyHttpModule::CreateQueue()
{
    return easy_http_pack_.Add(EasyHttpPack{});
}
