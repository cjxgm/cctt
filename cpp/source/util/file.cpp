#include "file.hpp"
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace cctt
{
    namespace util
    {
        inline namespace file
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
    }
}

