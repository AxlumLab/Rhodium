package("transitive-deps-example")
version("0.1.0")
default_config("debug")
default_platform("host")
default_arch("x64")

require("fmt", {
    source = "https://github.com/fmtlib/fmt.git",
    ref = "10.2.1",
    build = "cmake",
    cmake = {
        options = {
            FMT_DOC = false,
            FMT_TEST = false
        }
    },
    artifacts = {
        include_dirs = {"include"},
        libs = {"fmt"}
    }
})

require("spdlog", {
    source = "https://github.com/gabime/spdlog.git",
    ref = "v1.13.0",
    build = "cmake",
    dependencies = {"fmt"},
    cmake = {
        options = {
            SPDLOG_BUILD_EXAMPLE = false,
            SPDLOG_BUILD_TESTS = false
        }
    },
    artifacts = {
        include_dirs = {"include"},
        libs = {"spdlog"}
    }
})
