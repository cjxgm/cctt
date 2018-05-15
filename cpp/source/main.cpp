#include "token-tree/token-stream.hpp"
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
    std::string source{"\
([]//hello\n\
    // wo/*rl*/d\n\
    /* h\n\
el*/lo */\n\
#define HELLO(X) X \\\n\
    + X \\\r\n\
    - X \n\
1.5e3 .1 +3. a[3] = 10 \
auto x = L\"hello\";\n\
[[cctt::test]] std::cerr << \"hello \" << T<int> x->y << (x++ > 5);\n\
u8R\"asd( hello)a )as\"word)asd\"\"hi\"){}\
"
    };

    if (argc == 2) {
        path = argv[1];
        source = slurp(path.data());
    }

    for (auto token: cctt::Token_Stream{source}) {
        std::cout << token << "\n";
    }
}

