#include "rhodium/config/JsonModelWriter.hpp"
#include "rhodium/model/BuildModel.hpp"

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

namespace
{

    TEST(BuildModelTests, JsonWriterUsesDependencyManifestShape)
    {
        rhodium::BuildModel model;
        model.rootPackage.name = "rhodium-deps";
        model.workspace.defaultConfig = "debug";

        rhodium::DependencyDesc dep;
        dep.name = "fmt";
        dep.source = "https://github.com/fmtlib/fmt.git";
        dep.sourceType = "git";
        dep.ref = "10.2.1";
        dep.buildType = "cmake";
        dep.artifacts.includeDirs.push_back("include");
        model.rootPackage.dependencies.push_back(dep);

        auto json = rhodium::JsonModel::from(model);

        EXPECT_EQ(json["manifest"], "rhodium.lua");
        EXPECT_EQ(json["package"]["name"], "rhodium-deps");
        ASSERT_EQ(json["package"]["dependencies"].size(), 1u);
        EXPECT_EQ(json["package"]["dependencies"][0]["name"], "fmt");
    }

} // namespace
