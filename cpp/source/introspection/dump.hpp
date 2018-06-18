#pragma once
#include "handler.hpp"
#include <string>

namespace cctt
{
    struct Introspection_Dumper final: Introspection_Handler
    {
        Introspection_Dumper();

        auto empty() -> void override;

        auto start() -> void override;
        auto finish() -> void override;

        auto abort() -> void override;

        auto enter_namespace(Token const* name) -> void override;
        auto leave_namespace() -> void override;

        auto enter_enum(Token const* name) -> void override;
        auto leave_enum() -> void override;
        auto enumerator(Token const* name) -> void override;

        auto integral_constant(Token const* name) -> void override;

    private:
        std::string full_namespace;
    };
}

