#include "../token-tree/error.hpp"
#include "introspect.hpp"
#include <vector>
#include <cstddef>

namespace cctt
{
    namespace
    {
        template <std::size_t len, class... Tags>
        auto token_is(Token const* tk, char const (&text)[len], Tags... subset) -> bool
        {
            if (!tk->tags.has_all_of({subset...})) return false;
            if (tk->last - tk->first != len - 1) return false;
            for (auto a=tk->first, b=text; *b; a++, b++)
                if (*a != *b)
                    return false;
            return true;
        }

        // Parse this pattern:
        //
        //   CCTT_INTROSPECT ( .... ) ....
        //           ^                 ^
        //           |                 `-- tk will be here if succeeds.
        //           `-------------------- return value will be this if succeeds.
        //
        // If failed, returns nullptr and tk is not modified.
        // Throws if the pattern appears at illegal places.
        auto parse_introspect_attribute(Token_Tree const& tt, Token const* & tk) -> Token const*
        {
            if (!token_is(tk+0, "CCTT_INTROSPECT", Token_Tag::identifier)) return nullptr;

            auto content = tk;

            if (!token_is(tk+1, "(", Token_Tag::symbol)) {
                auto bad = tk+1;
                auto bad_loc = tt.source_location_of(bad->first);
                auto content_loc = tt.source_location_of(content->first);
                throw_parsing_error2(bad_loc, bad, content_loc, content, "missing parenthesis `()`. CCTT_INTROSPECT() or CCTT_INTROSPECT(arguments) expected.");
            }

            // Ensure introspection appears at legal places
            for (auto p=tk->parent; p; p=p->parent) {
                if (!token_is(p, "{", Token_Tag::symbol)) {
                    auto bad = p;
                    auto bad_loc = tt.source_location_of(bad->first);
                    auto content_loc = tt.source_location_of(content->first);
                    throw_parsing_error2(bad_loc, bad, content_loc, content, "introspection must be directly inside a named namespace.");
                }

                if (p-1 >= tt.begin() && token_is(p-1, "namespace", Token_Tag::identifier)) {
                    auto loc = tt.source_location_of(p[-1].first);
                    throw_parsing_error(loc, p-1, "anonymous namespaces cannot be introspected.");
                }

                if (p-2 >= tt.begin() && !token_is(p-2, "namespace", Token_Tag::identifier)) {
                    auto bad = p - 2;
                    auto bad_loc = tt.source_location_of(bad->first);
                    auto content_loc = tt.source_location_of(content->first);
                    throw_parsing_error2(bad_loc, bad, content_loc, content, "introspection must be directly inside a named namespace.");
                }
            }

            tk = tk[1].next();
            return content;
        }

        // Parse this pattern:
        //
        //   namespace @name { ....
        //               ^       ^
        //               |       `-- tk will be here if succeeds.
        //               `---------- return value will be this if succeeds.
        //
        // If failed, returns nullptr and tk is not modified.
        auto parse_namespace_heading(Token const* & tk) -> Token const*
        {

            if (!token_is(tk, "namespace", Token_Tag::identifier)) return nullptr;
            if (tk[1].tags.has_none_of({Token_Tag::identifier})) return nullptr;
            if (!token_is(tk+2, "{", Token_Tag::symbol)) return nullptr;

            auto name = tk + 1;
            tk += 3;
            return name;
        }

        // Parse these patterns:
        //
        //   enum [struct|class] @name [: ....] { ....
        //                         ^                ^
        //                         |                `-- tk will be here if succeeds.
        //                         `------------------- return value will be this if succeeds.
        //
        //   enum [struct|class] [: ....] { ....
        //                                    ^
        //                                    +-- tk will be here if succeeds.
        //                                    `-- return value will be this if succeeds.
        //
        // When encountered this pattern, exceptions are thrown:
        //
        //   enum [struct|class] [@name] [: ....] ;
        //
        // If none of the above patterns match, returns nullptr and tk is not modified.
        auto parse_enum_heading(Token_Tree const& tt, Token const* & tk) -> Token const*
        {
            auto p = tk;
            auto name = (Token const*) nullptr;

            if (!token_is(p, "enum", Token_Tag::identifier)) return nullptr;
            p++;

            if (token_is(p, "struct", Token_Tag::identifier) || token_is(p, "class", Token_Tag::identifier))
                p++;

            if (p->tags.has_all_of({Token_Tag::identifier})) {
                name = p;
                p++;
            }

            if (token_is(p, ":", Token_Tag::symbol)) {
                p++;

                for (auto last=tt.end(); p < last; p=p->next()) {
                    if (token_is(p, "{", Token_Tag::symbol))
                        break;

                    if (token_is(p, ";", Token_Tag::symbol)) {
                        auto loc = tt.source_location_of(p->first);
                        throw_parsing_error(loc, p, "enum declaration cannot be introspected.");
                    }
                }
            }

            if (!token_is(p, "{", Token_Tag::symbol)) {
                auto loc = tt.source_location_of(p->first);
                throw_parsing_error(loc, p, "failed to introspect enum.");
            }

            p++;
            tk = p;
            return (name == nullptr ? p : name);
        }

        template <class Report>
        auto parse_enum_body(Token const* & tk, Report&& report) -> void
        {
            while (!token_is(tk, "}", Token_Tag::symbol)) {
                if (tk->tags.has_all_of({Token_Tag::identifier})) {
                    report(tk);
                    tk++;

                    while (!token_is(tk, ",", Token_Tag::symbol) && !token_is(tk, "}", Token_Tag::symbol))
                        tk = tk->next();

                    continue;
                } else {
                    tk = tk->next();
                }
            }
            tk++;
        }

        auto has_introspect_attribute(Token_Tree const& tt) -> bool
        {
            for (auto tk=tt.begin(), last=tt.end(); tk < last; tk++)
                if (parse_introspect_attribute(tt, tk))
                    return true;
            return false;
        }
    }

    auto introspect(Token_Tree const& tt, Introspection_Handler& ih) -> void
    {
        if (!has_introspect_attribute(tt)) {
            ih.empty();
            return;
        }

        try {
            ih.start();

            for (auto tk=tt.begin(), last=tt.end(); tk < last;) {
                if (token_is(tk, "}", Token_Tag::symbol)) {
                    ih.leave_namespace();
                    tk++;
                    continue;
                }

                if (auto name = parse_namespace_heading(tk)) {
                    ih.enter_namespace(name);
                    continue;
                }

                if (parse_introspect_attribute(tt, tk)) {
                    if (auto name = parse_enum_heading(tt, tk)) {
                        if (name == tk) {
                            parse_enum_body(tk, [&] (auto enumerant) { ih.integral_constant(enumerant); });
                        } else {
                            ih.enter_enum(name);
                            parse_enum_body(tk, [&] (auto enumerant) { ih.enumerator(enumerant); });
                            ih.leave_enum();
                        }
                        continue;
                    }

                    auto loc = tt.source_location_of(tk->first);
                    throw_parsing_error(loc, tk, "not introspectable.");
                }

                tk = tk->next();
            }

            ih.finish();
        }
        catch (...) {
            ih.abort();
            throw;
        }
    }
}

