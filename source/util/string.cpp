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
            oss << std::quoted(x);
            auto result = oss.str();

            return format_to_oneline(std::move(result));
        }

        auto quote_without_delimiters(std::string const& x) -> std::string
        {
            auto quoted = quote(x);
            return quoted.substr(1, quoted.size() - 2);
        }

        auto format_to_oneline(std::string x) -> std::string
        {
            replace_char_in_place(
                x,
                [] (char ch) -> nonstd::optional<std::string> {
                    if (is_printable_in_ascii(ch)) {
                        return {};
                    } else {
                        switch (ch) {
                            case '\x20': return std::string{"␣"};
                            case '\t': return std::string{"\\t"};
                            case '\n': return std::string{"\\n"};
                            case '\r': return std::string{"\\r"};
                            case '\f': return std::string{"\\f"};
                            case '\v': return std::string{"\\v"};
                            default: return fmt::format("\\x{:02x}", ch);
                        }
                    }
                }
            );

            return x;
        }
    }
}

