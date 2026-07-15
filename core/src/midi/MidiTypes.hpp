#pragma once

#include <cstdint>

namespace redfox::midi {

enum class MidiType {
    NoteOn,
    NoteOff,
    ControlChange,
    Other,
};

// A parsed MIDI channel-voice message.
struct MidiMessage {
    MidiType type = MidiType::Other;
    std::uint8_t channel = 0; // 0..15
    std::uint8_t data1 = 0;   // note number / CC number
    std::uint8_t data2 = 0;   // velocity / CC value (0..127)
};

// What a bound control does when triggered.
enum class ActionType {
    None,
    TriggerCue,
    Blackout,
    Arm,
    SetMasterBrightness,
};

struct Action {
    ActionType type = ActionType::None;
    int cueIndex = 0;    // for TriggerCue
    float value = 0.0f;  // for SetMasterBrightness (0..1)
};

// Maps a MIDI message signature (type + channel + data1) to an action. For a
// SetMasterBrightness binding the message's value byte becomes the action value.
struct MidiBinding {
    MidiType type = MidiType::NoteOn;
    std::uint8_t channel = 0;
    std::uint8_t data1 = 0;
    ActionType action = ActionType::None;
    int cueIndex = 0;
};

} // namespace redfox::midi
