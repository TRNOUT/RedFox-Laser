#pragma once

#include "midi/MidiTypes.hpp"

#include <cstddef>
#include <cstdint>

namespace redfox::midi {

// Parse a raw MIDI message. NoteOn with velocity 0 is normalised to NoteOff
// (the common release convention). Messages shorter than 3 bytes, or with an
// unrecognised status, come back as MidiType::Other.
MidiMessage parseMidi(const std::uint8_t* bytes, std::size_t length);

} // namespace redfox::midi
