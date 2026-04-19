#include "EasyHttpModule.h"
#include "easy_http/EasyHttp.h"
#include "utils/TraceLog.h"
#include <utility>

using namespace ezhttp;

EasyHttpModule::EasyHttpModule(std::string ca_cert_path) :
    ca_cert_path_(std::move(ca_cert_path))
{
    // as this is a first insertion in queue then these EasyHttps will have QueueId == 1 and therefore QueueId == QueueId::Main
    CreateQueue();
    ezhttp::trace::Writef("EasyHttpModule", "ctor this=%p main_queue_created queues=%zu", this, easy_http_pack_.size());
}

EasyHttpModule::~EasyHttpModule()
{
    ezhttp::trace::Writef("EasyHttpModule", "dtor begin this=%p forgotten=%zu requests=%zu queues=%zu", this, forgotten_easy_http_.size(), requests_.size(), easy_http_pack_.size());
    ShutdownWithoutCallbacks();

    while (!forgotten_easy_http_.empty())
        RunCleanupFrameForForgottenEasyHttp();

    ezhttp::trace::Writef("EasyHttpModule", "dtor end this=%p", this);
}

void EasyHttpModule::RunFrame()
{
    RunFrameEasyHttp();
    RunCleanupFrameForForgottenEasyHttp();
    CleanupCompletedForgottenRequests();
}

void EasyHttpModule::ServerDeactivate()
{
    ezhttp::trace::Writef("EasyHttpModule", "ServerDeactivate enter requests=%zu queues=%zu forgotten=%zu options=%zu", requests_.size(), easy_http_pack_.size(), forgotten_easy_http_.size(), options_.size());
    ResetForMapChangeWithoutCallbacks();
    ezhttp::trace::Writef("EasyHttpModule", "ServerDeactivate exit requests=%zu queues=%zu forgotten=%zu options=%zu", requests_.size(), easy_http_pack_.size(), forgotten_easy_http_.size(), options_.size());
}

RequestId EasyHttpModule::SendRequest(RequestMethod method, const std::string& url, OptionsData options, int callback_id, std::unique_ptr<cell[]> callback_data, int callback_data_len)
{
    RequestId request_id = requests_.Add(RequestData());
    RequestData& request = GetRequest(request_id);
    const uint32_t request_generation = ++next_request_generation_;

    request.generation = request_generation;
    request.user_data = options.user_data;
    request.callback_data = std::move(callback_data);
    request.callback_data_len = callback_data_len;
    request.callback_id = callback_id;

    QueueId queue_id = options.queue_id;
    if (!IsQueueExists(queue_id)) // not so good, error suppression is going on, need to fix this in future
        queue_id = QueueId::Main;

    auto& easy_http = GetEasyHttp(queue_id, options.plugin_end_behaviour);
    const RequestOptions request_options = options.options_builder.BuildOptions();

    EasyHttpInterface::ResponseCallback cb_proxy = [this, request_id, request_generation](const Response& response) {
        ezhttp::trace::Writef("EasyHttpModule", "callback enter request=%d generation=%u", static_cast<int>(request_id), request_generation);
        if (!IsRequestExists(request_id))
        {
            ezhttp::trace::Writef("EasyHttpModule", "callback skip missing request=%d generation=%u", static_cast<int>(request_id), request_generation);
            return;
        }

        RequestData& current_request = GetRequest(request_id);
        if (current_request.generation != request_generation)
        {
            ezhttp::trace::Writef("EasyHttpModule", "callback skip generation mismatch request=%d current=%u expected=%u", static_cast<int>(request_id), current_request.generation, request_generation);
            return;
        }

        current_request.response = response;
        ezhttp::trace::Writef("EasyHttpModule", "callback finalize request=%d generation=%u status=%ld error=%d", static_cast<int>(request_id), request_generation, response.status_code, static_cast<int>(response.error.code));
        FinalizeRequest(request_id);
    };
    request.request_control = easy_http->SendRequest(method, url, request_options, cb_proxy);

    ezhttp::trace::Writef(
        "EasyHttpModule",
        "SendRequest request=%d generation=%u queue=%d behaviour=%d callback_id=%d control=%p url=%s",
        static_cast<int>(request_id),
        request_generation,
        static_cast<int>(queue_id),
        static_cast<int>(options.plugin_end_behaviour),
        callback_id,
        request.request_control.get(),
        url.c_str()
    );

    return request_id;
}

