#pragma once
#include <utility>

#include <easy_http/RequestControlInterface.h>

#include "Handles.h"

namespace ezhttp_amxx
{
    class RequestData
    {
        std::shared_ptr<ezhttp::RequestControlInterface> request_control_{};
        // options_id is always valid as long as the RequestId associated with the object exists
        OptionsId options_id_{};
        ezhttp::Response response_{};

    public:
        void Initialize(OptionsId options_id, std::shared_ptr<ezhttp::RequestControlInterface> request_control)
        {
            options_id_ = options_id;
            request_control_ = std::move(request_control);
        }

        [[nodiscard]] ezhttp::RequestControlInterface& get_request_control() const { return *request_control_; }
        [[nodiscard]] OptionsId get_options_id() const { return options_id_; }
        [[nodiscard]] const ezhttp::Response& get_response() const { return response_; }

        void set_response(ezhttp::Response response) { response_ = std::move(response); }
    };
}
