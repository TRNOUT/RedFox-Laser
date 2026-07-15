#pragma once

#include "ilda/IldaTypes.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace redfox::ilda {

// Serialise frames to ILDA IDTF bytes. 2D frames are written as format 5
// (2D true colour), 3D frames as format 4 (3D true colour); the stream ends
// with the conventional zero-record EOF header.
std::vector<std::uint8_t> writeIlda(const IldaShow& show);

struct ParseResult {
    bool ok = false;
    std::string error;
    IldaShow frames;
};

// Parse ILDA IDTF bytes into frames. Supports formats 0/1/2/4/5, resolving
// indexed colours against the current palette (a preceding format-2 palette
// section, or the standard ILDA default palette).
ParseResult readIlda(const std::vector<std::uint8_t>& bytes);

// Convenience file wrappers around writeIlda/readIlda.
bool writeIldaFile(const std::string& path, const IldaShow& show);
ParseResult readIldaFile(const std::string& path);

} // namespace redfox::ilda
