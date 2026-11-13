/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by AxlumLab, All Rights Reserved.
 */

#include "rhodium/package/ArtifactValidator.hpp"

#include "rhodium/core/Environment.hpp"
#include "rhodium/core/StringUtil.hpp"
#include "rhodium/diagnostics/DiagnosticSink.hpp"

#include <filesystem>
#include <sstream>
#include <string>
#include <vector>

namespace rhodium
{
    bool ArtifactCheck::dirs(const BuildUtil::Context &context,
                             const std::vector<std::string> &dirs,
                             const char *field,
                             DiagnosticSink &diagnostics)
    {
        for (const auto &value : dirs)
        {
            if (value.empty())
            {
                diagnostics.error(std::string("package artifact ") + field + " path is empty");
                return false;
            }

            auto path = std::filesystem::path(value);
            if (path.is_relative())
            {
                path = context.artifactDir / path;
            }
            if (!std::filesystem::is_directory(path))
            {
                diagnostics.error(std::string("package artifact ") + field + " path is missing: " + path.string());
                return false;
            }
        }
        return true;
    }

    std::vector<std::string> ArtifactCheck::libCandidates(const std::string &library, const BuildUtil::Context &context)
    {
        std::filesystem::path path(library);
        if (path.has_extension())
        {
            return {library};
        }

        const auto platform = StringUtils::toLower(context.platform.empty() ? Environment::hostPlatformName() : context.platform);
        const auto archiver = StringUtils::toLower(context.archiver);
        const bool mingw = platform == "mingw" ||
                           platform.find("mingw") != std::string::npos ||
                           archiver.find("mingw") != std::string::npos ||
                           archiver.find("llvm-ar") != std::string::npos ||
                           archiver == "ar" ||
                           (archiver.size() >= 6 && archiver.substr(archiver.size() - 6) == "ar.exe");
        if (platform == "windows" && archiver.empty())
        {
            return {library + ".lib", "lib" + library + ".lib", "lib" + library + ".a", "lib" + library + ".dll.a", library};
        }
        if (platform == "windows" && mingw)
        {
            return {"lib" + library + ".a", library + ".lib", "lib" + library + ".dll.a", library};
        }
        if (platform == "windows")
        {
            return {library + ".lib", "lib" + library + ".lib", library};
        }
        if (platform == "macos" || platform == "darwin")
        {
            return {"lib" + library + ".a", "lib" + library + ".dylib", library + ".lib", "lib" + library + ".lib", library};
        }
        return {"lib" + library + ".a", "lib" + library + ".so", library + ".lib", "lib" + library + ".lib", library};
    }

    bool ArtifactCheck::libs(const BuildUtil::Context &context,
                             const DependencyArtifacts &artifacts,
                             DiagnosticSink &diagnostics)
    {
        for (const auto &value : artifacts.libFiles)
        {
            if (value.empty())
            {
                diagnostics.error("package artifact lib_files path is empty");
                return false;
            }
            auto path = std::filesystem::path(value);
            if (path.is_relative())
            {
                path = context.artifactDir / path;
            }
            if (!std::filesystem::is_regular_file(path))
            {
                diagnostics.error("package artifact lib_files path is missing: " + path.string());
                return false;
            }
        }
        if (artifacts.libs.empty())
        {
            return true;
        }
        if (artifacts.libDirs.empty())
        {
            diagnostics.error("package artifact libs are declared without lib_dirs");
            return false;
        }

        for (const auto &library : artifacts.libs)
        {
            bool found = false;
            std::vector<std::filesystem::path> attempted;
            for (const auto &libDir : artifacts.libDirs)
            {
                auto root = std::filesystem::path(libDir);
                if (root.is_relative())
                {
                    root = context.artifactDir / root;
                }
                for (const auto &candidate : libCandidates(library, context))
                {
                    const auto candidatePath = root / candidate;
                    attempted.push_back(candidatePath);
                    if (std::filesystem::is_regular_file(candidatePath))
                    {
                        found = true;
                        break;
                    }
                }
                if (found)
                {
                    break;
                }
            }
            if (!found)
            {
                std::ostringstream message;
                message << "package artifact library is missing: " << library << "; tried: ";
                for (std::size_t i = 0; i < attempted.size(); ++i)
                {
                    if (i != 0)
                    {
                        message << ", ";
                    }
                    message << attempted[i].string();
                }
                diagnostics.error(message.str());
                return false;
            }
        }
        return true;
    }

    bool ArtifactCheck::files(const BuildUtil::Context &context,
                              const std::vector<std::string> &files,
                              const char *field,
                              DiagnosticSink &diagnostics)
    {
        for (const auto &value : files)
        {
            if (value.empty())
            {
                diagnostics.error(std::string("package artifact ") + field + " path is empty");
                return false;
            }
            auto path = std::filesystem::path(value);
            if (path.is_relative())
            {
                path = context.artifactDir / path;
            }
            if (!std::filesystem::is_regular_file(path))
            {
                diagnostics.error(std::string("package artifact ") + field + " path is missing: " + path.string());
                return false;
            }
        }
        return true;
    }

    bool ArtifactCheck::validate(const BuildUtil::Context &context,
                                 const DependencyArtifacts &artifacts,
                                 DiagnosticSink &diagnostics)
    {
        return dirs(context, artifacts.includeDirs, "include_dirs", diagnostics) &&
               dirs(context, artifacts.libDirs, "lib_dirs", diagnostics) &&
               dirs(context, artifacts.binDirs, "bin_dirs", diagnostics) &&
               files(context, artifacts.binFiles, "bin_files", diagnostics) &&
               libs(context, artifacts, diagnostics);
    }

} // namespace rhodium
