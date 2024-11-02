#pragma once
#include <variant>
#include <string>

class ResultError
{
    std::string error_;

public:
    explicit ResultError(std::string error) noexcept :
        error_(std::move(error))
    { }

    [[nodiscard]] const std::string& get_error() const
    {
        return error_;
    }
};

template<class TError = ResultError>
class Result
{
    std::optional<TError> error_;

public:
    Result() = default;

    Result(TError&& error) :
        error_(std::move(error))
    { }

    [[nodiscard]] bool has_error() const
    {
        return error_.has_value();
    }

    [[nodiscard]] const TError& get_error() const
    {
        return error_.value();
    }
};

template<class T, class TError = ResultError>
class ResultT
{
    std::variant<T, TError> value_;

public:
    ResultT(T&& value) :
        value_(std::forward<T>(value))
    { }

    ResultT(TError&& error) :
        value_(std::move(error))
    { }

    [[nodiscard]] bool has_error() const
    {
        return std::holds_alternative<TError>(value_);
    }

    [[nodiscard]] const TError& get_error() const
    {
        return std::get<TError>(value_);
    }

    const T& operator*() const
    {
        return std::get<T>(value_);
    }

    const T* operator->() const
    {
        return &std::get<T>(value_);
    }
};