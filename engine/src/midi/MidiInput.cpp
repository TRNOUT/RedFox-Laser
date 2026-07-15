#include "midi/MidiInput.hpp"

#include "midi/MidiParser.hpp"

#include <libremidi/libremidi.hpp>

#include <mutex>

namespace redfox::engine {

struct MidiInput::Impl {
    std::mutex mutex;
    redfox::midi::MidiMap map;
    ActionCallback callback;
    std::unique_ptr<libremidi::midi_in> midiin;
    bool open = false;

    void onMessage(const libremidi::message& message) {
        redfox::midi::Action action;
        ActionCallback cb;
        {
            std::lock_guard<std::mutex> lock(mutex);
            if (message.bytes.empty()) {
                return;
            }
            const redfox::midi::MidiMessage parsed =
                redfox::midi::parseMidi(message.bytes.data(), message.bytes.size());
            action = map.resolve(parsed);
            cb = callback;
        }
        if (action.type != redfox::midi::ActionType::None && cb) {
            cb(action);
        }
    }
};

MidiInput::MidiInput() : impl_(std::make_unique<Impl>()) {}

MidiInput::~MidiInput() = default;

void MidiInput::setMap(const redfox::midi::MidiMap& map) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->map = map;
}

void MidiInput::setActionCallback(ActionCallback callback) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->callback = std::move(callback);
}

std::string MidiInput::openFirstPort() {
    libremidi::observer observer;
    const auto ports = observer.get_input_ports();
    if (ports.empty()) {
        return {};
    }

    Impl* impl = impl_.get();
    impl->midiin = std::make_unique<libremidi::midi_in>(libremidi::input_configuration{
        .on_message = [impl](const libremidi::message& message) { impl->onMessage(message); }});
    impl->midiin->open_port(ports.front());
    impl->open = true;
    return std::string(ports.front().port_name);
}

bool MidiInput::isOpen() const { return impl_->open; }

} // namespace redfox::engine
