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

# Enable testing
enable_testing()

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

# Put tests in their own folder
set_target_properties(tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/tests/Debug
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/tests/Release
)

# Optional: make "make check" or VS "Build → Run CTest" more convenient
add_custom_target(check
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS tests
)

# ================================================================
#  Make gtest_discover_tests work on Windows with Qt DLLs
# ================================================================
if(WIN32)

    # Make this a proper Windows GUI application (no console window) and set the
    # correct entry point.
    set_target_properties(tests PROPERTIES
        WIN32_EXECUTABLE TRUE
        LINK_FLAGS "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup"
    )

    # Copy all runtime DLL dependencies (including those from Qt) next to the executable.
    # The generator expression $<TARGET_RUNTIME_DLLS:...> automatically expands to
    # the list of DLLs required by the target. This is much cleaner than manual copying.
    add_custom_command(TARGET tests POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:tests> $<TARGET_FILE_DIR:tests>
        COMMAND_EXPAND_LISTS
    )

    # Run Qt's official deployment tool (windeployqt) to copy Qt plugins 
    # (e.g. platforms/qwindows.dll), QML imports (if any), and other required Qt files.
    # First we locate the Qt bin directory by asking for the location of qmake (i.e. 
    # 'get_target_property' and 'get_filename_component').
    #
    # WARNING: If you omit the following section, you will get an executable in the output
    #          folder that simply crash on execution, complaining about missing DLLs. 
    #          This applies to Windows only.
    get_target_property(QT_BIN_DIR Qt${QT_VERSION_MAJOR}::qmake LOCATION)
    get_filename_component(QT_BIN_DIR "${QT_BIN_DIR}" DIRECTORY)

    add_custom_command(TARGET tests POST_BUILD
        COMMAND "${QT_BIN_DIR}/windeployqt.exe"
            --dir "$<TARGET_FILE_DIR:tests>"
            --libdir "$<TARGET_FILE_DIR:tests>"
            --plugindir "$<TARGET_FILE_DIR:tests>/plugins"
            --no-compiler-runtime    # Skip MSVC redist if you handle it separately
            --no-translations
            "$<TARGET_FILE:tests>"
        COMMENT "Running windeployqt to copy Qt plugins and DLLs (including platforms/qwindows.dll)"
    )
endif()

# Need to include 'GoogleTest' as they are here. Also we need to force discovery of
# tests by GTest.
include(GoogleTest)
gtest_discover_tests(tests)