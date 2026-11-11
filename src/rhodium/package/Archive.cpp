/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by FlybirdGames, All Rights Reserved. 
 */

#include "rhodium/package/Archive.hpp"

#include "rhodium/core/FileSystem.hpp"
#include "rhodium/core/Hash.hpp"
#include "rhodium/core/Path.hpp"
#include "rhodium/diagnostics/DiagnosticSink.hpp"
#include "rhodium/package/PatchApplier.hpp"
#include "rhodium/utils/Sha256.hpp"

#include <archive.h>
#include <archive_entry.h>
#include <curl/curl.h>

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <exception>
#include <fstream>
#include <sstream>
#include <vector>

namespace rhodium
{
    class ArchiveData
    {
    public:
        struct Entry
        {
            std::filesystem::path path;
            bool skipped = false;
            bool unsafe = false;
            std::string reason;
        };

        static bool starts(const std::string &value, const std::string &prefix);
        static bool remote(const std::string &source);
        static bool fileUrl(const std::string &source);
        static std::filesystem::path local(const std::filesystem::path &workspaceRoot, const std::string &source);
        static std::string filename(const std::string &source);
        static std::string key(const DependencyDesc &dependency);
        static std::string marker(const DependencyDesc &dependency,
                                  const std::string &archiveHash,
                                  const std::vector<std::string> &patchHashes);
        static bool matches(const std::filesystem::path &sourceDir,
                            const DependencyDesc &dependency,
                            const std::string &archiveHash,
                            const std::vector<std::string> &patchHashes);
        static size_t curlWrite(char *ptr, size_t size, size_t nmemb, void *userdata);
        static std::string env(const char *name);
        static bool github(const std::string &url);
        static bool copy(const std::filesystem::path &from, const std::filesystem::path &to, DiagnosticSink &diagnostics);
        static bool download(const std::string &url, const std::filesystem::path &to, DiagnosticSink &diagnostics);
        static bool remove(const std::filesystem::path &path);
        static bool downloadRemote(const DependencyDesc &dependency,
                                   const std::filesystem::path &downloadPath,
                                   DiagnosticSink &diagnostics);
        static bool supported(const std::filesystem::path &path);
        static Entry strip(const std::string &entryName, int stripComponents);
        static bool writeMarker(const std::filesystem::path &sourceDir,
                                const DependencyDesc &dependency,
                                const std::string &archiveHash,
                                const std::vector<std::string> &patchHashes,
                                DiagnosticSink &diagnostics);
    };

    bool ArchiveData::starts(const std::string &value, const std::string &prefix)
        {
            return value.rfind(prefix, 0) == 0;
        }

    bool ArchiveData::remote(const std::string &source)
        {
            return starts(source, "http://") || starts(source, "https://");
        }

    bool ArchiveData::fileUrl(const std::string &source)
        {
            return starts(source, "file://");
        }

    std::filesystem::path ArchiveData::local(const std::filesystem::path &workspaceRoot, const std::string &source)
        {
            std::string pathText = fileUrl(source) ? source.substr(7) : source;
#ifdef _WIN32
            if (pathText.size() >= 3 && pathText[0] == '/' && std::isalpha(static_cast<unsigned char>(pathText[1])) && pathText[2] == ':')
            {
                pathText.erase(pathText.begin());
            }
#endif
            auto path = std::filesystem::path(pathText);
            if (path.is_relative())
            {
                path = workspaceRoot / path;
            }
            return path;
        }

    std::string ArchiveData::filename(const std::string &source)
        {
            const auto slash = source.find_last_of("/\\");
            auto name = slash == std::string::npos ? source : source.substr(slash + 1);
            const auto query = name.find_first_of("?#");
            if (query != std::string::npos)
            {
                name = name.substr(0, query);
            }
            return name.empty() ? "archive" : name;
        }

    std::string ArchiveData::key(const DependencyDesc &dependency)
        {
            if (!dependency.sha256.empty())
            {
                return dependency.sha256;
            }
            return Hash::xxhash64Hex(dependency.source);
        }

