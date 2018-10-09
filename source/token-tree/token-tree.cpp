#include "../util/buffer.hpp"
#include "error.hpp"
#include "token-tree.hpp"
#include <algorithm>
#include <cstring>
#include <csignal>

namespace cctt
{
    namespace
    {
        namespace token_tree
        {
            template <std::size_t len>
            auto string_length(char const (&literal)[len]) -> std::size_t
            {
                return len - 1;
            }

            // This won't be selected during overload resolution:
            //
            //     auto string_length(char const* non_literal)
            //
            // That's because of some quirks in C++:
            //
            //   1. The non-templated version is preferred to the templated version.
            //   2. It's ambiguous if the parameter does not take a reference.
            //
            template <class = void>
            auto string_length(char const* & non_literal) -> std::size_t
            {
                return std::strlen(non_literal);
            }

            // Guaranteed self-destruction.
            [[noreturn]] auto unreachable() -> void
            {
                // Remove user-defined signal handlers to make sure
                // that the program will quit no matter what.
                std::signal(SIGSEGV, SIG_DFL);

                std::raise(SIGSEGV);

                // If it doesn't work for some reason, soft lock the program.
                while (true) {}
            }

            struct Start_of_Line_Index final
            {
                Start_of_Line_Index(char const* source)
                    : index{build_start_of_line_index(source)}
                {}

                // Assumes: at < index.end()[-1]; (that is, at - source < strlen(source))
                auto start_of_next_line(char const* at) const -> char const* const&
                {
                    return *std::upper_bound(index.begin(), index.end(), at);
                }

                // Assumes: at >= index[0];
                // Assumes: at < index.end()[-1]; (that is, at - source < strlen(source))
                auto source_location_of(char const* at) const -> Source_Location
                {
                    auto& sonl = start_of_next_line(at);
                    auto sol = (&sonl)[-1];

                    auto line = std::size_t(&sonl - index.begin());
                    auto column = std::size_t(at - sol + 1);

                    return { line, column };
                }

            private:
                util::Buffer<char const*> index;

                static auto build_start_of_line_index(char const* source) -> util::Buffer<char const*>
                {
                    auto index = util::Buffer<char const*>{count_lines(source) + 1};

                    auto p = index.data();
                    *p++ = source;

                    if (*source != '\0') {
                        while (*++source)
                            if (source[-1] == '\n')
                                *p++ = source;

                        // sentinel
                        *p++ = source;
                    }

                    return index;
                }

                static auto count_lines(char const* source) -> std::size_t
                {
                    if (*source == '\0') return 0;

                    std::size_t n{};
                    while (*source) n += (*source++ == '\n');
                    if (source[-1] != '\n') n++;

                    return n;
                }
            };
        }
    }

    struct Token_Tree::Impl final
    {
        Impl(char const* source)
            : source{source}
            , sol_index{source}
        {
            scan();
            build_token_pairs();
            build_token_tree();
        }

        auto begin() const { return tokens.data(); }
        auto   end() const { return tokens.data() + tokens.size() - 1; }

        auto source_location_of(char const* at) const { return sol_index.source_location_of(at); }

    private:
        char const* source;
        token_tree::Start_of_Line_Index sol_index;
        std::vector<Token> tokens;

        auto begin() { return tokens.data(); }
        auto   end() { return tokens.data() + tokens.size() - 1; }