bool EasyHttpModule::DeleteRequest(RequestId handle)
{
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

void EasyHttpModule::FinalizeRequest(RequestId handle)
{
    if (!IsRequestExists(handle))
        return;

    RequestData& request = GetRequest(handle);
    ezhttp::trace::Writef("EasyHttpModule", "FinalizeRequest request=%d callback_id=%d control=%p", static_cast<int>(handle), request.callback_id, request.request_control.get());
    if (request.callback_id != -1)
    {
        if (request.callback_data)
            MF_ExecuteForward(request.callback_id, handle, MF_PrepareCellArray(request.callback_data.get(), request.callback_data_len));
        else
            MF_ExecuteForward(request.callback_id, handle);

        MF_UnregisterSPForward(request.callback_id);
        request.callback_id = -1;
    }

    DeleteRequest(handle);
    ezhttp::trace::Writef("EasyHttpModule", "FinalizeRequest done request=%d", static_cast<int>(handle));
}

void EasyHttpModule::CleanupCompletedForgottenRequests()
{
    for (auto it = requests_.begin(); it != requests_.end();)
    {
        auto& request = it->second;
        if (!request.request_control || !request.request_control->completed.load() || !request.request_control->forgotten.load())
        {
            ++it;
            continue;
        }

        if (request.callback_id != -1)
            MF_UnregisterSPForward(request.callback_id);

        ezhttp::trace::Writef("EasyHttpModule", "CleanupCompletedForgottenRequests remove request=%d control=%p", static_cast<int>(it->first), request.request_control.get());
        it = requests_.Remove(it);
    }
}

void EasyHttpModule::ShutdownWithoutCallbacks()
{
    ezhttp::trace::Writef("EasyHttpModule", "ShutdownWithoutCallbacks begin forgotten=%zu queues=%zu requests=%zu options=%zu", forgotten_easy_http_.size(), easy_http_pack_.size(), requests_.size(), options_.size());
    for (auto& pack_kv : easy_http_pack_)
    {
        auto& terminating_ez = pack_kv.second.terminating_easy_http;
        auto& forgettable_ez = pack_kv.second.forgettable_easy_http;

        if (terminating_ez)
        {
            terminating_ez->ForgetAllRequests();
            forgotten_easy_http_.emplace_back(std::move(terminating_ez));
        }

        if (forgettable_ez)
        {
            forgettable_ez->ForgetAllRequests();
            forgotten_easy_http_.emplace_back(std::move(forgettable_ez));
        }
    }

    for (auto& request_kv : requests_)
    {
        request_kv.second.callback_id = -1;
    }

    easy_http_pack_.clear();
    requests_.clear();
    options_.clear();
    ezhttp::trace::Writef("EasyHttpModule", "ShutdownWithoutCallbacks end forgotten=%zu queues=%zu requests=%zu options=%zu", forgotten_easy_http_.size(), easy_http_pack_.size(), requests_.size(), options_.size());
}


void EasyHttpModule::ResetForMapChangeWithoutCallbacks()
{
    ezhttp::trace::Writef("EasyHttpModule", "ResetForMapChangeWithoutCallbacks begin forgotten=%zu queues=%zu requests=%zu options=%zu", forgotten_easy_http_.size(), easy_http_pack_.size(), requests_.size(), options_.size());
    for (auto& request_kv : requests_)
    {
        ezhttp::trace::Writef(
            "EasyHttpModule",
            "mapchange forget request=%d callback_id=%d control=%p",
            static_cast<int>(request_kv.first),
            request_kv.second.callback_id,
            request_kv.second.request_control.get()
        );
        if (request_kv.second.callback_id != -1)
        {
            MF_UnregisterSPForward(request_kv.second.callback_id);
            request_kv.second.callback_id = -1;
        }

        if (request_kv.second.request_control)
            request_kv.second.request_control->forgotten.store(true);
    }

    for (auto& forgotten_ez : forgotten_easy_http_)
    {
        if (!forgotten_ez)
            continue;

        forgotten_ez->ForgetAllRequests();
        forgotten_ez->DropCompletedRequestsWithoutCallbacks();
    }

    for (auto& pack_kv : easy_http_pack_)
    {
        auto& terminating_ez = pack_kv.second.terminating_easy_http;
        auto& forgettable_ez = pack_kv.second.forgettable_easy_http;

        if (terminating_ez)
        {
            terminating_ez->CancelAllRequests();
            terminating_ez->ForgetAllRequests();
            terminating_ez->DropCompletedRequestsWithoutCallbacks();
            forgotten_easy_http_.emplace_back(std::move(terminating_ez));
        }

        if (forgettable_ez)
        {
            forgettable_ez->ForgetAllRequests();
            forgettable_ez->DropCompletedRequestsWithoutCallbacks();
            forgotten_easy_http_.emplace_back(std::move(forgettable_ez));
        }
    }

    easy_http_pack_.clear();
    requests_.clear();
    options_.clear();
    CreateQueue();
    ezhttp::trace::Writef("EasyHttpModule", "ResetForMapChangeWithoutCallbacks end forgotten=%zu queues=%zu requests=%zu options=%zu", forgotten_easy_http_.size(), easy_http_pack_.size(), requests_.size(), options_.size());
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
        it->get()->DropCompletedRequestsWithoutCallbacks();

        if (it->get()->GetActiveRequestCount() == 0)
        {
            ezhttp::trace::Writef("EasyHttpModule", "RunCleanupFrameForForgottenEasyHttp erase easy_http=%p", it->get());
            it = forgotten_easy_http_.erase(it);
        }
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
