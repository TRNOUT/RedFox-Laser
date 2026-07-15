include(FetchContent)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.5.4
)
FetchContent_MakeAvailable(Catch2)
list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)

add_executable(redfox_engine_tests
    test_smoke.cpp
    test_clock.cpp
    test_mock_laser_output.cpp
    test_safety_supervisor.cpp
    test_telemetry_channel.cpp
    test_command_pipe.cpp
)
target_link_libraries(redfox_engine_tests PRIVATE redfox_engine_core Catch2::Catch2WithMain)
redfox_copy_bundled_libusb_dll(redfox_engine_tests)

include(Catch)
catch_discover_tests(redfox_engine_tests)
