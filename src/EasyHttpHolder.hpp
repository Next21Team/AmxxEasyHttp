#pragma once
#include <utility>

#include "PluginEndBehaviour.h"

namespace ezhttp_amxx
{
    class EasyHttpHolder
    {
        std::unique_ptr<ezhttp::EasyHttp> easy_http_{};
        PluginEndBehaviour plugin_end_behaviour_{};
        std::string plugin_name_{};
        std::string name_{};

        bool marked_to_destroy_{};

    public:
        explicit EasyHttpHolder(std::unique_ptr<ezhttp::EasyHttp> easy_http, PluginEndBehaviour plugin_end_behaviour, std::string plugin_name, std::string name = "") :
            easy_http_(std::move(easy_http)),
            plugin_end_behaviour_(plugin_end_behaviour),
            plugin_name_(std::move(plugin_name)),
            name_(std::move(name))
        {
        }

        EasyHttpHolder(const EasyHttpHolder& other) = delete;

        EasyHttpHolder(EasyHttpHolder&& other) noexcept
        {
            easy_http_ = std::move(other.easy_http_);
            plugin_end_behaviour_ = other.plugin_end_behaviour_;
            plugin_name_ = std::move(other.plugin_name_);
            name_ = std::move(other.name_);
            marked_to_destroy_ = other.marked_to_destroy_;
        }

        EasyHttpHolder& operator=(EasyHttpHolder&& other) noexcept
        {
            easy_http_ = std::move(other.easy_http_);
            plugin_end_behaviour_ = other.plugin_end_behaviour_;
            plugin_name_ = std::move(other.plugin_name_);
            name_ = std::move(other.name_);
            marked_to_destroy_ = other.marked_to_destroy_;

            return *this;
        }

        void mark_to_destroy() { marked_to_destroy_ = true; }
        void cancel_mark_for_destroy() { marked_to_destroy_ = false; }

        [[nodiscard]] bool is_marked_to_destroy() const { return marked_to_destroy_; }
        [[nodiscard]] ezhttp::EasyHttp& get_easy_http() { return *easy_http_; }
        [[nodiscard]] PluginEndBehaviour get_plugin_end_behaviour() const { return plugin_end_behaviour_; }
        [[nodiscard]] const std::string& get_plugin_name() const { return plugin_name_; }
        [[nodiscard]] const std::string& get_name() const { return name_; }
    };
}
