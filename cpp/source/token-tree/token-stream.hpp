#pragma once
#include "../util/stream.hpp"
#include "token.hpp"
#include <nonstd/optional.hpp>
#include <memory>
#include <string>
#include <stdexcept>

namespace cctt
{
    struct Token_Stream: util::Stream<Token_Stream>
    {
        using value_type = Token;

        Token_Stream(char const* first, char const* last);
        ~Token_Stream();

        Token_Stream(std::string const& source): Token_Stream{&*source.begin(), &*source.rbegin() + 1} {}

        auto next() -> nonstd::optional<Token>;

    private:
        struct Impl;
        std::unique_ptr<Impl> impl;
    };

    struct Lexing_Error: std::runtime_error
    {
        Token_Span at;

        Lexing_Error(char const* reason, Token_Span at);
        Lexing_Error(std::string const& reason, Token_Span at)
            : Lexing_Error{reason.data(), at}
        {}
    };
}