        auto scan() -> void
        {
            // These symbols are special-cased:
            //
            //   >>  ]]
            //
            // because of these valid constructs:
            //
            //   T<U<V>> x      // template closing `>`
            //   x[y[i]]        // array index closing `]`
            //
            //
            // These are disallowed by the C++ Standard:
            //
            // - The `>=` in
            //
            //     template <class T> T x;
            //     x<int>=10;
            //
            //   Spaces are required between `>` and `=`, as in:
            //
            //     x<int> =10;
            //
            // - The `[[` in
            //
            //     x[[] { return 1; }];
            //
            //   Spaces are required between the two `[`, as in:
            //
            //     x[ [] { return 1; }];
            //
            // But, since our token-tree parser is sloppy,
            // we accept them, i.e. they will be special cased, too.
            //
            //
            // Thus, it is decided that:
            //
            //   `>` can be combined into `->`, but NOT any other symbol.
            //   `<` can be combined into `<<` and `<=`.
            //   `[` does NOT combine into other symbols.
            //   `]` does NOT combine into other symbols.
            //
            //
            // `/` has many followings so it will be special cased:
            //
            //    /  //  /=  /*
            //
            //
            // `-` has many followings so it will be special cased:
            //
            //    -  --  -=  ->
            //
            //
            // Since we are being sloppy, `$` in identifiers are accepted.
            // And "`" and "@" are accepted as symbols.
            // The so-called UTF-8 BOM at the beginning of file is also accepted.

            #define CASE_LOWER \
                case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g': \
                case 'h': case 'i': case 'j': case 'k': case 'l': case 'm': case 'n': \
                case 'o': case 'p': case 'q': case 'r': case 's': case 't': \
                case 'u': case 'v': case 'w': case 'x': case 'y': case 'z'
            #define CASE_UPPER \
                case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': \
                case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N': \
                case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': \
                case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z'
            #define CASE_DIGIT \
                case '0': case '1': case '2': case '3': case '4': \
                case '5': case '6': case '7': case '8': case '9'
            #define CASE_DIGIT_WITH_SEPARATOR \
                case '\'': \
                CASE_DIGIT
            #define CASE_WHITESPACE \
                case '\x20': case '\r': case '\n': \
                case '\t': case '\f': case '\v'
            #define CASE_INVALID_IDENT_FIRST \
                case '$'
            #define CASE_IDENT_FIRST \
                CASE_LOWER: \
                CASE_UPPER: \
                case '_': \
                CASE_INVALID_IDENT_FIRST
            #define CASE_IDENT_REST \
                CASE_IDENT_FIRST: \
                CASE_DIGIT
            #define CASE_ARITH_ASSIGN_OR_DOUBLE_FIRST \
                case '+': case '&': case '|': case '<'
            #define CASE_ARITH_ASSIGN_FIRST \
                case '=': case '!': case '*': case '^'
            #define CASE_DOUBLE_FIRST \
                case ':'
            #define CASE_INVALID_SINGLE_SYMBOL \
                case '`': case '@'
            #define CASE_SINGLE_SYMBOL \
                case '>': case '(': case ')': case '[': case ']': case '{': case '}': \
                case ',': case '?': case ';': case '~': case '%': case '\\': \
                CASE_INVALID_SINGLE_SYMBOL
            #define CASE_RAW_STRING_DELIMITER_BLACKLIST \
                CASE_WHITESPACE: \
                case ')': case '\\'

            auto first = source;
            auto  last = source;

            tokens.reserve(estimate_token_count(source));
            auto commit = [&] (auto... tags) {
                tokens.emplace_back(first, last, Token_Tag_Set{tags...});
            };

            auto abort = [&] (char const* reason) {
                auto loc = sol_index.source_location_of(first);
                throw_scanning_error(loc, first, last, reason);
            };

            auto abort_missing = [&] (char const* pair) {
                auto loc = sol_index.source_location_of(first);
                throw_scanning_error_of_missing_pair(loc, first, last, pair);
            };

            // Make sure `last` is on current line!
            // You probably need some kind of `last--`.
            auto skip_until_next_line = [&, this] {
                while (true) {
                    last = sol_index.start_of_next_line(last);
                    if (*last == '\0') break;

                    auto p = last;
                    if (p > first && p[-1] == '\n') p--;
                    if (p > first && p[-1] == '\r') p--;
                    if (p > first && p[-1] == '\\') continue;

                    break;
                }
            };

            auto skip_after_non_escaped_ch = [&] (char target) {
                for (auto p=last; *p; p++) {
                    if (*p == '\\') { p++; continue; }
                    if (*p != target) continue;

                    last = p + 1;
                    return;
                }

                char pair[2] = { target, '\0' };
                abort_missing(pair);
            };

            auto skip_after_str_of_known_length = [&] (char const* target, std::size_t target_len) {
                if (auto p = std::strstr(last, target)) {
                    last = p + target_len;
                } else {
                    abort_missing(target);
                }
            };

            auto skip_after_str = [&] (auto& target) {
                skip_after_str_of_known_length(target, token_tree::string_length(target));
            };

            auto skip_digits = [&] {
                switch (*last) {
                    CASE_DIGIT: last++; break;
                    default: return false;
                }

                while (true) {
                    switch (*last) {
                        CASE_DIGIT_WITH_SEPARATOR: last++; break;
                        default: return true;
                    }
                }
            };

            // Skip the so-called UTF-8 BOM at the beginning of file.
            if (last[0] == '\xef' && last[1] == '\xbb' && last[2] == '\xbf')
                last += 3;

            while (*last) {
                first = last;

                switch (*last++) {
                    CASE_WHITESPACE:
                        break;

                    CASE_SINGLE_SYMBOL:
                        commit(Token_Tag::symbol);
                        break;

                    CASE_ARITH_ASSIGN_OR_DOUBLE_FIRST:
                        if (*last == '=' || *last == *first) last++;
                        commit(Token_Tag::symbol);
                        break;

                    CASE_ARITH_ASSIGN_FIRST:
                        if (*last == '=') last++;
                        commit(Token_Tag::symbol);
                        break;

                    CASE_DOUBLE_FIRST:
                        if (*last == *first) last++;
                        commit(Token_Tag::symbol);
                        break;

                    case '.':
                        if (skip_digits()) {
                            commit(Token_Tag::literal, Token_Tag::number);
                        } else {
                            if (last[0] == '.' && last[1] == '.') last += 2;
                            commit(Token_Tag::symbol);
                        }
                        break;

                    case '-':
                        if (*last == '-' || *last == '=' || *last == '>') last++;
                        commit(Token_Tag::symbol);
                        break;

                    case '#':
                        last--;
                        skip_until_next_line();
                        // no commit(...) to ignore directives
                        break;

                    case '/':
                        if (*last == '/') {
                            skip_until_next_line();
                            // no commit(...) to ignore single-line comments
                            break;
                        }

                        if (*last == '*') {
                            last++;
                            skip_after_str("*/");
                            // no commit(...) to ignore multi-line comments
                            break;
                        }

                        if (*last == '=') last++;
                        commit(Token_Tag::symbol);
                        break;

                    case '"':
                        skip_after_non_escaped_ch('"');
                        commit(Token_Tag::literal, Token_Tag::string, Token_Tag::line);
                        break;

                    case '\'':
                        skip_after_non_escaped_ch('\'');
                        commit(Token_Tag::literal, Token_Tag::character);
                        break;

                    // Sloppy numbers
                    CASE_DIGIT: {
                        skip_digits();
                        if (*last == '.') last++;
                        skip_digits();
                        commit(Token_Tag::literal, Token_Tag::number);
                        break;
                    }

                    CASE_IDENT_FIRST: {
                        auto is_ident = true;
                        while (is_ident) {
                            switch (*last) {
                                CASE_IDENT_REST: last++; break;
                                default: is_ident = false; break;
                            }
                        }

                        // raw strings may starts with:
                        //     R"  u8R"  uR"  UR"  LR"
                        if (last[0] == '"' && last[-1] == 'R') {
                            last++;

                            constexpr auto max_delimiter_length = 16;   // defined by the C++ Standard
                            char closing[1+max_delimiter_length+1+1];
                            auto p = closing;
                            *p++ = ')';

                            auto is_delimiter = true;
                            for (int i=0; is_delimiter && i < max_delimiter_length; i++) {
                                switch (*last) {
                                    case '\0':
                                        abort("raw string requires R\"DELIMITER( )DELIMITER\"");
                                        token_tree::unreachable();

                                    CASE_RAW_STRING_DELIMITER_BLACKLIST:
                                        last++;
                                        abort("invalid raw string delimiter.");
                                        token_tree::unreachable();

                                    case '(':
                                        is_delimiter = false;
                                        last++;
                                        break;

                                    default:
                                        *p++ = *last++;
                                        break;
                                }
                            }

                            *p++ = '"';
                            *p = '\0';

                            skip_after_str_of_known_length(closing, p-closing);
                            commit(Token_Tag::literal, Token_Tag::string, Token_Tag::block);
                        } else {
                            commit(Token_Tag::identifier);
                        }
                        break;
                    }

                    default:
                        abort("unknown character.");
                        token_tree::unreachable();
                }
            }

            // sentinel
            first = last;
            commit(Token_Tag::end);
        }

