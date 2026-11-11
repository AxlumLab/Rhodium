#pragma once

#include <filesystem>
#include <string>

namespace rhodium
{
    // Publishes a completed same-directory temporary file by replacing its target.
    class AtomicFile
    {
    public:
        static bool write(const std::filesystem::path &path, const std::string &content, std::string *error);
    };
}