    std::string ArchiveData::marker(const DependencyDesc &dependency,
                                    const std::string &archiveHash,
                                    const std::vector<std::string> &patchHashes)
        {
            std::ostringstream marker;
            marker << "archive_hash=" << archiveHash << "\n";
            marker << "strip_components=" << dependency.stripComponents << "\n";
            marker << "source=" << dependency.source << "\n";
            for (std::size_t i = 0; i < dependency.patches.size(); ++i)
            {
                marker << "patch=" << dependency.patches[i] << "\n";
                marker << "patch_hash=" << (i < patchHashes.size() ? patchHashes[i] : std::string{}) << "\n";
            }
            return marker.str();
        }

    bool ArchiveData::matches(const std::filesystem::path &sourceDir,
                              const DependencyDesc &dependency,
                              const std::string &archiveHash,
                              const std::vector<std::string> &patchHashes)
        {
            const auto markerPath = sourceDir / ".tpkg_archive_marker";
            return File::dir(sourceDir) &&
                   File::exists(markerPath) &&
                   File::read(markerPath) == marker(dependency, archiveHash, patchHashes);
        }

    size_t ArchiveData::curlWrite(char *ptr, size_t size, size_t nmemb, void *userdata)
        {
            auto *output = static_cast<std::ofstream *>(userdata);
            const auto bytes = size * nmemb;
            output->write(ptr, static_cast<std::streamsize>(bytes));
            return output->good() ? bytes : 0;
        }

    std::string ArchiveData::env(const char *name)
        {
#ifdef _MSC_VER
            char *value = nullptr;
            std::size_t size = 0;
            if (_dupenv_s(&value, &size, name) == 0 && value)
            {
                std::string result(value);
                std::free(value);
                return result;
            }
            std::free(value);
            return {};
#else
            const char *value = std::getenv(name);
            return value ? std::string(value) : std::string{};
#endif
        }

    bool ArchiveData::github(const std::string &url)
        {
            return starts(url, "https://github.com/") || starts(url, "http://github.com/");
        }

    bool ArchiveData::copy(const std::filesystem::path &from, const std::filesystem::path &to, DiagnosticSink &diagnostics)
        {
            std::string error;
            if (!File::mkdir(to.parent_path(), &error))
            {
                diagnostics.error("failed to create archive cache directory: " + error);
                return false;
            }
            try
            {
                std::filesystem::copy_file(from, to, std::filesystem::copy_options::overwrite_existing);
                return true;
            }
            catch (const std::exception &ex)
            {
                diagnostics.error("failed to copy archive " + from.string() + " to " + to.string() + ": " + ex.what());
                return false;
            }
        }

