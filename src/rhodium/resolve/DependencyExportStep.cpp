#include "rhodium/resolve/DependencyExportStep.hpp"

#include "rhodium/config/BuildConfig.hpp"
#include "rhodium/core/FileSystem.hpp"
#include "rhodium/diagnostics/DiagnosticSink.hpp"
#include "rhodium/package/ArtifactGc.hpp"
#include "rhodium/package/PackageArtifactStore.hpp"
#include "rhodium/package/PackageBuilderUtil.hpp"

namespace rhodium
{

    bool DepExport::write(PackageArtifact &artifact,
                          LockedPackage &updated,
                          const std::filesystem::path &artifactDir,
                          DiagnosticSink &diagnostics)
    {
        artifact.artifactId = updated.artifactId;
        artifact.buildHash = updated.buildHash;
        artifact.root = artifactDir;
        if (!ArtifactStore::write(artifact, diagnostics))
        {
            return false;
        }

        updated.artifactId = artifact.artifactId;
        updated.artifacts = artifact.artifacts;
        return true;
    }

    bool DepExport::artifactsEqual(const DependencyArtifacts &left, const DependencyArtifacts &right)
    {
        return left.mode == right.mode &&
               left.includeDirs == right.includeDirs &&
               left.libDirs == right.libDirs &&
               left.binDirs == right.binDirs &&
               left.binFiles == right.binFiles &&
               left.libs == right.libs &&
               left.libFiles == right.libFiles &&
               left.libFilesByConfig == right.libFilesByConfig &&
               left.defines == right.defines &&
               left.systemLibs == right.systemLibs &&
               left.frameworks == right.frameworks;
    }

    bool DepExport::reusable(const LockedPackage &locked,
                             const LockedPackage &expected,
                             const std::filesystem::path &artifactDir)
    {
        class NullDiagnosticSink final : public DiagnosticSink
        {
        public:
            void report(const Diagnostic &) override {}
        };

        if (locked.source != expected.source ||
            locked.sourceType != expected.sourceType ||
            locked.sha256 != expected.sha256 ||
            locked.resolvedArchiveHash != expected.resolvedArchiveHash ||
            locked.ref != expected.ref ||
            locked.commit != expected.commit ||
            locked.subdir != expected.subdir ||
            locked.stripComponents != expected.stripComponents ||
            locked.patches != expected.patches ||
            locked.patchHashes != expected.patchHashes ||
            locked.buildType != expected.buildType ||
            locked.linkage != expected.linkage ||
            locked.runtime != expected.runtime ||
            locked.pic != expected.pic ||
            locked.buildHash.empty() ||
            locked.buildHash != expected.buildHash ||
            locked.artifactId != expected.artifactId ||
            locked.buildOptions != expected.buildOptions ||
            BuildConfig::normalize(locked.config) != BuildConfig::normalize(expected.config) ||
            locked.toolchainId != expected.toolchainId ||
            locked.overridden != expected.overridden ||
            locked.overridePath != expected.overridePath ||
            locked.overrideSource != expected.overrideSource ||
            locked.originalSource != expected.originalSource ||
            locked.originalSourceType != expected.originalSourceType ||
            locked.originalRef != expected.originalRef)
        {
            return false;
        }

        const auto metadataPath = artifactDir / "artifact.toml";
        if (!File::exists(metadataPath))
        {
            return false;
        }

        DependencyArtifacts metadata;
        NullDiagnosticSink diagnostics;
        if (!ArtifactStore::read(metadataPath, metadata, diagnostics))
        {
            return false;
        }
        if (!artifactsEqual(metadata, locked.artifacts))
        {
            return false;
        }

        BuildUtil::Context context;
        context.artifactDir = artifactDir;
        return BuildUtil::validate(context, metadata, diagnostics);
    }

    bool DepExport::gc(const LockFile &lockFile,
                       const std::filesystem::path &workspaceRoot,
                       const std::string &config,
                       const std::string &toolchainId,
                       const std::string &packageName,
                       DiagnosticSink &diagnostics)
    {
        return ArtifactGc::runArtifacts(lockFile, workspaceRoot, true, packageName, diagnostics) &&
               ArtifactGc::runBuildCaches(lockFile, workspaceRoot, config, toolchainId, true, packageName, diagnostics) &&
               ArtifactGc::runSourceCaches(lockFile, workspaceRoot, true, packageName, diagnostics);
    }

} // namespace rhodium