        auto build_token_pairs() -> void
        {
            #define CASE_AMBIGUOUS_OPEN_SYMBOL \
                case '<'
            #define CASE_AMBIGUOUS_CLOSING_SYMBOL \
                case '>'
            #define CASE_OPEN_SYMBOL \
                case '(': case '[': case '{'
            #define CASE_CLOSING_SYMBOL \
                case ')': case ']': case '}'
            #define CASE_DISAMBIGUATING_SYMBOL \
                case ';': \
                CASE_CLOSING_SYMBOL

            auto is_ambiguous_open_symbol = [] (char ch) {
                switch (ch) {
                    CASE_AMBIGUOUS_OPEN_SYMBOL: return true;
                    default: return false;
                }
            };

            auto paired_open_symbol_of = [] (char ch) {
                switch (ch) {
                    case '>': return '<';
                    case ')': return '(';
                    case ']': return '[';
                    case '}': return '{';
                    default: token_tree::unreachable();
                }
            };

            auto paired_closing_symbol_of = [] (char ch) {
                switch (ch) {
                    case '<': return '>';
                    case '(': return ')';
                    case '[': return ']';
                    case '{': return '}';
                    default: token_tree::unreachable();
                }
            };

            // Assume `tk` is a token of single-character symbol
            auto symbol_of = [] (Token const* tk) {
                return tk->first[0];
            };

            auto abort_unpaired = [&, this] (Token const* open, Token const* closing) {
                if (open && closing) {
                    auto open_loc = sol_index.source_location_of(open->first);
                    auto closing_loc = sol_index.source_location_of(closing->first);
                    throw_parsing_error_of_unpaired_pair(
                        open_loc, open,
                        closing_loc, closing
                    );
                }

                if (open) {
                    auto loc = sol_index.source_location_of(open->first);
                    char missing_pair[] = { paired_closing_symbol_of(symbol_of(open)), '\0' };
                    throw_parsing_error_of_missing_pair(loc, open, missing_pair);
                }

                if (closing) {
                    auto loc = sol_index.source_location_of(closing->first);
                    char missing_pair[] = { paired_open_symbol_of(symbol_of(closing)), '\0' };
                    throw_parsing_error_of_missing_pair(loc, closing, missing_pair);
                }

                token_tree::unreachable();
            };

            std::vector<Token*> blocks;
            blocks.reserve(tokens.size());

            for (auto& token: *this) {
                if (token.tags.has_none_of(Token_Tag::symbol)) continue;
                if (token.last - token.first != 1) continue;

                auto tk = &token;
                auto sym = symbol_of(tk);

                switch (sym) {
                    CASE_AMBIGUOUS_OPEN_SYMBOL:
                    CASE_OPEN_SYMBOL:
                        blocks.emplace_back(tk);
                        break;

                    default:
                        break;
                }

                switch (sym) {
                    CASE_DISAMBIGUATING_SYMBOL:
                        while (!blocks.empty() && is_ambiguous_open_symbol(symbol_of(blocks.back())))
                            blocks.pop_back();
                        break;

                    default:
                        break;
                }

                switch (sym) {
                    CASE_AMBIGUOUS_CLOSING_SYMBOL:
                    CASE_CLOSING_SYMBOL:
                        if (blocks.empty()) {
                            switch (sym) {
                                CASE_AMBIGUOUS_CLOSING_SYMBOL: break;
                                default:
                                    abort_unpaired(nullptr, tk);
                                    token_tree::unreachable();
                            }
                        } else {
                            auto open_token = blocks.back();
                            auto open_symbol = symbol_of(open_token);

                            if (open_symbol == paired_open_symbol_of(sym)) {
                                blocks.pop_back();
                                open_token->pair = tk;
                                tk->pair = open_token;
                            } else {
                                switch (sym) {
                                    CASE_AMBIGUOUS_CLOSING_SYMBOL: break;
                                    default:
                                        abort_unpaired(open_token, tk);
                                        token_tree::unreachable();
                                }
                            }
                        }
                        break;

                    default:
                        break;
                }
            }

            while (!blocks.empty() && is_ambiguous_open_symbol(symbol_of(blocks.back())))
                blocks.pop_back();

            if (!blocks.empty()) {
                abort_unpaired(blocks.back(), nullptr);
                token_tree::unreachable();
            }
        }

