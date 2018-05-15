#include "../util/slice.hpp"
#include "../util/string.hpp"
#include "token-stream.hpp"
#include "token.hpp"
#include <fmt/format.hpp>
#include <fmt/ostream.hpp>
#include <stdexcept>

#define CCTT_FALLTHROUGH() // intentional fall through

namespace cctt
{
    namespace
    {
        util::String_Slice long_symbols[] {
            "->", "::", "++", "--",
            "&&", "||", "...",
            "==", "!=", "<=", ">=",
            "+=", "-=", "*=", "/=",
            "&=", "|=", "^=",
        };

        util::String_Slice raw_string_open_delimiters[] {
            "R\"",  "u8R\"",
            "uR\"", "UR\"", "LR\"",
        };
    }

    struct Token_Stream::Impl
    {
        using String_Slice = util::String_Slice;

        Impl(char const* first, char const* last)
            : next_loc{first, first, 1, 1}
            , source_last{last}
        {}

        auto next() -> nonstd::optional<Token>
        {
            skip_whitespace();
            auto old_loc = next_loc;

            if (is_eof()) return {};

            // raw string
            for (auto open_delim: raw_string_open_delimiters) {
                if (advance_for(open_delim)) {
                    auto rest = rest_source();
                    auto n = rest.search("(");
                    if (n == -1)
                        throw Lexing_Error{"Raw strings should have delimiters", Token_Span{old_loc, next_loc}};

                    auto raw_delim = String_Slice(next_loc.at, n);
                    advance(n + 1);
                    rest = rest_source();

                    while (true) {
                        n = rest.search(raw_delim);
                        if (n == -1) {
                            auto raw_delim_str = std::string(raw_delim.data(), raw_delim.size());
                            auto end_delim = ")" + raw_delim_str + "\"";
                            throw Lexing_Error{
                                "Missing " + util::quote(end_delim) + " for the raw string.",
                                Token_Span{old_loc, next_loc},
                            };
                        }

                        auto prev = rest[n-1];
                        rest = rest.skip(n + raw_delim.size());
                        auto succ = (rest.size() == 0 ? -1 : int(rest[0]));

                        if (prev == ')' && succ == '"') break;
                    }

                    auto offset = int(rest.data() - rest_source().data());
                    advance(offset + 1);    // +1 for the last '"'

                    return Token{
                        old_loc,
                        next_loc,
                        { Token_Tag::literal, Token_Tag::string, Token_Tag::block },
                    };
                }
            }

            // identifier
            if (util::is_cpp_alpha(current_char())) {
                advance();

                while (util::is_cpp_alnum(current_char()))
                    advance();

                return Token{
                    old_loc,
                    next_loc,
                    { Token_Tag::identifier },
                };
            }

            // decimal numbers
            if (util::is_cpp_number(current_char())) {
                advance();

                while (util::is_cpp_number(current_char()))
                    advance();

                return Token{
                    old_loc,
                    next_loc,
                    { Token_Tag::literal, Token_Tag::number },
                };
            }

            for (auto sym: long_symbols) {
                if (advance_for(sym)) {
                    return Token{
                        old_loc,
                        next_loc,
                        { Token_Tag::symbol },
                    };
                }
            }

            switch (current_char()) {
                // directive
                case '#': {
                    advance_line_escapable();
                    return Token{
                        old_loc,
                        next_loc,
                        { Token_Tag::ignore, Token_Tag::directive, Token_Tag::line },
                    };
                }

                case '/': {
                    auto start_loc = next_loc;
                    advance();

                    // line comment
                    if (advance_for('/')) {
                        advance_line_escapable();
                        return Token{
                            old_loc,
                            next_loc,
                            { Token_Tag::ignore, Token_Tag::comment, Token_Tag::line },
                        };
                    }

                    // block comment
                    if (advance_for('*')) {
                        if (!advance_till_after("*/"))
                            throw Lexing_Error{"Missing \"*/\".", Token_Span{start_loc, next_loc}};

                        return Token{
                            old_loc,
                            next_loc,
                            { Token_Tag::ignore, Token_Tag::comment, Token_Tag::block },
                        };
                    }

                    // single-char symbol '/'
                    return Token{
                        old_loc,
                        next_loc,
                        { Token_Tag::symbol },
                    };
                }

                // string
                case '"': {
                    advance();
                    auto end_loc = next_loc;
                    if (!advance_quoted<'"'>())
                        throw Lexing_Error{"Missing '\"'.", Token_Span{old_loc, end_loc}};

                    return Token{
                        old_loc,
                        next_loc,
                        { Token_Tag::literal, Token_Tag::string },
                    };
                }

                // char
                case '\'': {
                    advance();
                    auto end_loc = next_loc;
                    if (!advance_quoted<'\''>())
                        throw Lexing_Error{"Missing \"'\".", Token_Span{old_loc, end_loc}};

                    return Token{
                        old_loc,
                        next_loc,
                        { Token_Tag::literal, Token_Tag::character },
                    };
                }

                // additional single-char symbols
                case '<': CCTT_FALLTHROUGH();
                case '>': CCTT_FALLTHROUGH();
                case '(': CCTT_FALLTHROUGH();
                case ')': CCTT_FALLTHROUGH();
                case '[': CCTT_FALLTHROUGH();
                case ']': CCTT_FALLTHROUGH();
                case '{': CCTT_FALLTHROUGH();
                case '}': CCTT_FALLTHROUGH();
                case ',': CCTT_FALLTHROUGH();
                case '.': CCTT_FALLTHROUGH();
                case '?': CCTT_FALLTHROUGH();
                case ';': CCTT_FALLTHROUGH();
                case ':': CCTT_FALLTHROUGH();
                case '|': CCTT_FALLTHROUGH();
                case '~': CCTT_FALLTHROUGH();
                case '!': CCTT_FALLTHROUGH();
                case '%': CCTT_FALLTHROUGH();
                case '^': CCTT_FALLTHROUGH();
                case '&': CCTT_FALLTHROUGH();
                case '*': CCTT_FALLTHROUGH();
                case '-': CCTT_FALLTHROUGH();
                case '=': CCTT_FALLTHROUGH();
                case '+': {
                    advance();
                    return Token{
                        old_loc,
                        next_loc,
                        { Token_Tag::symbol },
                    };
                }

                default: break;
            }

            {
                auto ch = current_char();
                advance();
                throw Lexing_Error{
                    fmt::format("Unknown character 0x{:02x}.", ch),
                    Token_Span{old_loc, next_loc},
                };
            }
        }

