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

        struct Request
        {
            DependencyDesc &dependency;
            LockedPackage *locked;
            bool wantsUpdate;
        };

        static bool source(const Request &request, const Context &context, Result &result, DiagnosticSink &diagnostics);

        static bool urlLike(const std::string &source);

    private:
        static bool sourceLocal(const Request &request, const Context &context, Result &result,
                                DiagnosticSink &diagnostics);
        static bool sourceArchive(const Request &request, const Context &context, Result &result,
                                  DiagnosticSink &diagnostics);
        static bool sourceGit(const Request &request, const Context &context, Result &result,
                              DiagnosticSink &diagnostics);
        static std::filesystem::path local(const std::filesystem::path &workspaceRoot,
                                           const DependencyDesc &dependency);
        static std::filesystem::path reusable(const PackageCache &cache, const DependencyDesc &dependency,
                                              const std::filesystem::path &preferredPath);
        static void move(const std::filesystem::path &from, const std::filesystem::path &to,
                         DiagnosticSink &diagnostics);
        static void removeSiblings(const PackageCache &cache, const DependencyDesc &dependency,
                                   const std::filesystem::path &keepPath, DiagnosticSink &diagnostics);
        static DependencyDesc forGit(const std::filesystem::path &workspaceRoot, const DependencyDesc &dependency);
    };

} // namespace rhodium
