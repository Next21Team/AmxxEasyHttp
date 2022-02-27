#include "string_utils.h"

namespace utils
{
    void split(const std::string_view& str, const std::string& splitBy, std::vector<std::string>& tokens)
    {
        tokens.emplace_back(str);

        size_t splitAt;
        size_t splitLen = splitBy.size();
        std::string frag;

        while(true)
        {
            frag = tokens.back();
            splitAt = frag.find(splitBy);
            if (splitAt == std::string::npos)
                break;

            tokens.back() = frag.substr(0, splitAt);
            tokens.push_back(frag.substr(splitAt+splitLen, frag.size()-(splitAt+splitLen)));
        }
    }
}