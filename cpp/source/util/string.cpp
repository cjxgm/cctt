#include "string.hpp"
#include <iomanip>
#include <sstream>

namespace cctt
{
    namespace util
    {
        auto is_printable_ascii(char ch) -> bool
        {
            return ('\x20' <= ch && ch <= '\x7e');
        }

        auto is_cpp_alpha(char ch) -> bool
        {
            if ('a' <= ch && ch <= 'z') return true;
            if ('A' <= ch && ch <= 'Z') return true;
            if (ch == '_') return true;
            if (ch == '$') return true;
            return false;
        }

        auto is_cpp_number(char ch) -> bool
        {
            return ('0' <= ch && ch <= '9');
        }

        auto is_cpp_alnum(char ch) -> bool
        {
            return (is_cpp_alpha(ch) || is_cpp_number(ch));
        }

        auto quote(std::string const& x) -> std::string
        {
            std::ostringstream oss;
            auto delimiter = (x.find('"') == x.npos ? '"' : '\'');
            oss << std::quoted(x, delimiter);
            auto result = oss.str();

            for (auto& ch: result)
                if (!is_printable_ascii(ch))
                    ch = '.';

            return result;
        }

        auto quote(char x) -> std::string
        {
            std::string s;
            s += x;
            return quote(s);
        }
    }
}