        auto build_token_tree() -> void
        {
            std::vector<Token const*> parents;
            parents.reserve(tokens.size());
            parents.emplace_back(nullptr);

            for (auto& token: *this) {
                auto tk = &token;

                if (tk->pair == nullptr) {
                    tk->parent = parents.back();
                    continue;
                }

                if (tk->pair > tk) {
                    tk->parent = parents.back();
                    parents.emplace_back(tk);
                    continue;
                }

                if (tk->pair < tk) {
                    parents.pop_back();
                    tk->parent = parents.back();
                    continue;
                }

                token_tree::unreachable();
            }
        }

        static auto estimate_token_count(char const* source) -> std::size_t
        {
            constexpr auto least_token_count = std::size_t(1024);
            constexpr auto length_count_ratio = std::size_t(4);
            constexpr auto least_length = least_token_count * length_count_ratio;

            auto len = token_tree::string_length(source);
            if (len <= least_length) {
                return least_token_count;
            } else {
                return len / length_count_ratio;
            }
        }
    };

    Token_Tree::Token_Tree(char const* source)
        : impl{std::make_unique<Impl>(source)}
    {}

    Token_Tree::~Token_Tree() = default;

    auto Token_Tree::begin() const -> Token const*
    {
        auto const* const_impl = impl.get();
        return const_impl->begin();
    }

    auto Token_Tree::end() const -> Token const*
    {
        auto const* const_impl = impl.get();
        return const_impl->end();
    }

    auto Token_Tree::source_location_of(char const* at) const -> Source_Location
    {
        auto const* const_impl = impl.get();
        return const_impl->source_location_of(at);
    }
}

