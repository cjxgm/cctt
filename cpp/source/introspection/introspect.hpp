#pragma once
#include "handler.hpp"
#include "../token-tree/token-tree.hpp"

namespace cctt
{
    auto introspect(Token_Tree const& tt, Introspection_Handler& ih) -> void;
}

