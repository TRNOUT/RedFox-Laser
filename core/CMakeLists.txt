# Shared, hardware-independent core library: file formats (ILDA), the show
# data model, and related logic used by both the engine and the UI.
add_library(redfox_core STATIC
    src/ilda/IldaCodec.cpp
    src/show/CuePlayback.cpp
    src/show/DemoContent.cpp
    src/show/ShowEditing.cpp
    src/show/ShowFile.cpp
    src/show/Timeline.cpp
    src/show/Transform.cpp
    src/effects/Oscillator.cpp
    src/audio/AudioAnalysis.cpp
    src/audio/BeatDetector.cpp
    src/midi/MidiParser.cpp
    src/midi/MidiMap.cpp
)
target_include_directories(redfox_core PUBLIC src)

if (BUILD_TESTING)
    add_subdirectory(tests)
endif()
