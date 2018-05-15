#pragma once
#include "../util/flag-set.hpp"
#include <cstdint>

namespace cctt
{
    enum struct Token_Tag: std::uint32_t
    {
        end,
        identifier,
        symbol,
        literal,

        number,
        string,
        character,

        block,
        line,

        last_tag_,
    };

    using Token_Tag_Set = util::Flag_Set<Token_Tag, Token_Tag::last_tag_>;

    struct Token final
    {
        char const* first;
        char const* last;
        Token_Tag_Set tags;

        Token(char const* first, char const* last, Token_Tag_Set tags)
            : first{first}, last{last}, tags{tags}
        {}
    };

    static_assert(sizeof(Token) <= 64, "Token is too big to fit into a typical cacheline.");
}

