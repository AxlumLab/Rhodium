package("cmake-zlib-example")
version("0.1.0")
default_config("debug")
default_platform("host")
default_arch("x64")

require("zlib", {
    source = "https://zlib.net/zlib-1.3.1.tar.gz",
    source_type = "archive",
    -- sha256 = "fill-this-before-production-use",
    build = "cmake",
    cmake = {
        options = {
            ZLIB_BUILD_EXAMPLES = false
        }
    },
    artifacts = {
        include_dirs = {"include"},
        libs = {"zlib"}
    }
})
