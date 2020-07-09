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
                    throw_parsing_error2(bad_loc, bad, content_loc, content, "introspection must be directly inside namespace/struct/class/union.");
                }

                if (p-1 >= tt.begin() && token_is(p-1, "namespace", Token_Tag::identifier)) {
                    auto loc = tt.source_location_of(p[-1].first);
                    throw_parsing_error(loc, p-1, "anonymous namespaces cannot be introspected.");
                }
            }

            tk = tk[1].next();
            return content + 1;
        }

        // Parse this pattern:
        //
        //   namespace @name [:: @name] { ....
        //               ^   ~~~~~^~~~~     ^
        //               |        |         `-- tk will be here if succeeds.
        //               |        `------------ arbitrary number of ":: @name". C++17 style nested namespace in a single declaration.
        //               `--------------------- return value will be this if succeeds.
        //
        // If failed, returns nullptr and tk is not modified.
        auto parse_namespace_heading(Token const* & tk) -> Token const*
        {

            if (!token_is(tk, "namespace", Token_Tag::identifier)) return nullptr;
            if (tk[1].tags.has_none_of({Token_Tag::identifier})) return nullptr;

            auto tk2 = tk + 2;
            while (token_is(tk2, "::", Token_Tag::symbol) && tk2[1].tags.has_all_of({Token_Tag::identifier}))
                tk2 += 2;

            if (!token_is(tk2, "{", Token_Tag::symbol)) return nullptr;

            auto name = tk + 1;
            tk = tk2 + 1;
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
        auto parse_enumerator(Token_Tree const& tt, Token const* & tk, Report&& report) -> void
        {
            if (tk->tags.has_all_of({Token_Tag::identifier})) {
                report(tk);
                tk++;

                while (true) {
                    if (token_is(tk, ",", Token_Tag::symbol)) {
                        tk++;
                        break;
                    }

                    if (token_is(tk, "}", Token_Tag::symbol))
                        break;

                    tk = tk->next();
                }
            } else {
                auto loc = tt.source_location_of(tk->first);
                throw_parsing_error(loc, tk, "unrecognized enum item.");
            }
        }

        template <class Report>
        auto parse_enum_body(Introspection_Handler& ih, Token_Tree const& tt, Token const* & tk, Report&& report) -> void
        {
            while (!token_is(tk, "}", Token_Tag::symbol)) {
                if (auto attribs = parse_introspect_attribute(tt, tk)) {
                    ih.add_attributes(attribs);
                    while (auto attribs = parse_introspect_attribute(tt, tk))
                        ih.add_attributes(attribs);

                    parse_enumerator(tt, tk, report);

                    ih.clear_attributes();
                } else {
                    parse_enumerator(tt, tk, report);
                }
            }

            tk++;
        }

        // Parse these patterns:
        //
        //   [struct|class|union] ... @name [final] [: ....] { ....
        //       ^     ^     ^     ^    ^                ^       ^
        //       |     |     |     |    |                |       `-- tk will be here if succeeds.
        //       |     |     |     |    |                `---------- bases will be here if succeeds.
        //       |     |     |     |    `--------------------------- return value will be this if succeeds.
        //       |     |     |     `-------------------------------- "alignas" and token trees are ignored.
        //       |     |     `-------------------------------------- publicity will be set to true
        //       |     `-------------------------------------------- publicity will be set to false
        //       `-------------------------------------------------- publicity will be set to true
        //
        //
        //   [struct|class|union] ... [final] [: ....] { ....
        //       ^     ^     ^     ^               ^       ^
        //       |     |     |     |               |       +-- tk will be here if succeeds.
        //       |     |     |     |               |       `-- return value will be this if succeeds.
        //       |     |     |     |               `---------- bases will be here if succeeds.
        //       |     |     |     `-------------------------- "alignas" and token trees are ignored.
        //       |     |     `-------------------------------- publicity will be set to true
        //       |     `-------------------------------------- publicity will be set to false
        //       `-------------------------------------------- publicity will be set to true
        //
        //   [struct|class|union] ... [@name] [final] [: ....] ; ...
        //       ^     ^     ^     ^                       ^   ^  ^
        //       |     |     |     |                       |   |  `-- tk will be here if succeeds.
        //       |     |     |     |                       |   `----- return value will be this if succeeds.
        //       |     |     |     |                       `--------- bases will be here if succeeds.
        //       |     |     |     `--------------------------------- "alignas" and token trees are ignored.
        //       |     |     `--------------------------------------- publicity will be set to true
        //       |     `--------------------------------------------- publicity will be set to false
        //       `--------------------------------------------------- publicity will be set to true
        //
        // If none of the above patterns match, returns nullptr and tk is not modified.
        auto parse_struct_heading(Token_Tree const& tt, Token const* & tk, bool& publicity, Token const*& bases) -> Token const*
        {
            auto p = tk;
            auto name = (Token const*) nullptr;
            auto kind = p;

            if (token_is(p, "struct", Token_Tag::identifier) || token_is(p, "class", Token_Tag::identifier) || token_is(p, "union", Token_Tag::identifier)) {
                publicity = (p->first[0] != 'c');
                p++;
                while (p->tags.has_none_of({Token_Tag::end}) && (p->pair != nullptr || token_is(p, "alignas", Token_Tag::identifier)))
                    p = p->next();
            } else {
                return nullptr;
            }

            if (p->tags.has_all_of({Token_Tag::identifier})) {
                name = p;
                p++;
            }

            if (token_is(p, "final", Token_Tag::identifier))
                p++;

            if (token_is(p, ":", Token_Tag::symbol)) {
                p++;
                bases = p;

                for (auto last=tt.end(); p < last; p=p->next()) {
                    if (token_is(p, "{", Token_Tag::symbol))
                        break;

                    if (token_is(p, ";", Token_Tag::symbol))
                        break;
                }
            }

            if (token_is(p, ";", Token_Tag::symbol)) {
                name = p++;
                tk = p;
                return name;
            }

            if (!token_is(p, "{", Token_Tag::symbol)) {
                auto loc_kind = tt.source_location_of(kind->first);
                auto loc = tt.source_location_of(p->first);
                throw_parsing_error2(loc_kind, kind, loc, p, "failed to introspect item.");
            }

            p++;
            tk = p;
            return (name == nullptr ? p : name);
        }

        // Parse these patterns:
        //
        //   [virtual|public|private|protected]* .... [, [virtual|public|private|protected]* .... [, ....]] {
        //                                         ^
        //                                         `-- base if public or publicity == true
        //
        // If none of the above patterns match, exceptions will be thrown
        auto parse_struct_bases(Introspection_Handler& ih, Token const* p, bool publicity) -> void
        {
            for (bool cont=true; cont; ) {
                bool is_public = publicity;
                while (true) {
                    if (false
                            || token_is(p, "virtual", Token_Tag::identifier)
                            || token_is(p, "public", Token_Tag::identifier)
                            || token_is(p, "private", Token_Tag::identifier)
                            || token_is(p, "protected", Token_Tag::identifier)
                            ) {
                        if (p->first[0] == 'p') is_public = (p->first[1] == 'u');
                        p++;
                    } else {
                        break;
                    }
                }

                auto base_first = p;
                while (true) {
                    if (token_is(p, ",", Token_Tag::symbol))
                        break;

                    if (token_is(p, "{", Token_Tag::symbol)) {
                        cont = false;
                        break;
                    }

                    p = p->next();
                }

                if (is_public) ih.parent(base_first, p);
                p++;
            }
        }

        auto parse_struct_body(Introspection_Handler& ih, Token_Tree const& tt, Token const* & tk, bool publicity) -> void;

        // Skip items until these patterns:
        //
        //   public : ....
        //              ^
        //              `-- tk will be here if succeeds.
        //
        //   }
        //   ^
        //   `-- tk will be here if succeeds.
        //
        // It is UNDEFINED BEHAVIOR if none of the above patterns match.
        auto skip_after_public(Token const* & tk) -> void
        {
            for (;; tk=tk->next()) {
                if (token_is(tk, "}", Token_Tag::symbol))
                    return;

                if (token_is(tk, "public", Token_Tag::identifier) && token_is(tk+1, ":", Token_Tag::symbol)) {
                    tk += 2;
                    return;
                }
            }
        }

        // Parse these patterns:
        //
        //   identifier .... name { .... } .... [; | , | { .... }] ....
        //   identifier .... name [ .... ] .... [; | , | { .... }] ....
        //   identifier .... name ( .... ) .... [; | , | { .... }] ....
        //   identifier .... name = ....   .... [; | , | { .... }] ....
        //   identifier .... name [; | ,]                          ....
        //               ^    ^                                      ^
        //               |    |                                      `-- tk will be here if succeeds.
        //               |    `----------------------------------------- return value will be this if succeeds.
        //               `---------------------------------------------- anything but `;` nor `}`
        //
        // If none of the above patterns match, returns nullptr and tk is not modified.
        auto parse_variable_or_function(Token_Tree const& tt, Token const* & tk) -> Token const*
        {
            if (!tk->tags.has_all_of({Token_Tag::identifier})) return nullptr;

            auto p = tk;
            auto name = (Token const*) nullptr;
            while (true) {
                if ((token_is(p, "decltype", Token_Tag::identifier) || token_is(p, "alignas", Token_Tag::identifier)) && token_is(p+1, "(", Token_Tag::symbol)) {
                    p = p[1].next();
                    continue;
                }

                if (token_is(p, "operator", Token_Tag::identifier) && p[1].tags.has_all_of({Token_Tag::symbol})) {
                    name = p;
                    p = p[1].next();
                    continue;
                }

                if (p->is_end()) return nullptr;
                if (token_is(p, "}", Token_Tag::symbol)) return nullptr;

                if (token_is(p, ";", Token_Tag::symbol)) break;
                if (token_is(p, ",", Token_Tag::symbol)) break;
                if (token_is(p, "{", Token_Tag::symbol)) break;
                if (token_is(p, "[", Token_Tag::symbol)) break;
                if (token_is(p, "(", Token_Tag::symbol)) break;
                if (token_is(p, "=", Token_Tag::symbol)) break;

                p = p->next();
            }

            if (name == nullptr) name = p - 1;
            tk = p->next();
            if (p->first[0] == ';' || p->first[0] == ',') return name;

            while (true) {
                if (tk->is_end()) {
                    auto name_loc = tt.source_location_of(name->first);
                    throw_parsing_error(name_loc, name, "unexpected eof.");
                }

                if (token_is(tk, "}", Token_Tag::symbol)) {
                    auto name_loc = tt.source_location_of(name->first);
                    auto loc = tt.source_location_of(tk->first);
                    throw_parsing_error2(name_loc, name, loc, tk, "unexpected symbol.");
                }

                if (token_is(tk, ";", Token_Tag::symbol) || token_is(tk, ",", Token_Tag::symbol)) {
                    tk++;
                    break;
                }

                if (token_is(tk, "{", Token_Tag::symbol)) {
                    tk = tk->next();
                    break;
                }

                // constructor's member initialization list
                if (token_is(tk, ":", Token_Tag::symbol)) {
                    tk++;

                    while (true) {
                        if (tk->is_end()) {
                            auto name_loc = tt.source_location_of(name->first);
                            throw_parsing_error(name_loc, name, "unexpected eof.");
                        }

                        if (token_is(tk, "{", Token_Tag::symbol) || token_is(tk, "(", Token_Tag::symbol) || token_is(tk, "...", Token_Tag::symbol)) {
                            tk = tk->next();

                            if (token_is(tk, ",", Token_Tag::symbol)) {
                                tk++;
                                continue;
                            }

                            break;
                        }

                        tk = tk->next();
                    }

                    continue;
                }

                tk = tk->next();
            }

            return name;
        }

        auto has_introspect_attribute(Token_Tree const& tt) -> bool
        {
            for (auto tk=tt.begin(), last=tt.end(); tk < last; tk++)
                if (parse_introspect_attribute(tt, tk))
                    return true;
            return false;
        }

        // Parse block level items, i.e. (member-)enums, (member-)integral constants,
        // (member-)variables, (member-)functions, and (member-)structs.
        // They may all optionally have a introspect attribute header each.
        //
        // On success, tk will be modified to the next unparsed token, and true will be returned;
        // On failure, tk will NOT be modified, and false will be returned;
        auto parse_block_item(Introspection_Handler& ih, Token_Tree const& tt, Token const* & tk) -> bool
        {
            if (auto name = parse_enum_heading(tt, tk)) {
                if (name == tk) {
                    parse_enum_body(ih, tt, tk, [&] (auto enum_name) { ih.integral_constant(enum_name); });
                } else {
                    ih.enter_enum(name);
                    parse_enum_body(ih, tt, tk, [&] (auto enum_name) { ih.enumerator(enum_name); });
                    ih.leave_enum();
                }
                return true;
            }

            bool publicity;
            auto bases = (Token const*) nullptr;
            if (auto name = parse_struct_heading(tt, tk, publicity, bases)) {
                if (name == tk) {
                    parse_struct_body(ih, tt, tk, publicity);
                } else if (name+1 == tk) {
                    // Empty intentionally: ignore forward declarations
                } else {
                    ih.structure(name);
                    if (bases != nullptr) parse_struct_bases(ih, bases, publicity);
                    ih.enter_namespace(name, name+1);
                    parse_struct_body(ih, tt, tk, publicity);
                    ih.leave_namespace();
                }
                return true;
            }

            if (auto name = parse_variable_or_function(tt, tk)) {
                if (!token_is(name, "operator", Token_Tag::identifier))
                    ih.variable_or_function(name);
                return true;
            }

            return false;
        }

        // Parse block level items with introspect attributes as headers.
        //
        // On success, tk will be modified to the next unparsed token, and true will be returned;
        // If tk is NOT an introspect attribute, tk will NOT be modified, and false will be returned;
        // If tk IS an introspect attribute but failed to parse block items, an exception will be thrown.
        auto parse_attributed_block_item(Introspection_Handler& ih, Token_Tree const& tt, Token const* & tk) -> bool
        {
            if (auto attribs = parse_introspect_attribute(tt, tk)) {
                ih.add_attributes(attribs);
                while (auto attribs = parse_introspect_attribute(tt, tk))
                    ih.add_attributes(attribs);

                if (parse_block_item(ih, tt, tk)) {
                    ih.clear_attributes();
                    return true;
                } else {
                    auto loc = tt.source_location_of(tk->first);
                    throw_parsing_error(loc, tk, "not introspectable.");
                }
            }

            return false;
        }

        auto parse_struct_body(Introspection_Handler& ih, Token_Tree const& tt, Token const* & tk, bool publicity) -> void
        {
            if (!publicity) skip_after_public(tk);

            while (true) {
                if ((token_is(tk, "private", Token_Tag::identifier) || token_is(tk, "protected", Token_Tag::identifier)) && token_is(tk+1, ":", Token_Tag::symbol)) {
                    tk += 2;
                    skip_after_public(tk);
                }

                if (token_is(tk, "using", Token_Tag::identifier) || token_is(tk, "typedef", Token_Tag::identifier)) {
                    while (!token_is(tk, ";", Token_Tag::symbol) && !token_is(tk, "}", Token_Tag::symbol))
                        tk = tk->next();
                }

                if (token_is(tk, "}", Token_Tag::symbol))
                    break;

                if (parse_attributed_block_item(ih, tt, tk))
                    continue;

                if (parse_block_item(ih, tt, tk))
                    continue;

                tk = tk->next();
            }

            tk++;
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
                    ih.enter_namespace(name, tk-1);
                    continue;
                }

                if (parse_attributed_block_item(ih, tt, tk)) {
                    continue;
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

