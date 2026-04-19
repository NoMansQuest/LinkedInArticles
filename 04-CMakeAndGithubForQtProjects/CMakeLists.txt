# 1. Specify the minimum CMake version required.
#    The range syntax (3.20...3.30) means "at least 3.20, but we have tested up to 3.30".
cmake_minimum_required(VERSION 3.20...3.30)

# 2. Declare the project name and language.
#    This must come before most other commands.
#    LANGUAGES CXX tells CMake we are building C++ code.
#    VERSION sets the version of your build.
project(QtProjectTemplate VERSION 1.0.0 LANGUAGES CXX)

# 3. Define output directories for binaries, libraries, and archives.
#    This keeps the build folder organized: all executables go into <build>/bin,
#    static libraries into <build>/lib, etc.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# 4. Set global CMake and Qt-related properties.
#
#    CMAKE_CXX_STANDARD / CMAKE_CXX_STANDARD_REQUIRED:
#    Enforce C++17 (or higher) across the entire project. This is a global setting,
#    but individual targets can override it with target_compile_features().
#
#    CMAKE_AUTOMOC / CMAKE_AUTORCC / CMAKE_AUTOUIC = ON:
#    Automatically run Qt's moc, rcc, and uic tools on files that need it.
#    This eliminates the need to manually call qt_wrap_cpp(), qt_add_resources(), etc.
#    in most cases and greatly simplifies your src/CMakeLists.txt.
#
#    CMAKE_AUTOUIC_SEARCH_PATHS:
#    Tells uic where to look for .ui files (useful if they are not in the same folder
#    as the source that includes them).
#
#    CMAKE_AUTOMOC_NO_MOC_COMPILATION (optional):
#    When ON, generated moc files are included directly into the source files instead
#    of being compiled separately. This can reduce build overhead in some projects,
#    but is not required for most Qt applications.
set(CMAKE_CXX_STANDARD          17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC               ON)
set(CMAKE_AUTORCC               ON)
set(CMAKE_AUTOUIC               ON)
set(CMAKE_AUTOUIC_SEARCH_PATHS "${CMAKE_CURRENT_SOURCE_DIR}")
set(CMAKE_AUTOMOC_NO_MOC_COMPILATION ON)

# 5. Find the required Qt packages.
#    We first determine whether Qt6 or Qt5 is available (QT_VERSION_MAJOR will be set),
#    then find the actual components we need. 'Test' is included because the 'tests' 
#    subdirectory likely uses QtTest.
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Gui Widgets PrintSupport Test)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Widgets PrintSupport Test)

# 6. Enable CTest support.
#    This allows you to run tests with `ctest` or from the IDE ("Run Tests" in Visual Studio).
#    Individual tests are usually defined inside the 'tests' subdirectory.
enable_testing()

# 7. Add subdirectories.
#
#    Each subdirectory must contain its own CMakeLists.txt file.
#    - 'src'   -> Contains the main application (library + executable)
#    - 'tests' -> Contains unit tests (usually a separate executable that links to app_core)
#
#    WARNING: Order matters, and 'src' should come before 'tests' so the main library is 
#             available.
add_subdirectory(src)
add_subdirectory(tests)