#pragma once
#include "../token-tree/token.hpp"

namespace cctt
{
    struct Introspection_Handler
    {
        virtual ~Introspection_Handler() = default;

        // There is no [[cctt::introspect]] anywhere.
        virtual auto empty() -> void = 0;

        // There is [[cctt::introspect]] somewhere.
        virtual auto start() -> void = 0;
        virtual auto finish() -> void = 0;

        // An error occurred between start() and finish().
        // When abort() is called, finish() won't be called.
        virtual auto abort() -> void = 0;

        // namespace @name { .... }
        virtual auto enter_namespace(Token const* name) -> void = 0;
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
    };
}

