/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by FlybirdGames, All Rights Reserved. 
 */

#include "rhodium/package/GitFetcher.hpp"

#include "rhodium/core/FileSystem.hpp"
#include "rhodium/diagnostics/DiagnosticSink.hpp"

#include <git2.h>
#include <git2/submodule.h>

#include <cstddef>
#include <cstdlib>
#include <string>
#include <iostream>
#include <iomanip>

namespace rhodium
{

    class GitFetch
    {
    public:
        struct Progress
        {
            DiagnosticSink *diagnostics = nullptr;
            std::string packageName;
            int lastPercent = -1;
            bool progressBarActive = false;
        };

        static std::string error(const std::string &prefix);
        static void clear();
        static void draw(const std::string &packageName, int percent, bool completed = false);
        static int progress(const git_indexer_progress *stats, void *payload);
        static std::string env(const char *key);
        static std::string proxyEnv();
        static std::string redact(const std::string &url);
        static void proxy(git_fetch_options &options, std::string &proxyUrl, DiagnosticSink &diagnostics);
        static std::string url(const std::string &source);
        static bool open(const DependencyDesc &dependency, const std::filesystem::path &destination,
                         git_repository **repo, bool &cloned, DiagnosticSink &diagnostics);
        static bool rememberHead(git_repository *repo, const git_oid *oid, DiagnosticSink &diagnostics);
        static bool fetch(git_repository *repo, const std::string &packageName, DiagnosticSink &diagnostics);
        static bool resolve(git_repository *repo, const std::string &ref, git_object **object);
        static bool checkout(git_repository *repo, git_object *object, std::string &commit,
                             DiagnosticSink &diagnostics);
        static bool submodules(git_repository *repo, const std::string &packageName, DiagnosticSink &diagnostics);
    };

    std::string GitFetch::error(const std::string &prefix)
    {
        const git_error *error = git_error_last();
        if (error && error->message)
        {
            return prefix + ": " + error->message;
        }
        return prefix;
    }

    void GitFetch::clear()
    {
        // Clear the current line and move cursor to beginning
        std::cout << "\r" << std::string(80, ' ') << "\r" << std::flush;
    }

    void GitFetch::draw(const std::string &packageName, int percent, bool completed)
    {
        const int barWidth = 40;
        int pos = (barWidth * percent) / 100;

        std::cout << "\r";
        std::cout << "info: git " << packageName << " transfer [";
        for (int i = 0; i < barWidth; ++i)
        {
            if (i < pos)
                std::cout << "=";
            else if (i == pos && !completed)
                std::cout << ">";
            else if (completed && i < barWidth)
                std::cout << "=";
            else
                std::cout << " ";
        }
        std::cout << "] " << std::setw(3) << percent << "%";

        if (completed)
        {
            std::cout << " completed" << std::endl;
        }
        else
        {
            std::cout << std::flush;
        }
    }

    int GitFetch::progress(const git_indexer_progress *stats, void *payload)
    {
        auto *state = static_cast<Progress *>(payload);
        if (!state || !state->diagnostics || !stats || stats->total_objects == 0)
        {
            return 0;
        }

        const int percent =
            static_cast<int>((static_cast<std::size_t>(stats->received_objects) * 100) / stats->total_objects);

        if (percent != state->lastPercent)
        {
            state->lastPercent = percent;
            state->progressBarActive = true;

            // When complete, show full progress bar with "completed" message
            if (percent >= 100)
            {
                draw(state->packageName, 100, true);
                state->progressBarActive = false;
            }
            else
            {
                draw(state->packageName, percent, false);
            }
        }
        return 0;
    }

    std::string GitFetch::env(const char *key)
    {
#ifdef _MSC_VER
        char *value = nullptr;
        std::size_t size = 0;
        if (_dupenv_s(&value, &size, key) == 0 && value && value[0] != '\0')
        {
            std::string result(value);
            std::free(value);
            return result;
        }
        std::free(value);
        return {};
#else
        if (const char *value = std::getenv(key); value && value[0] != '\0')
        {
            return value;
        }
        return {};
#endif
    }

