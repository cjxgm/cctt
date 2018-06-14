#include "pretty-print.hpp"
#include "../util/string.hpp"
#include <fmt/format.hpp>
#include <iostream>
#include <deque>

#include "../util/style.inl"

namespace cctt
{
    namespace
    {
        auto pretty_print_token_tree(Token const* first, Token const* last, char const* link) -> void
        {
            struct Block
            {
                Token const* first;
                Token const* last;
                std::string link;

                Block(Token const* first, Token const* last, char const* link)
                    : first{first}
                    , last{last}
                {
                    this->link = fmt::format(
                        STYLE_LINK "{}" STYLE_NORMAL
                        , link
                    );
                }

                Block(Token const* base, Token const* tk)
                    : first{tk->child()}
                    , last{tk->last_child()}
                {
                    if (first == nullptr) {
                        link = fmt::format(
                            STYLE_BLOCK "{}{}" STYLE_NORMAL
                            , std::string{tk->first, tk->last}
                            , std::string{tk->pair->first, tk->pair->last}
                        );
                    } else {
                        link = fmt::format(
                            STYLE_LINK "{}{:d}{}" STYLE_NORMAL
                            , std::string{tk->first, tk->last}
                            , first - base
                            , std::string{tk->pair->first, tk->pair->last}
                        );
                    }
                }
            };

            std::deque<Block> blocks;
            blocks.emplace_back(first, last, link);

            while (!blocks.empty()) {
                auto block = std::move(blocks.front());
                blocks.pop_front();

                std::cout << block.link << ":";

                for (auto p=block.first; p < block.last; p=p->next()) {
                    if (p->is_leaf()) {
                        std::cout << " " << util::format_to_oneline({p->first, p->last});
                    } else {
                        blocks.emplace_back(first, p);
                        std::cout << " " << blocks.back().link;
                        if (blocks.back().first == nullptr)
                            blocks.pop_back();
                    }
                }

                std::cout << "\n";
            }
        }
    }

    auto pretty_print_token_tree(Token const* first, Token const* last) -> void
    {
        pretty_print_token_tree(first, last, "*0*");
    }
}

