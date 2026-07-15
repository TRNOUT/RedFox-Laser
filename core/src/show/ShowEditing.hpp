#pragma once

#include "show/Show.hpp"

#include <cstddef>

namespace redfox::show {

// Remove the cue at `cueIndex` and keep the show's timeline consistent: steps
// that trigger the removed cue are dropped, and steps that reference a
// higher-indexed cue are shifted down by one so they still point at the same
// cue. Out-of-range indices are a no-op. This is the single place cue removal
// and its index bookkeeping live, so the UI can't get the fixup wrong.
void removeCue(Show& show, std::size_t cueIndex);

} // namespace redfox::show
