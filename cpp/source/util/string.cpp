#include "string.hpp"
#include <nonstd/optional.hpp>
#include <fmt/format.hpp>
#include <iomanip>
#include <sstream>

namespace cctt
{
    namespace util
    {
        namespace
        {
            auto is_printable_in_ascii(char ch) -> bool
            {
                return ('\x21' <= ch && ch <= '\x7e');
            }

            template <class Replace>
            auto replace_char_in_place(std::string& haystack, Replace&& replace)
            {
                for (auto pos = std::string::size_type(0); pos < haystack.size(); ) {
                    if (auto replacement = replace(haystack[pos])) {
                        haystack.replace(pos, 1, *replacement);
                        pos += replacement->size();
                    } else {
                        pos++;
                    }
                }
            }
        }

        auto quote(std::string const& x) -> std::string
        {
            std::ostringstream oss;
            auto delimiter = (x.find('"') == x.npos ? '"' : '\'');
            oss << std::quoted(x, delimiter);
            auto result = oss.str();

            return format_to_oneline(std::move(result));
        }

        auto format_to_oneline(std::string x) -> std::string
        {
            replace_char_in_place(
                x,
                [] (char ch) -> nonstd::optional<std::string> {
                    if (is_printable_in_ascii(ch)) {
                        return {};
                    } else if (ch == '\x20') {
                        return std::string{"␣"};
                    } else {
                        return fmt::format("\\x{:02x}", ch);
                    }
                }
            );

            return x;
        }
    }
}

