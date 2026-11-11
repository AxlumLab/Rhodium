find_package(GTest CONFIG REQUIRED)

set(RHODIUM_TEST_SOURCES
    tests/rhodium/BuildModelTests.cpp
    tests/rhodium/WorkspaceCompatibilityTests.cpp
    tests/rhodium/ArchiveTests.cpp
    tests/rhodium/ArtifactGcTests.cpp
    tests/rhodium/CMakeDependencyExporterTests.cpp
    tests/rhodium/DependencyResolverTests.cpp
    tests/rhodium/DependencyBuildStepTests.cpp
    tests/rhodium/LuaDslTests.cpp
    tests/rhodium/LockFileTests.cpp
    tests/rhodium/PackageArtifactResolverTests.cpp
    tests/rhodium/PackageBuilderUtilTests.cpp
    tests/rhodium/ToolchainTests.cpp
)

add_executable(rhodium_tests ${RHODIUM_TEST_SOURCES})
target_link_libraries(rhodium_tests PRIVATE rhodium_core GTest::gtest GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(rhodium_tests)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/tests" PREFIX tests FILES ${RHODIUM_TEST_SOURCES})
