#pragma once

#include "midi/MidiMap.hpp"

#include <functional>
#include <memory>
#include <string>

namespace redfox::engine {

// Device layer for MIDI input. Wraps a platform MIDI port (via libremidi) and
// turns each incoming message into an action using the core MidiMap. libremidi
// is hidden behind a pimpl so consumers don't need its headers. The action
// callback runs on libremidi's own thread; keep it quick and thread-safe.
class MidiInput {
public:
    using ActionCallback = std::function<void(const redfox::midi::Action&)>;

    MidiInput();
    ~MidiInput();

    MidiInput(const MidiInput&) = delete;
    MidiInput& operator=(const MidiInput&) = delete;

    void setMap(const redfox::midi::MidiMap& map);
    void setActionCallback(ActionCallback callback);

    // Open the first available MIDI input port. Returns its name, or an empty
    // string if no port is available / opening failed.
    std::string openFirstPort();
    bool isOpen() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace redfox::engine
