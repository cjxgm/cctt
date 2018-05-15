#pragma once
#include <string>

namespace cctt
{
    namespace util
    {
        auto is_printable_ascii(char ch) -> bool;
        auto is_cpp_alpha(char ch) -> bool;
        auto is_cpp_number(char ch) -> bool;
        auto is_cpp_alnum(char ch) -> bool;

        auto quote(std::string const& x) -> std::string;
        auto quote(char x) -> std::string;
    }
}

