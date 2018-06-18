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

    [[noreturn]] auto throw_parsing_error_of_missing_pair(
        Source_Location loc,
        Token const* pair,
        char const* missing_pair
    ) -> void
    {
        throw_scanning_error_of_missing_pair(
            loc,
            pair->first,
            pair->last,
            missing_pair
        );
    }

    [[noreturn]] auto throw_parsing_error_of_unpaired_pair(
        Source_Location open_loc,
        Token const* open,
        Source_Location closing_loc,
        Token const* closing
    ) -> void
    {
        auto msg = fmt::format(
            STYLE_LOCATION "{}:{}" STYLE_NORMAL " "
            STYLE_SOURCE "{}" STYLE_NORMAL " and "
            STYLE_LOCATION "{}:{}" STYLE_NORMAL " "
            STYLE_SOURCE "{}" STYLE_NORMAL ": "
            "unmatching pair."
            , open_loc.line
            , open_loc.column
            , util::quote({open->first, open->last})
            , closing_loc.line
            , closing_loc.column
            , util::quote({closing->first, closing->last})
        );
        throw Parsing_Error{msg};
    }
}

