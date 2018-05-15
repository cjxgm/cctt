#include "../util/string.hpp"
#include "token.hpp"
#include <iostream>
#include <fmt/format.hpp>
#include <fmt/ostream.hpp>

namespace cctt
{
    auto Token_Span::source_string() const -> std::string
    {
        return {
            first.at,
            last.at,
        };
    }

    auto operator << (std::ostream& o, Token_Location const& loc) -> std::ostream&
    {
        return (o << fmt::format("{:04d}:{:04d}(0x{:04x})", loc.line, loc.column, loc.at - loc.source));
    }

    auto operator << (std::ostream& o, Token_Span const& span) -> std::ostream&
    {
        return (o << fmt::format("[{} -> {}]", span.first, span.last));
    }

    auto operator << (std::ostream& o, Token const& tk) -> std::ostream&
    {
        return (o << fmt::format("({:016b}) {} {}", tk.tags.get(), tk.span, util::quote(tk.span.source_string())));
    }
}

