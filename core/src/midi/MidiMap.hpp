#pragma once

#include "midi/MidiTypes.hpp"

#include <vector>

namespace redfox::midi {

// Holds the MIDI-to-action bindings and resolves incoming messages against
// them. A later binding for the same signature overrides an earlier one
// (supports "MIDI learn" re-binding).
class MidiMap {
public:
    void bind(const MidiBinding& binding);
    void clear();
    std::size_t bindingCount() const { return bindings_.size(); }

    // Resolve a message to its action, or ActionType::None if unmapped.
    // For a SetMasterBrightness binding the message value maps 0..127 -> 0..1.
    Action resolve(const MidiMessage& message) const;

private:
    std::vector<MidiBinding> bindings_;
};

} // namespace redfox::midi
