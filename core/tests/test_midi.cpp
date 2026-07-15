#include <catch2/catch_test_macros.hpp>

#include "midi/MidiMap.hpp"
#include "midi/MidiParser.hpp"

#include <cmath>
#include <cstdint>

using namespace redfox::midi;

namespace {
MidiMessage parse3(std::uint8_t s, std::uint8_t d1, std::uint8_t d2) {
    const std::uint8_t bytes[3] = {s, d1, d2};
    return parseMidi(bytes, 3);
}
bool near(float a, float b) { return std::fabs(a - b) < 0.001f; }
} // namespace

TEST_CASE("parseMidi decodes NoteOn / NoteOff / CC", "[midi]") {
    const MidiMessage on = parse3(0x90, 60, 100);
    REQUIRE(on.type == MidiType::NoteOn);
    REQUIRE(on.channel == 0);
    REQUIRE(on.data1 == 60);
    REQUIRE(on.data2 == 100);

    const MidiMessage off = parse3(0x85, 40, 0);
    REQUIRE(off.type == MidiType::NoteOff);
    REQUIRE(off.channel == 5);

    const MidiMessage cc = parse3(0xB2, 7, 127);
    REQUIRE(cc.type == MidiType::ControlChange);
    REQUIRE(cc.channel == 2);
    REQUIRE(cc.data1 == 7);
    REQUIRE(cc.data2 == 127);
}

TEST_CASE("NoteOn with velocity 0 is normalised to NoteOff", "[midi]") {
    const MidiMessage m = parse3(0x93, 60, 0);
    REQUIRE(m.type == MidiType::NoteOff);
    REQUIRE(m.channel == 3);
}

TEST_CASE("short or unknown MIDI input parses as Other", "[midi]") {
    const std::uint8_t oneByte[1] = {0x90};
    REQUIRE(parseMidi(oneByte, 1).type == MidiType::Other);
    REQUIRE(parse3(0xF0, 0, 0).type == MidiType::Other); // system message
}

TEST_CASE("a note binding triggers its mapped cue", "[midi]") {
    MidiMap map;
    map.bind({MidiType::NoteOn, 0, 36, ActionType::TriggerCue, 5});

    const Action a = map.resolve(parse3(0x90, 36, 100));
    REQUIRE(a.type == ActionType::TriggerCue);
    REQUIRE(a.cueIndex == 5);
}

TEST_CASE("a CC binding maps the value byte to 0..1 brightness", "[midi]") {
    MidiMap map;
    map.bind({MidiType::ControlChange, 0, 7, ActionType::SetMasterBrightness, 0});

    REQUIRE(near(map.resolve(parse3(0xB0, 7, 127)).value, 1.0f));
    REQUIRE(near(map.resolve(parse3(0xB0, 7, 0)).value, 0.0f));
    const Action half = map.resolve(parse3(0xB0, 7, 64));
    REQUIRE(half.type == ActionType::SetMasterBrightness);
    REQUIRE(near(half.value, 64.0f / 127.0f));
}

TEST_CASE("unmapped messages and wrong channel resolve to None", "[midi]") {
    MidiMap map;
    map.bind({MidiType::NoteOn, 0, 36, ActionType::TriggerCue, 1});

    REQUIRE(map.resolve(parse3(0x90, 37, 100)).type == ActionType::None); // wrong note
    REQUIRE(map.resolve(parse3(0x91, 36, 100)).type == ActionType::None); // wrong channel
}

TEST_CASE("re-binding the same signature overrides the earlier action", "[midi]") {
    MidiMap map;
    map.bind({MidiType::NoteOn, 0, 36, ActionType::TriggerCue, 1});
    map.bind({MidiType::NoteOn, 0, 36, ActionType::Blackout, 0});

    REQUIRE(map.resolve(parse3(0x90, 36, 100)).type == ActionType::Blackout);
}
