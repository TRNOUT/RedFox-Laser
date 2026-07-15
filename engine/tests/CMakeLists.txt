add_executable(redfox_engine_tests
    test_smoke.cpp
    test_clock.cpp
    test_mock_laser_output.cpp
    test_safety_supervisor.cpp
    test_telemetry_channel.cpp
    test_command_pipe.cpp
    test_frame_generator.cpp
    test_sequencer.cpp
)
target_link_libraries(redfox_engine_tests PRIVATE redfox_engine_core Catch2::Catch2WithMain)
redfox_copy_bundled_libusb_dll(redfox_engine_tests)

include(Catch)
catch_discover_tests(redfox_engine_tests)
