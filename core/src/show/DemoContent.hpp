#pragma once

#include "show/Show.hpp"
#include "show/Timeline.hpp"

namespace redfox::show {

// A tiny built-in show and matching sequence so the playback and timeline paths
// can be exercised end-to-end (by the engine as a fallback, and by the UI as a
// starting point for authoring) before real content is loaded. The show has two
// cues, "Square" (a slowly spinning outline) and "Blink"; the timeline
// alternates them and loops.
Show makeDemoShow();
Timeline makeDemoTimeline();

} // namespace redfox::show