    std::string GitFetch::proxyEnv()
    {
        const char *keys[] = {
            "HTTPS_PROXY", "https_proxy", "HTTP_PROXY", "http_proxy", "ALL_PROXY", "all_proxy",
        };
        for (const char *key : keys)
        {
            if (auto value = env(key); !value.empty())
            {
                return value;
            }
        }
        return {};
    }

    std::string GitFetch::redact(const std::string &url)
    {
        const auto scheme = url.find("://");
        if (scheme == std::string::npos)
        {
            return url;
        }
        const auto authorityStart = scheme + 3;
        const auto at = url.find('@', authorityStart);
        if (at == std::string::npos)
        {
            return url;
        }
        return url.substr(0, authorityStart) + "***@" + url.substr(at + 1);
    }

    void GitFetch::proxy(git_fetch_options &options, std::string &proxyUrl, DiagnosticSink &diagnostics)
    {
        proxyUrl = proxyEnv();
        if (!proxyUrl.empty())
        {
            options.proxy_opts.type = GIT_PROXY_SPECIFIED;
            options.proxy_opts.url = proxyUrl.c_str();
            diagnostics.info("git proxy: environment " + redact(proxyUrl));
            return;
        }

        options.proxy_opts.type = GIT_PROXY_AUTO;
        diagnostics.info("git proxy: auto");
    }

    std::string GitFetch::url(const std::string &source)
    {
        if (source.rfind("http://", 0) == 0 || source.rfind("https://", 0) == 0 || source.rfind("file://", 0) == 0 ||
            source.rfind("git@", 0) == 0 || source.rfind("ssh://", 0) == 0)
        {
            return source;
        }
        if (source.rfind("github.com/", 0) == 0)
        {
            if (source.size() >= 4 && source.substr(source.size() - 4) == ".git")
            {
                return "https://" + source;
            }
            return "https://" + source + ".git";
        }
        return source;
    }

    bool GitFetch::open(const DependencyDesc &dependency, const std::filesystem::path &destination,
                        git_repository **repo, bool &cloned, DiagnosticSink &diagnostics)
    {
        cloned = false;
        const auto gitDir = destination / ".git";
        if (File::dir(gitDir))
        {
            if (git_repository_open(repo, destination.string().c_str()) != 0)
            {
                diagnostics.error(error("failed to open git package cache"));
                return false;
            }
            return true;
        }

        std::string mkdirError;
        if (!File::mkdir(destination.parent_path(), &mkdirError))
        {
            diagnostics.error("failed to create package source parent: " + mkdirError);
            return false;
        }

        // Try primary source first, then mirrors
        std::vector<std::string> urlsToTry;
        urlsToTry.push_back(url(dependency.source));
        for (const auto &mirror : dependency.mirrors)
        {
            urlsToTry.push_back(url(mirror));
        }

        Progress state;
        state.diagnostics = &diagnostics;
        state.packageName = dependency.name;
        state.progressBarActive = false;
        git_clone_options options = GIT_CLONE_OPTIONS_INIT;
        options.fetch_opts.callbacks.transfer_progress = GitFetch::progress;
        options.fetch_opts.callbacks.payload = &state;
        std::string proxyUrl;
        proxy(options.fetch_opts, proxyUrl, diagnostics);

        int result = -1;
        std::string lastUrl;
        for (size_t i = 0; i < urlsToTry.size(); ++i)
        {
            const auto &url = urlsToTry[i];
            lastUrl = url;

            if (i == 0)
            {
                diagnostics.info("cloning package " + dependency.name + " from " + url + " into " +
                                 destination.string());
            }
            else
            {
                diagnostics.info("trying mirror " + std::to_string(i) + ": " + url);
            }

            result = git_clone(repo, url.c_str(), destination.string().c_str(), &options);

            // If progress bar didn't complete naturally (error or no progress), clear it
            if (state.progressBarActive)
            {
                clear();
            }

            if (result == 0)
            {
                cloned = true;
                diagnostics.info("cloned package " + dependency.name + " from " + url);
                return true;
            }

            // Clean up failed clone attempt before trying next mirror
            if (i + 1 < urlsToTry.size())
            {
                try
                {
                    if (File::dir(destination))
                    {
                        std::filesystem::remove_all(destination);
                    }
                }
                catch (const std::exception &ex)
                {
                    diagnostics.error("failed to clean incomplete clone: " + std::string(ex.what()));
                    return false;
                }
                diagnostics.warning("failed to clone from " + url + ", trying next mirror");
            }
        }

        // All attempts failed
        diagnostics.error(
            error("failed to clone " + dependency.source + " (tried " + std::to_string(urlsToTry.size()) + " URL(s))"));
        return false;
    }

