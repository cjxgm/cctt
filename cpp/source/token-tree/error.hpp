#pragma once
#include "token-tree.hpp"
#include <stdexcept>

namespace cctt
{
    struct Parsing_Error: std::runtime_error
    {
        using runtime_error::runtime_error;
    };

    // scanning error: Errors about characters.
    // parsing  error: Errors about tokens.

    [[noreturn]] auto throw_scanning_error(
        Source_Location loc,
        char const* first,
        char const* last,
        char const* reason
    ) -> void;

    [[noreturn]] auto throw_scanning_error_of_missing_pair(
        Source_Location loc,
        char const* first,
        char const* last,
        char const* missing_pair
    ) -> void;

    [[noreturn]] auto throw_parsing_error_of_missing_pair(
        Source_Location loc,
        Token const* pair,
        char const* missing_pair
    ) -> void;

    [[noreturn]] auto throw_parsing_error_of_unpaired_pair(
        Source_Location open_loc,
        Token const* open,
        Source_Location closing_loc,
        Token const* closing
    ) -> void;
}

