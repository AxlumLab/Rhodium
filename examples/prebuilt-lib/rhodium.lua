package("prebuilt-lib-example")
version("0.1.0")
default_config("debug")
default_platform("host")
default_arch("x64")

require("vendorlib", {
    source = "vendor/prebuilt",
    source_type = "local",
    build = "prebuilt",
    artifacts = {
        include_dirs = {"include"},
        lib_dirs = {"lib"},
        libs = {"vendorlib"}
    }
})
