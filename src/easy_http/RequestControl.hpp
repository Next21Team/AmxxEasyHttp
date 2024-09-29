#pragma once
#include "RequestControlInterface.h"

namespace ezhttp
{
    class RequestControl : public RequestControlInterface
    {
        std::recursive_mutex mutex_{};

        bool is_completed_{};
        bool is_forgotten_{};
        bool is_canceled_{};
        RequestProgress progress_{};

    public:
        void CancelRequest() override
        {
            std::lock_guard lock_guard(mutex_);
            is_canceled_ = true;
        }

        [[nodiscard]] bool IsCompleted() override
        {
            std::lock_guard lock_guard(mutex_);
            return is_completed_;
        }

        [[nodiscard]] bool IsCancelled() override
        {
            std::lock_guard lock_guard(mutex_);
            return is_canceled_;
        }

        [[nodiscard]] RequestProgress GetProgress() override
        {
            std::lock_guard lock_guard(mutex_);
            return progress_;
        }

        void set_forgotten_unsafe() { is_forgotten_ = true; }
        void set_completed_unsafe() { is_completed_ = true; }
        void set_progress_unsafe(RequestProgress progress) { progress_ = progress; }

        [[nodiscard]] bool is_forgotten_unsafe() const { return is_forgotten_; }
        [[nodiscard]] bool is_cancelled_unsafe() const { return is_canceled_; }

        std::recursive_mutex& get_mutex() { return mutex_; }
    };
}