#include "cctt.hpp"
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <stdexcept>

namespace
{
    auto slurp(char const* path) -> std::string
    {
        if (std::ifstream ifs{path, std::ios::binary}) {
            std::stringstream ss;
            ss << ifs.rdbuf();
            return ss.str();
        }
        else {
            throw std::runtime_error{"Cannot load file: " + std::string{path}};
        }
    }
}

int main(int argc, char* argv[])
{
    std::string path{"@builtin"};
    std::string source{"#a\\\r\n!"
R"source(
/*!*/ //!
(a < b); (a > b);
(a > b && c < d);
(a << b);
(a < b && c > d;) :: [[
    a::b(
        u8"c" == ~0x1fL
        'a'
    )
]]
u8R"a(heR"w(l"lo)w")a"
)source"
    };

    if (argc == 2) {
        path = argv[1];
        source = slurp(path.data());
    }

    try {
        cctt::parse(source.data(), int(source.size()), path.data());
    }
    catch (std::runtime_error const& e) {
        std::cerr << "\e[1;31mError: \e[0m" << e.what() << "\n";
    }
}

