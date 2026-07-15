add_executable(redfox_core_tests
    test_ilda_roundtrip.cpp
    test_ilda_indexed.cpp
    test_ilda_file.cpp
    test_cue_playback.cpp
)
target_link_libraries(redfox_core_tests PRIVATE redfox_core Catch2::Catch2WithMain)

include(Catch)
catch_discover_tests(redfox_core_tests)
