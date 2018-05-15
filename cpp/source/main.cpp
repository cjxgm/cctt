#include "cctt.hpp"
#include <string>
#include <fstream>
#include <sstream>
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
    std::string source{"#a\\\r\n!\n/*!*/ //!"};

    if (argc == 2) {
        path = argv[1];
        source = slurp(path.data());
    }

    cctt::parse(source.data(), int(source.size()), path.data());
}
