package("custom-build-example")
version("0.1.0")
default_config("debug")
default_platform("host")
default_arch("x64")

require("codegenlib", {
    source = "https://example.com/codegenlib.git",
    ref = "v1.0.0",
    build = "custom",
    commands = {
        configure = {
            "python configure.py --prefix $(artifact)"
        },
        build = {
            "python build.py --config $(config)"
        },
        install = {
            "python install.py --prefix $(artifact)"
        }
    },
    artifacts = {
        include_dirs = {"include"},
        lib_dirs = {"lib"},
        libs = {"codegenlib"}
    }
})
