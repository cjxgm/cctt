#include "error.hpp"
#include "../util/string.hpp"
#include <fmt/format.hpp>

#define STYLE_NORMAL        "\e[0m"
#define STYLE_LOCATION      "\e[1;36m"
#define STYLE_SOURCE        "\e[1;35m"

namespace cctt
{
    namespace token_tree_internal
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
    }
}

