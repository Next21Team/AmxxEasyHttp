#include "EasyHttpModule.h"
#include <matchit.h>

#include <utils/string_utils.h>

using namespace std::chrono_literals;

namespace ezhttp_amxx
{
    EasyHttpModule::EasyHttpModule(std::string ca_cert_path) :
        ca_cert_path_(std::move(ca_cert_path))
    {
        main_forgetting_easy_handle_ = easy_http_holders_.Add(CreateEasyHttpInternal(PluginEndBehaviour::ForgetRequests, kMainHoldersThreads, ""));
        main_cancelling_easy_handle_ = easy_http_holders_.Add(CreateEasyHttpInternal(PluginEndBehaviour::CancelRequests, kMainHoldersThreads, ""));
    }

    EasyHttpModule::~EasyHttpModule()
    {
        CancelAndForgetEasyHttps();

        easy_http_holders_.at(main_forgetting_easy_handle_).mark_to_destroy();
        easy_http_holders_.at(main_cancelling_easy_handle_).mark_to_destroy();

        while (easy_http_holders_.size() > 0)
        {
            RunFrameEasyHttps();
            DestroyEasyHttps();

            std::this_thread::sleep_for(1ms);
        }
    }

    void EasyHttpModule::RunFrame()
    {
        RunFrameEasyHttps();
        DestroyEasyHttps();
    }

    void EasyHttpModule::ServerDeactivate()
    {
        CancelAndForgetEasyHttps();
    }

    ResultT<RequestId, SendRequestError> EasyHttpModule::SendRequest(ezhttp::RequestMethod method, const std::string& url, OptionsId options_id, const std::function<void(RequestId request_id)>& callback)
    {
        if (options_id == OptionsId::Null)
            options_id = CreateOptions();

        OptionsData& options_data = GetOptions(options_id);
        auto easy_handle = GetEasyHandleByOptions(options_data);

        if (easy_handle.has_error())
        {
            SendRequestError error = easy_handle.get_error();
            return error;
        }

        auto& easy_http = easy_http_holders_.at(*easy_handle).get_easy_http();

        RequestId request_id = requests_.Add(RequestData());
        RequestData& request_data = GetRequest(request_id);

        auto request_control = easy_http.SendRequest(
                method,
                url,
                options_data.get_options_builder().BuildOptions(),
                [this, request_id, callback](const ezhttp::Response& response)
                {
                    GetRequest(request_id).set_response(response);
                    callback(request_id);
                });

        request_data.Initialize(options_id, request_control);

        return request_id;
    }

    bool EasyHttpModule::IsRequestExists(RequestId handle)
    {
        return requests_.contains(handle);
    }

    RequestData &EasyHttpModule::GetRequest(RequestId handle)
    {
        return requests_.at(handle);
    }

    void EasyHttpModule::DeleteRequest(RequestId request_id)
    {
        DeleteOptions(GetRequest(request_id).get_options_id());

        requests_.Remove(request_id);
    }

    EasyHandle EasyHttpModule::CreateNamedEasyHttp(const std::string& plugin_name, const std::string& name, PluginEndBehaviour plugin_end_behaviour, int threads)
    {
        std::string plugin_name_lower = utils::to_lower_copy(plugin_name);
        std::string lower_name = utils::to_lower_copy(name);

        NamedHolderKey key(plugin_name_lower, lower_name);

        if (holders_names_.count(key) > 0)
            return EasyHandle::Null;

        EasyHttpHolder holder = CreateEasyHttpInternal(plugin_end_behaviour, threads, plugin_name, name);
        EasyHandle handle = easy_http_holders_.Add(std::move(holder));

        holders_names_[key] = handle;

        return handle;
    }

    EasyHandle EasyHttpModule::CreateEasyHttp(const std::string& plugin_name, PluginEndBehaviour plugin_end_behaviour, int threads)
    {
        EasyHttpHolder holder = CreateEasyHttpInternal(plugin_end_behaviour, threads, plugin_name);
        EasyHandle handle = easy_http_holders_.Add(std::move(holder));

        return handle;
    }

    EasyHandle EasyHttpModule::CreateLegacyEasyHttp(const std::string& plugin_name, int threads)
    {
        EasyHttpHolder holder_cancel = CreateEasyHttpInternal(PluginEndBehaviour::CancelRequests, threads, plugin_name);
        EasyHandle handle_cancel = easy_http_holders_.Add(std::move(holder_cancel));

        EasyHttpHolder holder_forget = CreateEasyHttpInternal(PluginEndBehaviour::ForgetRequests, threads, plugin_name);
        EasyHandle handle_forget = easy_http_holders_.Add(std::move(holder_forget));

        legacy_easy_http_linking_[handle_cancel] = handle_forget;

        return handle_cancel;
    }

