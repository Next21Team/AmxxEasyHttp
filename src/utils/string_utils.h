#pragma once
#include <algorithm>
#include <string>
#include <vector>

namespace utils
{
    inline void to_lower(std::string& s)
    {
        std::for_each(s.begin(), s.end(), [](char &c) {
            c = ::tolower(c);
        });
    }

    [[nodiscard]] inline std::string to_lower_copy(const std::string& s)
    {
        std::string copy(s);
        to_lower(copy);

        return copy;
    }

    void split(const std::string_view& str, const std::string& splitBy, std::vector<std::string>& tokens);
}
