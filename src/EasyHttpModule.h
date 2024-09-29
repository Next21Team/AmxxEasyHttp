#pragma once
#include <memory>
#include <utility>

#include <easy_http/EasyHttp.h>
#include <utils/ContainerWithHandles.h>

#include "Handles.h"
#include "EasyHttpHolder.hpp"
#include "OptionsData.hpp"
#include "RequestData.hpp"

namespace ezhttp_amxx
{
    class NamedHolderKey
    {
        std::string plugin_{};
        std::string name_{};

    public:
        NamedHolderKey(std::string plugin, std::string name) :
            plugin_(std::move(plugin)),
            name_(std::move(name))
        {
        }

        [[nodiscard]] const std::string& get_plugin() const
        {
            return plugin_;
        }

        [[nodiscard]] const std::string& get_name() const
        {
            return name_;
        }

        friend bool operator== (const NamedHolderKey& c1, const NamedHolderKey& c2)
        {
            return c1.plugin_ == c2.plugin_ &&
                   c2.name_ == c1.name_;
        }

        friend bool operator!= (const NamedHolderKey& c1, const NamedHolderKey& c2) { return !operator==(c1, c2); }
    };

    struct NamedHolderKeyHash
    {
        std::size_t operator()(const NamedHolderKey& key) const noexcept
        {
            std::size_t h1 = std::hash<std::string>{}(key.get_plugin());
            std::size_t h2 = std::hash<std::string>{}(key.get_name());
            return h1 ^ (h2 << 1);
        }
    };

    class EasyHttpModule
    {
        const int kMainHoldersThreads = 6;

        std::string ca_cert_path_{};

        utils::ContainerWithHandles<EasyHandle, EasyHttpHolder> easy_http_holders_{};
        // Key - EasyHandle belonging to PluginEndBehaviour::CancelRequests. It is this EasyHandle that returns the CreateLegacyEasyHttp method.
        // Value - EasyHandle belonging to PluginEndBehaviour::ForgetRequests
        std::unordered_map<EasyHandle, EasyHandle> legacy_easy_http_linking_{};
        std::unordered_map<NamedHolderKey, EasyHandle, NamedHolderKeyHash> holders_names_{};

        utils::ContainerWithHandles<OptionsId, OptionsData> options_{};
        utils::ContainerWithHandles<RequestId, RequestData> requests_{};

        EasyHandle main_forgetting_easy_handle_{};
        EasyHandle main_cancelling_easy_handle_{};

    public:
        explicit EasyHttpModule(std::string ca_cert_path);
        ~EasyHttpModule();

        void RunFrame();
        void ServerDeactivate();

        RequestId SendRequest(ezhttp::RequestMethod method, const std::string& url, OptionsId options_id, const std::function<void(RequestId request_id)>& callback);
        [[nodiscard]] bool IsRequestExists(RequestId handle);
        [[nodiscard]] RequestData& GetRequest(RequestId handle);
        void DeleteRequest(RequestId request_id);

        // returns EasyHandle::Null if queue already exists
        EasyHandle CreateNamedEasyHttp(const std::string& plugin_name, const std::string& name, PluginEndBehaviour plugin_end_behaviour, int threads);
        EasyHandle CreateEasyHttp(const std::string& plugin_name, PluginEndBehaviour plugin_end_behaviour, int threads);
        // Prior to 1.4.0, two easy_http were created for each queue created, one for each PluginEndBehaviour.
        // This method emulates that behavior.
        EasyHandle CreateLegacyEasyHttp(const std::string& plugin_name, int threads);
        [[nodiscard]] bool IsEasyHttpExists(EasyHandle easy_handle);
        [[nodiscard]] bool IsNamedEasyHttpExists(const std::string& plugin_name, const std::string& name);
        [[nodiscard]] EasyHandle GetNamedEasyHttpHandle(const std::string& plugin_name, const std::string& name);

        [[nodiscard]] OptionsId CreateOptions();
        [[nodiscard]] bool IsOptionsExists(OptionsId handle) const;
        [[nodiscard]] OptionsData& GetOptions(OptionsId handle);
        bool DeleteOptions(OptionsId handle);

    private:
        EasyHandle GetEasyHandleByOptions(const OptionsData& options);
        EasyHttpHolder CreateEasyHttpInternal(PluginEndBehaviour plugin_end_behaviour, int threads, const std::string& plugin_name, const std::string& name = "");
        void RunFrameEasyHttps();
        void CancelAndForgetEasyHttps();
        void DestroyEasyHttps();
    };
}
