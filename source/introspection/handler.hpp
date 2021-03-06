#pragma once
#include "../token-tree/token.hpp"

namespace cctt
{
    struct Introspection_Handler
    {
        virtual ~Introspection_Handler() = default;

        // There is no CCTT_INTROSPECT ( .... ) anywhere.
        virtual auto empty() -> void = 0;

        // There is CCTT_INTROSPECT ( .... ) somewhere.
        virtual auto start() -> void = 0;
        virtual auto finish() -> void = 0;

        // An error occurred between start() and finish().
        // When abort() is called, finish() won't be called.
        virtual auto abort() -> void = 0;

        // CCTT_INTROSPECT ( .... )
        //                 ^
        //                 `--------- attribs
        virtual auto add_attributes(Token const* attribs) -> void = 0;
        virtual auto clear_attributes() -> void = 0;

        // namespace @name [:: @name] { .... }
        //             ^              ^
        //             |              `--------- name_last
        //             `------------------------ name_first
        virtual auto enter_namespace(Token const* name_first, Token const* name_last) -> void = 0;
        virtual auto leave_namespace() -> void = 0;

        // enum @name { @enumerator1, @enumerator2 = 10, @enumerator3 };
        // enum @name: uint32_t { @enumerator1, @enumerator2 = 10, @enumerator3 };
        // enum struct @name { @enumerator1, @enumerator2 = 10, @enumerator3 };
        // enum class  @name { @enumerator1, @enumerator2 = 10, @enumerator3 };
        virtual auto enter_enum(Token const* name) -> void = 0;
        virtual auto leave_enum() -> void = 0;
        virtual auto enumerator(Token const* name) -> void = 0;

        // enum { @constant1, @constant2 = 10, @constant3 };
        // enum: int { @constant1, @constant2 = 10, @constant3 };
        virtual auto integral_constant(Token const* name) -> void = 0;

        virtual auto structure(Token const* name) -> void = 0;
        virtual auto parent(Token const* first, Token const* last) -> void = 0;

        // int name;
        // int name = 10;
        // int* name(nullptr);
        // int name{10};
        // extern int name();
        // auto name() { return 10; }
        // static inline constexpr auto name() -> int;
        // auto name = 10;
        // decltype(auto) name() { return 10; }
        virtual auto variable_or_function(Token const* name) -> void = 0;
    };
}