    bool EasyHttpModule::IsEasyHttpExists(EasyHandle easy_handle)
    {
        return easy_http_holders_.contains(easy_handle);
    }

    bool EasyHttpModule::IsNamedEasyHttpExists(const std::string &plugin_name, const std::string &name)
    {
        return holders_names_.count(NamedHolderKey(plugin_name, name)) > 0;
    }

    EasyHandle EasyHttpModule::GetNamedEasyHttpHandle(const std::string &plugin_name, const std::string &name)
    {
        auto it = holders_names_.find(NamedHolderKey(plugin_name, name));
        if (it != holders_names_.end())
        {
            EasyHandle handle = it->second;

            EasyHttpHolder& holder = easy_http_holders_.at(handle);
            holder.cancel_mark_for_destroy();

            return handle;
        }

        return EasyHandle::Null;
    }

    OptionsId EasyHttpModule::CreateOptions()
    {
        return options_.Add(OptionsData{});
    }

    bool EasyHttpModule::IsOptionsExists(OptionsId handle) const
    {
        return options_.contains(handle);
    }

    OptionsData &EasyHttpModule::GetOptions(OptionsId handle)
    {
        return options_.at(handle);
    }

    bool EasyHttpModule::DeleteOptions(OptionsId handle)
    {
        return options_.Remove(handle);
    }

    ResultT<EasyHandle, SendRequestError> EasyHttpModule::GetEasyHandleByOptions(const OptionsData& options)
    {
        using namespace matchit;

        if (options.get_easy_handle() == EasyHandle::Null)
        {
            return  match(options.get_plugin_end_behaviour()) (
                pattern | none = main_cancelling_easy_handle_,
                pattern | PluginEndBehaviour::CancelRequests = main_cancelling_easy_handle_,
                pattern | PluginEndBehaviour::ForgetRequests = main_forgetting_easy_handle_
            );
        }

        if (easy_http_holders_.contains(options.get_easy_handle()))
        {
            EasyHandle handle = match(options.get_plugin_end_behaviour()) (
                pattern | none = options.get_easy_handle(),
                pattern | PluginEndBehaviour::CancelRequests = options.get_easy_handle(),
                pattern | PluginEndBehaviour::ForgetRequests = [this, &options]()
                {
                    if (legacy_easy_http_linking_.count(options.get_easy_handle()) > 0)
                        return legacy_easy_http_linking_[options.get_easy_handle()];

                    // IncompatibleOptions_EasyHandleAndPluginEndBehaviour
                    return EasyHandle::Null; // TODO else means that CancelRequests was destroyed, or it is not a legacy queue
                }
            );

            if (handle == EasyHandle::Null)
                return SendRequestError::IncompatibleOptions_EasyHandleAndPluginEndBehaviour;

            return handle;
        }

        return SendRequestError::EasyHandleNotFound;
    }

    EasyHttpHolder EasyHttpModule::CreateEasyHttpInternal(PluginEndBehaviour plugin_end_behaviour, int threads, const std::string& plugin_name, const std::string& name)
    {
        auto easy_http = std::make_unique<ezhttp::EasyHttp>(ca_cert_path_, threads);

        return EasyHttpHolder(std::move(easy_http), plugin_end_behaviour, plugin_name, name);
    }

    void EasyHttpModule::RunFrameEasyHttps()
    {
        for (auto& [handle, holder] : easy_http_holders_)
        {
            holder.get_easy_http().RunFrame();
        }
    }

    void EasyHttpModule::CancelAndForgetEasyHttps()
    {
        for (auto& [handle, holder] : easy_http_holders_)
        {
            switch (holder.get_plugin_end_behaviour())
            {
            case PluginEndBehaviour::CancelRequests:
                holder.get_easy_http().CancelAllRequests();
                break;

            case PluginEndBehaviour::ForgetRequests:
                holder.get_easy_http().ForgetAllRequests();
                break;
            }

            if (handle != main_forgetting_easy_handle_ && handle != main_cancelling_easy_handle_)
                holder.mark_to_destroy();
        }
    }

    void EasyHttpModule::DestroyEasyHttps()
    {
        for (auto it = easy_http_holders_.begin(); it != easy_http_holders_.end(); )
        {
            auto& [handle, holder] = *it;

            if (holder.is_marked_to_destroy() && holder.get_easy_http().GetActiveRequestCount() == 0)
            {
                if (!holder.get_name().empty())
                    holders_names_.erase(NamedHolderKey(holder.get_plugin_name(), holder.get_name()));

                if (holder.get_plugin_end_behaviour() == PluginEndBehaviour::CancelRequests)
                    legacy_easy_http_linking_.erase(handle);

                it = easy_http_holders_.Remove(it);
            }
            else
            {
                ++it;
            }
        }
    }
}