    bool GitFetch::rememberHead(git_repository *repo, const git_oid *oid, DiagnosticSink &diagnostics)
    {
        git_reference *head = nullptr;
        const int result =
            oid ? git_reference_create(&head, repo, "refs/remotes/origin/HEAD", oid, 1, "refresh remote HEAD")
                : git_reference_remove(repo, "refs/remotes/origin/HEAD");
        git_reference_free(head);
        if (result != 0 && (oid || result != GIT_ENOTFOUND))
        {
            diagnostics.error(error("failed to refresh cached remote HEAD"));
            return false;
        }
        return true;
    }

    bool GitFetch::fetch(git_repository *repo, const std::string &packageName, DiagnosticSink &diagnostics)
    {
        git_remote *remote = nullptr;
        if (git_remote_lookup(&remote, repo, "origin") != 0)
        {
            diagnostics.error(error("failed to find origin remote"));
            return false;
        }

        diagnostics.info("fetching package " + packageName + " origin");
        Progress state;
        state.diagnostics = &diagnostics;
        state.packageName = packageName;
        state.progressBarActive = false;
        git_fetch_options options = GIT_FETCH_OPTIONS_INIT;
        options.callbacks.transfer_progress = GitFetch::progress;
        options.callbacks.payload = &state;
        std::string proxyUrl;
        proxy(options, proxyUrl, diagnostics);

        const int result = git_remote_fetch(remote, nullptr, &options, nullptr);

        // If progress bar didn't complete naturally (error or no progress), clear it
        if (state.progressBarActive)
        {
            clear();
        }

        if (result == 0)
        {
            const git_remote_head **heads = nullptr;
            size_t count = 0;
            if (git_remote_ls(&heads, &count, remote) != 0)
            {
                diagnostics.error(error("failed to read fetched remote references"));
                git_remote_free(remote);
                return false;
            }
            bool hasRemoteHead = false;
            for (size_t index = 0; index < count; ++index)
            {
                if (!heads[index]->name || std::string(heads[index]->name) != "HEAD")
                {
                    continue;
                }
                if (!rememberHead(repo, &heads[index]->oid, diagnostics))
                {
                    git_remote_free(remote);
                    return false;
                }
                hasRemoteHead = true;
                break;
            }
            if (!hasRemoteHead)
            {
                if (!rememberHead(repo, nullptr, diagnostics))
                {
                    git_remote_free(remote);
                    return false;
                }
            }
        }
        git_remote_free(remote);
        if (result != 0)
        {
            diagnostics.error(error("failed to fetch origin"));
            return false;
        }
        return true;
    }

