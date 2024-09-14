#pragma once
#include <memory>

#include <cpr/cpr.h>

struct CurlHolderComparer
{
    std::shared_ptr<cpr::CurlHolder> holder_to_compare{};

    explicit CurlHolderComparer(const std::shared_ptr<cpr::CurlHolder>& holder) :
        holder_to_compare(holder)
    {
    }

    bool operator()(const std::shared_ptr<cpr::CurlHolder>& holder) const
    {
        return holder_to_compare == holder;
    }
};
