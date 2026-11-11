#pragma once

#include "rhodium/config/LockFile.hpp"
#include "rhodium/model/DependencyDesc.hpp"
#include "rhodium/resolve/DependencyResolver.hpp"

#include <filesystem>
#include <string>

namespace rhodium
{

    class DiagnosticSink;
    class GitFetcher;
    class PackageCache;

    class DepFetch
    {
    public:
        struct Context
        {
            const std::filesystem::path &workspaceRoot;
            PackageCache &cache;
            GitFetcher &git;
            const LockFile &lockFile;
            const std::string &effectiveConfig;
            const std::string &preferredToolchain;
            const DependencyRestoreOptions &options;
        };

        struct Result
        {
            std::filesystem::path sourceRoot;
            std::string commit;
            bool skipPackage = false;
        };

        static bool source(DependencyDesc &dependency,
                           LockedPackage *locked,
                           bool wantsUpdate,
                           const Context &context,
                           Result &result,
                           DiagnosticSink &diagnostics);

        static bool urlLike(const std::string &source);

    private:
        static std::filesystem::path local(const std::filesystem::path &workspaceRoot,
                                           const DependencyDesc &dependency);
        static std::filesystem::path reusable(const Context &context,
                                              const DependencyDesc &dependency,
                                              const std::filesystem::path &preferredPath);
        static void move(const std::filesystem::path &from,
                         const std::filesystem::path &to,
                         DiagnosticSink &diagnostics);
        static DependencyDesc forGit(const std::filesystem::path &workspaceRoot,
                                     const DependencyDesc &dependency);
    };

} // namespace rhodium
