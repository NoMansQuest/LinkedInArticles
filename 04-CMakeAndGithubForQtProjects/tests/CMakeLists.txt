# ==================== GoogleTest Setup (FetchContent) ====================
include(FetchContent)

FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
  # Alternative (more stable on Windows): use a specific commit
  # GIT_REPOSITORY https://github.com/google/googletest.git
  # GIT_TAG        v1.15.2
)

# For Windows + MSVC: prevent overriding parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)
# =====================================================================

# Automatically find all test-related source files
file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

# The test executable — includes your custom main.cpp + all test classes
add_executable(tests
    ${TEST_SOURCES}
)

target_include_directories(tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(tests
    PRIVATE
        app_core          # We need access to production code
        GTest::gtest_main
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::PrintSupport
        Qt${QT_VERSION_MAJOR}::Test
)

# One single CTest entry — runs your TestRegistry which discovers everything
add_test(
    NAME AllRegisteredTests
    COMMAND tests
)

# Optional: nicer output when running ctest
set_tests_properties(AllRegisteredTests
    PROPERTIES
        LABELS "unit;auto"
)

# Optional: make "make check" or VS "Build → Run CTest" more convenient
add_custom_target(check
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS tests
)

# Need to include 'GoogleTest' as they are here. Also we need to force discovery of
# tests by GTest.
include(GoogleTest)
gtest_discover_tests(tests)