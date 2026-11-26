#pragma once

#include "CallbackDelivery.h"

#include <deque>
#include <iterator>
#include <list>
#include <utility>

namespace ezhttp
{
    // Both views share the same entries. An opted-in completion can pass a
    // deferred one during a pause without changing FIFO order on normal frames.
    template<class T>
    class CompletionQueue
    {
        std::list<T> pending_;
        std::deque<typename std::list<T>::iterator> opted_in_;

    public:
        CompletionQueue() = default;
        CompletionQueue(const CompletionQueue &) = delete;
        CompletionQueue &operator=(const CompletionQueue &) = delete;

        void Push(T value, bool dispatch_while_paused)
        {
            pending_.push_back(std::move(value));
            if (dispatch_while_paused)
                opted_in_.push_back(std::prev(pending_.end()));
        }

        bool Pop(T &out, CallbackDelivery delivery)
        {
            const bool paused_only = delivery == CallbackDelivery::OptedIn;
            if (delivery == CallbackDelivery::Suspended || pending_.empty() || (paused_only && opted_in_.empty()))
                return false;

            auto entry = paused_only ? opted_in_.front() : pending_.begin();
            if (!opted_in_.empty() && entry == opted_in_.front())
                opted_in_.pop_front();

            out = std::move(*entry);
            pending_.erase(entry);
            return true;
        }

        auto size() const { return pending_.size(); }
    };
}
