#pragma once
#include <memory>

#include <async++.h>

#include "RequestControl.hpp"

namespace ezhttp
{
    class TaskData
    {
        async::task<void> task_;
        std::shared_ptr<RequestControl> request_control_;

    public:
        explicit TaskData(async::task<void>&& task, std::shared_ptr<RequestControl> request_control) :
            task_(std::move(task)),
            request_control_(request_control)
        {
        }

        TaskData(const TaskData& other) = delete;

        TaskData(TaskData&& other) noexcept
        {
            task_ = std::move(other.task_);
            request_control_ = std::move(other.request_control_);
        }

        TaskData& operator=(TaskData&& other) noexcept
        {
            task_ = std::move(other.task_);
            request_control_ = std::move(other.request_control_);

            return *this;
        }

        async::task<void>& get_task()
        {
            return task_;
        }

        std::shared_ptr<RequestControl> get_request_control()
        {
            return request_control_;
        }
    };
}