#pragma once
#include <string>

namespace cctt
{
    namespace util
    {
        inline namespace file
        {
            auto slurp(char const* path) -> std::string;
        }
    }
}

