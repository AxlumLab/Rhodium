#include "rhodium/package/Archive.hpp"
#include "rhodium/utils/Sha256.hpp"
#include "rhodium/core/FileSystem.hpp"
#include "rhodium/diagnostics/JsonDiagnosticSink.hpp"

#include <archive.h>
#include <archive_entry.h>
#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <utility>
#include <vector>

namespace
{
    std::filesystem::path tempDir(const std::string &name)
    {
        const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
        auto path = std::filesystem::temp_directory_path() / ("rhodium-" + name + "-" + std::to_string(stamp));
        std::filesystem::create_directories(path);
        return path;
    }

    void writeFile(const std::filesystem::path &path, const std::string &text)
    {
        ASSERT_TRUE(rhodium::File::write(path, text, nullptr));
    }

    void addZipEntry(struct archive *zip, const std::string &name, const std::string &content)
    {
        struct archive_entry *entry = archive_entry_new();
        archive_entry_set_pathname(entry, name.c_str());
        archive_entry_set_filetype(entry, AE_IFREG);
        archive_entry_set_perm(entry, 0644);
        archive_entry_set_size(entry, static_cast<la_int64_t>(content.size()));
        ASSERT_EQ(archive_write_header(zip, entry), ARCHIVE_OK);
        ASSERT_EQ(archive_write_data(zip, content.data(), content.size()), static_cast<la_ssize_t>(content.size()));
        archive_entry_free(entry);
    }

    void createZip(const std::filesystem::path &path, const std::vector<std::pair<std::string, std::string>> &entries)
    {
        struct archive *zip = archive_write_new();
        archive_write_set_format_zip(zip);
        ASSERT_EQ(archive_write_open_filename(zip, path.string().c_str()), ARCHIVE_OK);
        for (const auto &[name, content] : entries)
        {
            addZipEntry(zip, name, content);
        }
        archive_write_close(zip);
        archive_write_free(zip);
    }
}

TEST(ArchiveTests, Sha256FileComputesExpectedHash)
{
    const auto dir = tempDir("sha256");
    writeFile(dir / "hello.txt", "hello\n");
    EXPECT_EQ(rhodium::Sha256::file(dir / "hello.txt"), "5891b5b522d5df086d0ff0b110fbd9d21bb4fc7163af34d08286a2e846f6be03");
    EXPECT_TRUE(rhodium::Sha256::equal("ABCDEF", "abcdef"));
    std::filesystem::remove_all(dir);
}

TEST(ArchiveTests, ExtractZipWithStripComponents)
{
    const auto dir = tempDir("archive-strip");
    const auto zip = dir / "hello.zip";
    createZip(zip, {{"hello-1.0/include/hello.hpp", "#pragma once\n"}});

    rhodium::ArchiveExtractor extractor;
    rhodium::JsonDiagnosticSink diagnostics;
    ASSERT_TRUE(extractor.extract(zip, dir / "out", 1, diagnostics)) << diagnostics.diagnostics().dump();
    EXPECT_TRUE(rhodium::File::exists(dir / "out" / "include" / "hello.hpp"));
    std::filesystem::remove_all(dir);
}

TEST(ArchiveTests, RejectsPathTraversalEntry)
{
    const auto dir = tempDir("archive-unsafe");
    const auto zip = dir / "bad.zip";
    createZip(zip, {{"../escape.txt", "bad"}});

    rhodium::ArchiveExtractor extractor;
    rhodium::JsonDiagnosticSink diagnostics;
    EXPECT_FALSE(extractor.extract(zip, dir / "out", 0, diagnostics));
    EXPECT_NE(diagnostics.diagnostics().dump().find("unsafe archive entry"), std::string::npos);
    std::filesystem::remove_all(dir);
}

TEST(ArchiveTests, FetcherReusesExtractedArchiveWhenMarkerMatches)
{
    const auto dir = tempDir("archive-fetch");
    const auto zip = dir / "hello.zip";
    createZip(zip, {{"hello-1.0/include/hello.hpp", "#pragma once\n"}});

    rhodium::DependencyDesc dep;
    dep.name = "hello";
    dep.source = zip.string();
    dep.sourceType = "archive";
    dep.sha256 = rhodium::Sha256::file(zip);
    dep.stripComponents = 1;

    rhodium::ArchiveFetcher fetcher;
    rhodium::ArchiveFetchResult first;
    rhodium::JsonDiagnosticSink diagnostics;
    ASSERT_TRUE(fetcher.fetchAndExtract(dep, dir, dir / ".tpkg" / "packages", first, diagnostics)) << diagnostics.diagnostics().dump();
    const auto marker = rhodium::File::read(first.sourceDir / ".tpkg_archive_marker");

    rhodium::ArchiveFetchResult second;
    ASSERT_TRUE(fetcher.fetchAndExtract(dep, dir, dir / ".tpkg" / "packages", second, diagnostics)) << diagnostics.diagnostics().dump();
    EXPECT_EQ(rhodium::File::read(second.sourceDir / ".tpkg_archive_marker"), marker);
    std::filesystem::remove_all(dir);
}
