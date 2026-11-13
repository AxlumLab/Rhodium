/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by AxlumLab, All Rights Reserved.
 */

#include "rhodium/core/FileSystem.hpp"
#include "rhodium/core/AtomicFile.hpp"

#include <fstream>
#include <sstream>

namespace rhodium
{
    bool File::exists(const std::filesystem::path &path)
    {
        return std::filesystem::is_regular_file(path);
    }

    bool File::dir(const std::filesystem::path &path)
    {
        return std::filesystem::is_directory(path);
    }

    bool File::mkdir(const std::filesystem::path &path, std::string *error)
    {
        try
        {
            if (path.empty())
            {
                return true;
            }
            std::filesystem::create_directories(path);
            return true;
        }
        catch (const std::exception &ex)
        {
            if (error)
            {
                *error = ex.what();
            }
            return false;
        }
    }

    bool File::write(const std::filesystem::path &path, const std::string &content, std::string *error)
    {
        try
        {
            if (std::filesystem::is_regular_file(path))
            {
                std::ifstream input(path, std::ios::binary);
                std::ostringstream stream;
                stream << input.rdbuf();
                if (input && !input.bad() && stream.str() == content)
                {
                    return true;
                }
            }
            if (!mkdir(path.parent_path(), error))
            {
                return false;
            }
            return AtomicFile::write(path, content, error);
        }
        catch (const std::exception &ex)
        {
            if (error)
            {
                *error = ex.what();
            }
            return false;
        }
    }

    std::string File::read(const std::filesystem::path &path)
    {
        std::ifstream input(path, std::ios::binary);
        std::ostringstream stream;
        stream << input.rdbuf();
        return stream.str();
    }

} // namespace rhodium
