#pragma once
#include <string>

namespace cctt
{
    namespace util
    {
        auto quote(std::string const& x) -> std::string;
        auto quote_without_delimiters(std::string const& x) -> std::string;
        auto format_to_oneline(std::string x) -> std::string;
    }
}

