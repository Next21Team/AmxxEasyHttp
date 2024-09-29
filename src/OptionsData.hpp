#pragma once
#include <vector>

#include <sdk/amxxmodule.h>
#include <easy_http/EasyHttp.h>

#include "Handles.h"
#include "PluginEndBehaviour.h"

namespace ezhttp_amxx
{
    class OptionsData
    {
        ezhttp::EasyHttpOptionsBuilder options_builder_{};

        std::optional<std::vector<cell>> user_data_{};
        std::optional<PluginEndBehaviour> plugin_end_behaviour_{};
        EasyHandle easy_handle_ = EasyHandle::Null;

    public:
        [[nodiscard]] ezhttp::EasyHttpOptionsBuilder& get_options_builder() { return options_builder_; }
        [[nodiscard]] const std::optional<std::vector<cell>>& get_user_data() const { return user_data_; }
        [[nodiscard]] const std::optional<PluginEndBehaviour>& get_plugin_end_behaviour() const { return plugin_end_behaviour_; }
        [[nodiscard]] EasyHandle get_easy_handle() const { return easy_handle_; }

        void set_user_data(const std::vector<cell>& user_data) { user_data_ = user_data; }
        void set_plugin_end_behaviour(PluginEndBehaviour plugin_end_behaviour) { plugin_end_behaviour_ = plugin_end_behaviour; }
        void set_easy_handle(EasyHandle easy_handle) { easy_handle_ = easy_handle; }
    };
}
