#include "rhodium/package/CacheReferences.hpp"
#include "rhodium/diagnostics/DiagnosticSink.hpp"

#include <system_error>

namespace rhodium
{
    bool CacheReferences::within(const std::filesystem::path &path, const std::filesystem::path &root)
    {
        const auto relative = path.lexically_relative(root);
        return !relative.empty() && !relative.is_absolute() && *relative.begin() != "..";
    }

    bool CacheReferences::localInputs(const LockFile &lockFile, const std::filesystem::path &workspaceRoot,
                                      const std::filesystem::path &cacheRoot,
                                      std::vector<std::filesystem::path> &inputs, DiagnosticSink &diagnostics)
    {
        std::error_code ec;
        const auto canonicalRoot = std::filesystem::weakly_canonical(cacheRoot, ec);
        if (ec)
        {
            diagnostics.error("cannot resolve cache root: " + ec.message());
            return false;
        }
        for (const auto &package : lockFile.packages)
        {
            if (package.sourceType != "local")
            {
                continue;
            }
            const auto value =
                package.overridden && !package.overridePath.empty() ? package.overridePath : package.source;
            if (value.empty())
            {
                diagnostics.error("cannot collect caches with an empty local source: " + package.name);
                return false;
            }
            std::filesystem::path path(value);
            path = std::filesystem::weakly_canonical(path.is_absolute() ? path : workspaceRoot / path, ec);
            if (ec)
            {
                diagnostics.error("cannot resolve local source for cache collection: " + ec.message());
                return false;
            }
            if (within(path, canonicalRoot))
            {
                inputs.push_back((cacheRoot / path.lexically_relative(canonicalRoot)).lexically_normal());
            }
        }
        return true;
    }

    bool CacheReferences::protectsInput(const std::filesystem::path &path,
                                        const std::vector<std::filesystem::path> &inputs)
    {
        const auto normalized = path.lexically_normal();
        for (const auto &input : inputs)
        {
            if (within(normalized, input) || within(input, normalized))
            {
                return true;
            }
        }
        return false;
    }

}
