#pragma once

#include "easy_http/EasyHttpInterface.h"
#include "easy_http/EasyHttpOptionsBuilder.h"
#include "utils/ContainerWithHandles.h"
#include "sdk/amxxmodule.h"
#include <memory>
#include <optional>
#include <vector>

enum class PluginEndBehaviour
{
    CancelRequests,
    ForgetRequests
};

enum class OptionsId : int
{
    Null = 0
};
enum class RequestId : int
{
    Null = 0
};
enum class QueueId : int
{
    Null = 0,
    Main = 1
};

struct OptionsData
{
    uint32_t generation = 0;
    ezhttp::EasyHttpOptionsBuilder options_builder;

    std::optional<std::vector<cell>> user_data;
    PluginEndBehaviour plugin_end_behaviour = PluginEndBehaviour::CancelRequests;
    QueueId queue_id = QueueId::Main;
    bool auto_destroy = true;
    size_t active_requests = 0;
};

struct RequestData
{
    uint32_t generation = 0;
    std::shared_ptr<ezhttp::RequestControl> request_control;
    ezhttp::Response response;
    std::optional<std::vector<cell>> user_data;
    std::unique_ptr<cell[]> callback_data;
    int callback_data_len = 0;
    int callback_id = -1;
    OptionsId auto_destroy_options_id = OptionsId::Null;
    uint32_t auto_destroy_options_generation = 0;
};

struct EasyHttpPack
{
    std::unique_ptr<ezhttp::EasyHttpInterface> forgettable_easy_http = nullptr;
    std::unique_ptr<ezhttp::EasyHttpInterface> terminating_easy_http = nullptr;

    EasyHttpPack() = default;

    EasyHttpPack(const EasyHttpPack &other) = delete;
    EasyHttpPack &operator=(const EasyHttpPack &other) = delete;

    EasyHttpPack(EasyHttpPack &&other) noexcept
    {
        forgettable_easy_http = std::move(other.forgettable_easy_http);
        terminating_easy_http = std::move(other.terminating_easy_http);
    }

    EasyHttpPack &operator=(EasyHttpPack &&other) noexcept
    {
        forgettable_easy_http = std::move(other.forgettable_easy_http);
        terminating_easy_http = std::move(other.terminating_easy_http);
        return *this;
    }
};

class EasyHttpModule
{
    const int kMainQueueThreads = 6;

    std::string ca_cert_path_;
    uint32_t next_request_generation_ = 0;
    uint32_t next_options_generation_ = 0;

    std::vector<std::unique_ptr<ezhttp::EasyHttpInterface>> forgotten_easy_http_;
    utils::ContainerWithHandles<QueueId, EasyHttpPack> easy_http_pack_;
    utils::ContainerWithHandles<OptionsId, OptionsData> options_;
    utils::ContainerWithHandles<RequestId, RequestData> requests_;

public:
    explicit EasyHttpModule(std::string ca_cert_path);
    ~EasyHttpModule();

    void RunFrame();
    void ServerDeactivate();

    RequestId SendRequest(
        ezhttp::RequestMethod method,
        const std::string &url,
        OptionsData options,
        int callback_id = -1,
        std::unique_ptr<cell[]> callback_data = nullptr,
        int callback_data_len = 0,
        OptionsId source_options_id = OptionsId::Null);

    bool DeleteRequest(RequestId handle);
    [[nodiscard]] bool IsRequestExists(RequestId handle) { return requests_.contains(handle); }
    [[nodiscard]] RequestData &GetRequest(RequestId handle) { return requests_.at(handle); }
    [[nodiscard]] const RequestData &GetRequest(RequestId handle) const { return requests_.at(handle); }

    OptionsId CreateOptions(bool auto_destroy = true);
    bool DeleteOptions(OptionsId handle);
    [[nodiscard]] bool IsOptionsExists(OptionsId handle) const { return options_.contains(handle); }
    [[nodiscard]] OptionsData &GetOptions(OptionsId handle) { return options_.at(handle); }
    [[nodiscard]] const OptionsData &GetOptions(OptionsId handle) const { return options_.at(handle); }
    [[nodiscard]] ezhttp::EasyHttpOptionsBuilder &GetOptionsBuilder(OptionsId handle) { return options_.at(handle).options_builder; }
    [[nodiscard]] OptionsData CreateOptionsSnapshot(OptionsId handle) const { return options_.at(handle); }

    QueueId CreateQueue();
    [[nodiscard]] bool IsQueueExists(QueueId handle) const { return easy_http_pack_.contains(handle); }

private:
    void FinalizeRequest(RequestId handle);
    void CleanupCompletedForgottenRequests();
    void TrackAutoDestroyOptions(RequestData &request, OptionsId options_id, uint32_t options_generation);
    void ReleaseAutoDestroyOptions(const RequestData &request);
    void ShutdownWithoutCallbacks();
    void ResetForMapChangeWithoutCallbacks();
    void RunFrameEasyHttp();
    void RunCleanupFrameForForgottenEasyHttp();
    std::unique_ptr<ezhttp::EasyHttpInterface> &GetEasyHttp(QueueId queue_id, PluginEndBehaviour end_map_behaviour);
};
