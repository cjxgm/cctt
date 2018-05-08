#include "cctt.hpp"
#include "grammar.hpp"
#include <peglib/peglib.hpp>
#include <sstream>
#include <iostream>
#include <iomanip>      // for std::quoted
#include <stdexcept>
#include <memory>

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

        {   // Error handling
            p["UNKNOWN_CHAR"] = [] (peg::SemanticValues const& sv) {
                auto row_col = sv.line_info();
                std::ostringstream ss;
                ss
                    << "Unknow character at "
                    << row_col.first << ":" << row_col.second
                    << ": " << sv.str()
                ;
                throw std::runtime_error{ss.str()};
            };

            {
                auto store_pair = [] (peg::SemanticValues const& sv) -> std::string {
                    auto content = sv.str();
                    if (content.empty()) {
                        return "EOF";
                    } else {
                        auto row_col = sv.line_info();
                        std::ostringstream ss;
                        ss
                            << std::quoted(content)
                            << " at " << row_col.first << ":" << row_col.second
                        ;
                        return ss.str();
                    }
                };
                p["OPEN_PAIRS"] = store_pair;
                p["CLOSING_PAIRS"] = store_pair;
                p["OPEN_CHAR_LIT"] = store_pair;
                p["OPEN_STR_LIT"] = store_pair;
                p["OPEN_RSTR_LIT"] = store_pair;
            }

            p["UNPAIRED"] = [] (peg::SemanticValues const& sv) {
                std::ostringstream ss;
                ss
                    << sv[0].get<std::string>()
                    << " and "
                    << sv[2].get<std::string>()
                    << " are not paired."
                ;
                throw std::runtime_error{ss.str()};
            };

            p["NOT_CLOSED"] = [] (peg::SemanticValues const& sv) {
                std::ostringstream ss;
                ss
                    << sv[0].get<std::string>()
                    << " is not closed."
                ;
                throw std::runtime_error{ss.str()};
            };

            p["NOT_OPEN"] = [] (peg::SemanticValues const& sv) {
                std::ostringstream ss;
                ss
                    << "Excessive "
                    << sv.back().get<std::string>()
                    << "."
                ;
                throw std::runtime_error{ss.str()};
            };
        }

        std::string raw_delim;
        {   // Special actions for raw string delimiter handling
            p["SAVE_RAW_DELIM"] = [&raw_delim] (peg::SemanticValues const& sv) {
                raw_delim = sv.str();
            };

            p["PAIR_RAW_DELIM"] = [&raw_delim] (peg::SemanticValues const& sv) {
                if (raw_delim != sv.str()) throw peg::parse_error{};
            };
        }

        std::cerr << "Enabling packrat parsing...\n";
        p.enable_packrat_parsing();

        // DEBUG
        p["token"] = [] (peg::SemanticValues const& sv) {
            auto content = sv.str();
            auto quote = (content.find('"') == content.npos ? '"' : '\'');
            std::cerr << std::quoted(sv.str(), quote) << "\n";
        };

        std::cerr << "Parsing...\n";
        if (!p.parse_n(source_buffer, source_length, path))
            throw std::runtime_error{"Syntax error"};
    }
}

