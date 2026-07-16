include(FetchContent)
FetchContent_Declare(libremidi
    GIT_REPOSITORY https://github.com/celtera/libremidi.git
    GIT_TAG v5.3.0
)
FetchContent_MakeAvailable(libremidi)

# miniaudio is a single header with no CMakeLists; just make its source dir
# available and add it to the include path.
FetchContent_Declare(miniaudio
    GIT_REPOSITORY https://github.com/mackron/miniaudio.git
    GIT_TAG 0.11.21
)
FetchContent_MakeAvailable(miniaudio)

add_library(redfox_engine_core STATIC
    src/safety/SafetySupervisor.cpp
    src/safety/SafetyEventLog.cpp
    src/output/LiberaLaserOutput.cpp
    src/output/ArtNetPacket.cpp
    src/output/ArtNetOutput.cpp
    src/playback/FrameGenerator.cpp
    src/playback/Sequencer.cpp
    src/midi/MidiInput.cpp
    src/audio/AudioInput.cpp
    src/audio/miniaudio_impl.cpp
    src/ipc/TelemetryHost.cpp
    src/ipc/TelemetryClient.cpp
    src/ipc/CommandPipeServer.cpp
    src/ipc/CommandPipeClient.cpp
)

target_include_directories(redfox_engine_core PUBLIC src)
target_include_directories(redfox_engine_core PRIVATE ${miniaudio_SOURCE_DIR})
target_link_libraries(redfox_engine_core PUBLIC libera-core redfox_core)
target_link_libraries(redfox_engine_core PRIVATE libremidi)
redfox_copy_bundled_libusb_dll(redfox_engine_core)

if (WIN32)
    target_link_libraries(redfox_engine_core PUBLIC ws2_32)
endif()

add_executable(redfox_engine src/main.cpp)
target_link_libraries(redfox_engine PRIVATE redfox_engine_core)
redfox_copy_bundled_libusb_dll(redfox_engine)

if (BUILD_TESTING)
    add_subdirectory(tests)
endif()
