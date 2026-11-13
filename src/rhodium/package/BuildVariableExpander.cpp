/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by AxlumLab, All Rights Reserved.
 */

#include "rhodium/package/BuildVariableExpander.hpp"

namespace rhodium
{
    void BuildVar::replace(std::string &value, const std::string &from, const std::string &to)
    {
        std::size_t pos = 0;
        while ((pos = value.find(from, pos)) != std::string::npos)
        {
            value.replace(pos, from.size(), to);
            pos += to.size();
        }
    }

    std::string BuildVar::expand(std::string value, const BuildUtil::Context &context)
    {
        replace(value, "$(source)", context.sourceDir.string());
        replace(value, "$(build)", context.buildDir.string());
        replace(value, "$(artifact)", context.artifactDir.string());
        replace(value, "$(workspace)", context.workspaceRoot.string());
        replace(value, "{workspace}", context.workspaceRoot.string());
        replace(value, "$(package)", context.packageName);
        replace(value, "$(config)", context.config);
        replace(value, "$(platform)", context.platform);
        replace(value, "$(arch)", context.arch);
        replace(value, "$(cc)", context.cc);
        replace(value, "$(cxx)", context.cxx);
        replace(value, "$(linker)", context.linker);
        replace(value, "$(archiver)", context.archiver);
        replace(value, "$(rc)", context.rc);
        replace(value, "$(mt)", context.mt);
        replace(value, "$(cmake)", context.cmake);
        replace(value, "$(ninja)", context.ninja);
        replace(value, "$(git)", context.git);
        return value;
    }

    std::vector<std::string> BuildVar::expand(const std::vector<std::string> &values, const BuildUtil::Context &context)
    {
        std::vector<std::string> result;
        for (const auto &value : values)
        {
            result.push_back(expand(value, context));
        }
        return result;
    }

    std::map<std::string, std::string> BuildVar::expand(const std::map<std::string, std::string> &values, const BuildUtil::Context &context)
    {
        std::map<std::string, std::string> result;
        for (const auto &[key, value] : values)
        {
            result[key] = expand(value, context);
        }
        return result;
    }

} // namespace rhodium
