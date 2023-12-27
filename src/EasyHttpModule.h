#pragma once

#include "easy_http/EasyHttpInterface.h"
#include "easy_http/EasyHttpOptionsBuilder.h"
#include "utils/ContainerWithHandles.h"
#include "sdk/amxxmodule.h"
#include <async++.h>
#include <vector>

enum class PluginEndBehaviour
{
    CancelRequests,
    ForgetRequests
};

enum class OptionsId : int { Null = 0 };
enum class RequestId : int { Null = 0 };
enum class QueueId : int { Null = 0, Main = 1 };

struct RequestData
{
    std::shared_ptr<ezhttp::RequestControl> request_control;
    // options_id is always valid as long as the RequestId associated with the object exists
    OptionsId options_id;
    ezhttp::Response response;
};

struct OptionsData
{
    ezhttp::EasyHttpOptionsBuilder options_builder;

    std::optional<std::vector<cell>> user_data;
    PluginEndBehaviour plugin_end_behaviour = PluginEndBehaviour::CancelRequests;
    QueueId queue_id = QueueId::Main;
};

struct EasyHttpPack
{
    std::unique_ptr<ezhttp::EasyHttpInterface> forgettable_easy_http = nullptr;
    std::unique_ptr<ezhttp::EasyHttpInterface> terminating_easy_http = nullptr;

    EasyHttpPack() = default;

    EasyHttpPack(const EasyHttpPack& other) = delete;
    EasyHttpPack& operator=(const EasyHttpPack& other) = delete;

    EasyHttpPack(EasyHttpPack&& other) noexcept
    {
        forgettable_easy_http = std::move(other.forgettable_easy_http);
        terminating_easy_http = std::move(other.terminating_easy_http);
    }

    EasyHttpPack& operator=(EasyHttpPack&& other) noexcept
    {
        forgettable_easy_http = std::move(other.forgettable_easy_http);
        terminating_easy_http = std::move(other.terminating_easy_http);
        return *this;
    }
};

using ModuleRequestCallback = std::function<void(RequestId request_id)>;

class EasyHttpModule
{
    const int kMainQueueThreads = 6;

    std::string ca_cert_path_;

    std::vector<std::unique_ptr<ezhttp::EasyHttpInterface>> forgotten_easy_http_;
    utils::ContainerWithHandles<QueueId, EasyHttpPack> easy_http_pack_;
    utils::ContainerWithHandles<OptionsId, OptionsData> options_;
    utils::ContainerWithHandles<RequestId, RequestData> requests_;

public:
    explicit EasyHttpModule(std::string ca_cert_path);
    ~EasyHttpModule();

    void RunFrame();
    void ServerDeactivate();

    RequestId SendRequest(ezhttp::RequestMethod method, const std::string& url, OptionsId options_id, const ModuleRequestCallback& callback);

    // When using delete_related_options, make sure that other requests do not use the same options as this one
    bool DeleteRequest(RequestId handle, bool delete_related_options = false);
    [[nodiscard]] inline bool IsRequestExists(RequestId handle) { return requests_.contains(handle); }
    [[nodiscard]] inline RequestData& GetRequest(RequestId handle) { return requests_.at(handle); }

    OptionsId CreateOptions();
    bool DeleteOptions(OptionsId handle);
    [[nodiscard]] inline bool IsOptionsExists(OptionsId handle) const { return options_.contains(handle); }
    [[nodiscard]] inline OptionsData& GetOptions(OptionsId handle) { return options_.at(handle); }
    [[nodiscard]] inline ezhttp::EasyHttpOptionsBuilder& GetOptionsBuilder(OptionsId handle) { return options_.at(handle).options_builder; }

    QueueId CreateQueue();
    [[nodiscard]] inline bool IsQueueExists(QueueId handle) const { return easy_http_pack_.contains(handle); }

private:
    void ResetMainAndRemoveUsersQueues();
    void RunFrameEasyHttp();
    void RunCleanupFrameForForgottenEasyHttp();
    std::unique_ptr<ezhttp::EasyHttpInterface>& GetEasyHttp(QueueId queue_id, PluginEndBehaviour end_map_behaviour);
};

