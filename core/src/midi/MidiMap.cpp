#include "midi/MidiMap.hpp"

namespace redfox::midi {

void MidiMap::bind(const MidiBinding& binding) {
    for (auto& existing : bindings_) {
        if (existing.type == binding.type && existing.channel == binding.channel &&
            existing.data1 == binding.data1) {
            existing = binding; // override an existing binding for the same signature
            return;
        }
    }
    bindings_.push_back(binding);
}

void MidiMap::clear() { bindings_.clear(); }

Action MidiMap::resolve(const MidiMessage& message) const {
    for (const auto& binding : bindings_) {
        if (binding.type != message.type || binding.channel != message.channel ||
            binding.data1 != message.data1) {
            continue;
        }
        Action action;
        action.type = binding.action;
        action.cueIndex = binding.cueIndex;
        if (binding.action == ActionType::SetMasterBrightness) {
            action.value = static_cast<float>(message.data2) / 127.0f;
        }
        return action;
    }
    return Action{}; // ActionType::None
}

} // namespace redfox::midi
