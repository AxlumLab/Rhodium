#include "rhodium/core/WorkspaceLock.hpp"
#include "rhodium/core/Path.hpp"

#include "rhodium/cli/Commands.hpp"

#include "rhodium/cmake/CMakeDependencyExporter.hpp"
#include "rhodium/config/LockFile.hpp"
#include "rhodium/core/FileSystem.hpp"
#include "rhodium/resolve/PackageArtifactResolver.hpp"

#include <nlohmann/json.hpp>

#include <filesystem>
#include <string>
#include <vector>

namespace rhodium
{
    namespace
    {
        class RestoredPackages final
        {
        public:
            struct Request
            {
                const std::filesystem::path &workspaceRoot;
                const std::string &toolchainId;
                const std::string &configOption;
                const std::vector<std::string> &cliOverrides;
            };

            struct Result
            {
                std::vector<ResolvedDependencyArtifact> packages;
                CMakeDependencyExportMetadata metadata;
                BuildModel model;
            };

            static bool load(const Request &request, Result &result, ConsoleDiagnosticSink &diagnostics)
            {
                const auto &[workspaceRoot, toolchainId, configOption, cliOverrides] = request;
                BuildModel model;
                if (!CliWorkspace::load(workspaceRoot, model, diagnostics))
                {
                    return false;
                }
                const auto config = CliWorkspace::config(model, configOption, diagnostics);
                if (config.empty())
                {
                    return false;
                }
                const auto effectiveToolchain = CliWorkspace::toolchain(workspaceRoot, toolchainId, diagnostics);
                if (effectiveToolchain.empty())
                {
                    return false;
                }
                ToolchainProfile selectedToolchain;
                if (!CliWorkspace::selectToolchain(workspaceRoot, effectiveToolchain, selectedToolchain, diagnostics))
                {
                    return false;
                }

                DependencyOverrideSet overrides;
                if (!CliWorkspace::overrides(model, workspaceRoot, cliOverrides, overrides, diagnostics))
                {
                    return false;
                }

                LockFile lockFile;
                if (!LockFile::read(workspaceRoot / "tpkg.lock.toml", lockFile, diagnostics))
                {
                    return false;
                }
                if (!model.rootPackage.dependencies.empty() && lockFile.packages.empty())
                {
                    diagnostics.error(CliWorkspace::missingDependencies(config, selectedToolchain.id));
                    return false;
                }

                PackageArtifactResolver packageResolver;
                if (!packageResolver.load(lockFile, workspaceRoot, BuildSelection{config, selectedToolchain.id},
                                          overrides, diagnostics))
                {
                    return false;
                }
                for (const auto &dependency : model.rootPackage.dependencies)
                {
                    if (!packageResolver.find(dependency.name))
                    {
                        diagnostics.error(CliWorkspace::missingDependencies(config, selectedToolchain.id));
                        return false;
                    }
                }
                result.packages = packageResolver.packages();
                result.model = model;
                {
                    result.metadata.toolchainId = selectedToolchain.id;
                    result.metadata.compilerKind = selectedToolchain.compilerKind;
                    result.metadata.config = config;
                    result.metadata.arch = selectedToolchain.targetArch;
                    result.metadata.runtime = CliWorkspace::runtime(lockFile, config, selectedToolchain.id);
                    result.metadata.linkage = CliWorkspace::linkage(lockFile, config, selectedToolchain.id);
                }
                return true;
            }
        };
    }

    int Commands::generate(const std::filesystem::path &workspaceRoot, const std::string &toolchainId,
                           const std::string &configOption, const std::filesystem::path &outputDir,
                           const std::vector<std::string> &cliOverrides)
    {
        ConsoleDiagnosticSink diagnostics;
        auto workspaceLock = WorkspaceLock::acquire(workspaceRoot, diagnostics);
        if (!workspaceLock)
        {
            return 1;
        }
        RestoredPackages::Result result;
        if (!RestoredPackages::load({workspaceRoot, toolchainId, configOption, cliOverrides}, result, diagnostics))
        {
            return 1;
        }
        const auto &[packages, metadata, model] = result;
        const auto resolvedOutputDir = outputDir.empty()
                                           ? Path::stateRoot(workspaceRoot) / "generated" / "cmake"
                                           : (outputDir.is_relative() ? workspaceRoot / outputDir : outputDir);
        if (!CMakeDependency::writeFiles(packages, resolvedOutputDir, metadata, diagnostics))
        {
            return 1;
        }
        diagnostics.info("wrote CMake dependency files under " + resolvedOutputDir.string());
        diagnostics.info("CMake consumers can use: find_package(rhodium CONFIG REQUIRED)");
        diagnostics.info("add this path to CMAKE_PREFIX_PATH: " + resolvedOutputDir.string());

        const auto presetPrefix = std::string("${sourceDir}/") +
                                  Path::stateRoot(workspaceRoot).filename().generic_string() + "/generated/cmake";
        if (model.workspace.generateCMakeUserPresets)
        {
            const auto presetsPath = workspaceRoot / "CMakeUserPresets.json";
            if (File::exists(presetsPath))
            {
                diagnostics.info("CMakeUserPresets.json already exists; not overwriting it");
                diagnostics.info("add this cache variable to an existing preset if desired: CMAKE_PREFIX_PATH=" +
                                 presetPrefix);
            }
            else
            {
                nlohmann::json preset;
                preset["version"] = 2;
                preset["configurePresets"] = nlohmann::json::array({{
                    {"name", "rhodium-deps"},
                    {"displayName", "rhodium dependency prefix"},
                    {"hidden", true},
                    {"cacheVariables", {{"CMAKE_PREFIX_PATH", presetPrefix}}},
                }});
                std::string error;
                if (!File::write(presetsPath, preset.dump(2) + "\n", &error))
                {
                    diagnostics.error("failed to write CMakeUserPresets.json: " + error);
                    return 1;
                }
                diagnostics.info("wrote optional CMakeUserPresets.json with hidden preset 'rhodium-deps'");
            }
        }
        return 0;
    }

} // namespace rhodium
