#include "dump.hpp"
#include <iostream>

namespace cctt
{
    Introspection_Dumper::Introspection_Dumper()
    {
        constexpr auto estimated_full_namespace_size = 1024;
        full_namespace.reserve(estimated_full_namespace_size);
    }

    auto Introspection_Dumper::empty() -> void
    {
        std::cout << "Nothing interesting.\n";
        std::cout.flush();
    }

    auto Introspection_Dumper::start() -> void
    {
        std::cout << "Start processing.\n";
        std::cout.flush();
    }

    auto Introspection_Dumper::finish() -> void
    {
        std::cout << "All processed.\n";
        std::cout.flush();
    }

    auto Introspection_Dumper::abort() -> void
    {
        std::cout << "Aborted.\n";
        std::cout.flush();
    }

    auto Introspection_Dumper::add_attributes(Token const* attribs) -> void
    {
        std::cout << "  attributes: ";
        std::cout.write(attribs->first, attribs->pair->last - attribs->first);
        std::cout << "\n";
        std::cout.flush();
    }

    auto Introspection_Dumper::clear_attributes() -> void
    {
        std::cout << "  attributes: clear\n";
        std::cout.flush();
    }

    auto Introspection_Dumper::enter_namespace(Token const* name) -> void
    {
        full_namespace += "::";
        full_namespace.append(name->first, name->last);

        std::cout << "  namespace " << full_namespace << " {\n";
        std::cout.flush();
    }

    auto Introspection_Dumper::leave_namespace() -> void
    {
        std::cout << "  } // namespace " << full_namespace << " -> ";

        full_namespace.erase(full_namespace.rfind("::"));

        std::cout << (full_namespace.empty() ? "::" : full_namespace.data()) << "\n";
        std::cout.flush();
    }

    auto Introspection_Dumper::enter_enum(Token const* name) -> void
    {
        auto full_name = full_namespace;
        full_name += "::";
        full_name.append(name->first, name->last);

        std::cout << "  enum " << full_name << " {\n";
        std::cout.flush();
    }

    auto Introspection_Dumper::leave_enum() -> void
    {
        std::cout << "  } // enum\n";
        std::cout.flush();
    }

    auto Introspection_Dumper::enumerator(Token const* name) -> void
    {
        std::cout << "      enumerator ";
        std::cout.write(name->first, name->last - name->first);
        std::cout << "\n";
        std::cout.flush();
    }

    auto Introspection_Dumper::integral_constant(Token const* name) -> void
    {
        auto full_name = full_namespace;
        full_name += "::";
        full_name.append(name->first, name->last);

        std::cout << "  int constant " << full_name << "\n";
        std::cout.flush();
    }

    auto Introspection_Dumper::variable_or_function(Token const* name) -> void
    {
        auto full_name = full_namespace;
        full_name += "::";
        full_name.append(name->first, name->last);

        std::cout << "  var or fn " << full_name << "\n";
        std::cout.flush();
    }
}

