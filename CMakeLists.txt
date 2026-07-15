cmake_minimum_required(VERSION 3.15)
project(RedFoxLaser LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (NOT WIN32)
    message(FATAL_ERROR "RedFox-Laser targets Windows only (design spec, section 13).")
endif()

include(CTest)

# We only need the libera-core library itself, not its own example/test/GUI
# targets, and we use its bundled libusb binaries (from the helios_dac
# submodule) rather than requiring a system libusb install via vcpkg.
set(LIBERA_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(LIBERA_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(LIBERA_BUILD_LASER_TOOL OFF CACHE BOOL "" FORCE)
set(LIBERA_USE_BUNDLED_LIBUSB ON CACHE BOOL "" FORCE)

add_subdirectory(third_party/libera-laser)

# libera-laser's bundled libusb ships as a DLL; copy it next to every
# executable that links libera-core so Windows can find it at runtime.
function(redfox_copy_bundled_libusb_dll target)
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<IF:$<CONFIG:Debug>,${LIBERA_BUNDLED_LIBUSB_DLL_DIR_DEBUG},${LIBERA_BUNDLED_LIBUSB_DLL_DIR_RELEASE}>/libusb-1.0.dll"
            "$<TARGET_FILE_DIR:${target}>"
    )
endfunction()

add_subdirectory(tools/idn_probe)
add_subdirectory(engine)
add_subdirectory(tools/engine_console)
