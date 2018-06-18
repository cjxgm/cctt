#include "error.hpp"
#include "../util/string.hpp"
#include <fmt/format.hpp>

#include "../util/style.inl"

namespace cctt
{
    [[noreturn]] auto throw_scanning_error(
        Source_Location loc,
        char const* first,
        char const* last,
        char const* reason
    ) -> void
    {
        auto msg = fmt::format(
            STYLE_LOCATION "{}:{}" STYLE_NORMAL " "
            STYLE_SOURCE "{}" STYLE_NORMAL ": {}"
            , loc.line
            , loc.column
            , util::quote({first, last})
            , reason
        );
        throw Parsing_Error{msg};
    }

    [[noreturn]] auto throw_scanning_error_of_missing_pair(
        Source_Location loc,
        char const* first,
        char const* last,
        char const* missing_pair
    ) -> void
    {
        auto reason = fmt::format(
            "missing paired " STYLE_SOURCE "{}" STYLE_NORMAL "."
            , util::quote(missing_pair)
        );
        throw_scanning_error(loc, first, last, reason.data());
    }

    [[noreturn]] auto throw_parsing_error(
        Source_Location loc,
        Token const* tk,
        char const* reason
    ) -> void
    {
        throw_scanning_error(loc, tk->first, tk->last, reason);
    }

    [[noreturn]] auto throw_parsing_error2(
        Source_Location loc0,
        Token const* tk0,
        Source_Location loc1,
        Token const* tk1,
        char const* reason
    ) -> void
    {
        auto msg = fmt::format(
            STYLE_LOCATION "{}:{}" STYLE_NORMAL " "
            STYLE_SOURCE "{}" STYLE_NORMAL " and "
            STYLE_LOCATION "{}:{}" STYLE_NORMAL " "
            STYLE_SOURCE "{}" STYLE_NORMAL ": "
            "{}"
            , loc0.line
            , loc0.column
            , util::quote({tk0->first, tk0->last})
            , loc1.line
            , loc1.column
            , util::quote({tk1->first, tk1->last})
            , reason
        );
        throw Parsing_Error{msg};
    }

    [[noreturn]] auto throw_parsing_error_of_missing_pair(
        Source_Location loc,
        Token const* pair,
        char const* missing_pair
    ) -> void
    {
        auto reason = fmt::format(
            "missing paired " STYLE_SOURCE "{}" STYLE_NORMAL "."
            , util::quote(missing_pair)
        );
        throw_parsing_error(loc, pair, reason.data());
    }

    [[noreturn]] auto throw_parsing_error_of_unpaired_pair(
        Source_Location open_loc,
        Token const* open,
        Source_Location closing_loc,
        Token const* closing
    ) -> void
    {
        throw_parsing_error2(open_loc, open, closing_loc, closing, "unmatching pair.");
    }
}