    bool GitFetch::resolve(git_repository *repo, const std::string &ref, git_object **object)
    {
        const std::string requested = ref.empty() ? "HEAD" : ref;
        git_oid oid;
        if (requested.size() == GIT_OID_HEXSZ && git_oid_fromstr(&oid, requested.c_str()) == 0)
        {
            return git_object_lookup(object, repo, &oid, GIT_OBJECT_ANY) == 0;
        }
        if (requested == "HEAD")
        {
            return git_revparse_single(object, repo, "refs/remotes/origin/HEAD") == 0;
        }
        const bool explicitBranch = requested.rfind("refs/heads/", 0) == 0;
        if (requested.rfind("refs/", 0) == 0 && !explicitBranch)
        {
            return git_revparse_single(object, repo, requested.c_str()) == 0;
        }
        if (!explicitBranch)
        {
            const std::string tagRef = "refs/tags/" + requested;
            if (git_revparse_single(object, repo, tagRef.c_str()) == 0)
            {
                return true;
            }
        }

        const std::string branch = explicitBranch ? requested.substr(11) : requested;
        const std::string remoteRef = "refs/remotes/origin/" + branch;
        if (git_revparse_single(object, repo, remoteRef.c_str()) == 0)
        {
            return true;
        }

        return git_revparse_single(object, repo, requested.c_str()) == 0;
    }

    bool GitFetch::checkout(git_repository *repo, git_object *object, std::string &commit, DiagnosticSink &diagnostics)
    {
        git_object *commitObject = nullptr;
        if (git_object_peel(&commitObject, object, GIT_OBJECT_COMMIT) != 0)
        {
            diagnostics.error(error("failed to resolve dependency ref to commit"));
            return false;
        }

        const git_oid *oid = git_object_id(commitObject);
        char oidText[GIT_OID_HEXSZ + 1] = {};
        git_oid_tostr(oidText, sizeof(oidText), oid);
        commit = oidText;

        git_checkout_options checkoutOptions = GIT_CHECKOUT_OPTIONS_INIT;
        checkoutOptions.checkout_strategy = GIT_CHECKOUT_FORCE;
        if (git_checkout_tree(repo, commitObject, &checkoutOptions) != 0)
        {
            git_object_free(commitObject);
            diagnostics.error(error("failed to checkout dependency commit"));
            return false;
        }
        if (git_repository_set_head_detached(repo, oid) != 0)
        {
            git_object_free(commitObject);
            diagnostics.error(error("failed to detach dependency HEAD"));
            return false;
        }

        git_object_free(commitObject);
        return true;
    }

    bool GitFetch::submodules(git_repository *repo, const std::string &packageName, DiagnosticSink &diagnostics)
    {
        struct Payload
        {
            DiagnosticSink *diagnostics;
            std::string *proxyUrl;
            int count;
        };

        std::string proxyUrl = proxyEnv();
        Payload payload = {&diagnostics, &proxyUrl, 0};

        const int result = git_submodule_foreach(
            repo,
            [](git_submodule *sm, const char *name, void *payload) -> int
            {
                auto *ctx = static_cast<Payload *>(payload);
                if (git_submodule_init(sm, 0) != 0)
                {
                    ctx->diagnostics->error(GitFetch::error(std::string("failed to initialize submodule ") + name));
                    return GIT_EUSER;
                }
                git_submodule_update_options opts = GIT_SUBMODULE_UPDATE_OPTIONS_INIT;
                if (!ctx->proxyUrl->empty())
                {
                    opts.fetch_opts.proxy_opts.type = GIT_PROXY_SPECIFIED;
                    opts.fetch_opts.proxy_opts.url = ctx->proxyUrl->c_str();
                }
                if (git_submodule_update(sm, 1, &opts) != 0)
                {
                    ctx->diagnostics->error(GitFetch::error(std::string("failed to update submodule ") + name));
                    return GIT_EUSER;
                }
                ++ctx->count;
                return 0;
            },
            &payload);

        if (result != 0)
        {
            diagnostics.error("failed to restore submodules for " + packageName);
            return false;
        }

        if (payload.count > 0)
        {
            diagnostics.info("initialized " + std::to_string(payload.count) + " git submodule(s) for " + packageName);
        }
        return true;
    }

