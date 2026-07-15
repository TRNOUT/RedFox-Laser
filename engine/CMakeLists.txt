add_library(redfox_engine_core STATIC
    src/safety/SafetySupervisor.cpp
    src/safety/SafetyEventLog.cpp
    src/output/LiberaLaserOutput.cpp
    src/ipc/TelemetryHost.cpp
    src/ipc/TelemetryClient.cpp
    src/ipc/CommandPipeServer.cpp
    src/ipc/CommandPipeClient.cpp
)

target_include_directories(redfox_engine_core PUBLIC src)
target_link_libraries(redfox_engine_core PUBLIC libera-core)
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
