# 1. Collect all relevant source files into a variable called PROJECT_SOURCES.
#    We use file(GLOB_RECURSE ...) to search recursively (including subdirectories)
#    for files matching the given patterns.
#
#    Note: Many CMake experts discourage GLOB/GLOB_RECURSE for source files because
#    CMake normally won't automatically detect when you add/remove/rename files
#    unless you also modify a CMakeLists.txt. However, adding CONFIGURE_DEPENDS
#    (available since CMake 3.12) largely solves this for most generators.
file(GLOB_RECURSE PROJECT_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.qrc"
)

# 2. Remove files that should NOT be part of the library.
#    The list(FILTER ...) command modifies the PROJECT_SOURCES list in place.
#    Here we exclude main.cpp (using a regex that matches any path ending in "main.cpp").
#    This prevents main.cpp from being compiled into the static library (it belongs
#    only in the final executable).
list(FILTER PROJECT_SOURCES EXCLUDE REGEX "main\\.cpp$")

# 3. Create a static library target named "app_core" and add all of the collected
#    source files to it. This library will contain the bulk of your application logic.
add_library(app_core STATIC ${PROJECT_SOURCES})

# 4. Specify the include directory for the app_core library.
#    This tells the compiler where to look for header files (#include directives)
#    when building the library.
#
#    Using PRIVATE means this directory is only used for compiling app_core itself
#    and is not automatically inherited by other targets that link against it.
#
#    Setting this to "${CMAKE_CURRENT_SOURCE_DIR}" is very common in this pattern
#    because it allows you to write clean includes like:
#        #include "mainwindow.h"
#    instead of relative paths or full paths.
#    If this is missing or incorrect, you will get "file not found" compiler errors.
target_include_directories(app_core 
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
)

# 5. We want app-core to support C++17. If you desire a higher version go ahead and adjust
#    this accordingly.
target_compile_features(app_core
    PRIVATE 
        cxx_std_17
)

# 6. (Optional) If you have any macros you want defined by the compiler prior to build you 
#    can put them here. For example the project I took this template from needed 'SCINTILLA_QT'
#    to have been defined.
target_compile_definitions(app_core
    PRIVATE 
        SCINTILLA_QT
)

# 7. Here we include all of the Qt libraries that need to be linked to our library. 
#    These entries will depend on your specific project, but 'Core', 'Gui' and 'Widgets' 
#    is almost always needed.
target_link_libraries(app_core PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::PrintSupport
)

# 8. (Optional) Should we have resource files in our project, they need to be added to
#    resource via 'qt_add_resources'.
qt_add_resources(APP_RESOURCES resources.qrc)

# 9. Now we need to declare our actual executable. The project I took this from was 
#    called 'VisualLinkerScript', but your executable could be named anything. The output
#    binary would be named accordingly.
add_executable(VisualLinkerScript
    main.cpp
    ${APP_RESOURCES}
)

# 10. Since our executable also supports Qt, we need to include the same Qt libraries
#     as we did for our app_core library.
target_link_libraries(VisualLinkerScript 
    PRIVATE 
        app_core
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::PrintSupport
)

# 11. (Optional, Windows Only) Auto-copy Qt + MSVC runtime DLLs to output folder.
if(WIN32)

    # Make this a proper Windows GUI application (no console window) and set the
    # correct entry point.
    set_target_properties(VisualLinkerScript PROPERTIES
        WIN32_EXECUTABLE TRUE
        LINK_FLAGS "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup"
    )

    # Copy all runtime DLL dependencies (including those from Qt) next to the executable.
    # The generator expression $<TARGET_RUNTIME_DLLS:...> automatically expands to
    # the list of DLLs required by the target. This is much cleaner than manual copying.
    add_custom_command(TARGET VisualLinkerScript POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:VisualLinkerScript> $<TARGET_FILE_DIR:VisualLinkerScript>
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

    add_custom_command(TARGET VisualLinkerScript POST_BUILD
        COMMAND "${QT_BIN_DIR}/windeployqt.exe"
            --dir "$<TARGET_FILE_DIR:VisualLinkerScript>"
            --libdir "$<TARGET_FILE_DIR:VisualLinkerScript>"
            --plugindir "$<TARGET_FILE_DIR:VisualLinkerScript>/plugins"
            --no-compiler-runtime    # Skip MSVC redist if you handle it separately
            --no-translations
            "$<TARGET_FILE:VisualLinkerScript>"
        COMMENT "Running windeployqt to copy Qt plugins and DLLs (including platforms/qwindows.dll)"
    )
endif()