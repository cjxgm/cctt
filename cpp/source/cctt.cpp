#include "cctt.hpp"
#include "grammar.hpp"
#include <peglib/peglib.hpp>
#include <iostream>
#include <stdexcept>

namespace cctt
{
    void parse(char const* source_buffer, int source_length, char const* path)
    {
        peg::parser p;
        p.log = [] (auto line, auto row, auto& msg) {
            std::cerr << "  " << line << ":" << row << ": " << msg << "\n";
        };

        std::cerr << "Loading grammar...\n";
        if (!p.load_grammar(grammar))
            throw std::logic_error{"Failed to load grammar"};

        std::cerr << "Enabling packrat parsing...\n";
        p.enable_packrat_parsing();

        std::cerr << "Parsing...\n";
        if (!p.parse_n(source_buffer, source_length, path))
            throw std::runtime_error{"Syntax error"};
    }
}