    private:
        Token_Location next_loc;
        char const* source_last;
        char prev_char = '\0';

        auto rest_source() const -> String_Slice { return {next_loc.at, source_last}; }
        auto is_eof() const -> bool { return (rest_source().size() == 0); }
        auto current_char() const -> int { return peek(0); }

        auto peek(int ahead=1) const -> int
        {
            auto rest = rest_source().skip(ahead);
            return (rest.size() == 0 ? -1 : int(rest[0]));
        }

        auto expect(String_Slice expected) const -> bool
        {
            return rest_source().starts_with(expected);
        }

        void update_line_info(String_Slice x)
        {
            auto new_line = [this] {
                next_loc.line++;
                next_loc.column = 1;
            };

            auto next_char = [this] {
                next_loc.column++;
            };

            for (auto ch: x) {
                switch (ch) {
                    case '\r':
                        new_line();
                        break;
                    case '\n':
                        if (prev_char != '\r')
                            new_line();
                        break;
                    default:
                        next_char();
                        break;
                }
                prev_char = ch;
            }
        }

        void advance(char const* to)
        {
            update_line_info({next_loc.at, to});
            next_loc.at = to;
        }

        void advance(int n=1)
        {
            advance(rest_source().skip(n).data());
        }

        template <class Fn>
        auto advance_if(Fn fn) -> bool
        {
            if (auto opt_rest = fn(rest_source())) {
                advance(opt_rest->data());
                return true;
            } else {
                return false;
            }
        }

        auto advance_for(String_Slice expected) -> bool
        {
            return advance_if(
                [expected] (String_Slice rest) -> nonstd::optional<String_Slice> {
                    if (rest.starts_with(expected))
                        return rest.skip(expected.size());
                    return {};
                }
            );
        }

        auto advance_for(char expected) -> bool
        {
            if (current_char() == expected) {
                advance();
                return true;
            } else {
                return false;
            }
        }

        auto advance_till_after(String_Slice end_delimiter) -> bool
        {
            auto n = rest_source().search(end_delimiter);
            if (n == -1) {
                return false;
            } else {
                advance(n + end_delimiter.size());
                return true;
            }
        }

        void advance_line_escapable()
        {
            while (true) {
                switch (current_char()) {
                    case -1: return;
                    case '\r': return;
                    case '\n': return;
                    case '\\': {
                        advance();
                        if (advance_for('\r')) {
                            advance_for('\n');
                        } else {
                            advance();
                        }
                        break;
                    }
                    default: {
                        advance();
                        break;
                    }
                }
            }
        }

        template <char delimiter, char escape='\\'>
        auto advance_quoted() -> bool
        {
            while (true) {
                switch (current_char()) {
                    case -1: return false;
                    case escape: {
                        advance();
                        if (advance_for('\r')) {
                            advance_for('\n');
                        } else {
                            advance();
                        }
                        break;
                    }
                    case delimiter: {
                        advance();
                        return true;
                    }
                    default: {
                        advance();
                        break;
                    }
                }
            }
        }

        void skip_whitespace()
        {
            auto skip_once = [this] {
                switch (current_char()) {
                    case '\x20': advance(); return true;
                    case '\r': advance(); advance_for('\n'); return true;
                    case '\n': advance(); return true;
                    case '\t': advance(); return true;
                    case '\f': advance(); return true;
                    case '\v': advance(); return true;
                    default: return false;
                }
            };

            while (skip_once()) {}
        }
    };

    Token_Stream::Token_Stream(char const* first, char const* last)
        : impl{std::make_unique<Impl>(first, last)}
    {}

    Token_Stream::~Token_Stream() = default;

    auto Token_Stream::next() -> nonstd::optional<Token> { return impl->next(); }

    namespace
    {
        auto format_error(char const* reason, Token_Span at) -> std::string
        {
            return fmt::format("{} at {}: {}", util::quote(at.source_string()), at.first, reason);
        }
    }

    Lexing_Error::Lexing_Error(char const* reason, Token_Span at)
        : runtime_error{format_error(reason, at)}
        , at{at}
    {}
}

