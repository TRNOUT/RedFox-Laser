# The Qt UI. Optional: only built when Qt6 is found (point CMAKE_PREFIX_PATH at
# your Qt install, e.g. -DCMAKE_PREFIX_PATH=C:/Dev/Qt/6.8.3/msvc2022_64).
find_package(Qt6 QUIET COMPONENTS Widgets)
if (NOT Qt6_FOUND)
    message(STATUS "Qt6 not found - skipping the redfox_ui target. "
                   "Set CMAKE_PREFIX_PATH to a Qt install to build it.")
    return()
endif()

set(CMAKE_AUTOMOC ON)

add_executable(redfox_ui WIN32
    src/main.cpp
    src/MainWindow.cpp
    src/MainWindow.hpp
)
target_link_libraries(redfox_ui PRIVATE redfox_engine_core Qt6::Widgets)
redfox_copy_bundled_libusb_dll(redfox_ui)

# Copy the required Qt runtime DLLs next to the exe so it can be launched.
find_program(WINDEPLOYQT_EXE windeployqt HINTS "${Qt6_DIR}/../../../bin")
if (WINDEPLOYQT_EXE)
    add_custom_command(TARGET redfox_ui POST_BUILD
        COMMAND "${WINDEPLOYQT_EXE}" --no-translations --no-compiler-runtime
                "$<TARGET_FILE:redfox_ui>"
        VERBATIM)
endif()
