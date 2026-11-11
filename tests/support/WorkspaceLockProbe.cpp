#include "rhodium/core/WorkspaceLock.hpp"

#include <filesystem>
#include <iostream>
#include <string>

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        return 64;
    }
    std::string error;
    rhodium::WorkspaceLock lock(std::filesystem::path(argv[1]), &error);
    if (!lock)
    {
        std::cerr << error;
        return 2;
    }
    return 0;
}
