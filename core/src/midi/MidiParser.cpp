#include "midi/MidiParser.hpp"

namespace redfox::midi {

MidiMessage parseMidi(const std::uint8_t* bytes, std::size_t length) {
    MidiMessage message;
    if (bytes == nullptr || length < 3) {
        return message; // Other
    }

    const std::uint8_t status = bytes[0];
    const std::uint8_t high = status & 0xF0;
    message.channel = static_cast<std::uint8_t>(status & 0x0F);
    message.data1 = bytes[1] & 0x7F;
    message.data2 = bytes[2] & 0x7F;

    switch (high) {
        case 0x80:
            message.type = MidiType::NoteOff;
            break;
        case 0x90:
            // NoteOn with zero velocity is the common "note released" encoding.
            message.type = (message.data2 == 0) ? MidiType::NoteOff : MidiType::NoteOn;
            break;
        case 0xB0:
            message.type = MidiType::ControlChange;
            break;
        default:
            message.type = MidiType::Other;
            message.channel = 0;
            message.data1 = 0;
            message.data2 = 0;
            break;
    }
    return message;
}

} // namespace redfox::midi
