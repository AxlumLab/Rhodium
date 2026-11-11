package("header-only-example")
version("0.1.0")
default_config("debug")
default_platform("host")
default_arch("x64")

require("nameof", {
    source = "https://github.com/Neargye/nameof.git",
    ref = "v0.10.3",
    build = "header_only",
    artifacts = {
        include_dirs = {"include"}
    }
})
