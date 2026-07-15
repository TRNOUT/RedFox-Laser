#pragma once

#include "show/Show.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace redfox::show {

// A compact binary container for a Show: a small header, then per cue the name,
// playback settings, and the cue's frames encoded as an embedded ILDA blob
// (reusing the ILDA codec). Little-endian; magic "RFSH", version 1.
std::vector<std::uint8_t> writeShow(const Show& show);

struct ShowParseResult {
    bool ok = false;
    std::string error;
    Show show;
};

ShowParseResult readShow(const std::vector<std::uint8_t>& bytes);

bool writeShowFile(const std::string& path, const Show& show);
ShowParseResult readShowFile(const std::string& path);

} // namespace redfox::show
