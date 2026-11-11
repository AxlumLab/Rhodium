find_package(GTest CONFIG REQUIRED)

set(RHODIUM_TEST_SOURCES
    tests/rhodium/BuildModelTests.cpp
    tests/rhodium/FileSystemTests.cpp
    tests/rhodium/WorkspaceLockTests.cpp
    tests/rhodium/WorkspaceCompatibilityTests.cpp
    tests/rhodium/ArchiveTests.cpp
    tests/rhodium/ArtifactGcTests.cpp
    tests/rhodium/CMakeDependencyExporterTests.cpp
    tests/rhodium/DependencyResolverTests.cpp
    tests/rhodium/DependencyBuildStepTests.cpp
    tests/rhodium/DependencyLockStepTests.cpp
    tests/rhodium/GitFetcherTests.cpp
    tests/rhodium/LuaDslTests.cpp
    tests/rhodium/LockFileTests.cpp
    tests/rhodium/PackageArtifactResolverTests.cpp
    tests/rhodium/PackageBuilderUtilTests.cpp
    tests/rhodium/ToolchainTests.cpp
)

add_executable(rhodium_tests ${RHODIUM_TEST_SOURCES})
target_link_libraries(rhodium_tests PRIVATE rhodium_core GTest::gtest GTest::gtest_main)

add_executable(rhodium_lock_probe tests/support/WorkspaceLockProbe.cpp)
target_link_libraries(rhodium_lock_probe PRIVATE rhodium_core)
target_compile_definitions(rhodium_tests PRIVATE RHODIUM_LOCK_PROBE=\"$<TARGET_FILE:rhodium_lock_probe>\")
add_dependencies(rhodium_tests rhodium_lock_probe)

include(GoogleTest)
gtest_discover_tests(rhodium_tests)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/tests" PREFIX tests FILES ${RHODIUM_TEST_SOURCES})