    bool GitFetcher::cachedOrigin(const std::filesystem::path &repository, std::string &url)
    {
        if (git_libgit2_init() < 0)
        {
            return false;
        }
        git_repository *repo = nullptr;
        git_remote *remote = nullptr;
        const bool found = git_repository_open(&repo, repository.string().c_str()) == 0 &&
                           git_remote_lookup(&remote, repo, "origin") == 0 && git_remote_url(remote);
        if (found)
        {
            url = git_remote_url(remote);
        }
        git_remote_free(remote);
        git_repository_free(repo);
        git_libgit2_shutdown();
        return found;
    }

    std::filesystem::path GitFetcher::localPath(const std::string &source, const std::filesystem::path &workspaceRoot)
    {
        const auto url = GitFetch::url(source);
        const auto colon = url.find(':');
        const auto separator = url.find_first_of("/\\");
        if (url.find("://") != std::string::npos ||
            (colon != std::string::npos && colon > 1 && (separator == std::string::npos || colon < separator)))
        {
            return {};
        }
        const std::filesystem::path path(url);
        return std::filesystem::weakly_canonical(path.is_absolute() ? path : workspaceRoot / path);
    }

    bool GitFetcher::initialize(DiagnosticSink &diagnostics) const
    {
        const int result = git_libgit2_init();
        if (result < 0)
        {
            diagnostics.error("libgit2 initialization failed");
            return false;
        }
        git_libgit2_shutdown();
        return true;
    }

    bool GitFetcher::fetch(const DependencyDesc &dependency, const std::filesystem::path &destination,
                           DiagnosticSink &diagnostics) const
    {
        std::string commit;
        return fetch(dependency, destination, commit, diagnostics);
    }

    bool GitFetcher::fetch(const DependencyDesc &dependency, const std::filesystem::path &destination,
                           std::string &commit, DiagnosticSink &diagnostics, bool refresh) const
    {
        if (git_libgit2_init() < 0)
        {
            diagnostics.error("libgit2 initialization failed");
            return false;
        }
        git_repository *repo = nullptr;
        bool cloned = false;
        if (!GitFetch::open(dependency, destination, &repo, cloned, diagnostics))
        {
            git_repository_free(repo);
            git_libgit2_shutdown();
            return false;
        }

        bool ok = true;
        if (cloned)
        {
            git_reference *head = nullptr;
            ok = git_repository_head(&head, repo) == 0;
            if (ok)
            {
                ok = GitFetch::rememberHead(repo, git_reference_target(head), diagnostics);
            }
            else
            {
                if (dependency.ref.empty() || dependency.ref == "HEAD")
                {
                    diagnostics.error(GitFetch::error("cloned repository has no default commit"));
                }
                else
                {
                    ok = GitFetch::rememberHead(repo, nullptr, diagnostics);
                }
            }
            git_reference_free(head);
        }
        git_object *object = nullptr;
        git_oid oid;
        const bool immutable =
            dependency.ref.size() == GIT_OID_HEXSZ && git_oid_fromstr(&oid, dependency.ref.c_str()) == 0;
        if (ok && immutable)
        {
            GitFetch::resolve(repo, dependency.ref, &object);
        }
        if (ok && !cloned && (refresh || !immutable || !object))
        {
            git_object_free(object);
            object = nullptr;
            ok = GitFetch::fetch(repo, dependency.name, diagnostics);
        }
        if (ok && !object && !GitFetch::resolve(repo, dependency.ref, &object))
        {
            diagnostics.error(GitFetch::error("failed to resolve dependency ref " + dependency.ref));
            ok = false;
        }
        if (ok)
        {
            ok = GitFetch::checkout(repo, object, commit, diagnostics);
        }
        if (ok)
        {
            ok = GitFetch::submodules(repo, dependency.name, diagnostics);
        }

        if (object)
        {
            git_object_free(object);
        }
        git_repository_free(repo);
        git_libgit2_shutdown();

        if (ok)
        {
            diagnostics.info("restored git package " + dependency.name + " at " + commit);
        }
        return ok;
    }

} // namespace rhodium
