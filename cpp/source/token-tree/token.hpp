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

        Token const* pair{};
        Token const* parent{};

        Token(char const* first, char const* last, Token_Tag_Set tags)
            : first{first}, last{last}, tags{tags}
        {}

        auto is_end() const -> bool { return tags.has_all_of(Token_Tag::end); }
        auto is_leaf() const -> bool { return (pair == nullptr); }
        auto child() const -> Token const* { return (pair > this + 1 ? this + 1 : nullptr); }
        auto last_child() const -> Token const* { return (pair > this + 1 ? pair : nullptr); }
        auto closing_pair() const -> Token const* { return (pair > this ? pair : this); }
        auto next() const -> Token const* { return closing_pair() + 1; }
    };

    static_assert(sizeof(Token) <= 64, "Token is too big to fit into a typical cacheline.");
}

