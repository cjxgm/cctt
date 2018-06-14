#include "util/file.hpp"
#include "token-tree/token-tree.hpp"
#include "token-tree/error.hpp"
#include <string>
#include <iostream>
#include <stdexcept>

#include "util/style.inl"

int main(int argc, char* argv[])
{
    std::string path{"@builtin"};
    std::string source{
        #include "test-source.inl"
    };

    auto scan = [&] {
        try {
            cctt::Token_Tree tt{source};
            std::cout << path << ": token count = " << tt.end() - tt.begin() << "\n";
            std::cout.flush();
        }
        catch (cctt::Parsing_Error const& e) {
            std::clog
                << STYLE_ERROR "Error" STYLE_NORMAL " parsing "
                << STYLE_PATH << path << STYLE_NORMAL
                << " at " << e.what()
                << "\n";
            std::clog.flush();
        }
    };

    try {
        if (argc < 2) {
            scan();
        } else {
            for (int i=1; i < argc; i++) {
                path = argv[i];
                source = cctt::util::slurp(path.data());
                scan();
            }
        }
    }
    catch (std::runtime_error const& e) {
        std::clog << STYLE_ERROR << e.what() << STYLE_NORMAL << "\n";
        std::clog.flush();
    }
}

