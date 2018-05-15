#pragma once
#include "token-tree.hpp"
#include <stdexcept>

namespace cctt
{
    struct Parsing_Error: std::runtime_error
    {
        using runtime_error::runtime_error;
    };

    namespace token_tree_internal
    {
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
    }
}

