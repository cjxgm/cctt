#pragma once
#include "../util/flag-set.hpp"
#include <iosfwd>
#include <string>

namespace cctt
{
    struct Token_Location
    {
        char const* source;
        char const* at;
        int line;
        int column;

        Token_Location(char const* source, char const* at, int line, int column)
            : source{source}, at{at}, line{line}, column{column}
        {}
    };

    struct Token_Span
    {
        Token_Location first;
        Token_Location last;

        Token_Span(Token_Location first, Token_Location last)
            : first{first}, last{last}
        {}

        auto source_string() const -> std::string;
    };

    enum struct Token_Tag
    {
        ignore,
        identifier,
        symbol,
        literal,

        directive,
        comment,
        number,
        string,
        character,

        block,
        line,

        last_tag_,
    };
    using Token_Tag_Set = util::Flag_Set<Token_Tag, Token_Tag::last_tag_>;

    struct Token
    {
        Token_Span span;
        Token_Tag_Set tags;

        Token(Token_Span span, Token_Tag_Set tags)
            : span{span}, tags{tags}
        {}

        Token(Token_Location first, Token_Location last, Token_Tag_Set tags)
            : Token{Token_Span{first, last}, tags}
        {}
    };

    auto operator << (std::ostream& o, Token_Location const& loc) -> std::ostream&;
    auto operator << (std::ostream& o, Token_Span const& span) -> std::ostream&;
    auto operator << (std::ostream& o, Token const& tk) -> std::ostream&;
}

