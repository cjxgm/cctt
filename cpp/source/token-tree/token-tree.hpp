#pragma once
#include "token.hpp"
#include <memory>
#include <string>
#include <vector>
#include <cstddef>      // for std::size_t

namespace cctt
{
    struct Token_Tree final
    {
        // source must be zero-terminated.
        Token_Tree(char const* source);
        ~Token_Tree();

        Token_Tree(std::string const& x): Token_Tree{x.data()} {}

        // begin() returns pointer to the first token (which will be end() if there is no token).
        // end()   returns pointer to the token with Token_Tag::end.
        //
        // It is guaranteed that accessing *begin() and *end() is never undefined behavior.
        // Accessing *(end()+1) is undefined behavior.
        //
        // It is guaranteed that the sequence [begin(), end()] lies on a continuous memory.
        auto begin() const -> Token const*;
        auto   end() const -> Token const*;

    private:
        struct Impl;
        std::unique_ptr<Impl> impl;
    };

    struct Source_Location final
    {
        std::size_t line;
        std::size_t column;
    };
}

