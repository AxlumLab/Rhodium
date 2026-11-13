/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by AxlumLab, All Rights Reserved.
 */

#include "ToolchainSerializer.hpp"

namespace rhodium
{
    void ToolchainSerializer::appendPath(std::ostringstream &stream,
                                        const char *name,
                                        const std::filesystem::path &path)
    {
        stream << name << "=" << path.generic_string() << "\n";
    }

    void ToolchainSerializer::appendString(std::ostringstream &stream,
                                          const char *name,
                                          const std::string &value)
    {
        stream << name << "=" << value << "\n";
    }

    void ToolchainSerializer::appendStringArray(std::ostringstream &stream,
                                               const char *name,
                                               const std::vector<std::string> &values)
    {
        if (values.empty())
        {
            return;
        }
        stream << name << "=[";
        for (std::size_t i = 0; i < values.size(); ++i)
        {
            if (i != 0)
            {
                stream << ",";
            }
            stream << "\"" << values[i] << "\"";
        }
        stream << "]\n";
    }

    void ToolchainSerializer::appendPathArray(std::ostringstream &stream,
                                             const char *name,
                                             const std::vector<std::filesystem::path> &paths)
    {
        if (paths.empty())
        {
            return;
        }
        stream << name << "=[";
        for (std::size_t i = 0; i < paths.size(); ++i)
        {
            if (i != 0)
            {
                stream << ",";
            }
            stream << "\"" << paths[i].generic_string() << "\"";
        }
        stream << "]\n";
    }

} // namespace rhodium