    bool ArchiveData::download(const std::string &url, const std::filesystem::path &to, DiagnosticSink &diagnostics)
        {
            std::string error;
            if (!File::mkdir(to.parent_path(), &error))
            {
                diagnostics.error("failed to create archive download directory: " + error);
                return false;
            }
            const auto part = to.string() + ".part";
            std::ofstream output(part, std::ios::binary | std::ios::trunc);
            if (!output)
            {
                diagnostics.error("failed to open archive download target: " + part);
                return false;
            }

            CURL *curl = curl_easy_init();
            if (!curl)
            {
                diagnostics.error("failed to initialize curl for " + url);
                return false;
            }
            char errorBuffer[CURL_ERROR_SIZE] = {};
            curl_slist *headers = nullptr;
            const auto token = !env("GITHUB_TOKEN").empty()
                                   ? env("GITHUB_TOKEN")
                                   : env("GH_TOKEN");
            const auto authHeader = "Authorization: Bearer " + token;
            if (github(url) && !token.empty())
            {
                headers = curl_slist_append(headers, authHeader.c_str());
                curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
            }
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
            curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
            curl_easy_setopt(curl, CURLOPT_NETRC, CURL_NETRC_OPTIONAL);
            curl_easy_setopt(curl, CURLOPT_USERAGENT, "Rhodium/0");
            curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errorBuffer);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWrite);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &output);
            const auto result = curl_easy_perform(curl);
            long responseCode = 0;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);
            if (headers)
            {
                curl_slist_free_all(headers);
            }
            curl_easy_cleanup(curl);
            output.close();
            if (result != CURLE_OK)
            {
                std::filesystem::remove(part);
                diagnostics.error("failed to download archive " + url + " to " + to.string() + ": " +
                                  (errorBuffer[0] ? errorBuffer : curl_easy_strerror(result)));
                return false;
            }
            if (responseCode >= 400)
            {
                std::filesystem::remove(part);
                diagnostics.error("failed to download archive " + url + " to " + to.string() +
                                  ": HTTP " + std::to_string(responseCode));
                return false;
            }
            try
            {
                std::filesystem::rename(part, to);
            }
            catch (const std::exception &ex)
            {
                diagnostics.error("failed to finalize archive download " + to.string() + ": " + ex.what());
                return false;
            }
            return true;
        }

    bool ArchiveData::remove(const std::filesystem::path &path)
        {
            try
            {
                return !File::exists(path) || std::filesystem::remove(path);
            }
            catch (...)
            {
                return false;
            }
        }

    bool ArchiveData::downloadRemote(const DependencyDesc &dependency,
                                     const std::filesystem::path &downloadPath,
                                     DiagnosticSink &diagnostics)
        {
            if (dependency.sha256.empty())
            {
                diagnostics.warning("remote archive dependency has no sha256: " + dependency.name);
            }

            std::vector<std::string> urlsToTry;
            urlsToTry.push_back(dependency.source);
            for (const auto &mirror : dependency.mirrors)
            {
                urlsToTry.push_back(mirror);
            }

            for (std::size_t i = 0; i < urlsToTry.size(); ++i)
            {
                const auto &url = urlsToTry[i];
                if (i == 0)
                {
                    diagnostics.info("downloading archive " + dependency.name + " from " + url);
                }
                else
                {
                    diagnostics.info("trying mirror " + std::to_string(i) + ": " + url);
                }

                if (download(url, downloadPath, diagnostics))
                {
                    return true;
                }

                if (i + 1 < urlsToTry.size())
                {
                    remove(downloadPath);
                    diagnostics.warning("failed to download from " + url + ", trying next mirror");
                }
            }

            diagnostics.error("failed to download archive from all sources (tried " + std::to_string(urlsToTry.size()) + " URL(s))");
            return false;
        }

    bool ArchiveData::supported(const std::filesystem::path &path)
        {
            const auto name = [&] {
                auto text = path.filename().string();
                std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
                return text;
            }();
            for (const auto &suffix : {".zip", ".tar", ".tar.gz", ".tgz", ".tar.xz", ".txz", ".tar.bz2", ".tbz2"})
            {
                if (name.size() >= std::string(suffix).size() && name.substr(name.size() - std::string(suffix).size()) == suffix)
                {
                    return true;
                }
            }
            return false;
        }

    ArchiveData::Entry ArchiveData::strip(const std::string &entryName, int stripComponents)
        {
            Entry result;
            std::filesystem::path raw(entryName);
            if (raw.is_absolute())
            {
                result.unsafe = true;
                result.reason = "absolute path";
                return result;
            }
            std::vector<std::filesystem::path> parts;
            for (const auto &part : raw)
            {
                const auto text = part.string();
                if (text.empty() || text == ".")
                {
                    continue;
                }
                if (text == "..")
                {
                    result.unsafe = true;
                    result.reason = "path traversal";
                    return result;
                }
                parts.push_back(part);
            }
            if (static_cast<int>(parts.size()) <= stripComponents)
            {
                result.skipped = true;
                return result;
            }
            std::filesystem::path out;
            for (std::size_t i = static_cast<std::size_t>(stripComponents); i < parts.size(); ++i)
            {
                out /= parts[i];
            }
            result.path = out;
            return result;
        }

    bool ArchiveData::writeMarker(const std::filesystem::path &sourceDir,
                                  const DependencyDesc &dependency,
                                  const std::string &archiveHash,
                                  const std::vector<std::string> &patchHashes,
                                  DiagnosticSink &diagnostics)
        {
            std::string error;
            if (!File::write(sourceDir / ".tpkg_archive_marker",
                                                    marker(dependency, archiveHash, patchHashes),
                                                    &error))
            {
                diagnostics.error("failed to write archive marker: " + error);
                return false;
            }
            return true;
        }

    bool ArchiveExtractor::available() const
    {
        return archive_version_string() != nullptr;
    }

    bool ArchiveExtractor::extract(const std::filesystem::path &archivePath,
                                   const std::filesystem::path &destination,
                                   int stripComponents,
                                   DiagnosticSink &diagnostics) const
    {
        if (!ArchiveData::supported(archivePath))
        {
            diagnostics.error("unsupported archive extension: " + archivePath.extension().string() +
                              " supported: zip, tar, tar.gz, tgz, tar.xz, txz, tar.bz2, tbz2");
            return false;
        }

        struct archive *archive = archive_read_new();
        archive_read_support_filter_all(archive);
        archive_read_support_format_all(archive);
        if (archive_read_open_filename(archive, archivePath.string().c_str(), 10240) != ARCHIVE_OK)
        {
            diagnostics.error("archive extraction failed: " + archivePath.string() + ": " + archive_error_string(archive));
            archive_read_free(archive);
            return false;
        }

        std::string error;
        if (!File::mkdir(destination, &error))
        {
            diagnostics.error("failed to create archive extraction directory: " + error);
            archive_read_free(archive);
            return false;
        }

        struct archive_entry *entry = nullptr;
        while (archive_read_next_header(archive, &entry) == ARCHIVE_OK)
        {
            const auto entryName = std::string(archive_entry_pathname(entry) ? archive_entry_pathname(entry) : "");
            const auto relative = ArchiveData::strip(entryName, stripComponents);
            if (relative.unsafe)
            {
                diagnostics.error("unsafe archive entry: " + entryName + " reason: " + relative.reason);
                archive_read_free(archive);
                return false;
            }
            if (relative.skipped || relative.path.empty())
            {
                archive_read_data_skip(archive);
                continue;
            }
            const auto outPath = (destination / relative.path).lexically_normal();
            const auto relToDest = outPath.lexically_relative(destination);
            if (relToDest.empty() || relToDest.string().rfind("..", 0) == 0)
            {
                diagnostics.error("unsafe archive entry: " + entryName + " reason: path traversal");
                archive_read_free(archive);
                return false;
            }

            archive_entry_set_pathname(entry, outPath.string().c_str());
            const auto type = archive_entry_filetype(entry);
            if (type == AE_IFDIR)
            {
                File::mkdir(outPath, nullptr);
                continue;
            }
            if (!File::mkdir(outPath.parent_path(), &error))
            {
                diagnostics.error("failed to create archive entry parent: " + error);
                archive_read_free(archive);
                return false;
            }
            const int result = archive_read_extract(archive, entry, ARCHIVE_EXTRACT_TIME | ARCHIVE_EXTRACT_PERM);
            if (result != ARCHIVE_OK)
            {
                diagnostics.error("archive extraction failed: " + archivePath.string() + " entry=" + entryName + ": " + archive_error_string(archive));
                archive_read_free(archive);
                return false;
            }
        }

        archive_read_free(archive);
        return true;
    }

    bool ArchiveFetcher::available() const
    {
        return curl_version_info(CURLVERSION_NOW) != nullptr;
    }

    bool ArchiveFetcher::fetchAndExtract(const DependencyDesc &dependency,
                                         const std::filesystem::path &workspaceRoot,
                                         const std::filesystem::path &cacheRoot,
                                         ArchiveFetchResult &result,
                                         DiagnosticSink &diagnostics) const
    {
        const auto key = ArchiveData::key(dependency);
        const auto archiveRoot = cacheRoot / "_archives" / Path::sanitizePackagePath(dependency.name) / Path::sanitizePackagePath(key);
        const auto downloadPath = archiveRoot / "download" / ArchiveData::filename(dependency.source);
        const auto sourceDir = archiveRoot / "src";
        result.archivePath = downloadPath;
        result.sourceDir = sourceDir;

        if (!File::exists(downloadPath))
        {
            if (ArchiveData::remote(dependency.source))
            {
                if (!ArchiveData::downloadRemote(dependency, downloadPath, diagnostics))
                {
                    return false;
                }
            }
            else
            {
                const auto local = ArchiveData::local(workspaceRoot, dependency.source);
                if (!File::exists(local))
                {
                    diagnostics.error("archive source is missing: " + local.string());
                    return false;
                }
                if (!ArchiveData::copy(local, downloadPath, diagnostics))
                {
                    return false;
                }
            }
        }

        std::string hashError;
        result.archiveHash = sha256File(downloadPath, &hashError);
        if (result.archiveHash.empty())
        {
            diagnostics.error("failed to hash archive " + downloadPath.string() + ": " + hashError);
            return false;
        }
        if (!dependency.sha256.empty() && !sha256Equals(result.archiveHash, dependency.sha256))
        {
            const auto badPath = downloadPath.string() + ".bad";
            std::filesystem::rename(downloadPath, badPath);
            diagnostics.error("sha256 mismatch for archive " + downloadPath.string() +
                              " expected=" + dependency.sha256 + " actual=" + result.archiveHash);
            return false;
        }

        std::vector<std::string> patchHashes;
        for (const auto &patch : dependency.patches)
        {
            std::string patchError;
            const auto patchPath = std::filesystem::path(patch).is_absolute() ? std::filesystem::path(patch) : workspaceRoot / patch;
            const auto hash = sha256File(patchPath, &patchError);
            if (hash.empty())
            {
                diagnostics.error("failed to hash patch " + patchPath.string() + ": " + patchError);
                return false;
            }
            patchHashes.push_back(hash);
        }
        if (ArchiveData::matches(sourceDir, dependency, result.archiveHash, patchHashes))
        {
            result.patchHashes = patchHashes;
            diagnostics.info("archive source is up to date: " + dependency.name);
            return true;
        }

        const auto tmp = archiveRoot / "src.tmp";
        try
        {
            std::filesystem::remove_all(tmp);
            std::filesystem::remove_all(sourceDir);
        }
        catch (const std::exception &ex)
        {
            diagnostics.error("failed to clean archive extraction directory: " + std::string(ex.what()));
            return false;
        }

        ArchiveExtractor extractor;
        if (!extractor.extract(downloadPath, tmp, dependency.stripComponents, diagnostics))
        {
            std::filesystem::remove_all(tmp);
            if (!ArchiveData::remote(dependency.source) || !ArchiveData::remove(downloadPath))
            {
                return false;
            }

            diagnostics.warning("cached archive was invalid; re-downloading: " + dependency.name);
            if (!ArchiveData::downloadRemote(dependency, downloadPath, diagnostics))
            {
                return false;
            }

            result.archiveHash = sha256File(downloadPath, &hashError);
            if (result.archiveHash.empty())
            {
                diagnostics.error("failed to hash archive " + downloadPath.string() + ": " + hashError);
                return false;
            }
            if (!dependency.sha256.empty() && !sha256Equals(result.archiveHash, dependency.sha256))
            {
                const auto badPath = downloadPath.string() + ".bad";
                std::filesystem::rename(downloadPath, badPath);
                diagnostics.error("sha256 mismatch for archive " + downloadPath.string() +
                                  " expected=" + dependency.sha256 + " actual=" + result.archiveHash);
                return false;
            }

            if (!extractor.extract(downloadPath, tmp, dependency.stripComponents, diagnostics))
            {
                std::filesystem::remove_all(tmp);
                return false;
            }
        }
        if (!dependency.patches.empty())
        {
            PatchApplier patcher;
            if (!patcher.apply(tmp, workspaceRoot, dependency.patches, dependency.stripComponents, result.patchHashes, diagnostics))
            {
                std::filesystem::remove_all(tmp);
                return false;
            }
        }
        else
        {
            result.patchHashes = patchHashes;
        }
        if (!ArchiveData::writeMarker(tmp, dependency, result.archiveHash, result.patchHashes, diagnostics))
        {
            std::filesystem::remove_all(tmp);
            return false;
        }
        try
        {
            std::filesystem::rename(tmp, sourceDir);
        }
        catch (const std::exception &ex)
        {
            diagnostics.error("failed to finalize archive source directory: " + std::string(ex.what()));
            return false;
        }
        return true;
    }
}
